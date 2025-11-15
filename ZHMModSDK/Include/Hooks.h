#pragma once

#include <Windows.h>
#include <directx/d3d12.h>

#include "Common.h"
#include "Hook.h"

#include <Glacier/EUpdateMode.h>
#include <Glacier/Enums.h>
#include <Glacier/TArray.h>
#include <Glacier/THashMap.h>
#include <Glacier/ZDelegate.h>
#include <Glacier/ZMath.h>
#include <Glacier/ZLevelManager.h>
#include <Glacier/ZResource.h>

#include "Scaleform.h"

class ZRenderDepthStencilView;
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
struct SSceneInitParameters;
class ZString;
class ZEntityRef;
class ZRenderDevice;
class ZRenderSwapChain;
class ZKeyboardWindows;
class ZRenderGraphNodeCamera;
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
class ZAspectEntityFactory;
class ZCppEntityFactory;
class ZUIControlEntityFactory;
class ZRenderMaterialEntityFactory;
class ZUIControlBlueprintFactory;
class ZCppEntityBlueprintFactory;
class IEntityFactory;
class ZEntityType;
class ZTemplateEntityBlueprintFactory;
class STemplateEntityBlueprint;
class ZResourcePending;
class ZPlayerRegistry;
class ZHitman5;
class ZDynamicPageController;
class ZGameLobbyManagerEpic;
template <class>
class TEntityRef;
struct EOS_PlatformHandle;
struct EOS_Platform_Options;
class ZRenderTargetView;
class ZAsyncContext;
struct SHttpRequestBehavior;
class ZPFObstacleHandle;
enum EPFObstacleClient;
class ZPFObstacleEntity;
class ZOnlineVersionConfig;
struct SExternalReferences;
class ZLevelManager;
enum class ESceneLoadingStage;
class ZSecuritySystemCameraManager;
class ZSecuritySystemCamera;
class ZExtendedCppEntityTypeInstaller;

class ZHMSDK_API Hooks {
public:
    static Hook<void(ZActor*, ZComponentCreateInfo*)>* ZActor_ZActor;
    static Hook<void(ZEntitySceneContext*, SSceneInitParameters& parameters)>* ZEntitySceneContext_LoadScene;
    static Hook<void(ZEntitySceneContext*, bool bFullyUnloadScene)>* ZEntitySceneContext_ClearScene;

    static Hook<void(
        ZUpdateEventContainer*, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode
    )>* ZUpdateEventContainer_AddDelegate;

    static Hook<void(ZUpdateEventContainer*, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)>*
    ZUpdateEventContainer_RemoveDelegate;

    static Hook<bool(void*, void*)>* Engine_Init;
    static Hook<void(ZKnowledge*, EGameTension)>* ZKnowledge_SetGameTension;
    static Hook<bool(const ZString& optionName, bool defaultValue)>* GetApplicationOptionBool;
    static Hook<void(ZApplicationEngineWin32* th, bool activated)>* ZApplicationEngineWin32_OnMainWindowActivated;
    static Hook<LRESULT(ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM)>* ZApplicationEngineWin32_MainWindowProc;

    static Hook<bool(
        ZEntityRef entity, uint32_t propertyId, const ZObjectRef& value, bool invokeChangeHandlers
    )>* SetPropertyValue;

    static Hook<bool(ZEntityRef entity, uint32_t pinId, const ZObjectRef& data)>* SignalOutputPin;
    static Hook<bool(ZEntityRef entity, uint32_t pinId, const ZObjectRef& data)>* SignalInputPin;

    static Hook<HRESULT(
        IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice
    )>* D3D12CreateDevice;

    static Hook<HRESULT(REFIID riid, void** ppFactory)>* CreateDXGIFactory1;
    static Hook<bool(void*, void*)>* Check_SSL_Cert;

    static Hook<void(ZApplicationEngineWin32* th, const ZString& info, const ZString& details)>*
    ZApplicationEngineWin32_OnDebugInfo;

