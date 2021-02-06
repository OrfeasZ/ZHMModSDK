#pragma once

#include "IPluginInterface.h"

class WakingUpNpcs : public IPluginInterface
{
public:
	void Init() override;
	void OnEngineInitialized() override;

private:
	DEFINE_PLUGIN_DETOUR(WakingUpNpcs, void, OnLoadScene, ZEntitySceneContext*, const ZSceneData&)
};

DEFINE_ZHM_PLUGIN(WakingUpNpcs)