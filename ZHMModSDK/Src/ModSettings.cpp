#include "ModSettings.h"

#include <filesystem>
#include <utility>
#include <ini.h>

ModSettings::ModSettings(std::string  p_ModName, std::filesystem::path  p_ModDirectory) :
	m_ModName(std::move(p_ModName)),
	m_ModDirectory(std::move(p_ModDirectory))
{
	Reload();
}

ModSettings::~ModSettings() {
	Save();
}

void ModSettings::Reload() {
	m_Settings.clear();

	std::filesystem::path s_SettingsIniPath = m_ModDirectory / (m_ModName + ".ini");

	if (!std::filesystem::exists(s_SettingsIniPath)) {
		return;
	}

	mINI::INIFile s_File(s_SettingsIniPath.string());

	mINI::INIStructure s_Ini;

	s_File.read(s_Ini);

	for (auto const& it: s_Ini) {
		auto const& s_SectionName = it.first;
		auto const& s_SectionItems = it.second;

		std::unordered_map<std::string, std::string> s_Section;

		for (auto const& it2: s_SectionItems) {
			s_Section[it2.first] = it2.second;
		}

		m_Settings[s_SectionName] = s_Section;
	}
}

void ModSettings::Save() {
	std::filesystem::path s_SettingsIniPath = m_ModDirectory / (m_ModName + ".ini");

	mINI::INIFile s_File(s_SettingsIniPath.string());

	mINI::INIStructure s_Ini;

	for (auto const& it: m_Settings) {
		auto const& s_SectionName = it.first;
		auto const& s_SectionItems = it.second;

		mINI::INIMap<std::string> s_Section;

		for (auto const& it2: s_SectionItems) {
			s_Section.set(it2.first, it2.second);
		}

		s_Ini.set(s_SectionName, s_Section);
	}

	s_File.generate(s_Ini, true);
}

bool ModSettings::HasSetting(const std::string& p_Section, const std::string& p_Name) {
	return m_Settings.find(p_Section) != m_Settings.end() && m_Settings[p_Section].find(p_Name) != m_Settings[p_Section].end();
}

std::string ModSettings::GetSetting(const std::string& p_Section, const std::string& p_Name, const std::string& p_DefaultValue) {
	if (!HasSetting(p_Section, p_Name)) {
		return p_DefaultValue;
	}

	return m_Settings[p_Section][p_Name];
}

void ModSettings::SetSetting(const std::string& p_Section, const std::string& p_Name, const std::string& p_Value) {
	if (m_Settings.find(p_Section) == m_Settings.end()) {
		m_Settings[p_Section] = std::unordered_map<std::string, std::string>();
	}

	m_Settings[p_Section][p_Name] = p_Value;
	Save();
}

void ModSettings::RemoveSetting(const std::string& p_Section, const std::string& p_Name) {
	if (m_Settings.find(p_Section) == m_Settings.end()) {
		return;
	}

	if (m_Settings[p_Section].find(p_Name) == m_Settings[p_Section].end()) {
		return;
	}

	m_Settings[p_Section].erase(p_Name);
	Save();
}