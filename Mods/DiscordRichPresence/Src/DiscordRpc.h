#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

class DiscordRpc : public IPluginInterface
{
public:
	~DiscordRpc() override;
	void PreInit() override;

private:
	void PopulateScenes();
	void PopulateGameModes();
	void PopulateCodenameHints();
	std::string lowercaseString(std::string in);
	std::string findLocationForScene(ZString scene);

private:
	DEFINE_PLUGIN_DETOUR(DiscordRpc, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);

private:
	std::unordered_map<std::string_view, const char*> m_scenes;
	std::unordered_map<std::string_view, const char*> m_gameModes;
	std::unordered_map<std::string_view, const char*> m_codenameHints;
};

DEFINE_ZHM_PLUGIN(DiscordRpc)
