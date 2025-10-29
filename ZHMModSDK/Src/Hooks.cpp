#include "Hooks.h"
#include "HookImpl.h"

#include <Glacier/ZEntity.h>

std::unordered_set<HookBase*>* HookRegistry::g_Hooks = nullptr;

DetourTrampoline* Trampolines::g_Trampolines = nullptr;
size_t Trampolines::g_TrampolineCount = 0;

PATTERN_HOOK(
    "\x40\x53\x48\x83\xEC\x00\x4C\x8B\xC9\xE8",
    "xxxxx?xxxx",
    ZActor_ZActor,
    void(ZActor* th, ZComponentCreateInfo* createInfo)
);

PATTERN_HOOK(
    "\x48\x8B\xC4\x48\x89\x48\x08\x41\x54\x41\x56\x41\x57",
    "xxxxxxxxxxxxx",
    ZEntitySceneContext_LoadScene,
    void(ZEntitySceneContext* th, SSceneInitParameters& parameters)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x41\x56\x48\x83\xEC\x00\x8B\x05",
    "xxxxxxxxxxxxxxxxxxxx?xx",
    ZEntitySceneContext_ClearScene,
    void(ZEntitySceneContext*, bool bFullyUnloadScene)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x10\x48\x89\x6C\x24\x18\x56\x57\x41\x56\x48\x83\xEC\x00\x41\x0F\xB6\xE9",
    "xxxxxxxxxxxxxxxxx?xxxx",
    ZUpdateEventContainer_AddDelegate,
    void(ZUpdateEventContainer* th, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x18\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8D\x79\x50",
    "xxxxxxxxxxxxxxxxxxx?xxxx",
    ZUpdateEventContainer_RemoveDelegate,
    void(ZUpdateEventContainer* th, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)
);

// Look for ProfileWholeApplication string
PATTERN_HOOK(
    "\x48\x89\x54\x24\x10\x55\x56\x41\x54\x41\x56\x41\x57\x48\x8D\xAC\x24\xF0\xFB\xFF\xFF",
    "xxxxxxxxxxxxxxxxxxxxx",
    Engine_Init,
    bool(void*, void*)
);

PATTERN_HOOK(
    "\x40\x55\x57\x48\x83\xEC\x00\x8B\xEA",
    "xxxxxx?xx",
    ZKnowledge_SetGameTension,
    void(ZKnowledge*, EGameTension)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x00\x0F\xB6\xF2\x48\x8B\xD9\xE8",
    "xxxxxxxxxxxxxx?xxxxxxx",
    GetApplicationOptionBool,
    bool(const ZString&, bool)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x18\x48\x89\x74\x24\x20\x57\x48\x83\xEC\x00\x0F\xB6\xF2",
    "xxxxxxxxxxxxxxxxxxx?xxx",
    ZApplicationEngineWin32_OnMainWindowActivated,
    void(ZApplicationEngineWin32*, bool)
);

// Look for DefWindowProcW import
PATTERN_HOOK(
    "\x48\x89\x5C\x24\x08\x48\x89\x54\x24\x10\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C\x24\xE1",
    "xxxxxxxxxxxxxxxxxxxxxxxxxx",
    ZApplicationEngineWin32_MainWindowProc,
    LRESULT(ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM)
);

PATTERN_HOOK(
    "\x40\x55\x56\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8B\x29",
    "xxxxxxxxxxxx?xxx",
    SetPropertyValue,
    bool(ZEntityRef, uint32_t, const ZObjectRef&, bool)
);

PATTERN_HOOK(
    "\x48\x89\x6C\x24\x18\x56\x57\x41\x56\x48\x83\xEC\x00\x48\x8B\x31",
    "xxxxxxxxxxxx?xxx",
    SignalOutputPin,
    bool(ZEntityRef, uint32_t, const ZObjectRef&)
);

PATTERN_HOOK(
    "\x48\x89\x6C\x24\x20\x56\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8B\x29",
    "xxxxxxxxxxxxx?xxx",
    SignalInputPin,
    bool(ZEntityRef, uint32_t, const ZObjectRef&)
);

MODULE_HOOK(
    "d3d12.dll", "D3D12CreateDevice",
    D3D12CreateDevice,
    HRESULT(IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice)
);

MODULE_HOOK(
    "dxgi.dll", "CreateDXGIFactory1",
    CreateDXGIFactory1,
    HRESULT(REFIID riid, void** ppFactory)
);

PATTERN_HOOK(
    "\x48\x89\x54\x24\x10\x48\x89\x4C\x24\x08\x55\x53\x41\x57",
    "xxxxxxxxxxxxxx",
    Check_SSL_Cert,
    bool(void*, void*)
);

PATTERN_HOOK(
    "\x40\x53\x48\x83\xEC\x00\x41\xF7\x00\x00\x00\x00\x00\x48\x8D\x59\x08",
    "xxxxx?xxx????xxxx",
    ZApplicationEngineWin32_OnDebugInfo,
    void(ZApplicationEngineWin32* th, const ZString& info, const ZString& details)
);

// Look for method that uses ??_7ZKeyboardWindows@@6B@ and calls GetAsyncKeyState(161)
PATTERN_HOOK(
    "\x40\x53\x41\x54\x48\x83\xEC\x00\x48\x8B\xD9",
    "xxxxxxx?xxx",
    ZKeyboardWindows_Update,
    void(ZKeyboardWindows* th, bool a2)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x00\x89\x11",
    "xxxxxxxxxxxxxxxxxxx?xx",
    ZPackageManagerPackage_ZPackageManagerPackage,
    void*(ZPackageManagerPackage* th, void* a2, const ZString& a3, int a4, int patchLevel)
);

PATTERN_HOOK(
    "\x40\x55\x56\x48\x83\xEC\x00\x48\x8D\xB1\x20\x01\x00\x00",
    "xxxxxx?xxxxxxx",
    ZGameLoopManager_ReleasePause,
    void(ZGameLoopManager* th, const ZString& a2)
);

PATTERN_HOOK(
    "\x40\x56\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x00\x4C\x8B\x49\x28",
    "xxxxxxxxxxx?xxxx",
    ZGameUIManagerEntity_TryOpenMenu,
    bool(ZGameUIManagerEntity* th, EGameUIMenu menu, bool force)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8B\xF9\x48\x81\xC1",
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx?xxxxxx",
    ZGameStatsManager_SendAISignals01,
    void(ZGameStatsManager* th)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x41\x56\x48\x83\xEC\x00\x48\x8B\xF9\x48\x81\xC1",
    "xxxxxxxxxxxxxxxxxxxx?xxxxxx",
    ZGameStatsManager_SendAISignals02,
    void(ZGameStatsManager* th)
);

// Look for AchievementTopOfTheClass string
PATTERN_HOOK(
    "\x48\x89\x5C\x24\x08\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x20\xFD\xFF\xFF",
    "xxxxxxxxxxxxxxxxxxxxxxxx",
    ZAchievementManagerSimple_OnEventReceived,
    void(ZAchievementManagerSimple* th, const SOnlineEvent& event)
);

PATTERN_VTABLE_HOOK(
    "\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x07\x48\x8D\x55\x97",
    "xxx????xxxxxxx",
    7,
    ZAchievementManagerSimple_OnEventSent,
    void(ZAchievementManagerSimple* th, uint32_t eventIndex, const ZDynamicObject& event)
);

PATTERN_HOOK(
    "\x40\x53\x41\x56\x48\x83\xEC\x00\x8B\xDA",
    "xxxxxxx?xx",
    ZInputAction_Digital,
    bool(ZInputAction* th, int a2)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x20\x41\x56\x48\x83\xEC\x00\x8B\xDA",
    "xxxxxxxxxx?xx",
    ZInputAction_Analog,
    double(ZInputAction* th, int a2)
);

PATTERN_HOOK(
    "\x40\x56\x41\x54\x41\x57\x48\x83\xEC\x00\x80\x79\x48",
    "xxxxxxxxx?xxx",
    ZEntityManager_DeleteEntities,
    void(ZEntityManager* th, TArrayRef<ZEntityRef> aEntities, const SExternalReferences& externalRefs,
        bool bPrintTimings)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x10\x48\x89\x7C\x24\x18\x41\x56\x48\x83\xEC\x00\x80\x3D",
    "xxxxxxxxxxxxxxx?xx",
    ZEntityManager_ActivateEntity,
    void(ZEntityManager* th, ZEntityRef* entity, void* a3)
);

// Look for reference to WinHttpReceiveResponse method
PATTERN_HOOK(
    "\x48\x8B\xC4\x4C\x89\x40\x18\x55\x56\x57\x48\x8D\xA8\xA8\xFE\xFF\xFF",
    "xxxxxxxxxxxxxxxxx",
    Http_WinHttpCallback,
    void(void* dwContext, void* hInternet, void* param_3, int dwInternetStatus, void* param_5, int param_6)
);

// Function at ??_7ZHttpResultDynamicObject@@6B@ index 4
PATTERN_HOOK(
    "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x00\x80\x79\x50\x00\x48\x8B\xD9",
    "xxxxxxxxxxxxxx?xxx?xxx",
    ZHttpResultDynamicObject_OnBufferReady,
    void(ZHttpResultDynamicObject* th)
);

/*// Vtable index 6
PATTERN_VTABLE_HOOK(
    "\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x01\x4D\x8B\xE8",
    "xxx????xxxxxx",
    6,
    ZTemplateEntityFactory_ConfigureEntity,
    void(ZTemplateEntityFactory* th, ZEntityRef entity, void* a3, void* a4)
);

PATTERN_VTABLE_HOOK(
    "\x48\x8D\x05\x00\x00\x00\x00\x49\x89\x07\x48\xB8",
    "xxx????xxxxx",
    6,
    ZCppEntityFactory_ConfigureEntity,
    void(ZCppEntityFactory* th, ZEntityRef entity, void* a3, void* a4)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x00\x8B\x41\x08\x45\x33\xD2\x48\x8B\x1D\x00\x00\x00\x00\x85\xC0\x78\x00\x8B\xC8\x48\x8B\x43\x08\x48\xC1\xE1\x00\x83\x7C\x01\x20\x00\x75\x00\x48\x8B\x4C\x01\x08\xEB\x00\x49\x8B\xCA\x48\x8B\x01\x48\x8D\x3D\x00\x00\x00\x00\x4C\x8B\x58\x30",
    "xxxxxxxxx?xxxxxxxxx????xxx?xxxxxxxxx?xxxx?x?xxxxxx?xxxxxxxxx????xxxx",
    ZBehaviorTreeEntityFactory_ConfigureEntity,
    void(ZBehaviorTreeEntityFactory* th, ZEntityRef entity, void* a3, void* a4)
);

// This is also used by ZAudioStateEntityFactory, ZUIControlEntityFactory, and ZExtendedCppEntityFactory.
PATTERN_VTABLE_HOOK(
    "\x48\x8D\x15\x00\x00\x00\x00\x4C\x8B\x48\x48",
    "xxx????xxxx",
    6,
    ZAudioSwitchEntityFactory_ConfigureEntity,
    void(ZAudioSwitchEntityFactory* th, ZEntityRef entity, void* a3, void* a4)
);

PATTERN_VTABLE_HOOK(
    "\x48\x8D\x05\x00\x00\x00\x00\x41\xBE\x00\x00\x00\x00\x48\x89\x01",
    "xxx????xx????xxx",
    6,
    ZAspectEntityFactory_ConfigureEntity,
    void(ZAspectEntityFactory* th, ZEntityRef entity, void* a3, void* a4)
);

PATTERN_VTABLE_HOOK(
    "\x48\x8D\x0D\x00\x00\x00\x00\x89\x5C\x24\x34",
    "xxx????xxxx",
    6,
    ZRenderMaterialEntityFactory_ConfigureEntity,
    void(ZRenderMaterialEntityFactory* th, ZEntityRef entity, void* a3, void* a4)
);

PATTERN_VTABLE_HOOK(
    "\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x01\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x79\x10",
    "xxx????xxxxxx????xxxx",
    7,
    ZCppEntityBlueprintFactory_DestroyEntity,
    void(ZCppEntityBlueprintFactory* th, ZEntityRef entity, void* a3)
);*/

PATTERN_HOOK(
    "\x4C\x89\x44\x24\x18\x48\x89\x54\x24\x10\x48\x89\x4C\x24\x08\x53\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x00\x41\x8B\x00",
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx?xxx",
    ZTemplateEntityBlueprintFactory_ZTemplateEntityBlueprintFactory,
    ZTemplateEntityBlueprintFactory*(ZTemplateEntityBlueprintFactory* th, STemplateEntityBlueprint*
        pTemplateEntityBlueprint, ZResourcePending& ResourcePending)
);

/*
PATTERN_RELATIVE_CALL_HOOK(
    "\xE8\x00\x00\x00\x00\x48\x8B\x4C\x24\x38\x48\x85\xC9\x74\x00\x48\x81\xC1",
    "x????xxxxxxxxx?xxx",
    ZPlayerRegistry_GetLocalPlayer,
    TEntityRef<ZHitman5>*(ZPlayerRegistry* th, TEntityRef<ZHitman5>* out)
);
*/

PATTERN_HOOK(
    "\x40\x53\x56\x57\x41\x54\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8D\x05",
    "xxxxxxxxxxx????xxx",
    ZDynamicPageController_Expand,
    void(ZDynamicPageController* th, ZDynamicObject& data, void* a3, void* a4, void* a5)
);

PATTERN_HOOK(
    "\x40\x55\x57\x48\x8B\xEC\x48\x83\xEC\x00\x4C\x8B\x1A",
    "xxxxxxxxx?xxx",
    ZDynamicPageController_HandleActionObject2,
    void(ZDynamicPageController* th, ZDynamicObject& actionObj, void* menuNode)
);

/*
PATTERN_HOOK(
    "\x40\x53\x48\x83\xEC\x00\x48\x8B\xD9\x48\x8D\x0D\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x48\x8B\xCB\xE8\x00\x00\x00\x00\x83\xBB\xE8\x00\x00\x00",
    "xxx????xxxxxxxxxxxxx????xxxxxx????xxxxxxxxxxx????xxx????x???????xxx????xxxxxxxxxxxxx????xxxxxxxxxx????xxx????x??xxx",
    ZLevelManagerStateCondition_ZLevelManagerStateCondition,
    void*(void* th, void* a2)
);
*/

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x00\x48\x8B\xF9\x0F\xB6\xDA\x48\x8D\x0D",
    "xxxxxxxxxxxxxx?xxxxxxxxx",
    ZLoadingScreenVideo_ActivateLoadingScreen,
    void*(void* th, void* a1)
);

