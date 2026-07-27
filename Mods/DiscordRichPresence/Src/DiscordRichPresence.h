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
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    void OnEngineInitialized() override;

private:
    void BuildSceneMappings();
    void BuildGameModeMappings();

private:
    DECLARE_PLUGIN_DETOUR(DiscordRichPresence, void, ZLevelManager_StartGame, ZLevelManager* th);

private:
    std::unordered_map<std::string, std::string> m_CodeNameHintToSceneName;
    std::unordered_map<std::string, std::string> m_CodeNameHintToTitle;
    std::unordered_map<std::string, std::string> m_TypeToGameMode;
    discord::Core* m_DiscordCore;
};

DECLARE_ZHM_PLUGIN(DiscordRichPresence)
