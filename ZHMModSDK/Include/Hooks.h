#pragma once

#include "Common.h"
#include "EUpdateMode.h"
#include "Hook.h"
#include "ZDelegate.h"
#include "ZScene.h"

class ZActor;
class ZComponentCreateInfo;
class SGameUpdateEvent;
class ZGameLoopManager;

class ZHMSDK_API Hooks
{
public:
	static Hook<void(ZActor*, ZComponentCreateInfo*)>* ZActor_ZActor;
	static Hook<void(ZEntitySceneContext*, const ZSceneData&)>* ZEntitySceneContext_LoadScene;
	static Hook<void(ZGameLoopManager*, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)>* ZGameLoopManager_RegisterFrameUpdate;
};
