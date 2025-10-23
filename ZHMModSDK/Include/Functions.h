#pragma once

#include "Common.h"
#include "EngineFunction.h"
#include "Glacier/ZConfigCommand.h"
#include "Glacier/ZInput.h"
#include "Glacier/ZMath.h"
#include "Glacier/ZPrimitives.h"
#include "Glacier/ZResource.h"
#include "Glacier/Reflection.h"
#include "Glacier/ZInventory.h"

class ZHitman5;
class ZActor;
class ZDynamicObject;
class ZString;
class ZHM5BaseCharacter;
class ZCameraEntity;
class ZSpatialEntity;
class ZEngineAppCommon;
class IRenderDestinationEntity;
class ZInputAction;
class ZHM5InputControl;
class ZEntityRef;
class ZEntityManager;
class IEntityFactory;
struct SMatrix;
class ZGlobalOutfitKit;
class ZItemSpawner;
class ZResourceContainer;
class ZResourceIndex;
class ZHM5Animator;
class ZHM5CrippleBox;
class ZRagdollHandler;
class ZInputActionManager;
class IItem;
class ZSetpieceEntity;
struct SExternalReferences;
class ZTemplateEntityFactory;
class STemplateEntityFactory;
class ZTemplateInstaller;
class ZTemplateBlueprintInstaller;
class ZResourcePending;
class ZEntityType;
class ZEntityImpl;
class ZUIText;

class ZHMSDK_API Functions {
public:
    static EngineFunction<void(ZActor* th)>* ZActor_OnOutfitChanged;
    static EngineFunction<void(ZActor* th)>* ZActor_ReviveActor;
    static EngineFunction<void(ZDynamicObject* th, ZString* a2)>* ZDynamicObject_ToString;
    static EngineFunction<ZDynamicObject*(ZDynamicObject* th, char* jsonStr, int strLen)>* ZDynamicObject_ParseString;
    static EngineFunction<void(ZHM5BaseCharacter* th, bool inMotion)>* ZHM5BaseCharacter_ActivateRagdoll;
    static EngineFunction<void(ZHM5BaseCharacter* th)>* ZHM5BaseCharacter_DeactivateRagdoll;
    static EngineFunction<ZCameraEntity*()>* GetCurrentCamera;
    //static EngineFunction<void(ZSpatialEntity* th, SMatrix* out)>* ZSpatialEntity_WorldTransform;
    static EngineFunction<void(ZEngineAppCommon* th)>* ZEngineAppCommon_CreateFreeCamera;

    static EngineFunction<TEntityRef<IRenderDestinationEntity>*(
        ZCameraManager* th, TEntityRef<IRenderDestinationEntity>* result
    )>* ZCameraManager_GetActiveRenderDestinationEntity;

    static EngineFunction<double(ZInputAction* th, int a2)>* ZInputAction_Analog;
    static EngineFunction<bool(ZInputAction* th, int a2)>* ZInputAction_Digital;
    //static EngineFunction<TEntityRef<ZHitman5>*(ZPlayerRegistry* th, TEntityRef<ZHitman5>* out)>* ZPlayerRegistry_GetLocalPlayer;
    static EngineFunction<ZHM5InputControl*(ZHM5InputManager* th)>* ZHM5InputManager_GetInputControlForLocalPlayer;
    static EngineFunction<void(ZResourceManager* th, int index)>* ZResourceManager_UninstallResource;

    static EngineFunction<ZEntityRef*(
        ZEntityManager* th, ZEntityRef& result, const ZString& sDebugName, IEntityFactory* pEntityFactory,
        const ZEntityRef& transformParent, const SExternalReferences& externalRefs, uint64_t entityId
    )>* ZEntityManager_NewEntity;

    static EngineFunction<void(const ZSpatialEntity* th)>* ZSpatialEntity_UpdateCachedWorldMat;

    static EngineFunction<void(
        ZEntityManager* th, const ZEntityRef& entityRef, const SExternalReferences& externalRefs
    )>* ZEntityManager_DeleteEntity;

    static EngineFunction<void(
        ZHitman5* th, TEntityRef<ZGlobalOutfitKit> rOutfitKit, int32_t nCharset, int32_t nVariation,
        bool bEnableOutfitModifiers, bool bIgnoreOutifChange
    )>* ZHitman5_SetOutfit;

