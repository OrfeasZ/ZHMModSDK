#pragma once

#include "ZEntity.h"
#include "ZSpatialEntity.h"
#include "ZGeomEntity.h"
#include "ZHitman5.h"
#include "ZGameTime.h"

class IItemBase : public IComponentInterface {};

class IKeywordProvider : public IComponentInterface {};

class IItem : public IKeywordProvider {};

class IKeywordHolder : public IComponentInterface {};

class ZUIDataProvider {
public:
    PAD(0x40);
};

class IBoolConditionListener : public IComponentInterface {};

class IColliderController : public IComponentInterface {};

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
class ZItemRepositoryKeyEntity;

struct SItemConfig {
    eItemType m_ItemType;
    eItemSize m_ItemSize;
    EAnimSetType m_eMainAnimSetType;
    EAnimSetType m_eCarryAnimSetType;
    EItemGripType m_eGripAnimType;
    EItemGripType m_eGripAnimTypeVR;
    eItemHands m_ItemHandsIdle;
    eItemHands m_ItemHandsUse;
    PAD(0xDA);
};

class ZItemConfigDescriptor {
public:
    virtual ~ZItemConfigDescriptor() = 0;

    const ZRepositoryID m_ItemID;
    const TArray<ZRepositoryID> m_ModifierIDs;
    bool m_bConfigurationApplied;
    ZRuntimeResourceID m_RuntimeResource;
    ZString m_sTitle;
    SItemConfig m_ItemConfig;
};

class ZVrHandAlignPose;

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
    enum class EUseTypes {
        EUT_CantUse = 0,
        EUT_Toggle  = 1,
        EUT_TurnOn  = 2,
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
    TEntityRef<ZSpatialEntity> m_PosPrimingHandAttach; // 0x158
    TEntityRef<ZSpatialEntity> m_PosLeftHandAttach; // 0x168
    TEntityRef<ZSpatialEntity> m_PosLeftHandAttachVROverride; // 0x178
    TEntityRef<ZSpatialEntity> m_PosLeftPrimingHandAttach; // 0x188
    TEntityRef<ZSpatialEntity> m_PosHolsterAttachVR; // 0x198
    TArray<TEntityRef<ZVrHandAlignPose>> m_aHandAlignPoses; // 0x1A8
    TEntityRef<ZSpatialEntity> m_PivotAttacher; // 0x1C0
    TEntityRef<ZSpatialEntity> m_PosFreeBoneAttach; // 0x1D0
    TEntityRef<ZSpatialEntity> m_PosBackAttach; // 0x1E0
    TEntityRef<ZSpatialEntity> m_PosDualBackAttach; // 0x1F0
    TEntityRef<ZSpatialEntity> m_PosPlacementAttach; // 0x200
    TEntityRef<ZBoxVolumeEntity> m_PlacementVolume; // 0x210
    float32 m_fWakeTimeOnPlaceInWorld; // 0x220
    TEntityRef<IAnimPlayerEntity> m_AnimPlayer; // 0x228
    TEntityRef<ZBehaviorEntityBase> m_rPickupBehavior; // 0x238
    TEntityRef<IItemContainer> m_rItemContainer; // 0x248
    TEntityRef<ZEventConsumerCollection> m_EventConsumerCollection; // 0x258
    TEntityRef<ZSpatialEntity> m_rBeaconSpatial; // 0x268
    TEntityRef<IBoolCondition> m_rVisibleInInventory; // 0x278
    TEntityRef<ZValueBool> m_rItemPickupEnabled; // 0x288
    TEntityRef<ZValueBool> m_rItemVisible; // 0x298
    TEntityRef<ZValueBool> m_rItemInPhysicsWorld; // 0x2A8
    TEntityRef<ZValueBool> m_rItemDestroyed; // 0x2B8
    TEntityRef<ZValueBool> m_rItemTurnedOn; // 0x2C8
    TEntityRef<ZValueBool> m_rItemOwned; // 0x2D8
    TEntityRef<ZValueBool> m_rItemCanTurnOn; // 0x2E8
    TEntityRef<ZValueBool> m_rItemCanTurnOff; // 0x2F8
    TEntityRef<IVariationResourceEntity> m_pVariationResource; // 0x308
    ZEntityRef m_rSpawner;
    ZEntityRef m_rFactoryEntity;
    PAD(0x10);
    TEntityRef<ZGeomEntity> m_pGeomEntity; //0x2C0
    PAD(0x1A8);
};

