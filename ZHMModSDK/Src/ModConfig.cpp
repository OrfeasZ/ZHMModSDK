#include "ModConfig.h"
#include "ModLoader.h"
#include "ModSDK.h"
#include "Util/FileUtils.h"
#include "Util/StringUtils.h"
#include <ini.h>
#include <algorithm>
#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>

ModSetting* ModConfigSection::FindSetting(std::string_view p_Name)
{
    auto s_SettingIt = find_if(begin(Settings), end(Settings), [p_Name](const std::pair<std::string, ModSetting>& p_Pair) {
        return Util::StringUtils::CompareInsensitive(p_Pair.first, p_Name);
    });

    if (s_SettingIt == end(Settings))
        return nullptr;

    return &s_SettingIt->second;
}

ModSetting* ModConfigSection::FindSettingDeep(std::string_view p_Name)
{
    auto s_Section = this;
    auto s_Tokens = Util::StringUtils::Split(p_Name, ".");

    for (auto s_Idx = 0; s_Idx < s_Tokens.size(); ++s_Idx) {
        if (s_Idx < s_Tokens.size() - 1) {
            // Find and navigate to subsection.
            auto s_It = find_if(begin(s_Section->Subsections), end(s_Section->Subsections), [&s_Tokens, s_Idx](const ModConfigSection& p_Section) {
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

ModConfigManager::~ModConfigManager()
{
    SaveModConfigurations();
}

void ModConfigManager::LoadConfiguration()
{
    m_ModConfigs = LoadModConfigurations();
}

bool ModConfigManager::IsModAvailable(const std::string& p_Name) const
{
    auto s_ModLoader = ModSDK::GetInstance()->GetModLoader();

    return s_ModLoader && s_ModLoader->IsModAvailable(p_Name);
}

std::vector<ModConfig> ModConfigManager::LoadModConfigurations()
{
    const auto s_IniPath = absolute(Util::FileUtils::GetExecutablePath().parent_path() / "mods.ini");

    if (!is_regular_file(s_IniPath))
        return {};

    mINI::INIFile s_File(s_IniPath.string());
    mINI::INIStructure s_Ini;
    
    if (!s_File.read(s_Ini))
        return {}; // TODO: throw exception?

    std::vector<ModConfig> s_Configuration;

    for (const auto& s_Section : s_Ini) {
        const auto s_SplitIniSectionKey = Util::StringUtils::Split(s_Section.first, ".");
        const auto& s_ModName = s_SplitIniSectionKey[0];

        // Find or add entry for mod configuration.
        auto s_It = find_if(s_Configuration.begin(), s_Configuration.end(), [&s_ModName](const ModConfig& v) {
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
                auto s_It = find_if(begin(s_ModConfigSection->Subsections), end(s_ModConfigSection->Subsections), [&s_Key](const ModConfigSection& p_Section) {
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

void ModConfigManager::EnableMod(std::string_view p_Name)
{
    auto s_Config = GetModConfiguration(p_Name);

    Lock();

    if (!s_Config)
        s_Config = &m_ModConfigs.emplace_back();

    s_Config->Enabled = true;
    s_Config->Config.Name = p_Name;

    Unlock();
}

void ModConfigManager::DisableMod(std::string_view p_Name)
{
    auto s_Config = GetModConfiguration(p_Name);

    Lock();
    if (!s_Config)
        s_Config = &m_ModConfigs.emplace_back();

    s_Config->Enabled = false;
    s_Config->Config.Name = p_Name;
    Unlock();
}

std::vector<ModConfig*> ModConfigManager::GetEnabledModConfigs()
{
    auto s_EnabledMods = std::vector<ModConfig*>{};

    LockRead();

    for (auto& s_Mod : m_ModConfigs) {
        // Ignore the SDK entry. It's used for configuring the SDK itself.
        if (Util::StringUtils::CompareInsensitive(s_Mod.Config.Name, "sdk"))
            continue;

        if (!s_Mod.Enabled)
            continue;

        if (!IsModAvailable(s_Mod.Config.Name))
            continue;

        s_EnabledMods.push_back(&s_Mod);
    }

    UnlockRead();

    return s_EnabledMods;
}

ModConfig* ModConfigManager::GetModConfiguration(std::string_view p_Name)
{
    std::shared_lock s_Lock(m_Mutex);

    auto s_It = find_if(begin(m_ModConfigs), end(m_ModConfigs), [p_Name](const ModConfig& p_Config) {
        return Util::StringUtils::CompareInsensitive(p_Config.Config.Name, p_Name);
    });

    if (s_It == end(m_ModConfigs))
        return nullptr;

    return &*s_It;
}

ModConfig* ModConfigManager::GetLoadedModConfiguration(IPluginInterface* p_Mod) const
{
    auto s_ModLoader = ModSDK::GetInstance()->GetModLoader();

    if (!s_ModLoader)
        return nullptr;

    return s_ModLoader->GetModConfiguration(p_Mod);
}

ModConfig* ModConfigManager::GetLoadedModConfigurationByName(const std::string& p_Name) const
{
    auto s_ModLoader = ModSDK::GetInstance()->GetModLoader();

    if (!s_ModLoader)
        return nullptr;

    return s_ModLoader->GetModConfigurationByName(p_Name);
}

ModSetting* ModConfigManager::GetSDKSetting(std::string_view p_Name)
{
    auto s_Configuration = GetModConfiguration("sdk");

    if (!s_Configuration)
        return nullptr;

    return s_Configuration->Config.FindSetting(p_Name);
}

ModSetting* ModConfigManager::GetModSetting(IPluginInterface* p_Mod, std::string_view p_Name)
{
    auto s_Configuration = GetLoadedModConfiguration(p_Mod);

    if (!s_Configuration)
        return nullptr;

    return s_Configuration->Config.FindSettingDeep(p_Name);
}

ModSetting* ModConfigManager::GetOrCreateModSetting(IPluginInterface* p_Mod, std::string_view p_Name)
{
    auto s_Configuration = GetLoadedModConfiguration(p_Mod);

    if (!s_Configuration)
        return nullptr;

    std::unique_lock s_Lock(m_Mutex);

    auto s_Section = &s_Configuration->Config;
    auto s_SplitKey = Util::StringUtils::Split(p_Name, ".");

    for (auto s_Idx = 0; s_Idx < s_SplitKey.size(); ++s_Idx) {
        if (s_Idx < s_SplitKey.size() - 1) {
            // Find/create and navigate to subsection.
            auto s_It = find_if(begin(s_Section->Subsections), end(s_Section->Subsections), [&s_SplitKey, s_Idx](const ModConfigSection& p_Section) {
                return Util::StringUtils::CompareInsensitive(p_Section.Name, s_SplitKey[s_Idx]);
            });

            if (s_It == s_Section->Subsections.end()) {
                s_Section = &s_Section->Subsections.emplace_back();
                continue;
            }

            s_Section = &*s_It;
            continue;
        }

        // Last part of the key, find/create the setting.
        auto s_It = find_if(begin(s_Section->Settings), end(s_Section->Settings), [&s_SplitKey, s_Idx](const std::pair<std::string, ModSetting>& p_Pair) {
            return Util::StringUtils::CompareInsensitive(p_Pair.first, s_SplitKey[s_Idx]);
        });

        if (s_It != s_Section->Settings.end())
            return &s_It->second;

        return &s_Section->Settings.emplace_back(s_SplitKey[s_Idx], ModSetting{}).second;
    }

    return nullptr;
}

void ModConfigManager::SaveModConfigurations()
{
    // Persist the mods to the ini file.
    auto s_ExePath = Util::FileUtils::GetExecutablePath();
    if (s_ExePath.empty())
        return;

    auto s_ExeDir = s_ExePath.parent_path();

    const auto s_IniPath = absolute(s_ExeDir / "mods.ini");

    std::thread([s_IniPath]() {

    }).detach();

    mINI::INIFile s_File(s_IniPath.string());

    mINI::INIStructure s_Ini;

    LockRead();

    for (const auto& s_Config : m_ModConfigs) {
        // Use loaded mod's settings or user config.
        auto s_LoadedConfig = GetLoadedModConfigurationByName(s_Config.Config.Name);
        WriteModConfigurationToIni(s_Ini, s_LoadedConfig ? *s_LoadedConfig : s_Config);
    }

    UnlockRead();

    s_File.generate(s_Ini, true);
}

void ModConfigManager::LoadModConfigurationFromIniSection(ModConfigSection& p_ModConfiguration, const mINI::INIMap<std::string>& p_IniSection)
{
    for (const auto& s_Entry : p_IniSection) {
        auto s_Setting = p_ModConfiguration.FindSetting(s_Entry.first);

        if (!s_Setting)
            p_ModConfiguration.Settings.emplace_back(s_Entry.first, ModSetting(s_Entry.second));
        else
            s_Setting->Set(s_Entry.second);
    }
}

void ModConfigManager::WriteModConfigurationToIni(mINI::INIStructure& p_Ini, const ModConfig& p_ModConfiguration)
{
    mINI::INIMap<std::string> s_IniMap;

    // Write all settings under main section, starting with the 'enable' setting.
    s_IniMap.set("enable", p_ModConfiguration.Enabled ? "yes" : "no");

    WriteModConfigurationSettingsToIni(s_IniMap, p_ModConfiguration.Config);

    p_Ini.set(p_ModConfiguration.Config.Name, s_IniMap);

    // Write any mod config subsections.
    WriteModConfigurationSubsectionsToIni(p_Ini, p_ModConfiguration.Config.Name, p_ModConfiguration.Config);
}

void ModConfigManager::WriteModConfigurationSubsectionsToIni(mINI::INIStructure& p_Ini, const std::string& p_ParentKey, const ModConfigSection& p_Section)
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

void ModConfigManager::WriteModConfigurationSettingsToIni(mINI::INIMap<std::string>& p_Ini, const ModConfigSection& p_Section)
{
    for (const auto& s_Setting : p_Section.Settings) {
        // Skip 'enable' setting as we write that first.
        if (Util::StringUtils::CompareInsensitive(s_Setting.first, "enable"))
            continue;

        p_Ini.set(s_Setting.first, s_Setting.second.Get());
    }
}
