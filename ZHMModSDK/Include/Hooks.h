#pragma once

#include <Windows.h>
#include <d3d12.h>

#include "Hook.h"
#include "Common.h"

#include <Glacier/Enums.h>
#include <Glacier/EUpdateMode.h>
#include <Glacier/ZDelegate.h>

#include "Glacier/TArray.h"
#include "Glacier/THashMap.h"

class ZRuntimeResourceID;
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
class ZKeyboardWindows;
class ZRenderGraphNodeCamera;
class ZPackageManagerPackage;
class ZGameUIManagerEntity;
class ZGameStatsManager;
class ZRenderContext;
class ZDynamicObject;
class ZAchievementManagerSimple;
class SOnlineEvent;
class ZUpdateEventContainer;
class ZInputAction;
class ZEntityManager;
class ZHttpResultDynamicObject;

class ZHMSDK_API Hooks
{
public:
	static Hook<void(ZActor*, ZComponentCreateInfo*)>* ZActor_ZActor;
	static Hook<void(ZEntitySceneContext*, ZSceneData&)>* ZEntitySceneContext_LoadScene;
	static Hook<void(ZEntitySceneContext*, bool fullyClear)>* ZEntitySceneContext_ClearScene;
	static Hook<void(ZUpdateEventContainer*, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)>* ZUpdateEventContainer_AddDelegate;
	static Hook<void(ZUpdateEventContainer*, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)>* ZUpdateEventContainer_RemoveDelegate;
	static Hook<bool(void*, void*)>* Engine_Init;
	static Hook<void(ZKnowledge*, EGameTension)>* ZKnowledge_SetGameTension;
	static Hook<bool(const ZString& optionName, bool defaultValue)>* GetApplicationOptionBool;
	static Hook<void(ZApplicationEngineWin32* th, bool activated)>* ZApplicationEngineWin32_OnMainWindowActivated;
	static Hook<LRESULT(ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM)>* ZApplicationEngineWin32_MainWindowProc;
	static Hook<bool(ZEntityRef entity, uint32_t propertyId, const ZObjectRef& value, bool invokeChangeHandlers)>* SetPropertyValue;
	static Hook<bool(ZEntityRef entity, uint32_t pinId, const ZObjectRef& data)>* SignalOutputPin;
	static Hook<bool(ZEntityRef entity, uint32_t pinId, const ZObjectRef& data)>* SignalInputPin;
	static Hook<ZRenderDevice* (ZRenderDevice* th)>* ZRenderDevice_ZRenderDevice;
	static Hook<HRESULT(IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice)>* D3D12CreateDevice;
	static Hook<void(ZRenderSwapChain* th, void* a2, bool a3)>* ZRenderSwapChain_Resize;
	static Hook<bool(void*, void*)>* Check_SSL_Cert;
	static Hook<void(ZApplicationEngineWin32* th, const ZString& info, const ZString& details)>* ZApplicationEngineWin32_OnDebugInfo;
	static Hook<void(ZKeyboardWindows* th, bool a2)>* ZKeyboardWindows_Update;
	static Hook<void*(ZPackageManagerPackage* th, void* a2, const ZString& a3, int a4, int patchLevel)>* ZPackageManagerPackage_ZPackageManagerPackage;
	static Hook<void(ZGameLoopManager* th, const ZString& a2)>* ZGameLoopManager_ReleasePause;
	static Hook<bool(ZGameUIManagerEntity* th, EGameUIMenu menu, bool force)>* ZGameUIManagerEntity_TryOpenMenu;
	static Hook<void(ZGameStatsManager* th)>* ZGameStatsManager_SendAISignals01;
	static Hook<void(ZGameStatsManager* th)>* ZGameStatsManager_SendAISignals02;
	static Hook<void(ZAchievementManagerSimple* th, const SOnlineEvent& event)>* ZAchievementManagerSimple_OnEventReceived;
	static Hook<void(ZAchievementManagerSimple* th, uint32_t eventIndex, const ZDynamicObject& event)>* ZAchievementManagerSimple_OnEventSent;
	static Hook<bool(ZInputAction* th, int a2)>* ZInputAction_Digital;
	static Hook<double(ZInputAction* th, int a2)>* ZInputAction_Analog;
	static Hook<void(ZEntityManager* th, const TFixedArray<ZEntityRef>& entities, THashMap<ZRuntimeResourceID, ZEntityRef>& references)>* ZEntityManager_DeleteEntities;
	static Hook<void(ZEntityManager* th, ZEntityRef* entity, void* a3)>* ZEntityManager_ActivateEntity;
	static Hook<void(void* dwContext, void* hInternet, void* param_3, int dwInternetStatus, void* param_5, int param_6)>* Http_WinHttpCallback;
	static Hook<void(ZHttpResultDynamicObject* th)>* ZHttpResultDynamicObject_OnBufferReady;
};
