#include "Functions.h"
#include "EngineFunctionImpl.h"

PATTERN_FUNCTION(
    "\x40\x53\x48\x83\xEC\x00\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x81\x20\x04\x00\x00",
    "xxxxx?xxx????xxxxxxx",
    ZActor_OnOutfitChanged,
    void(ZActor*)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x10\x48\x89\x7C\x24\x18\x55\x48\x8D\x6C\x24\xA9\x48\x81\xEC\x00\x00\x00\x00\x48\x8D\x99\xD8\x02\x00\x00",
    "xxxxxxxxxxxxxxxxxxx????xxxxxxx",
    ZActor_ReviveActor,
    void(ZActor*)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x00\x48\x8B\xF1\x0F\x57\xC0\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00\xB9",
    "xxxxxxxxxxxxxx?xxxxxxxx????????x",
    ZDynamicObject_ToString,
    void(ZDynamicObject*, ZString&)
);

PATTERN_FUNCTION(
    "\x44\x88\x44\x24\x18\x48\x89\x4C\x24\x08\x55\x53",
    "xxxxxxxxxxxx",
    ZJsonDeserializer_Deserialize,
    ZDynamicObject*(ZDynamicObject& result, const uint8_t* pData, uint64_t nLength)
);

PATTERN_FUNCTION(
    "\x48\x89\x4C\x24\x08\x53\x55\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x01\x0F\xB6\xDA",
    "xxxxxxxxxx????xxxxxx",
    ZHM5BaseCharacter_ActivateRagdoll,
    void(ZHM5BaseCharacter*, bool)
);

PATTERN_FUNCTION(
    "\x48\x89\x4C\x24\x08\x53\x55\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x01\x0F\xB6\xDA",
    "xxxxxxxxxx????xxxxxx",
    ZHM5BaseCharacter_DeactivateRagdoll,
    void(ZHM5BaseCharacter*)
);

PATTERN_FUNCTION(
    "\x40\x53\x48\x83\xEC\x00\x48\x8B\x1D\x00\x00\x00\x00\x48\x85\xDB\x74\x00\x48\x8B\x03\x48\x8B\xCB\xFF\x50\x28\x84\xC0\x74\x00\x48\x8B\x03\x48\x8B\xCB\xFF\x50\x70\x83\xF8\x00\x75",
    "xxxxx?xxx????xxxx?xxxxxxxxxxxx?xxxxxxxxxxx?x",
    GetCurrentCamera,
    ZCameraEntity*()
);

/*
PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x00\xF7\x41\x6C\x00\x00\x00\x00\x48\x8B\xFA\x48\x8B\xD9\x74\x00\xE8\x00\x00\x00\x00\x0F\x10\x53\x20",
    "xxxxxxxxx?xxx????xxxxxxx?x????xxxx",
    ZSpatialEntity_WorldTransform,
    void(ZSpatialEntity* th, SMatrix* out)
);
*/

PATTERN_FUNCTION(
    "\x40\x55\x53\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\xF8\xFC\xFF\xFF",
    "xxxxxxxxxxxxxxxxxxxxx",
    ZEngineAppCommon_CreateFreeCameraAndControl,
    void(ZEngineAppCommon* th)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x18\x48\x89\x6C\x24\x20\x56\x57\x41\x54\x41\x55\x41\x56\x48\x83\xEC\x00\x48\x8D\x79\x08",
    "xxxxxxxxxxxxxxxxxxxxx?xxxx",
    ZCameraManager_GetActiveRenderDestinationEntity,
    TEntityRef<IRenderDestinationEntity>*(ZCameraManager* th, TEntityRef<IRenderDestinationEntity>* result)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x20\x41\x56\x48\x83\xEC\x00\x8B\xDA",
    "xxxxxxxxxx?xx",
    ZInputAction_Analog,
    double(ZInputAction* th, int a2)
);

PATTERN_FUNCTION(
    "\x40\x53\x41\x56\x48\x83\xEC\x00\x8B\xDA",
    "xxxxxxx?xx",
    ZInputAction_Digital,
    bool(ZInputAction* th, int a2)
);

