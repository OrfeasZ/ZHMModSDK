#include "SkipIntro.h"

#include "Hooks.h"
#include "Logging.h"

void SkipIntro::Init() {
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &SkipIntro::OnLoadScene);
}

DEFINE_PLUGIN_DETOUR(SkipIntro, bool, OnLoadScene, ZEntitySceneContext* th, SSceneInitParameters& p_parameters) {
    Logger::Debug("Loading scene: {}", p_parameters.m_SceneResource);

    for (auto& s_Brick : p_parameters.m_aAdditionalBrickResources)
        Logger::Debug("+ With brick: {}", s_Brick);

    if (p_parameters.m_SceneResource == "assembly:/_PRO/Scenes/Frontend/Boot.entity")
        p_parameters.m_SceneResource = "assembly:/_PRO/Scenes/Frontend/MainMenu.entity";

    return HookResult<bool>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(SkipIntro);
