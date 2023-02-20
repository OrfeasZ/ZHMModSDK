#pragma once

#include <Windows.h>
#include <d3d12.h>

#include "Hook.h"
#include "Common.h"

#include <Glacier/Enums.h>
#include <Glacier/EUpdateMode.h>
#include <Glacier/ZDelegate.h>
#include <Glacier/TArray.h>
#include <Glacier/THashMap.h>

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
class ZTemplateEntityFactory;
class ZCppEntityFactory;
class ZBehaviorTreeEntityFactory;
class ZAudioSwitchEntityFactory;
class ZAudioStateEntityFactory;
class ZAspectEntityFactory;
class ZUIControlEntityFactory;
class ZRenderMaterialEntityFactory;
class ZExtendedCppEntityFactory;
class ZUIControlBlueprintFactory;
class ZCppEntityBlueprintFactory;
class IEntityFactory;
class ZEntityType;
class ZTemplateEntityBlueprintFactory;
class STemplateEntityBlueprint;
class ZResourcePending;

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
    static Hook<HRESULT(IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice)>* D3D12CreateDevice;
    static Hook<HRESULT(REFIID riid, void** ppFactory)>* CreateDXGIFactory1;
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
    static Hook<ZTemplateEntityBlueprintFactory*(ZTemplateEntityBlueprintFactory* th, STemplateEntityBlueprint* pTemplateEntityBlueprint, ZResourcePending& ResourcePending)>* ZTemplateEntityBlueprintFactory_ZTemplateEntityBlueprintFactory;
    //static Hook<void(ZTemplateEntityFactory* th, ZEntityRef entity, void* a3, void* a4)>* ZTemplateEntityFactory_ConfigureEntity;
    //static Hook<void(ZCppEntityFactory* th, ZEntityRef entity, void* a3, void* a4)>* ZCppEntityFactory_ConfigureEntity;
    //static Hook<void(ZBehaviorTreeEntityFactory* th, ZEntityRef entity, void* a3, void* a4)>* ZBehaviorTreeEntityFactory_ConfigureEntity;
    //static Hook<void(ZAudioSwitchEntityFactory* th, ZEntityRef entity, void* a3, void* a4)>* ZAudioSwitchEntityFactory_ConfigureEntity;
    //static Hook<void(ZAspectEntityFactory* th, ZEntityRef entity, void* a3, void* a4)>* ZAspectEntityFactory_ConfigureEntity;
    //static Hook<void(ZRenderMaterialEntityFactory* th, ZEntityRef entity, void* a3, void* a4)>* ZRenderMaterialEntityFactory_ConfigureEntity;
    //static Hook<void(ZUIControlBlueprintFactory* th, ZEntityRef entity, void* a3)>* ZUIControlBlueprintFactory_DestroyEntity;
    //static Hook<void(ZRenderMaterialEntityFactory* th, ZEntityRef entity, void* a3)>* ZTemplateEntityBlueprintFactory_DestroyEntity;
    //static Hook<void(ZRenderMaterialEntityFactory* th, ZEntityRef entity, void* a3)>* ZRenderMaterialEntityBlueprintFactory_DestroyEntity;
    //static Hook<void(ZRenderMaterialEntityFactory* th, ZEntityRef entity, void* a3)>* ZExtendedCppEntityBlueprintFactory_DestroyEntity;
    //static Hook<void(ZRenderMaterialEntityFactory* th, ZEntityRef entity, void* a3)>* ZEntityBlueprintFactoryBase_DestroyEntity;
    //static Hook<void(ZCppEntityBlueprintFactory* th, ZEntityRef entity, void* a3)>* ZCppEntityBlueprintFactory_DestroyEntity;
    //static Hook<void(ZRenderMaterialEntityFactory* th, ZEntityRef entity, void* a3)>* ZCompositeEntityBlueprintFactoryBase_DestroyEntity;
    //static Hook<void(ZRenderMaterialEntityFactory* th, ZEntityRef entity, void* a3)>* ZBehaviorTreeEntityBlueprintFactory_DestroyEntity;
    //static Hook<void(ZRenderMaterialEntityFactory* th, ZEntityRef entity, void* a3)>* ZAudioSwitchBlueprintFactory_DestroyEntity;
    //static Hook<void(ZRenderMaterialEntityFactory* th, ZEntityRef entity, void* a3)>* ZAudioStateBlueprintFactory_DestroyEntity;
    //static Hook<void(ZRenderMaterialEntityFactory* th, ZEntityRef entity, void* a3)>* ZAspectEntityBlueprintFactory_DestroyEntity;
};