/*
PATTERN_RELATIVE_FUNCTION(
    "\xE8\x00\x00\x00\x00\x48\x8B\x4C\x24\x38\x48\x85\xC9\x74\x00\x48\x81\xC1",
    "x????xxxxxxxxx?xxx",
    ZPlayerRegistry_GetLocalPlayer,
    TEntityRef<ZHitman5>*(ZPlayerRegistry* th, TEntityRef<ZHitman5>* out)
);
*/

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x00\x48\x8B\x15\x00\x00\x00\x00\x4C\x8D\x05",
    "xxxxxxxxx?xxx????xxx",
    ZHM5InputManager_GetInputControlForLocalPlayer,
    ZHM5InputControl*(ZHM5InputManager* th)
);

PATTERN_FUNCTION(
    "\x89\x54\x24\x10\x57\x48\x83\xEC\x00\x48\x89\x5C\x24\x60",
    "xxxxxxxx?xxxxx",
    ZResourceManager_UninstallResource,
    void(ZResourceManager* th, ZResourceIndex index)
);

PATTERN_FUNCTION(
    "\x4C\x8B\xDC\x49\x89\x5B\x08\x49\x89\x6B\x10\x4D\x89\x43\x18",
    "xxxxxxxxxxxxxxx",
    ZEntityManager_NewEntity,
    ZEntityRef*(ZEntityManager* th, ZEntityRef& result, const ZString& sDebugName, IEntityFactory* pEntityFactory,
        const ZEntityRef& transformParent, const SExternalReferences& externalRefs, uint64_t entityId)
);

// Look for camAlign_ string, go to parent xref of function using said string: function called in if flag check.
PATTERN_FUNCTION(
    "\x48\x8B\xC4\x53\x48\x81\xEC\x00\x00\x00\x00\xF7\x41\x6C\x00\x00\x00\x00\x48\x8B\xD9\x0F\x84",
    "xxxxxxx????xxx????xxxxx",
    ZSpatialEntity_UpdateCachedWorldMat,
    void(const ZSpatialEntity* th)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x00\x48\x8B\xF1\x0F\x57\xC0\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00\x48\x8D\x4C\x24\x30",
    "xxxxxxxxxxxxxx?xxxxxxxx????????xxxxx",
    ZEntityManager_DeleteEntity,
    void(ZEntityManager* th, const ZEntityRef& entityRef, const SExternalReferences& externalRefs)
);

PATTERN_FUNCTION(
    "\x4C\x8B\xDC\x55\x56\x41\x54\x41\x56\x41\x57\x48\x83\xEC",
    "xxxxxxxxxxxxxx",
    ZHitman5_SetOutfit,
    void(ZHitman5* th, TEntityRef<ZGlobalOutfitKit> rOutfitKit, int nCharset, int nVariation,
        bool bEnableOutfitModifiers, bool bIgnoreOutifChange)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x55\x41\x56\x41\x57\x48\x8B\xEC\x48\x83\xEC\x00\x41\x8B\xF9",
    "xxxxxxxxxxxxxxxxxxxxxxxxxx?xxx",
    ZActor_SetOutfit,
    void(ZActor* th, TEntityRef<ZGlobalOutfitKit> rOutfit, int charset, int variation, bool bNude)
);

PATTERN_FUNCTION(
    "\x40\x56\x48\x81\xEC\x00\x00\x00\x00\xF6\x81\x34\x01\x00\x00",
    "xxxxx????xxxxxx",
    ZItemSpawner_RequestContentLoad,
    void(ZItemSpawner* th)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x55\x57\x41\x54\x41\x56\x41\x57\x48\x8D\x6C\x24\xD1",
    "xxxxxxxxxxxxxxxxxxxxxxx",
    ZCharacterSubcontrollerInventory_CreateItem,
    ZCharacterSubcontrollerInventory::SCreateItem* (
        ZCharacterSubcontrollerInventory* th, const ZRepositoryID& repId, const ZString& sOnlineInstanceId,
        const TArray<ZRepositoryID>& instanceModifiersToApply,
        ZCharacterSubcontrollerInventory::ECreateItemType createItemType)
);

PATTERN_FUNCTION(
    "\x40\x57\x48\x83\xEC\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x89\x5C\x24\x50",
    "xxxxx?xxx????xxxxx",
    ZResourceContainer_GetResourceReferences,
    void(ZResourceContainer* th, ZResourceIndex index, TArray<ZResourceIndex>& indices, TArray<unsigned char>& flags)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x00\x8B\x81\xA0\x02\x00\x00",
    "xxxxxxxxxxxxxx?xxxxxx",
    ZHM5BaseCharacter_SendRequestToChildNetworks,
    void(ZHM5BaseCharacter* th, const ZString& request)
);