class IItemWeapon : public IComponentInterface {
};

struct SWeaponConfig;

class IFirearm : public IComponentInterface {
public:
    virtual ~IFirearm() = 0;
    virtual void IFirearm_unk5() = 0;
    virtual void IFirearm_unk6() = 0;
    virtual void IFirearm_unk7() = 0;
    virtual void IFirearm_unk8() = 0;
    virtual void IFirearm_unk9() = 0;
    virtual void IFirearm_unk10() = 0;
    virtual void IFirearm_unk11() = 0;
    virtual void IFirearm_unk12() = 0;
    virtual void IFirearm_unk13() = 0;
    virtual void IFirearm_unk14() = 0;
    virtual void IFirearm_unk15() = 0;
    virtual void IFirearm_unk16() = 0;
    virtual const SWeaponConfig& GetWeaponConfig() const = 0;
    virtual void IFirearm_unk18() = 0;
    virtual void IFirearm_unk19() = 0;
    virtual void SetBulletsInMagazine(int32_t nBullets) = 0;
    virtual uint16_t GetBulletsInMagazine() const = 0;
    virtual void IFirearm_unk22() = 0;
    virtual void IFirearm_unk23() = 0;
    virtual void IFirearm_unk24() = 0;
    virtual void IFirearm_unk25() = 0;
    virtual void IFirearm_unk26() = 0;
    virtual void IFirearm_unk27() = 0;
    virtual void IFirearm_unk28() = 0;
    virtual int GetMagazineCapacity() const = 0;
    virtual eAmmoType GetAmmoType() const = 0;
    virtual void IFirearm_unk31() = 0;
    virtual void IFirearm_unk32() = 0;
    virtual void IFirearm_unk33() = 0;
    virtual void IFirearm_unk34() = 0;
    virtual void IFirearm_unk35() = 0;
    virtual void IFirearm_unk36() = 0;
    virtual void IFirearm_unk37() = 0;
    virtual void IFirearm_unk38() = 0;
    virtual void IFirearm_unk39() = 0;
    virtual void IFirearm_unk40() = 0;
    virtual void IFirearm_unk41() = 0;
    virtual void IFirearm_unk42() = 0;
    virtual void IFirearm_unk43() = 0;
    virtual void IFirearm_unk44() = 0;
    virtual void IFirearm_unk45() = 0;
    virtual void IFirearm_unk46() = 0;
    virtual void IFirearm_unk47() = 0;
    virtual void IFirearm_unk48() = 0;
    virtual void IFirearm_unk49() = 0;
    virtual void IFirearm_unk50() = 0;
    virtual void IFirearm_unk51() = 0;
    virtual void IFirearm_unk52() = 0;
    virtual void IFirearm_unk53() = 0;
    virtual void IFirearm_unk54() = 0;
    virtual void IFirearm_unk55() = 0;
    virtual void IFirearm_unk56() = 0;
    virtual void IFirearm_unk57() = 0;
    virtual void IFirearm_unk58() = 0;
    virtual bool IsFiring() const = 0;
    virtual float GetTimeBetweenBullets() const = 0;
    virtual void IFirearm_unk61() = 0;
    virtual void IFirearm_unk62() = 0;
    virtual void IFirearm_unk63() = 0;
    virtual void IFirearm_unk64() = 0;
    virtual void IFirearm_unk65() = 0;
    virtual void IFirearm_unk66() = 0;
    virtual void IFirearm_unk67() = 0;
    virtual void IFirearm_unk68() = 0;
    virtual void IFirearm_unk69() = 0;
    virtual void IFirearm_unk70() = 0;
    virtual void IFirearm_unk71() = 0;
    virtual void IFirearm_unk72() = 0;
    virtual void IFirearm_unk73() = 0;
    virtual void IFirearm_unk74() = 0;
    virtual void IFirearm_unk75() = 0;
    virtual void IFirearm_unk76() = 0;
    virtual void IFirearm_unk77() = 0;
    virtual void IFirearm_unk78() = 0;
    virtual void IFirearm_unk79() = 0;
    virtual void IFirearm_unk80() = 0;
    virtual void IFirearm_unk81() = 0;
    virtual void IFirearm_unk82() = 0;
    virtual void IFirearm_unk83() = 0;
    virtual void IFirearm_unk84() = 0;
    virtual void IFirearm_unk85() = 0;
    virtual void IFirearm_unk86() = 0;
    virtual void IFirearm_unk87() = 0;
    virtual void IFirearm_unk88() = 0;
    virtual void IFirearm_unk89() = 0;
    virtual void IFirearm_unk90() = 0;
};

