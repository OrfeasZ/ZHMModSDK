#pragma once

#include <filesystem>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <Windows.h>
#include <ini.h>
#include "ModSettings.h"

class IPluginInterface;

class ModLoader
{
private:
    struct ModConfigurationSection
    {
        std::string Name;
        std::vector<std::pair<std::string, ModSetting>> Settings;
        std::vector<ModConfigurationSection> Subsections;
    };

    struct ModConfiguration
    {
        ModConfigurationSection Config;
        bool Enabled = false;
    };

    struct LoadedMod
    {
        HMODULE Module;
        IPluginInterface* PluginInterface;
        ModConfiguration Configuration;
    };

public:
    ModLoader();
    ~ModLoader();

public:
    void Startup();
    void LoadAllMods();
    void ScanAvailableMods();
    std::unordered_set<std::string> GetAvailableMods();
    std::unordered_set<std::string> GetActiveMods();
    void SetActiveMods(const std::unordered_set<std::string>& p_Mods);
    void LoadMod(const std::string& p_Name, bool p_LiveLoad, ModConfiguration p_Configuration = {});
    void UnloadMod(const std::string& p_Name);
    void ReloadMod(const std::string& p_Name);
    void UnloadAllMods();
    void ReloadAllMods();
    IPluginInterface* GetModByName(const std::string& p_Name);
    ModSetting* GetModSetting(IPluginInterface* p_Mod, std::string_view p_Name);
    ModSetting* GetOrCreateModSetting(IPluginInterface* p_Mod, std::string_view p_Name);
    void SaveModConfigurations();

    std::vector<IPluginInterface*> GetLoadedMods() const
    {
        return m_ModList;
    }

    void LockRead()
    {
        m_Mutex.lock_shared();
    }

    void UnlockRead()
    {
        m_Mutex.unlock_shared();
    }

    void Lock()
    {
        m_Mutex.lock();
    }

    void Unlock()
    {
        m_Mutex.unlock();
    }

protected:
    std::vector<ModConfiguration> LoadModConfigurations();
    void LoadModConfigurationFromIniSection(ModConfigurationSection& p_ModConfiguration, const mINI::INIMap<std::string>& p_IniMap);
    void WriteModConfigurationToIni(mINI::INIStructure& p_Ini, const ModConfiguration& p_ModConfiguration);
    void WriteModConfigurationSubsectionsToIni(mINI::INIStructure& p_Ini, const std::string& p_ParentKey, const ModConfigurationSection& p_Section);
    void WriteModConfigurationSettingsToIni(mINI::INIMap<std::string>& p_Ini, const ModConfigurationSection& p_Section);

private:
    std::unordered_set<std::string> m_AvailableMods;
    std::unordered_set<std::string> m_AvailableModsLower;
    std::vector<IPluginInterface*> m_ModList;
    std::unordered_map<std::string, LoadedMod> m_LoadedMods;
    std::shared_mutex m_Mutex;
    std::shared_mutex m_ConfigMutex;
    std::vector<ModConfiguration> m_ModConfigurations;
};