PATTERN_HOOK(
    "\x40\x53\x48\x83\xEC\x00\x48\x8B\xD9\x48\x8D\x0D\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x48\x8B\xCB\xE8\x00\x00\x00\x00\x83\xBB\xE8\x00\x00\x00",
    "xxxxx?xxxxxx????xx????xxxx????xxxxxx",
    ZLoadingScreenVideo_StartNewVideo,
    bool(void* th, void* a1)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x00\x55\x48\x8D\x6C\x24\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xDA\x48\x8D\x0D",
    "xxxx?xxxxx?xxx????xxxxxx",
    ZOnlineVersionConfig_GetConfigHost,
    ZString*(ZOnlineVersionConfig* th, ZString* out)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x00\x48\x89\x7C\x24\x00\x55\x48\x8D\x6C\x24\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xDA\xC7\x44\x24",
    "xxxx?xxxx?xxxxx?xxx????xxxxxx",
    ZOnlineVersionConfig_GetConfigUrl,
    ZString*(ZOnlineVersionConfig* th, ZString* out)
);

MODULE_HOOK(
    "EOSSDK-Win64-Shipping.dll",
    "EOS_Platform_Create",
    EOS_Platform_Create,
    EOS_PlatformHandle*(EOS_Platform_Options* Options)
);

