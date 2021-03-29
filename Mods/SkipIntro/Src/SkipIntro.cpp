#include "SkipIntro.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZScene.h>

void SkipIntro::PreInit()
{
	Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &SkipIntro::OnLoadScene);
}

DECLARE_PLUGIN_DETOUR(SkipIntro, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData)
{
	Logger::Debug("Loading scene: {}", p_SceneData.m_sceneName.c_str());

	for (auto& s_Brick : p_SceneData.m_sceneBricks)
		Logger::Debug("+ With brick: {}", s_Brick.c_str());

	if (std::string(p_SceneData.m_sceneName.c_str()) == "assembly:/_PRO/Scenes/Frontend/Boot.entity")
		p_SceneData.m_sceneName = "assembly:/_PRO/Scenes/Frontend/MainMenu.entity";

	return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(SkipIntro);
