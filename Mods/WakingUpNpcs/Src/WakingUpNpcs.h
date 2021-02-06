#pragma once

#include "IPluginInterface.h"

class WakingUpNpcs : public IPluginInterface
{
public:
	~WakingUpNpcs() override;
	void Init() override;
	void OnEngineInitialized() override;

private:
	void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

private:
	DEFINE_PLUGIN_DETOUR(WakingUpNpcs, void, OnLoadScene, ZEntitySceneContext*, const ZSceneData&)
	DEFINE_PLUGIN_LISTENER(WakingUpNpcs, OnConsoleCommand)
};

DEFINE_ZHM_PLUGIN(WakingUpNpcs)