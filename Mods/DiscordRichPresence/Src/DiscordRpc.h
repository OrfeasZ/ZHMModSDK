#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"
#include "DiscordClient.h"

class DiscordRpc : public IPluginInterface
{
public:
	~DiscordRpc() override;
	void PreInit() override;

private:
	void PopulateScenes();
	void PopulateGameModes();
	void PopulateCodenameHints();
	std::string LowercaseString(const std::string& p_In);
	std::string FindLocationForScene(ZString p_Scene);

private:
	DEFINE_PLUGIN_DETOUR(DiscordRpc, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);

private:
	std::unordered_map<std::string_view, const char*> m_Scenes;
	std::unordered_map<std::string_view, const char*> m_GameModes;
	std::unordered_map<std::string_view, const char*> m_CodenameHints;
	DiscordClient m_DiscordClient;
};

DEFINE_ZHM_PLUGIN(DiscordRpc)
