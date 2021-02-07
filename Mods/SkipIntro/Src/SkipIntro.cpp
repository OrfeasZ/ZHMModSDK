#include "SkipIntro.h"

#include "Hooks.h"
#include "Logging.h"
#include "ZScene.h"

void SkipIntro::Init()
{
	Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &SkipIntro::OnLoadScene);
}

DECLARE_PLUGIN_DETOUR(SkipIntro, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData)
{
	if (std::string(p_SceneData.m_sceneName.c_str()) == "assembly:/_PRO/Scenes/Frontend/Boot.entity")
		p_SceneData.m_sceneName = "assembly:/_PRO/Scenes/Frontend/MainMenu.entity";
	
	return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(SkipIntro);