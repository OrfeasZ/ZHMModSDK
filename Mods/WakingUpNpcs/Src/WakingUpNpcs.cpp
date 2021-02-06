#include "WakingUpNpcs.h"

#include "Logging.h"

void WakingUpNpcs::Init()
{
	Logger::Info("Hello world!");

	Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &OnLoadScene);
}

void WakingUpNpcs::OnEngineInitialized()
{
}

DECLARE_PLUGIN_DETOUR(WakingUpNpcs, void, OnLoadScene, ZEntitySceneContext* th, const ZSceneData& sceneData)
{
	Logger::Debug("Loading scene: {}", sceneData.m_sceneName.c_str());
	return HookResult<void>(HookAction::Continue{});
}

DECLARE_ZHM_PLUGIN(WakingUpNpcs);