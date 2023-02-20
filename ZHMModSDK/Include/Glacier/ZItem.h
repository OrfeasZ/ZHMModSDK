#pragma once

#include "ZEntity.h"
#include "ZSpatialEntity.h"
#include "ZGeomEntity.h"
#include "ZHitman5.h"

class IItemBase : public IComponentInterface
{

};

class IKeywordProvider : public IComponentInterface
{

};

class IItem : public IKeywordProvider
{

};

class IKeywordHolder : public IComponentInterface
{

};

class ZUIDataProvider
{
public:
    PAD(0x40);
};

class IBoolConditionListener : public IComponentInterface
{

};

class IColliderController : public IComponentInterface
{

};

class ZHM5Action;
class IPhysicsAccessor;
class ZBoxVolumeEntity;
class IAnimPlayerEntity;
class ZBehaviorEntityBase;
class IItemContainer;
class ZEventConsumerCollection;
class IBoolCondition;
class ZValueBool;
class IVariationResourceEntity;

class ZItemConfigDescriptor
{
public:
    virtual ~ZItemConfigDescriptor() = 0;

    ZRepositoryID m_RepositoryId;
    PAD(0x28);
    ZString m_sTitle;
};

class ZHM5Item :
    public ZEntityImpl, // Offset 0x0
    public IItemBase, // Offset 0x18
    public IItem, // Offset 0x20
    public IKeywordHolder, // Offset 0x28
    public ZUIDataProvider, // Offset 0x30
    public IAIGameplayConcept, // Offset 0x70
    public IBoolConditionListener, // Offset 0x78
    public IColliderController // Offset 0x80
{
public:
    enum class EUseTypes
    {
        EUT_CantUse = 0,
        EUT_Toggle = 1,
        EUT_TurnOn = 2,
    };

    PAD(0x10);
    ZItemConfigDescriptor* m_pItemConfigDescriptor;
    PAD(0x78);
    //PAD(0x58);
    //EDisposalType m_DisposalType; // 0xE0
    //bool m_bKinematic; // 0xE4
    //EActorSoundDefs m_eInvestigateSoundDef; // 0xE8
    //TEntityRef<ZHM5Action> m_pUseAction; // 0xF0
    //ZHM5Item::EUseTypes m_eUseType; // 0x100
    //bool m_bForceGeomGlowType; // 0x104
    //ERenderGlowTypes m_eGeomGlowType; // 0x105
    //TEntityRef<ZHM5Action> m_rPickupAction; // 0x108
    TEntityRef<ZGeomEntity> m_rGeomentity; // 0x118
    TEntityRef<IPhysicsAccessor> m_rPhysicsAccessor; // 0x128
    TEntityRef<ZSpatialEntity> m_PosHandAttach; // 0x138
    TEntityRef<ZSpatialEntity> m_PosHandAttachVROverride; // 0x148
    TEntityRef<ZSpatialEntity> m_PosLeftHandAttach; // 0x158
    TEntityRef<ZSpatialEntity> m_PosLeftPrimingHandAttach; // 0x168
    TEntityRef<ZSpatialEntity> m_PosFreeBoneAttach; // 0x178
    TEntityRef<ZSpatialEntity> m_PosBackAttach; // 0x188
    TEntityRef<ZSpatialEntity> m_PosPlacementAttach; // 0x198
    TEntityRef<ZBoxVolumeEntity> m_PlacementVolume; // 0x1A8
    float32 m_fWakeTimeOnPlaceInWorld; // 0x1B8
    TEntityRef<IAnimPlayerEntity> m_AnimPlayer; // 0x1C0
    TEntityRef<ZBehaviorEntityBase> m_rPickupBehavior; // 0x1D0
    TEntityRef<IItemContainer> m_rItemContainer; // 0x1E0
    TEntityRef<ZEventConsumerCollection> m_EventConsumerCollection; // 0x1F0
    TEntityRef<ZSpatialEntity> m_rBeaconSpatial; // 0x200
    TEntityRef<IBoolCondition> m_rVisibleInInventory; // 0x210
    TEntityRef<ZValueBool> m_rItemPickupEnabled; // 0x220
    TEntityRef<ZValueBool> m_rItemVisible; // 0x230
    TEntityRef<ZValueBool> m_rItemInPhysicsWorld; // 0x240
    TEntityRef<ZValueBool> m_rItemDestroyed; // 0x250
    TEntityRef<ZValueBool> m_rItemTurnedOn; // 0x260
    TEntityRef<ZValueBool> m_rItemOwned; // 0x270
    TEntityRef<ZValueBool> m_rItemCanTurnOn; // 0x280
    TEntityRef<ZValueBool> m_rItemCanTurnOff; // 0x290
    TEntityRef<IVariationResourceEntity> m_pVariationResource; // 0x2A0
    ZEntityRef m_rSpawner;
    ZEntityRef m_rFactoryEntity;
    PAD(0x10);
    TEntityRef<ZGeomEntity> m_pGeomEntity; //0x2C0
    PAD(0x170);
};

class ZItemSpawner : public ZSpatialEntity, public IItemOwner, public ISavableEntity //Size: 0x138
{
public:
    enum EPhysicsMode
    {
        EPM_DEFINED_BY_ITEM = 0,
        EPM_DYNAMIC = 1,
        EPM_SLEEPING = 2,
        EPM_KINEMATIC = 3
    };

    PAD(0x40);
    TEntityRef<ZItemRepositoryKeyEntity> m_rMainItemKey; // 0xF0
    TArray<TEntityRef<ZItemRepositoryKeyEntity>> m_rInventoryItems; // 0x100
    ZItemSpawner::EPhysicsMode m_ePhysicsMode; // 0x118
    EDisposalType m_eDisposalTypeOverwrite; // 0x11C
    ZEntityRef m_rKeywords; // 0x120
    ZEntityRef m_rSetpieceUsed; // 0x128
    bool m_bSpawnOnStart; // 0x130
    bool m_bUsePlacementAttach; // 0x131
    bool m_bSetAIPerceptible; // 0x132
};
