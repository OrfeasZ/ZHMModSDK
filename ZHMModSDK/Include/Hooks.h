#pragma once

#include <Windows.h>

#include "Common.h"
#include "Enums.h"
#include "EUpdateMode.h"
#include "Hook.h"
#include "ZDelegate.h"
#include "ZScene.h"

class ZActor;
class ZComponentCreateInfo;
class SGameUpdateEvent;
class ZGameLoopManager;
class ZKnowledge;
class ZActorManager;
class ZApplicationEngineWin32;

class ZHMSDK_API Hooks
{
public:
	static Hook<void(ZActor*, ZComponentCreateInfo*)>* ZActor_ZActor;
	static Hook<void(ZEntitySceneContext*, const ZSceneData&)>* ZEntitySceneContext_LoadScene;
	static Hook<void(ZGameLoopManager*, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)>* ZGameLoopManager_RegisterFrameUpdate;
	static Hook<void(ZGameLoopManager*, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)>* ZGameLoopManager_UnregisterFrameUpdate;
	static Hook<bool(void*, void*)>* Engine_Init;
	static Hook<void(ZKnowledge*, EGameTension)>* ZKnowledge_SetGameTension;
	static Hook<void(ZActorManager*, EAISharedEventType, bool)>* ZActorManager_SetHitmanSharedEvent;
	static Hook<bool(const ZString&, bool)>* GetApplicationOptionBool;
	static Hook<void(ZApplicationEngineWin32*, bool)>* ZApplicationEngineWin32_OnMainWindowActivated;
	static Hook<LRESULT(ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM)>* ZApplicationEngineWin32_MainWindowProc;
};
