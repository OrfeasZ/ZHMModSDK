#include "ModLoader.h"

#include <Windows.h>
#include <filesystem>

#include "EventDispatcherImpl.h"
#include "HookImpl.h"
#include "IPluginInterface.h"
#include "Logging.h"
#include "Util/FileUtils.h"
#include "Util/StringUtils.h"
#include "UI/ModSelector.h"
#include <ini.h>

ModLoader::ModLoader()
{

}

ModLoader::~ModLoader()
{
    SaveModConfigurations();
    Hooks::Engine_Init->RemoveDetoursWithContext(this);
    UnloadAllMods();
}

void ModLoader::Startup()
{
    m_ModConfigurations = LoadModConfigurations();
    LoadAllMods();
}

void ModLoader::ScanAvailableMods()
{
    m_AvailableMods.clear();
    m_AvailableModsLower.clear();

    // Discover and load mods.
    auto s_ExePath = Util::FileUtils::GetExecutablePath();

    if (s_ExePath.empty())
        return;

    auto s_ExeDir = s_ExePath.parent_path();

    const auto s_ModPath = absolute(s_ExeDir / "mods");

    if (exists(s_ModPath) && is_directory(s_ModPath))
    {
        Logger::Debug("Looking for mods in '{}'...", s_ModPath.string());

        for (const auto& s_Entry : std::filesystem::directory_iterator(s_ModPath))
        {
            if (!s_Entry.is_regular_file())
                continue;

            if (s_Entry.path().extension() != ".dll")
                continue;

            m_AvailableMods.insert(s_Entry.path().filename().stem().string());
            m_AvailableModsLower.insert(Util::StringUtils::ToLowerCase(s_Entry.path().filename().stem().string()));
        }
    }
    else
    {
        Logger::Warn("Mod directory '{}' not found.", s_ModPath.string());
    }

    ModSDK::GetInstance()->GetUIModSelector()->UpdateAvailableMods(m_AvailableMods, GetActiveMods());
}

std::unordered_set<std::string> ModLoader::GetActiveMods()
{
    std::unordered_set<std::string> s_Mods;

    LockRead();

    for (auto& s_LoadedMod : m_LoadedMods)
        s_Mods.insert(s_LoadedMod.first);

    UnlockRead();

    return s_Mods;
}

void ModLoader::SetActiveMods(const std::unordered_set<std::string>& p_Mods)
{
    std::unordered_set<std::string> s_LowerModNames;

    for (auto& s_Mod : p_Mods)
        s_LowerModNames.insert(Util::StringUtils::ToLowerCase(s_Mod));

    // First unload any mods that don't exist in the new list.
    std::vector<std::string> s_ModsToUnload;

    Lock();
    m_ConfigMutex.lock();

    for (auto& s_Pair : m_LoadedMods) {
        if (s_LowerModNames.contains(s_Pair.first))
            continue;

        s_ModsToUnload.push_back(s_Pair.first);

        // Disable mod in configuration.
        auto s_ConfigIt = find_if(begin(m_ModConfigurations), end(m_ModConfigurations), [&s_Pair](const ModConfiguration& p_ModConfig) {
            return Util::StringUtils::CompareInsensitive(s_Pair.first, p_ModConfig.Config.Name);
        });

        if (s_ConfigIt == m_ModConfigurations.end())
            s_ConfigIt = m_ModConfigurations.emplace(s_ConfigIt);
        
        s_ConfigIt->Enabled = false;
        s_ConfigIt->Config.Name = s_Pair.second.Configuration.Config.Name;
    }

    m_ConfigMutex.unlock();
    Unlock();

    SaveModConfigurations();

    for (auto& s_Mod : s_ModsToUnload)
        UnloadMod(s_Mod);

    std::vector<std::pair<std::string, ModConfiguration&>> s_ModsToLoad;

    Lock();
    m_ConfigMutex.lock();

    // Then load any mods that aren't already loaded.
    for (auto& s_Mod : s_LowerModNames)
    {
        // Mod is already loaded; skip.
        if (m_LoadedMods.contains(s_Mod))
            continue;

        // Enable mod in configuration.
        auto s_ConfigIt = find_if(begin(m_ModConfigurations), end(m_ModConfigurations), [&s_Mod](const ModConfiguration& p_ModConfig) {
            return Util::StringUtils::CompareInsensitive(s_Mod, p_ModConfig.Config.Name);
        });

        if (s_ConfigIt == m_ModConfigurations.end())
            s_ConfigIt = m_ModConfigurations.emplace(s_ConfigIt);
        
        s_ConfigIt->Enabled = true;
        s_ConfigIt->Config.Name = s_Mod;

        s_ModsToLoad.emplace_back(s_Mod, *s_ConfigIt);
    }

    m_ConfigMutex.unlock();
    Unlock();

    for (auto& s_Mod : s_ModsToLoad)
    {
        LoadMod(s_Mod.first, true, s_Mod.second);
    }

    SaveModConfigurations();

    ModSDK::GetInstance()->GetUIModSelector()->UpdateAvailableMods(m_AvailableMods, GetActiveMods());
}

