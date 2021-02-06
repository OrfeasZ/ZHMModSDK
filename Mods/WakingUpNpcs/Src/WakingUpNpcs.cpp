#include "WakingUpNpcs.h"


#include "Events.h"
#include "Globals.h"
#include "Logging.h"

#include "SGameUpdateEvent.h"

WakingUpNpcs::~WakingUpNpcs()
{
	const ZMemberDelegate<WakingUpNpcs, void(const SGameUpdateEvent&)> s_Delegate(this, &WakingUpNpcs::OnFrameUpdate);
	Hooks::ZGameLoopManager_UnregisterFrameUpdate->Call(Globals::GameLoopManager, s_Delegate, 0, EUpdateMode::eUpdatePlayMode);
}

void WakingUpNpcs::Init()
{
	Logger::Info("Hello world!");

	Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &OnLoadScene);
	Events::OnConsoleCommand->AddListener(this, &OnConsoleCommand);
}

void WakingUpNpcs::OnEngineInitialized()
{
	Logger::Debug("Engine was initialized!");

	const ZMemberDelegate<WakingUpNpcs, void(const SGameUpdateEvent&)> s_Delegate(this, &WakingUpNpcs::OnFrameUpdate);
	Hooks::ZGameLoopManager_RegisterFrameUpdate->Call(Globals::GameLoopManager, s_Delegate, 0, EUpdateMode::eUpdatePlayMode);
}

void WakingUpNpcs::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
	Logger::Trace("On frame update. Real dt = {}. Sim dt = {}", p_UpdateEvent.m_RealTimeDelta.ToSeconds(), p_UpdateEvent.m_GameTimeDelta.ToSeconds());
}

DECLARE_PLUGIN_DETOUR(WakingUpNpcs, void, OnLoadScene, ZEntitySceneContext* th, const ZSceneData& sceneData)
{
	Logger::Debug("Loading scene: {}", sceneData.m_sceneName.c_str());
	return HookResult<void>(HookAction::Continue{});
}

DECLARE_PLUGIN_LISTENER(WakingUpNpcs, OnConsoleCommand)
{
	Logger::Debug("On console command!");
}

DECLARE_ZHM_PLUGIN(WakingUpNpcs);