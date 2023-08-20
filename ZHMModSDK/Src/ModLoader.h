#pragma once

#include <charconv>
#include <filesystem>
#include <optional>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <Windows.h>
#include <ini.h>
#include "ISetting.h"

class IPluginInterface;

class ModSetting : public ISetting
{
public:
    ModSetting() noexcept = default;

    ModSetting(std::string p_Value)
    {
        m_Value = std::move(p_Value);
    }

    ~ModSetting();

    const std::string& Get() const
    {
        return m_Value;
    }

    void Set(std::string p_Value)
    {
        m_Value = std::move(p_Value);
    }

    void Set(bool p_Value)
    {
        m_Value = p_Value ? "yes" : "no";
    }

    template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> && std::is_integral_v<T> && !std::is_same_v<T, bool>>> 
    void Set(T p_Value)
    {
        m_Value = std::format("{}", p_Value);
    }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
    std::optional<T> AsInt(std::optional<T> p_Default = std::nullopt) const
    {
        return AsNumber<T>(p_Default);
    }

    template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    std::optional<T> AsFloat(std::optional<T> p_Default = std::nullopt) const
    {
        return AsNumber<T>(p_Default);
    }

    template<typename T, typename = std::enable_if_t<(std::is_integral_v<T> || std::is_floating_point_v<T>) && !std::is_same_v<T, bool>>>
    std::optional<T> AsNumber(std::optional<T> p_Default = std::nullopt) const
    {
        if (!m_Value.empty()) {
            T value;
            auto result = std::from_chars(m_Value.data(), m_Value.data() + m_Value.size(), value);
            if (result.ec == std::errc())
                return value;
        }
        return p_Default;
    }

    bool AsBool() const
    {
        if (m_Value.empty())
            return false;

        // Assume f(alse)/n(o)/o(ff)/0 based on first character.
        switch (std::tolower(m_Value[0])) {
            case 'f':
            case 'n':
            case '0':
                return false;
            case 'o':
                // Except 'o' could be 'on' so check for that.
                return m_Value.size() > 1 && m_Value[1] == 'n';
        }

        return true;
    }

    bool Read(ZString& p_Out) override;
    bool ReadBool(bool& p_Out) override;
    bool ReadInt(int32& p_Out) override;
    bool ReadUnsignedInt(uint32& p_Out) override;
    bool ReadDouble(float64& p_Out) override;

private:
    std::string m_Value;
};

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