class IParticleEmitterEntity;
class ZItemWeaponEffectGroup;
class ZHM5WeaponSoundSetupEntity;
class ZWeaponSoundSetupEntity;
class ZHM5WeaponHandPosBox;
class ZHM5ClipSpawnerEntity;
class ZManualReloadSettings;

class ZHM5ItemWeapon :
    public ZHM5Item,
    public IItemWeapon,
    public IFirearm,
    public ISavableEntity {
public:
    PAD(0xC);
    eWeaponType m_WeaponType; // 0x514
    ZRuntimeResourceID m_ridClipTemplate; // 0x518
    EWeaponAnimationCategory m_eAnimationCategory; // 0x520
    ZEntityRef m_MuzzleExit; // 0x528
    ZEntityRef m_CartridgeEject; // 0x530
    float32 m_fCartridgeEjectForceMultiplier; // 0x538
    TEntityRef<IParticleEmitterEntity> m_MuzzleFlashEffect; // 0x540
    TEntityRef<IParticleEmitterEntity> m_MuzzleSmokeEffect; // 0x550
    TEntityRef<ZItemWeaponEffectGroup> m_MuzzleFlashEffectGroup; // 0x560
    TEntityRef<ZItemWeaponEffectGroup> m_MuzzleSmokeEffectGroup; // 0x570
    TEntityRef<ZHM5WeaponSoundSetupEntity> m_SoundSetup; // 0x580
    TEntityRef<ZWeaponSoundSetupEntity> m_AudioSetup; // 0x590
    TEntityRef<ZHM5WeaponHandPosBox> m_LeftHandPos; // 0x5A0
    ZEntityRef m_AmmoProperties; // 0x5B0
    bool m_bConnectsToTarget; // 0x5B8
    float32 m_fMuzzleEnergyMultiplier; // 0x5BC
    bool m_bScopedWeapon; // 0x5C0
    ZEntityRef m_ScopePosition; // 0x5C8
    ZEntityRef m_ScopeCrossHair; // 0x5D0
    ZEntityRef m_rSpecialImpactAct; // 0x5D8
    ZEntityRef m_rSuperSpecialTriggerEffect; // 0x5E0
    TArray<TEntityRef<ZHM5ClipSpawnerEntity>> m_rClipMeshProviders; // 0x5E8
    TArray<TEntityRef<ZManualReloadSettings>> m_aManualReloadSettings; // 0x600
    PAD(0x310); // 0x618
    uint16 m_nBulletsToFire; // 0x928
    PAD(0x6); // 0x92A
    uint16 m_nBulletsFired; // 0x930
    PAD(0xE); // 0x932
    ZGameTime m_tLastShootTime; // 0x940
    float32 m_fTimeBetweenBullets; // 0x948
    float m_fPrecisionFactor; // 0x94C
    PAD(0x60);
};

class ZItemSpawner : public ZSpatialEntity, public IItemOwner, public ISavableEntity //Size: 0x138
{
public:
    enum EPhysicsMode {
        EPM_DEFINED_BY_ITEM = 0,
        EPM_DYNAMIC         = 1,
        EPM_SLEEPING        = 2,
        EPM_KINEMATIC       = 3
    };

    PAD(0x40);
    TEntityRef<ZItemRepositoryKeyEntity> m_rMainItemKey; // 0xF0
    TArray<TEntityRef<ZItemRepositoryKeyEntity>> m_rInventoryItems; // 0x100
    EPhysicsMode m_ePhysicsMode; // 0x118
    EDisposalType m_eDisposalTypeOverwrite; // 0x11C
    ZEntityRef m_rKeywords; // 0x120
    ZEntityRef m_rSetpieceUsed; // 0x128
    bool m_bSpawnOnStart; // 0x130
    bool m_bUsePlacementAttach; // 0x131
    bool m_bSetAIPerceptible; // 0x132
};