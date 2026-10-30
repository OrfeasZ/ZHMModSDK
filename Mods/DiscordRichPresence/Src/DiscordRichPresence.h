#pragma once

#include <unordered_map>

#include "IPluginInterface.h"

namespace discord {
    class Core;
}

class DiscordRichPresence : public IPluginInterface {
public:
    DiscordRichPresence();
    ~DiscordRichPresence() override;

    void Init() override;
    void OnEngineInitialized() override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

    std::string NormalizeAssetKey(std::string p_Name);

    std::string GetActivityImageKey(const std::string& p_GameMode, std::string p_Location, std::string p_Title);

    DECLARE_PLUGIN_DETOUR(DiscordRichPresence, void, ZLevelManager_StartGame, ZLevelManager* th);

    static const std::unordered_map<std::string, std::string> m_TypeToGameMode;

    discord::Core* m_DiscordCore;
};

DECLARE_ZHM_PLUGIN(DiscordRichPresence)