    static Hook<void(ZKeyboardWindows* th, bool a2)>* ZKeyboardWindows_Update;

    static Hook<void*(IPackageManager::SPartitionInfo* th, void* a2, const ZString& a3, int a4, int patchLevel)>*
    IPackageManager_SPartitionInfo_IPackageManager_SPartitionInfo;

    static Hook<void(ZGameLoopManager* th, const ZString& a2)>* ZGameLoopManager_ReleasePause;
    static Hook<bool(ZGameUIManagerEntity* th, EGameUIMenu menu, bool force)>* ZGameUIManagerEntity_TryOpenMenu;
    static Hook<void(ZGameStatsManager* th)>* ZGameStatsManager_SendAISignals01;
    static Hook<void(ZGameStatsManager* th)>* ZGameStatsManager_SendAISignals02;

    static Hook<void(ZAchievementManagerSimple* th, const SOnlineEvent& event)>*
    ZAchievementManagerSimple_OnEventReceived;

    static Hook<void(ZAchievementManagerSimple* th, uint32_t eventIndex, const ZDynamicObject& event)>*
    ZAchievementManagerSimple_OnEventSent;

    static Hook<bool(ZInputAction* th, int a2)>* ZInputAction_Digital;
    static Hook<double(ZInputAction* th, int a2)>* ZInputAction_Analog;

    static Hook<void(
        ZEntityManager* th, TArrayRef<ZEntityRef> aEntities, const SExternalReferences& externalRefs,
        bool bPrintTimings
    )>* ZEntityManager_DeleteEntities;

    static Hook<void(ZEntityManager* th, ZEntityRef* entity, void* a3)>* ZEntityManager_ActivateEntity;

    static Hook<void(
        void* dwContext, void* hInternet, void* param_3, int dwInternetStatus, void* param_5, int param_6
    )>* Http_WinHttpCallback;

    static Hook<void(ZHttpResultDynamicObject* th)>* ZHttpResultDynamicObject_OnBufferReady;

    static Hook<ZTemplateEntityBlueprintFactory*(
        ZTemplateEntityBlueprintFactory* th, STemplateEntityBlueprint* pTemplateEntityBlueprint,
        ZResourcePending& ResourcePending
    )>* ZTemplateEntityBlueprintFactory_ZTemplateEntityBlueprintFactory;

    //static Hook<TEntityRef<ZHitman5>*(ZPlayerRegistry* th, TEntityRef<ZHitman5>* out)>* ZPlayerRegistry_GetLocalPlayer;

    static Hook<void(ZDynamicPageController* th, ZDynamicObject& data, void* a3, void* a4, void* a5)>*
    ZDynamicPageController_Expand;

    static Hook<void(ZDynamicPageController* th, ZDynamicObject& actionObj, void* menuNode)>*
    ZDynamicPageController_HandleActionObject2;

    //static Hook<void*(void* th, void* a2)>* ZLevelManagerStateCondition_ZLevelManagerStateCondition;
    static Hook<void*(void* th, void* a1)>* ZLoadingScreenVideo_ActivateLoadingScreen;
    static Hook<bool(void* th, void* a1)>* ZLoadingScreenVideo_StartNewVideo;
    static Hook<ZString*(ZOnlineVersionConfig* th, ZString* out)>* ZOnlineVersionConfig_GetConfigHost;
    static Hook<ZString*(ZOnlineVersionConfig* th, ZString* out)>* ZOnlineVersionConfig_GetConfigUrl;
    static Hook<EOS_PlatformHandle*(EOS_Platform_Options* Options)>* EOS_Platform_Create;

