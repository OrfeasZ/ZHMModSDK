#pragma once

#include <Windows.h>

#include "Hook.h"
#include "Common.h"

#include <Glacier/Enums.h>
#include <Glacier/EUpdateMode.h>
#include <Glacier/ZDelegate.h>
#include <Glacier/ZEntity.h>

class ZActor;
class ZComponentCreateInfo;
class SGameUpdateEvent;
class ZGameLoopManager;
class ZKnowledge;
class ZActorManager;
class ZApplicationEngineWin32;
class ZEntityImpl;
class ZObjectRef;
class ZEntitySceneContext;
class ZSceneData;

class ZHMSDK_API Hooks
{
public:
	static Hook<void(ZActor*, ZComponentCreateInfo*)>* ZActor_ZActor;
	static Hook<void(ZEntitySceneContext*, ZSceneData&)>* ZEntitySceneContext_LoadScene;
	static Hook<void(ZGameLoopManager*, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)>* ZGameLoopManager_RegisterFrameUpdate;
	static Hook<void(ZGameLoopManager*, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)>* ZGameLoopManager_UnregisterFrameUpdate;
	static Hook<bool(void*, void*)>* Engine_Init;
	static Hook<void(ZKnowledge*, EGameTension)>* ZKnowledge_SetGameTension;
	static Hook<void(ZActorManager*, EAISharedEventType, bool)>* ZActorManager_SetHitmanSharedEvent;
	static Hook<bool(const ZString& optionName, bool defaultValue)>* GetApplicationOptionBool;
	static Hook<void(ZApplicationEngineWin32* th, bool activated)>* ZApplicationEngineWin32_OnMainWindowActivated;
	static Hook<LRESULT(ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM)>* ZApplicationEngineWin32_MainWindowProc;
	static Hook<bool(ZEntityRef entity, uint32_t propertyId, const ZObjectRef& value, bool invokeChangeHandlers)>* SetPropertyValue;
	static Hook<bool(ZEntityRef entity, uint32_t propertyId, void* output)>* GetPropertyValue;
	static Hook<bool(ZEntityRef entity, uint32_t pinId, const ZObjectRef& data)>* SignalOutputPin;
	static Hook<bool(ZEntityRef entity, uint32_t pinId, const ZObjectRef& data)>* SignalInputPin;
};