PATTERN_FUNCTION(
    "\x48\x89\x74\x24\x20\x57\x48\x83\xEC\x00\x0F\x57\xC0",
    "xxxxxxxxx?xxx",
    ZHM5Animator_ActivateRagdollToAnimationBlend,
    void(ZHM5Animator* th, float* time)
);

PATTERN_FUNCTION(
    "\x40\x55\x48\x83\xEC\x00\x48\x89\x5C\x24\x60",
    "xxxxx?xxxxx",
    ZHM5BaseCharacter_ActivatePoweredRagdoll,
    void(ZHM5BaseCharacter* th, float time, bool inMotion, bool upperBody, float a5, bool a6)
);

PATTERN_FUNCTION(
    "\x40\x57\x48\x83\xEC\x00\x80\xBC\x24\x90\x00\x00\x00",
    "xxxxx?xxxxxxx",
    ZRagdollHandler_ApplyImpulseOnRagdoll,
    void(ZRagdollHandler* th, const float4& position, const float4& impulse, uint32_t boneIndex, bool randomize)
);

PATTERN_RELATIVE_FUNCTION(
    "\xE8\x00\x00\x00\x00\x48\x8D\x95\x00\x03\x00\x00",
    "x????xxxxxxx",
    ZInputTokenStream_ParseToken,
    ZInputTokenStream::ZTokenData*(ZInputTokenStream* th, ZInputTokenStream::ZTokenData* result)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x8B\x42\x08",
    "xxxxxxxxxxxxxxxxxxxxxxx????xxx",
    ZInputActionManager_ParseAsignment,
    bool(ZInputActionManager* th, ZInputTokenStream* pkStream)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x10\x48\x89\x6C\x24\x18\x56\x57\x41\x56\x48\x83\xEC\x00\x48\x8B\x81\xD8\x02\x00\x00\x48\x8B\xE9",
    "xxxxxxxxxxxxxxxxx?xxxxxxxxxx",
    ZActor_KillActor,
    void(ZActor* th, TEntityRef<IItem> rKillItem, TEntityRef<ZSetpieceEntity> rKillSetpiece, EDamageEvent eDamageEvent,
        EDeathBehavior eDeathBehavior)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x00\x48\x8B\xFA\x48\x8B\xD9\x4C\x8B\xC9",
    "xxxx?xxxx?xxxxxxxxx",
    ZConfigCommand_ExecuteCommand,
    void(const char* pCommandName, const char* argv)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x00\x8B\xF9\xE8",
    "xxxx?xxxx?xxx",
    ZConfigCommand_GetConfigCommand,
    ZConfigCommand*(uint32_t commandNameHash)
)

PATTERN_FUNCTION(
    "\x40\x53\x48\x83\xEC\x00\xE8\x00\x00\x00\x00\x48\x8B\xC8\xFF\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xD8\xE8",
    "xxxxx?x????xxxxx????xxx????x????xxxx",
    ZConfigCommand_First,
    ZConfigCommand*()
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x10\x48\x89\x6C\x24\x18\x48\x89\x74\x24\x20\x41\x56\x48\x83\xEC\x00\x4C\x8B\x51\x08",
    "xxxxxxxxxxxxxxxxxxxx?xxxx",
    ZDynamicObject_Set,
    ZDynamicObject*(ZDynamicObject* th, const ZString& p_Key, const ZDynamicObject& p_Value)
);

PATTERN_FUNCTION(
    "\x4C\x89\x44\x24\x18\x55\x53\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8B\xEC\x48\x83\xEC\x00\x4C\x8B\xF1",
    "xxxxxxxxxxxxxxxxxxxxxxx?xxx",
    ZTemplateEntityFactory_ZTemplateEntityFactory,
    void(ZTemplateEntityFactory* th, STemplateEntityFactory* data, ZResourcePending& pending)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x48\x89\x7C\x24\x20\x4C\x89\x44\x24\x18\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC",
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
    ZResourceContainer_AddResourceInternal,
    void(ZResourceContainer* th, ZResourceIndex& out, const ZRuntimeResourceID rid)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x00\x33\xED\x48\x8D\x05\x00\x00\x00\x00\x89\x69\x08",
    "xxxxxxxxxxxxxxxxxxx?xxxxx????xxx",
    ZResourceReader_ZResourceReader,
    void(ZResourceReader* th, ZResourceIndex* idx, ZResourceDataPtr* pData, uint32_t dataSize)
);

