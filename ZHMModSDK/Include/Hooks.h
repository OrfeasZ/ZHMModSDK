#pragma once

#include <Windows.h>
#include <d3d12.h>

#include "Hook.h"
#include "Common.h"

#include <Glacier/Enums.h>
#include <Glacier/EUpdateMode.h>
#include <Glacier/ZDelegate.h>

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
class ZString;
class ZEntityRef;
class ZRenderDevice;
class ZRenderSwapChain;

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
	static Hook<ZRenderDevice* (ZRenderDevice* th)>* ZRenderDevice_ZRenderDevice;
	static Hook<HRESULT(IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice)>* D3D12CreateDevice;
	static Hook<void(ZRenderSwapChain* th, void* a2, bool a3)>* ZRenderSwapChain_Resize;
	static Hook<bool(void*, void*)>* Check_SSL_Cert;
	static Hook<void(ZApplicationEngineWin32* th, const ZString& info, const ZString& details)>* ZApplicationEngineWin32_OnDebugInfo;
};
