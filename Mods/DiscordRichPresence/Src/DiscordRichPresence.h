#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

namespace discord
{
	class Core;
}

class DiscordRichPresence : public IPluginInterface
{
public:
	DiscordRichPresence();
	~DiscordRichPresence() override;
	void Init() override;
	void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
	void OnEngineInitialized() override;

private:
	void PopulateScenes();
	void PopulateGameModes();
	void PopulateCodenameHints();
	std::string LowercaseString(const std::string& p_In) const;
	std::string FindLocationForScene(ZString p_Scene) const;

private:
	DEFINE_PLUGIN_DETOUR(DiscordRichPresence, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);

private:
	std::unordered_map<std::string_view, const char*> m_Scenes;
	std::unordered_map<std::string_view, const char*> m_GameModes;
	std::unordered_map<std::string_view, const char*> m_CodenameHints;
	discord::Core* m_DiscordCore;
};

DEFINE_ZHM_PLUGIN(DiscordRichPresence)
