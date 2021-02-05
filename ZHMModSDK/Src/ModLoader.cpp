#include "ModLoader.h"

#include <Windows.h>
#include <filesystem>



#include "EventDispatcherImpl.h"
#include "HookImpl.h"
#include "IPluginInterface.h"
#include "Logging.h"
#include "Util/StringUtils.h"

ModLoader::ModLoader()
{
	
}

ModLoader::~ModLoader()
{
	UnloadAllMods();
}

void ModLoader::LoadAllMods()
{
	// Discover and load mods.
	const auto s_ModPath = std::filesystem::absolute("mods");
	
	if (exists(s_ModPath) && is_directory(s_ModPath))
	{
		Logger::Debug("Looking for mods in '{}'...", s_ModPath.string());
		
		for (const auto& s_Entry : std::filesystem::directory_iterator(s_ModPath))
		{
			if (!s_Entry.is_regular_file())
				continue;

			if (s_Entry.path().extension() != ".dll")
				continue;

			LoadMod(s_Entry.path().stem().string());
		}
	}
	else
	{
		Logger::Warn("Mod directory '{}' not found.", s_ModPath.string());
	}
}

void ModLoader::LoadMod(const std::string& p_Name)
{
	const std::string s_Name = Util::StringUtils::ToLowerCase(p_Name);
	
	if (m_LoadedMods.find(s_Name) != m_LoadedMods.end())
	{
		Logger::Warn("A mod with the same name ({}) is already loaded. Skipping.", p_Name);
		return;
	}

	const auto s_ModulePath = std::filesystem::absolute("mods\\" + p_Name + ".dll");

	if (!exists(s_ModulePath) || !is_regular_file(s_ModulePath))
	{
		Logger::Warn("Could not find mod '{}'.", p_Name);
	}
	
	Logger::Info("Attempting to load mod '{}'.", p_Name);
	Logger::Debug("Module path is '{}'.", s_ModulePath.string().c_str());

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
	
	s_PluginInterface->Init();

	Logger::Info("Initialized mod '{}'.", p_Name);

	LoadedMod s_Mod{};
	s_Mod.Module = s_Module;
	s_Mod.PluginInterface = s_PluginInterface;

	m_LoadedMods[s_Name] = s_Mod;
}

void ModLoader::UnloadMod(const std::string& p_Name)
{
	const std::string s_Name = Util::StringUtils::ToLowerCase(p_Name);

	auto it = m_LoadedMods.find(s_Name);

	if (it == m_LoadedMods.end())
		return;

	Logger::Info("Unloading mod '{}'.", p_Name);

	HookRegistry::ClearPluginDetours(it->second.PluginInterface);
	EventDispatcherRegistry::ClearPluginListeners(it->second.PluginInterface);

	delete it->second.PluginInterface;
	FreeLibrary(it->second.Module);

	m_LoadedMods.erase(it);
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
	LoadMod(p_Name);
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
