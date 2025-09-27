#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
#include <shared_mutex>

class ModSettings {
public:
    ModSettings(std::string p_ModName, std::filesystem::path p_ModDirectory);
    ~ModSettings();

public:
    void Reload();
    void Save();
    bool HasSetting(const std::string& p_Section, const std::string& p_Name);
    std::string GetSetting(const std::string& p_Section, const std::string& p_Name, const std::string& p_DefaultValue);
    void SetSetting(const std::string& p_Section, const std::string& p_Name, const std::string& p_Value);
    void RemoveSetting(const std::string& p_Section, const std::string& p_Name);

private:
    std::string m_ModName;
    std::filesystem::path m_ModDirectory;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_Settings;
    std::shared_mutex m_SettingsMutex;
};