    static EngineFunction<void(
        ZActor* th, TEntityRef<ZGlobalOutfitKit> rOutfit, int32_t charset, int32_t variation, bool bNude
    )>* ZActor_SetOutfit;

    static EngineFunction<void(ZItemSpawner* th)>* ZItemSpawner_RequestContentLoad;

    static EngineFunction<ZCharacterSubcontrollerInventory::SCreateItem*(
        ZCharacterSubcontrollerInventory* th,
        const ZRepositoryID& repId,
        const ZString& sOnlineInstanceId,
        const TArray<ZRepositoryID>& instanceModifiersToApply,
        ZCharacterSubcontrollerInventory::ECreateItemType createItemType
    )>* ZCharacterSubcontrollerInventory_CreateItem;

    static EngineFunction<void(
        ZResourceContainer* th, ZResourceIndex index, TArray<ZResourceIndex>& indices, TArray<unsigned char>& flags
    )>* ZResourceContainer_GetResourceReferences;

    static EngineFunction<void(ZHM5BaseCharacter* th, const ZString& request)>*
    ZHM5BaseCharacter_SendRequestToChildNetworks;

    static EngineFunction<void(ZHM5Animator* th, float* time)>* ZHM5Animator_ActivateRagdollToAnimationBlend;

    static EngineFunction<void(ZHM5BaseCharacter* th, float time, bool inMotion, bool upperBody, float a5, bool a6)>*
    ZHM5BaseCharacter_ActivatePoweredRagdoll;

    static EngineFunction<void(
        ZRagdollHandler* th, const float4& position, const float4& impulse, uint32_t boneIndex, bool randomize
    )>* ZRagdollHandler_ApplyImpulseOnRagdoll;

    static EngineFunction<ZInputTokenStream::ZTokenData*(ZInputTokenStream* th, ZInputTokenStream::ZTokenData* result)>*
    ZInputTokenStream_ParseToken;

    static EngineFunction<bool(ZInputActionManager* th, ZInputTokenStream* pkStream)>*
    ZInputActionManager_ParseAsignment;

    static EngineFunction<void(
        ZActor* th, TEntityRef<IItem> rKillItem, TEntityRef<ZSetpieceEntity> rKillSetpiece, EDamageEvent eDamageEvent,
        EDeathBehavior eDeathBehavior
    )>* ZActor_KillActor;

    static EngineFunction<void(const char* pCommandName, const char* argv)>* ZConfigCommand_ExecuteCommand;
    static EngineFunction<ZConfigCommand*(uint32_t commandNameHash)>* ZConfigCommand_GetConfigCommand;
    static EngineFunction<ZConfigCommand*()>* ZConfigCommand_First;

    static EngineFunction<ZDynamicObject*(
        ZDynamicObject* th, const ZString& p_Key, const ZDynamicObject& p_Value
    )>* ZDynamicObject_Set;

    static EngineFunction<void(ZTemplateEntityFactory* th, STemplateEntityFactory* data, ZResourcePending& pending)>*
    ZTemplateEntityFactory_ZTemplateEntityFactory;

    static EngineFunction<void(ZResourceContainer* th, ZResourceIndex& out, const ZRuntimeResourceID rid)>*
    ZResourceContainer_AddResourceInternal;

    static EngineFunction<void(
        ZResourceReader* th, ZResourceIndex* idx, ZResourceDataPtr* pData, uint32_t dataSize
    )>* ZResourceReader_ZResourceReader;

    static EngineFunction<bool(ZTemplateInstaller* th, ZResourcePending* ResourcePending)>*
    ZTemplateInstaller_Install;

    static EngineFunction<bool(ZTemplateBlueprintInstaller* th, ZResourcePending* ResourcePending)>*
    ZTemplateBlueprintInstaller_Install;

    static EngineFunction<ZEntityType*(ZEntityImpl* th, unsigned int nUniqueMapMask)>* ZEntityImpl_EnsureUniqueType;

    static EngineFunction<void(ZResourceContainer* th, ZRuntimeResourceID rid, SResourceReferenceFlags flags)>*
    ZResourceContainer_AddResourceReferenceInternal;

    static EngineFunction<void(ZResourceContainer* th, ZResourceIndex index)>*
    ZResourceContainer_AcquireReferences;

    static EngineFunction<void(ZString::ZImpl* th)>* ZString_ZImpl_Free;

    static EngineFunction<ZString::ZImpl*(const char* buf, size_t size)>* ZStringCollection_Allocate;
};