PATTERN_HOOK(
    "\x40\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8D\x6C\x24\x40\x0F\xB6\x05",
    "xxxxxxxxxxxxx????xxxxxxxx",
    DrawScaleform,
    void(ZRenderContext* ctx, ZRenderTargetView** rtv, uint32_t a3, ZRenderDepthStencilView** dsv, uint32_t a5, bool
        bCaptureOnly)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x48\x89\x7C\x24\x18\x44\x89\x4C\x24\x20\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x20\xFD\xFF\xFF",
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
    ZUserChannelContractsProxyBase_GetForPlay2,
    void(const ZString& id, const ZString& locationId, const ZDynamicObject& extraGameChangedIds, int difficulty,
        const std::function<void(const ZDynamicObject&)>& onOk, const std::function<void(int)>& onError,
        ZAsyncContext* ctx, const SHttpRequestBehavior& behavior)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x18\x55\x56\x57\x41\x56\x41\x57\x48\x83\xEC\x00\x45\x33\xFF\x49\x8B\xF1",
    "xxxxxxxxxxxxxxx?xxxxxx",
    ZPathfinder_CreateObstacle,
    ZPFObstacleHandle* (ZPathfinder* th, ZPFObstacleHandle* result, const SMatrix& mTransform, float4 vHalfSize,
        float32 fPenaltyMultiplier, uint32 nObstacleBlockageFlags, EPFObstacleClient eDebugObstacleClient)
);