std::unordered_set<std::string> ModLoader::GetAvailableMods()
{
    return m_AvailableMods;
}

void ModLoader::LoadAllMods()
{
    ScanAvailableMods();

    // Get the mods we want to load.
    LockRead();
    m_ConfigMutex.lock_shared();

    std::vector<std::pair<std::string, const ModConfiguration&>> s_ModsToLoad;
    
    for (const auto& s_Mod : m_ModConfigurations) {
        // Ignore the SDK entry. It's used for configuring the SDK itself.
        if (s_Mod.Config.Name == "sdk")
            continue;
        if (!s_Mod.Enabled)
            continue;
        if (!m_AvailableModsLower.contains(Util::StringUtils::ToLowerCase(s_Mod.Config.Name)))
            continue;

        s_ModsToLoad.emplace_back(s_Mod.Config.Name, s_Mod);
    }

    m_ConfigMutex.unlock_shared();
    UnlockRead();

    // Load them all
    for (const auto& s_Mod : s_ModsToLoad)
        LoadMod(s_Mod.first, false, s_Mod.second);

    ModSDK::GetInstance()->GetUIModSelector()->UpdateAvailableMods(m_AvailableMods, GetActiveMods());
}

void ModLoader::LoadModConfigurationFromIniSection(ModLoader::ModConfigurationSection& p_ModConfiguration, const mINI::INIMap<std::string>& p_IniSection)
{
    for (const auto& s_Entry : p_IniSection) {
        auto& s_Settings = p_ModConfiguration.Settings;
        auto s_It = find_if(s_Settings.begin(), s_Settings.end(), [&s_Entry](const std::pair<std::string, ModSetting>& s_Pair) {
            return Util::StringUtils::CompareInsensitive(s_Pair.first, s_Entry.first);
        });

        if (s_It == s_Settings.end())
            s_Settings.emplace_back(s_Entry.first, ModSetting(s_Entry.second));
        else
            s_It->second.Set(s_Entry.second);
    }
}

void ModLoader::LoadMod(const std::string& p_Name, bool p_LiveLoad, ModConfiguration p_Configuration) {
    std::unique_lock s_Lock(m_Mutex);

    const std::string s_Name = Util::StringUtils::ToLowerCase(p_Name);

    if (m_LoadedMods.contains(s_Name))
    {
        Logger::Warn("A mod with the same name ({}) is already loaded. Skipping.", p_Name);
        return;
    }

    auto s_ExePath = Util::FileUtils::GetExecutablePath();

    if (s_ExePath.empty())
        return;

    auto s_ExeDir = s_ExePath.parent_path();

    const auto s_ModulePath = absolute(s_ExeDir / ("mods/" + p_Name + ".dll"));

    if (!exists(s_ModulePath) || !is_regular_file(s_ModulePath))
    {
        Logger::Warn("Could not find mod '{}'.", p_Name);
    }

    Logger::Info("Attempting to load mod '{}'.", p_Name);
    Logger::Debug("Module path is '{}'.", s_ModulePath.string());

    const auto s_Module = LoadLibraryA(s_ModulePath.string().c_str());

    if (s_Module == nullptr)
    {
        Logger::Warn("Failed to load mod. Error: {}", GetLastError());
        return;
    }

    const auto s_GetPluginInterfaceAddr = GetProcAddress(s_Module, "GetPluginInterface");

    if (s_GetPluginInterfaceAddr == nullptr)
    {
        Logger::Warn("Could not find plugin interface for mod. Make sure that the 'GetPluginInterface' method is exported.");
        FreeLibrary(s_Module);
        return;
    }

    const auto s_GetPluginInterface = reinterpret_cast<GetPluginInterface_t>(s_GetPluginInterfaceAddr);

    auto* s_PluginInterface = s_GetPluginInterface();

    if (s_PluginInterface == nullptr)
    {
        Logger::Warn("Mod returned a null plugin interface.");
        FreeLibrary(s_Module);
        return;
    }

    LoadedMod s_Mod {};
    s_Mod.Module = s_Module;
    s_Mod.PluginInterface = s_PluginInterface;
    s_Mod.Configuration = std::move(p_Configuration);

    m_LoadedMods[s_Name] = s_Mod;
    m_ModList.push_back(s_PluginInterface);

    s_Lock.unlock();

    ModSDK::GetInstance()->OnModLoaded(s_Name, s_PluginInterface, p_LiveLoad);
}

