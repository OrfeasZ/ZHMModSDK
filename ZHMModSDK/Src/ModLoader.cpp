#include "ModLoader.h"

#include <Windows.h>
#include <filesystem>

#include "EventDispatcherImpl.h"
#include "HookImpl.h"
#include "IPluginInterface.h"
#include "Logging.h"
#include "Util/StringUtils.h"
#include "UI/ModSelector.h"
#include <ini.h>

ModLoader::ModLoader()
{

}

ModLoader::~ModLoader()
{
	Hooks::Engine_Init->RemoveDetoursWithContext(this);
	UnloadAllMods();
}

void ModLoader::Startup()
{
	LoadAllMods();
}

void ModLoader::ScanAvailableMods()
{
	m_AvailableMods.clear();
	m_AvailableModsLower.clear();
	
	// Discover and load mods.
	char s_ExePathStr[MAX_PATH];
	auto s_PathSize = GetModuleFileNameA(nullptr, s_ExePathStr, MAX_PATH);

	if (s_PathSize == 0)
		return;

	std::filesystem::path s_ExePath(s_ExePathStr);
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

	for (auto& s_LoadedMod : m_LoadedMods)
		s_Mods.insert(s_LoadedMod.first);

	return s_Mods;
}

void ModLoader::SetActiveMods(const std::unordered_set<std::string>& p_Mods)
{
	std::unordered_set<std::string> s_LowerModNames;

	for (auto& s_Mod : p_Mods)
		s_LowerModNames.insert(Util::StringUtils::ToLowerCase(s_Mod));
	
	// First unload any mods that don't exist in the new list.
	std::vector<std::string> s_ModsToUnload;

	for (auto& s_Pair : m_LoadedMods)
		if (!s_LowerModNames.contains(s_Pair.first))
			s_ModsToUnload.push_back(s_Pair.first);

	for (auto& s_Mod : s_ModsToUnload)
		UnloadMod(s_Mod);

	// Then load any mods that aren't already loaded.
	for (auto& s_Mod : s_LowerModNames)
	{
		// Mod is already loaded; skip.
		if (m_LoadedMods.find(s_Mod) != m_LoadedMods.end())
			continue;

		LoadMod(s_Mod, true);
	}

	// And persist the mods to the ini file.
	char s_ExePathStr[MAX_PATH];
	auto s_PathSize = GetModuleFileNameA(nullptr, s_ExePathStr, MAX_PATH);

	if (s_PathSize == 0)
		return;

	std::filesystem::path s_ExePath(s_ExePathStr);
	auto s_ExeDir = s_ExePath.parent_path();

	const auto s_IniPath = absolute(s_ExeDir / "mods.ini");

	mINI::INIFile s_File(s_IniPath.string());
	mINI::INIStructure s_Ini;

	for (auto& s_Mod : p_Mods)
	{
		mINI::INIMap<std::string> s_EmptyMap;
		s_Ini.set(s_Mod, s_EmptyMap);
	}

	s_File.generate(s_Ini, true);

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
	char s_ExePathStr[MAX_PATH];
	auto s_PathSize = GetModuleFileNameA(nullptr, s_ExePathStr, MAX_PATH);

	if (s_PathSize == 0)
		return;

	std::filesystem::path s_ExePath(s_ExePathStr);
	auto s_ExeDir = s_ExePath.parent_path();

	const auto s_IniPath = absolute(s_ExeDir / "mods.ini");

	mINI::INIFile s_File(s_IniPath.string());
	mINI::INIStructure s_Ini;

	// now we can read the file
	s_File.read(s_Ini);

	for (auto& s_Mod : s_Ini)
	{
		if (m_AvailableModsLower.contains(Util::StringUtils::ToLowerCase(s_Mod.first)))
		{
			LoadMod(s_Mod.first, false);
		}
	}

	ModSDK::GetInstance()->GetUIModSelector()->UpdateAvailableMods(m_AvailableMods, GetActiveMods());
}

void ModLoader::LoadMod(const std::string& p_Name, bool p_LiveLoad)
{
	const std::string s_Name = Util::StringUtils::ToLowerCase(p_Name);

	if (m_LoadedMods.find(s_Name) != m_LoadedMods.end())
	{
		Logger::Warn("A mod with the same name ({}) is already loaded. Skipping.", p_Name);
		return;
	}

	char s_ExePathStr[MAX_PATH];
	auto s_PathSize = GetModuleFileNameA(nullptr, s_ExePathStr, MAX_PATH);

	if (s_PathSize == 0)
		return;

	std::filesystem::path s_ExePath(s_ExePathStr);
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

	m_LoadedMods[s_Name] = s_Mod;
	m_ModList.push_back(s_PluginInterface);

	ModSDK::GetInstance()->OnModLoaded(s_Name, s_PluginInterface, p_LiveLoad);
}

void ModLoader::UnloadMod(const std::string& p_Name)
{
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
	const std::string s_Name = Util::StringUtils::ToLowerCase(p_Name);

	auto it = m_LoadedMods.find(s_Name);

	if (it == m_LoadedMods.end())
	{
		Logger::Warn("Could not find mod '{}' to reload.", p_Name);
		return;
	}

	UnloadMod(p_Name);
	LoadMod(p_Name, true);
}

void ModLoader::UnloadAllMods()
{
	std::vector<std::string> s_ModNames;

	for (auto& s_Pair : m_LoadedMods)
		s_ModNames.push_back(s_Pair.first);

	for (auto& s_Mod : s_ModNames)
		UnloadMod(s_Mod);
}

void ModLoader::ReloadAllMods()
{
	std::vector<std::string> s_ModNames;

	for (auto& s_Pair : m_LoadedMods)
		s_ModNames.push_back(s_Pair.first);

	for (auto& s_Mod : s_ModNames)
		ReloadMod(s_Mod);
}

IPluginInterface* ModLoader::GetModByName(const std::string& p_Name)
{
	std::string s_Name = Util::StringUtils::ToLowerCase(p_Name);

	auto it = m_LoadedMods.find(s_Name);

	if (it == m_LoadedMods.end())
		return nullptr;

	return it->second.PluginInterface;
}
