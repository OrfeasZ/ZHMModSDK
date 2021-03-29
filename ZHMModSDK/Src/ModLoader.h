#pragma once

#include <filesystem>
#include <unordered_map>
#include <Windows.h>

class IPluginInterface;

class ModLoader
{
private:
	struct LoadedMod
	{
		HMODULE Module;
		IPluginInterface* PluginInterface;
	};

public:
	ModLoader();
	~ModLoader();

public:
	void Startup();
	void LoadAllMods();
	void LoadMod(const std::string& p_Name);
	void UnloadMod(const std::string& p_Name);
	void ReloadMod(const std::string& p_Name);
	void UnloadAllMods();
	void ReloadAllMods();
	IPluginInterface* GetModByName(const std::string& p_Name);

	std::vector<IPluginInterface*> GetLoadedMods() const
	{
		return m_ModList;
	}
	
private:
	std::vector<IPluginInterface*> m_ModList;
	std::unordered_map<std::string, LoadedMod> m_LoadedMods;
};