void ModLoader::UnloadMod(const std::string& p_Name)
{
    std::unique_lock s_Lock(m_Mutex);

    const std::string s_Name = Util::StringUtils::ToLowerCase(p_Name);

    auto s_ModMapIt = m_LoadedMods.find(s_Name);

    if (s_ModMapIt == m_LoadedMods.end())
        return;

    Logger::Info("Unloading mod '{}'.", p_Name);

    HookRegistry::ClearDetoursWithContext(s_ModMapIt->second.PluginInterface);
    EventDispatcherRegistry::ClearPluginListeners(s_ModMapIt->second.PluginInterface);

    for (auto it = m_ModList.begin(); it != m_ModList.end();)
    {
        if (*it == s_ModMapIt->second.PluginInterface)
            it = m_ModList.erase(it);
        else
            ++it;
    }

    delete s_ModMapIt->second.PluginInterface;
    FreeLibrary(s_ModMapIt->second.Module);

    m_LoadedMods.erase(s_ModMapIt);

    ModSDK::GetInstance()->OnModUnloaded(s_Name);
}

void ModLoader::ReloadMod(const std::string& p_Name)
{
    LockRead();

    const std::string s_Name = Util::StringUtils::ToLowerCase(p_Name);

    auto it = m_LoadedMods.find(s_Name);

    if (it == m_LoadedMods.end())
    {
        Logger::Warn("Could not find mod '{}' to reload.", p_Name);
        UnlockRead();
        return;
    }

    UnlockRead();

    auto s_Configuration = it->second.Configuration;
    UnloadMod(p_Name);
    LoadMod(p_Name, true, std::move(s_Configuration));
}

void ModLoader::UnloadAllMods()
{
    LockRead();

    std::vector<std::string> s_ModNames;

    for (auto& s_Pair : m_LoadedMods)
        s_ModNames.push_back(s_Pair.first);

    UnlockRead();

    for (auto& s_Mod : s_ModNames)
        UnloadMod(s_Mod);
}

void ModLoader::ReloadAllMods()
{
    LockRead();

    std::vector<std::string> s_ModNames;

    for (auto& s_Pair : m_LoadedMods)
        s_ModNames.push_back(s_Pair.first);

    UnlockRead();

    for (auto& s_Mod : s_ModNames)
        ReloadMod(s_Mod);
}

IPluginInterface* ModLoader::GetModByName(const std::string& p_Name)
{
    std::shared_lock s_Lock(m_Mutex);

    std::string s_Name = Util::StringUtils::ToLowerCase(p_Name);

    auto it = m_LoadedMods.find(s_Name);

    if (it == m_LoadedMods.end())
        return nullptr;

    return it->second.PluginInterface;
}

ModSetting* ModLoader::GetOrCreateModSetting(IPluginInterface* p_Mod, std::string_view p_Name)
{
    std::unique_lock s_Lock(m_ConfigMutex);

    // Find the mod configuration.
    auto s_It = find_if(begin(m_LoadedMods), end(m_LoadedMods), [p_Mod](const std::pair<std::string, LoadedMod>& p_LoadedMod) {
        return p_LoadedMod.second.PluginInterface == p_Mod;
    });

    if (s_It == end(m_LoadedMods))
        return nullptr;

    auto* s_Section = &s_It->second.Configuration.Config;
    auto s_SplitKey = Util::StringUtils::Split(p_Name, ".");

    for (auto s_Idx = 0; s_Idx < s_SplitKey.size(); ++s_Idx) {
        if (s_Idx < s_SplitKey.size() - 1) {
            // Find and navigate to subsection.
            auto s_It = find_if(begin(s_Section->Subsections), end(s_Section->Subsections), [&s_SplitKey, s_Idx](const ModConfigurationSection& p_Section) {
                return Util::StringUtils::CompareInsensitive(p_Section.Name, s_SplitKey[s_Idx]);
            });

            if (s_It == s_Section->Subsections.end()) {
                s_Section = &s_Section->Subsections.emplace_back();
                continue;
            }

            s_Section = &*s_It;
            continue;
        }

        // Last part of the key, find the setting.
        auto s_It = find_if(begin(s_Section->Settings), end(s_Section->Settings), [&s_SplitKey, s_Idx](const std::pair<std::string, ModSetting>& p_Pair) {
            return Util::StringUtils::CompareInsensitive(p_Pair.first, s_SplitKey[s_Idx]);
        });

        if (s_It != s_Section->Settings.end())
            return &s_It->second;
        
        return &s_Section->Settings.emplace_back(s_SplitKey[s_Idx], ModSetting{}).second;
    }

    return nullptr;
}

