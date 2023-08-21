#pragma once

#include <filesystem>
#include <shared_mutex>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <Windows.h>
#include <ini.h>
#include "ModSettings.h"

class IPluginInterface;

struct ModConfigSection
{
    std::string Name;
    std::vector<std::pair<std::string, ModSetting>> Settings;
    std::vector<ModConfigSection> Subsections;

    ModSetting* FindSetting(std::string_view p_Name);
    ModSetting* FindSettingDeep(std::string_view p_Name);
};

struct ModConfig
{
    ModConfigSection Config;
    bool Enabled = false;
};

class ModConfigManager
{
public:
    ModConfigManager() noexcept = default;
    ~ModConfigManager();

public:
    void LoadConfiguration();
    void SaveModConfigurations();
    void EnableMod(std::string_view p_Name);
    void DisableMod(std::string_view p_Name);
    std::vector<ModConfig*> GetEnabledModConfigs();
    ModConfig* GetModConfiguration(std::string_view p_Name);
    ModConfig* GetLoadedModConfiguration(IPluginInterface* p_Mod) const;
    ModConfig* GetLoadedModConfigurationByName(const std::string& p_Name) const;
    ModSetting* GetSDKSetting(std::string_view p_Name);
    ModSetting* GetModSetting(IPluginInterface* p_Mod, std::string_view p_Name);
    ModSetting* GetOrCreateModSetting(IPluginInterface* p_Mod, std::string_view p_Name);

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
    bool IsModAvailable(const std::string& p_Name) const;
    std::vector<ModConfig> LoadModConfigurations();
    void LoadModConfigurationFromIniSection(ModConfigSection& p_ModConfiguration, const mINI::INIMap<std::string>& p_IniMap);
    void WriteModConfigurationToIni(mINI::INIStructure& p_Ini, const ModConfig& p_ModConfiguration);
    void WriteModConfigurationSubsectionsToIni(mINI::INIStructure& p_Ini, const std::string& p_ParentKey, const ModConfigSection& p_Section);
    void WriteModConfigurationSettingsToIni(mINI::INIMap<std::string>& p_Ini, const ModConfigSection& p_Section);

private:
    std::shared_mutex m_Mutex;
    std::vector<ModConfig> m_ModConfigs;
};