PATTERN_HOOK(
    "\x48\x8B\xC4\x48\x89\x58\x10\x48\x89\x70\x18\x48\x89\x78\x20\x55\x41\x56\x41\x57\x48\x8D\x68\xA1\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9",
    "xxxxxxxxxxxxxxxxxxxxxxxxxxx????xxx",
    ZPFObstacleEntity_UpdateObstacle,
    void(ZPFObstacleEntity* th, uint32 nObstacleBlockageFlags, bool bEnabled, bool forceUpdate)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8D\x59\x28",
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx?xxxx",
    ZUIText_TryGetTextFromNameHash,
    bool(ZUIText* th, int32 nNameHash, ZString& sResult, int32_t& outMarkupResult)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x10\x57\x48\x83\xEC\x00\x48\x8B\xD9\x8B\xFA\x48\x8D\x4C\x24\x50",
    "xxxxxxxxx?xxxxxxxxxx",
    ZLevelManager_SetGameState,
    void(ZLevelManager* th, ZLevelManager::EGameState state)
);

PATTERN_HOOK(
    "\x89\x91\x78\x01\x00\x00\x83\xFA",
    "xxxxxxxx",
    ZEntitySceneContext_SetLoadingStage,
    void(ZEntitySceneContext* th, ESceneLoadingStage stage)
);