PATTERN_VTABLE_FUNCTION(
    "\x48\x8D\x05\x00\x00\x00\x00\x41\xB8\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x3D\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x8B\xD7\xFF\x90\xA0\x00\x00\x00\x48\x8B\x0D",
    "xxx????xx????xxx????xxx????xxx????xxx????xxxxxxxxxxxx",
    7,
    ZTemplateInstaller_Install,
    bool(ZTemplateInstaller* th, ZResourcePending* ResourcePending)
);

PATTERN_VTABLE_FUNCTION(
    "\x48\x8D\x05\x00\x00\x00\x00\x41\xB8\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x3D\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x8B\xD7\xFF\x90\xA0\x00\x00\x00\x48\x8B\x4B\x60\x48\x8D\x55\x20\x48\x89\x7D\x20\xE8\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x41\xB8\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x3D\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x8B\xD7\xFF\x90\xA0\x00\x00\x00\x48\x8B\x4B\x60\x48\x8D\x55\x20\x48\x89\x7D\x20\xE8\x00\x00\x00\x00\x66\x0F\x6F\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05",
    "xxx????xx????xxx????xxx????xxx????xxx????xxxxxxxxxxxxxxxxxxxxxx????xxx????xx????xxx????xxx????xxx????xxx????xxxxxxxxxxxxxxxxxxxxxx????xxxx????xxx????xxx",
    7,
    ZTemplateBlueprintInstaller_Install,
    bool(ZTemplateBlueprintInstaller* th, ZResourcePending* ResourcePending)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x00\x48\x8B\x41\x08\x8B\xF2",
    "xxxxxxxxxxxxxx?xxxxxx",
    ZEntityImpl_EnsureUniqueType,
    ZEntityType* (ZEntityImpl* th, unsigned int nUniqueMapMask)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x08\x48\x89\x54\x24\x10\x57\x48\x83\xEC\x00\x4C\x8B\x51\x48",
    "xxxxxxxxxxxxxx?xxxx",
    ZResourceContainer_AddResourceReferenceInternal,
    void(ZResourceContainer* th, ZRuntimeResourceID rid, SResourceReferenceFlags flags)
);

PATTERN_FUNCTION(
    "\x48\x8B\xC4\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x89\x58\x10\x4C\x89\x60\xF0",
    "xxxxxxx????xxxxxxxx",
    ZResourceContainer_AcquireReferences,
    void(ZResourceContainer* th, ZResourceIndex index)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x18\x48\x89\x6C\x24\x20\x56\x48\x83\xEC\x00\x48\x8B\xF1\xE8",
    "xxxxxxxxxxxxxx?xxxx",
    ZString_ZImpl_Free,
    void(ZString::ZImpl* th)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x20\x57\x41\x56\x41\x57\x48\x83\xEC\x00\x8B\xEA",
    "xxxxxxxxxxxxxxxxxxxxxxx?xx",
    ZStringCollection_Allocate,
    ZString::ZImpl* (const char* buf, size_t size)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x10\x55\x56\x57\x48\x8D\x6C\x24\xB9\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x41\x20",
    "xxxxxxxxxxxxxxxx????xxxx",
    ZActorInventoryHandler_RequestItem,
    bool(ZActorInventoryHandler* th, const ZRepositoryID& id)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x18\x48\x89\x6C\x24\x20\x56\x48\x83\xEC\x00\x4C\x8B\x49\x60",
    "xxxxxxxxxxxxxx?xxxx",
    ZActorInventoryHandler_ItemAddToInventory,
    bool(ZActorInventoryHandler* th, const TEntityRef<IItem>& rItem)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x18\x56\x57\x41\x57\x48\x83\xEC\x00\x80\x7C\x24\x78",
    "xxxxxxxxxxxx?xxxx",
    ZActorInventoryHandler_ItemPickup,
    bool(ZActorInventoryHandler* th, const TEntityRef<IItem>& rItem, EAttachLocation eLocation, EGameTension eMaxTension,
        bool bLeftHand, bool bMainWeapon, bool bGiveItem)
);

