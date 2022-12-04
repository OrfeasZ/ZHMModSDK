#pragma once

#include <filesystem>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
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
	void ScanAvailableMods();
	std::unordered_set<std::string> GetAvailableMods();
	std::unordered_set<std::string> GetActiveMods();
	void SetActiveMods(const std::unordered_set<std::string>& p_Mods);
	void LoadMod(const std::string& p_Name, bool p_LiveLoad);
	void UnloadMod(const std::string& p_Name);
	void ReloadMod(const std::string& p_Name);
	void UnloadAllMods();
	void ReloadAllMods();
	IPluginInterface* GetModByName(const std::string& p_Name);

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
	
private:
	std::unordered_set<std::string> m_AvailableMods;
	std::unordered_set<std::string> m_AvailableModsLower;
	std::vector<IPluginInterface*> m_ModList;
	std::unordered_map<std::string, LoadedMod> m_LoadedMods;
	std::shared_mutex m_Mutex;
};