    static Hook<void(
        IEntityFactory* th, ZEntityType** pEntity, const SExternalReferences& externalRefs, uint8_t* unk0
    )>* IEntityFactory_ConfigureEntity;
    static Hook<void(
        ZTemplateEntityFactory* th, ZEntityType** pEntity, const SExternalReferences& externalRefs, uint8_t* unk0
    )>* ZTemplateEntityFactory_ConfigureEntity;
    static Hook<void(
        ZAspectEntityFactory* th, ZEntityType** pEntity, const SExternalReferences& externalRefs, uint8_t* unk0
    )>* ZAspectEntityFactory_ConfigureEntity;
    static Hook<void(
        ZCppEntityFactory* th, ZEntityType** pEntity, const SExternalReferences& externalRefs, uint8_t* unk0
    )>* ZCppEntityFactory_ConfigureEntity;
    static Hook<void(
        ZRenderMaterialEntityFactory* th, ZEntityType** pEntity, const SExternalReferences& externalRefs, uint8_t* unk0
    )>* ZRenderMaterialEntityFactory_ConfigureEntity;
    
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
    static Hook<void(
        ZRenderContext* ctx, ZRenderTargetView** rtv, uint32_t a3, ZRenderDepthStencilView** dsv, uint32_t a5,
        bool bCaptureOnly
    )>* DrawScaleform;

    static Hook<void(
        const ZString& id, const ZString& locationId, const ZDynamicObject& extraGameChangedIds, int difficulty,
        const std::function<void(const ZDynamicObject&)>& onOk, const std::function<void(int)>& onError,
        ZAsyncContext* ctx, const SHttpRequestBehavior& behavior
    )>* ZUserChannelContractsProxyBase_GetForPlay2;

    static Hook<ZPFObstacleHandle*(
        ZPathfinder* th,
        ZPFObstacleHandle* result,
        const SMatrix& mTransform,
        float4 vHalfSize,
        float32 fPenaltyMultiplier,
        uint32 nObstacleBlockageFlags,
        EPFObstacleClient eDebugObstacleClient
    )>* ZPathfinder_CreateObstacle;

    static Hook<void(ZPFObstacleEntity* th, uint32 nObstacleBlockageFlags, bool bEnabled, bool forceUpdate)>*
    ZPFObstacleEntity_UpdateObstacle;

    static Hook<bool(ZUIText* th, int32 nNameHash, ZString& sResult, int32_t& outMarkupResult)>* ZUIText_TryGetTextFromNameHash;

    static Hook<void(ZLevelManager* th, ZLevelManager::EGameState state)>* ZLevelManager_SetGameState;

    static Hook<void(ZEntitySceneContext* th, ESceneLoadingStage stage)>* ZEntitySceneContext_SetLoadingStage;

    static Hook<void(ZSecuritySystemCameraManager* th, bool bReactionSituations)>* ZSecuritySystemCameraManager_UpdateCameraState;
    static Hook<void(ZSecuritySystemCameraManager* th, const SGameUpdateEvent* const updateEvent)>* ZSecuritySystemCameraManager_OnFrameUpdate;
    static Hook<void(ZSecuritySystemCamera* th, const SGameUpdateEvent* const a2)>* ZSecuritySystemCamera_FrameUpdate;

    static Hook<ZEntityRef*(
        ZEntityManager* th,
        ZEntityRef& result,
        const ZString& sDebugName,
        IEntityFactory* pEntityFactory,
        const ZEntityRef& logicalParent,
        uint64_t entityID,
        const SExternalReferences& externalRefs,
        bool unk0
    )>* ZEntityManager_NewUninitializedEntity;

    static Hook<void(
        ZEntityManager* th, const ZEntityRef& entityRef, const SExternalReferences& externalRefs
    )>* ZEntityManager_DeleteEntity;

    static Hook<bool(
        ZExtendedCppEntityTypeInstaller* th, ZResourcePending& ResourcePending
    )>* ZExtendedCppEntityTypeInstaller_Install;

    static Hook<void(ZResourceManager* th, ZResourceIndex index)>* ZResourceManager_UninstallResource;

    static Hook<void(
        Scaleform::GFx::AS3::MovieRoot* th,
        Scaleform::GFx::AS3::FlashUI::OutputMessageType type,
        const char* msg
    )>* Scaleform_GFx_AS3_MovieRoot_Output;
};