PATTERN_FUNCTION(
    "\x40\x53\x56\x57\x48\x83\xEC\x00\x48\x8B\xD9\x33\xFF",
    "xxxxxxx?xxxxx",
    ZActorInventoryHandler_UpdateAfterAttachChange,
    void(ZActorInventoryHandler* th)
);

PATTERN_FUNCTION(
    "\x40\x57\x41\x57\x48\x83\xEC\x00\x48\x8B\x51\x18",
    "xxxxxxx?xxxx",
    ZActorInventoryHandler_FinalizePendingItems,
    void(ZActorInventoryHandler* th)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x00\x48\x8B\x02\x4C\x8B\xD1",
    "xxxxxxxxxxxxxx?xxxxxx",
    ZEntityManager_GenerateDynamicObjectID,
    uint64_t(ZEntityManager* th, ZEntityRef& entityRef, EDynamicEntityType dynamicEntityType, uint8 flags)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x10\x57\x48\x83\xEC\x00\x49\x63\xC0",
    "xxxxxxxxx?xxx",
    ZEntityManager_GenerateDynamicObjectID2,
    uint64_t(ZEntityManager* th, uint64_t baseID, EDynamicEntityType dynamicEntityType)
);

PATTERN_FUNCTION(
    "\x4C\x8B\xDC\x49\x89\x6B\x08",
    "xxxxxxx",
    ZWorldInventory_RequestNewItem,
    uint32(ZWorldInventory* th, const ZRepositoryID& repId, ZDelegate<void(unsigned int, TEntityRef<IItemBase>)> callback,
        uint64_t entityID, bool bLoading, const ZEntityRef& rParentSpatial, const ZEntityRef& rCreator)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x20\x48\x89\x54\x24\x10\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x00\x4C\x8B\xE9",
    "xxxxxxxxxxxxxxxxxxxxxxxx?xxx",
    ZWorldInventory_DestroyItem,
    void(ZWorldInventory* th, TEntityRef<IItemBase> rItemInc)
);

PATTERN_FUNCTION(
    "\x40\x53\x56\x41\x54\x41\x55\x41\x57\x48\x83\xEC\x00\x48\x8B\x84\x24\x80\x00\x00\x00",
    "xxxxxxxxxxxx?xxxxxxxx",
    ZResourceManager_GetResourceIndex,
    ZResourceIndex* (ZResourceManager* th, ZResourceIndex& result, const ZRuntimeResourceID& ridResource,
        int32_t nPriority, bool& bOutStartLoading)
);

PATTERN_FUNCTION(
    "\x40\x55\x56\x57\x41\x54\x41\x55\x48\x8D\x6C\x24\xC9",
    "xxxxxxxxxxxxx",
    ZResourceContainer_UnmountPackages,
    bool(ZResourceContainer* th, uint8 packageId)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x41\x56\x48\x83\xEC\x00\x48\x8B\xEA\x48\x8B\xF1\x33\xDB",
    "xxxxxxxxxxxxxxxxxxxxxxxxx?xxxxxxxx",
    ZRoomManager_GetRoomFromPoint,
    uint16(ZRoomManager* th, const float4& vPointWS)
);

PATTERN_FUNCTION(
    "\x40\x55\x56\x41\x57\x48\x8D\x6C\x24\xB9\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xFF",
    "xxxxxxxxxxxxx????xxx",
    ZStashPointEntity_RequestContentLoad,
    void(ZStashPointEntity* th)
);

PATTERN_FUNCTION(
    "\x48\x89\x5C\x24\x10\x57\x48\x83\xEC\x00\x48\x8B\xDA\x48\x8B\xF9\xB2",
    "xxxxxxxxx?xxxxxxx",
    ZStashPointEntity_SpawnOutfit,
    void(ZStashPointEntity* th, const ZRepositoryID& id)
);

PATTERN_FUNCTION(
    "\x48\x89\x6C\x24\x20\x56\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x05",
    "xxxxxxxxxxxxx????xxx",
    ZStashPointEntity_SpawnItem,
    uint32_t(ZStashPointEntity* th, const ZRepositoryID& repId, const TArray<ZRepositoryID>& instanceModifiersToApply, bool isContainerItem)
);