ModSetting* ModLoader::GetModSetting(IPluginInterface* p_Mod, std::string_view p_Name)
{
    std::shared_lock s_Lock(m_ConfigMutex);
    
    // Find the mod configuration.
    auto s_It = find_if(begin(m_LoadedMods), end(m_LoadedMods), [p_Mod](const std::pair<std::string, LoadedMod>& p_LoadedMod) {
        return p_LoadedMod.second.PluginInterface == p_Mod;
    });
    
    if (s_It == end(m_LoadedMods))
        return nullptr;

    auto* s_Section = &s_It->second.Configuration.Config;
    auto s_Tokens = Util::StringUtils::Split(p_Name, ".");

    for (auto s_Idx = 0; s_Idx < s_Tokens.size(); ++s_Idx) {
        if (s_Idx < s_Tokens.size() - 1) {
            // Find and navigate to subsection.
            auto s_It = find_if(begin(s_Section->Subsections), end(s_Section->Subsections), [&s_Tokens, s_Idx](const ModConfigurationSection& p_Section) {
                return Util::StringUtils::CompareInsensitive(p_Section.Name, s_Tokens[s_Idx]);
            });
        
            if (s_It == s_Section->Subsections.end())
                break;

            s_Section = &*s_It;
            continue;
        }

        // Last part of the key, find the setting.
        auto s_It = find_if(begin(s_Section->Settings), end(s_Section->Settings), [&s_Tokens, s_Idx](const std::pair<std::string, ModSetting>& p_Pair) {
            return Util::StringUtils::CompareInsensitive(p_Pair.first, s_Tokens[s_Idx]);
        });

        if (s_It != s_Section->Settings.end())
            return &s_It->second;
    }

    return nullptr;
}

std::vector<ModLoader::ModConfiguration> ModLoader::LoadModConfigurations()
{
    const auto s_IniPath = absolute(Util::FileUtils::GetExecutablePath().parent_path() / "mods.ini");

    if (!is_regular_file(s_IniPath))
        return {};

    mINI::INIFile s_File(s_IniPath.string());
    mINI::INIStructure s_Ini;
    
    if (!s_File.read(s_Ini))
        return {}; // TODO: throw exception?

    std::vector<ModConfiguration> s_Configuration;

    for (const auto& s_Section : s_Ini) {
        const auto s_SplitIniSectionKey = Util::StringUtils::Split(s_Section.first, ".");
        const auto& s_ModName = s_SplitIniSectionKey[0];

        // Find or add entry for mod configuration.
        auto s_It = find_if(s_Configuration.begin(), s_Configuration.end(), [&s_ModName](const ModConfiguration& v) {
            return Util::StringUtils::CompareInsensitive(v.Config.Name, s_ModName);
        });
        
        auto& s_ModConfiguration = s_It == s_Configuration.end() ? s_Configuration.emplace_back() : *s_It;

        if (s_SplitIniSectionKey.size() == 1) {
            // Handle configuration main section for mod.
            LoadModConfigurationFromIniSection(s_ModConfiguration.Config, s_Section.second);

            auto s_It = find_if(begin(s_ModConfiguration.Config.Settings), end(s_ModConfiguration.Config.Settings), [](const std::pair<std::string, ModSetting>& p_Pair) {
                return Util::StringUtils::CompareInsensitive(p_Pair.first, "enable");
            });

            // Load the 'enable' setting value, or default to true if omitted.
            s_ModConfiguration.Enabled = s_It != s_ModConfiguration.Config.Settings.end() ? s_It->second.AsBool() : true;
            s_ModConfiguration.Config.Name = s_Section.first;
        }
        else if (s_SplitIniSectionKey.size() > 1) {
            // Handle configuration subsection for mod.
            auto s_SplitIniSectionKeyStack = s_SplitIniSectionKey;
            
            // Navigate to the configuration subsection, creating any inexistent subsections.
            std::reverse(s_SplitIniSectionKeyStack.begin(), s_SplitIniSectionKeyStack.end());
            s_SplitIniSectionKeyStack.pop_back();

            auto* s_ModConfigSection = &s_ModConfiguration.Config;

            for (; !s_SplitIniSectionKeyStack.empty(); s_SplitIniSectionKeyStack.pop_back()) {
                auto& s_Key = s_SplitIniSectionKeyStack.back();
                auto s_It = find_if(begin(s_ModConfigSection->Subsections), end(s_ModConfigSection->Subsections), [&s_Key](const ModConfigurationSection& p_Section) {
                    return Util::StringUtils::CompareInsensitive(p_Section.Name, s_Key);
                });

                if (s_It == s_ModConfigSection->Subsections.end())
                    s_ModConfigSection = &s_ModConfigSection->Subsections.emplace_back();
                else
                    s_ModConfigSection = &*s_It;
            }

            // Load the configuration into the subsection.
            LoadModConfigurationFromIniSection(*s_ModConfigSection, s_Section.second);
        }
    }

    return s_Configuration;
}