PATTERN_HOOK(
    "\x48\x8B\xC4\x88\x50\x10\x53",
    "xxxxxxx",
    ZSecuritySystemCameraManager_UpdateCameraState,
    void(ZSecuritySystemCameraManager* th, bool bReactionSituations)
);

PATTERN_HOOK(
    "\x48\x89\x4C\x24\x08\x55\x41\x56\x48\x8D\xAC\x24\x58\xEE\xFF\xFF",
    "xxxxxxxxxxxxxxxx",
    ZSecuritySystemCameraManager_OnFrameUpdate,
    void(ZSecuritySystemCameraManager* th, const SGameUpdateEvent* const updateEvent)
);

PATTERN_HOOK(
    "\x40\x55\x41\x56\x48\x8D\x6C\x24\x88",
    "xxxxxxxxx",
    ZSecuritySystemCamera_FrameUpdate,
    void(ZSecuritySystemCamera* th, const SGameUpdateEvent* const a2)
);

PATTERN_HOOK(
    "\x4C\x89\x44\x24\x18\x53\x55\x56\x57\x41\x54\x41\x56\x48\x83\xEC\x00\x49\x8B\x01",
    "xxxxxxxxxxxxxxxx?xxx",
    ZEntityManager_NewUninitializedEntity,
    ZEntityRef* (
        ZEntityManager* th,
        ZEntityRef& result,
        const ZString& sDebugName,
        IEntityFactory* pEntityFactory,
        const ZEntityRef& logicalParent,
        uint64_t entityID,
        const SExternalReferences& externalRefs,
        bool unk0)
);

PATTERN_HOOK(
    "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x00\x48\x8B\xF1\x0F\x57\xC0\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00\x48\x8D\x4C\x24\x30",
    "xxxxxxxxxxxxxx?xxxxxxxx????????xxxxx",
    ZEntityManager_DeleteEntity,
    void(ZEntityManager* th, const ZEntityRef& entityRef, const SExternalReferences& externalRefs)
);