void ModLoader::SaveModConfigurations()
{
    // Persist the mods to the ini file.
    auto s_ExePath = Util::FileUtils::GetExecutablePath();
    if (s_ExePath.empty())
        return;

    auto s_ExeDir = s_ExePath.parent_path();

    const auto s_IniPath = absolute(s_ExeDir / "mods.ini");

    mINI::INIFile s_File(s_IniPath.string());

    mINI::INIStructure s_Ini;

    LockRead();
    m_ConfigMutex.lock_shared();

    for (const auto& s_Config : m_ModConfigurations) {
        // Use loaded mod's settings or user config.
        auto s_It = m_LoadedMods.find(Util::StringUtils::ToLowerCase(s_Config.Config.Name));
        WriteModConfigurationToIni(s_Ini, s_It != m_LoadedMods.end() ? s_It->second.Configuration : s_Config);
    }
    
    m_ConfigMutex.unlock_shared();
    UnlockRead();

    s_File.generate(s_Ini, true);
}

void ModLoader::WriteModConfigurationToIni(mINI::INIStructure& p_Ini, const ModConfiguration& p_ModConfiguration)
{
    mINI::INIMap<std::string> s_IniMap;

    // Write all settings under main section, starting with the 'enable' setting.
    s_IniMap.set("enable", p_ModConfiguration.Enabled ? "yes" : "no");
    
    WriteModConfigurationSettingsToIni(s_IniMap, p_ModConfiguration.Config);

    p_Ini.set(p_ModConfiguration.Config.Name, s_IniMap);

    // Write any mod config subsections.
    WriteModConfigurationSubsectionsToIni(p_Ini, p_ModConfiguration.Config.Name, p_ModConfiguration.Config);
}

void ModLoader::WriteModConfigurationSubsectionsToIni(mINI::INIStructure& p_Ini, const std::string& p_ParentKey, const ModConfigurationSection& p_Section)
{
    for (const auto& s_Subsection: p_Section.Subsections) {
        // Write all settings under this section.
        mINI::INIMap<std::string> s_IniMap;
        WriteModConfigurationSettingsToIni(s_IniMap, s_Subsection);

        auto s_Key = p_ParentKey + "." + s_Subsection.Name;

        p_Ini.set(s_Key, s_IniMap);

        // Recurse to any deeper subsections.
        WriteModConfigurationSubsectionsToIni(p_Ini, s_Key, s_Subsection);
    }
}

void ModLoader::WriteModConfigurationSettingsToIni(mINI::INIMap<std::string>& p_Ini, const ModConfigurationSection& p_Section)
{
    for (const auto& s_Setting : p_Section.Settings) {
        if (Util::StringUtils::CompareInsensitive(s_Setting.first, "enable"))
            continue;

        p_Ini.set(s_Setting.first, s_Setting.second.Get());
    }
}


ModSetting::~ModSetting()
{
}

bool ModSetting::Read(ZString& p_Out)
{
    p_Out = ZString(Get());
    return true;
}

bool ModSetting::ReadBool(bool& p_Out)
{
    p_Out = AsBool();
    return true;
}

bool ModSetting::ReadInt(int32& p_Out)
{
    auto s_Result = AsInt<int32>();
    if (!s_Result)
        return false;
    p_Out = *s_Result;
    return true;
}

bool ModSetting::ReadUnsignedInt(uint32& p_Out)
{
    auto s_Result = AsInt<uint32>();
    if (!s_Result)
        return false;
    p_Out = *s_Result;
    return true;
}

bool ModSetting::ReadDouble(float64& p_Out)
{
    auto s_Result = AsFloat<float64>();
    if (!s_Result)
        return false;
    p_Out = *s_Result;
    return true;
}