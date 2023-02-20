#pragma once

#include "ZActor.h"
#include "ZHM5BaseCharacter.h"
#include "ZPhysics.h"
#include "ZPrimitives.h"

/*
class ZHitman5
{
public:
    PAD(0x710);
    ZRepositoryID m_InitialOutfitId;
};*/

class IFutureCameraState :
    public IComponentInterface
{
public:
    virtual ~IFutureCameraState() {}
    virtual void IFutureCameraState_unk0() = 0;
    virtual void IFutureCameraState_unk1() = 0;
    virtual void IFutureCameraState_unk2() = 0;
    virtual void IFutureCameraState_unk3() = 0;
};

class IInventoryOwner
{
public:
    virtual void IInventoryOwner_unk0() = 0;
    virtual void IInventoryOwner_unk1() = 0;
    virtual void IInventoryOwner_unk2() = 0;
    virtual void IInventoryOwner_unk3() = 0;
    virtual void IInventoryOwner_unk4() = 0;
    virtual void IInventoryOwner_unk5() = 0;
    virtual void IInventoryOwner_unk6() = 0;
    virtual void IInventoryOwner_unk7() = 0;
    virtual void IInventoryOwner_unk8() = 0;
};

class IIKControllerOwner :
    public IComponentInterface
{
public:
    virtual ~IIKControllerOwner() {}
    virtual void IIKControllerOwner_unk0() = 0;
    virtual void IIKControllerOwner_unk1() = 0;
    virtual void IIKControllerOwner_unk2() = 0;
    virtual void IIKControllerOwner_unk3() = 0;
    virtual void IIKControllerOwner_unk4() = 0;
    virtual void IIKControllerOwner_unk5() = 0;
    virtual void IIKControllerOwner_unk6() = 0;
    virtual void IIKControllerOwner_unk7() = 0;
    virtual void IIKControllerOwner_unk8() = 0;
    virtual void IIKControllerOwner_unk9() = 0;
    virtual void IIKControllerOwner_unk10() = 0;
    virtual void IIKControllerOwner_unk11() = 0;
    virtual void IIKControllerOwner_unk12() = 0;
    virtual void IIKControllerOwner_unk13() = 0;
    virtual void IIKControllerOwner_unk14() = 0;
    virtual void IIKControllerOwner_unk15() = 0;
    virtual void IIKControllerOwner_unk16() = 0;
    virtual void IIKControllerOwner_unk17() = 0;
    virtual void IIKControllerOwner_unk18() = 0;
    virtual void IIKControllerOwner_unk19() = 0;
    virtual void IIKControllerOwner_unk20() = 0;
    virtual void IIKControllerOwner_unk21() = 0;
    virtual void IIKControllerOwner_unk22() = 0;
    virtual void IIKControllerOwner_unk23() = 0;
    virtual void IIKControllerOwner_unk24() = 0;
    virtual void IIKControllerOwner_unk25() = 0;
    virtual void IIKControllerOwner_unk26() = 0;
    virtual void IIKControllerOwner_unk27() = 0;
    virtual void IIKControllerOwner_unk28() = 0;
    virtual void IIKControllerOwner_unk29() = 0;
    virtual void IIKControllerOwner_unk30() = 0;
    virtual void IIKControllerOwner_unk31() = 0;
    virtual void IIKControllerOwner_unk32() = 0;
    virtual void IIKControllerOwner_unk33() = 0;
    virtual void IIKControllerOwner_unk34() = 0;
};

class IControllableCharacter :
    public IComponentInterface
{
public:
    virtual ~IControllableCharacter() {}
    virtual void IControllableCharacter_unk0() = 0;
    virtual void IControllableCharacter_unk1() = 0;
    virtual void IControllableCharacter_unk2() = 0;
    virtual void IControllableCharacter_unk3() = 0;
    virtual void IControllableCharacter_unk4() = 0;
    virtual void IControllableCharacter_unk5() = 0;
    virtual void IControllableCharacter_unk6() = 0;
    virtual void IControllableCharacter_unk7() = 0;
    virtual void IControllableCharacter_unk8() = 0;
};

class IHM5ActionEntityListener :
    public IComponentInterface
{
public:
    virtual ~IHM5ActionEntityListener() {}
    virtual void IHM5ActionEntityListener_unk0() = 0;
    virtual void IHM5ActionEntityListener_unk1() = 0;
    virtual void IHM5ActionEntityListener_unk2() = 0;
    virtual void IHM5ActionEntityListener_unk3() = 0;
    virtual void IHM5ActionEntityListener_unk4() = 0;
    virtual void IHM5ActionEntityListener_unk5() = 0;
};

class ISavableEntity :
    public IComponentInterface
{
public:
    virtual ~ISavableEntity() {}
    virtual void ISavableEntity_unk0() = 0;
    virtual void ISavableEntity_unk1() = 0;
};

class IAIGameplayConcept :
    public IComponentInterface
{
public:
    virtual ~IAIGameplayConcept() {}
    virtual void IAIGameplayConcept_unk0() = 0;
};

class ICharacterMovementState :
    public IComponentInterface
{
public:
    virtual ~ICharacterMovementState() {}
    virtual void ICharacterMovementState_unk0() = 0;
    virtual void ICharacterMovementState_unk1() = 0;
    virtual void ICharacterMovementState_unk2() = 0;
    virtual void ICharacterMovementState_unk3() = 0;
    virtual void ICharacterMovementState_unk4() = 0;
    virtual void ICharacterMovementState_unk5() = 0;
    virtual void ICharacterMovementState_unk6() = 0;
    virtual void ICharacterMovementState_unk7() = 0;
    virtual void ICharacterMovementState_unk8() = 0;
    virtual void ICharacterMovementState_unk9() = 0;
    virtual void ICharacterMovementState_unk10() = 0;
    virtual void ICharacterMovementState_unk11() = 0;
    virtual void ICharacterMovementState_unk12() = 0;
    virtual void ICharacterMovementState_unk13() = 0;
    virtual void ICharacterMovementState_unk14() = 0;
    virtual void ICharacterMovementState_unk15() = 0;
    virtual void ICharacterMovementState_unk16() = 0;
    virtual void ICharacterMovementState_unk17() = 0;
    virtual void ICharacterMovementState_unk18() = 0;
    virtual void ICharacterMovementState_unk19() = 0;
    virtual void ICharacterMovementState_unk20() = 0;
    virtual void ICharacterMovementState_unk21() = 0;
    virtual void ICharacterMovementState_unk22() = 0;
    virtual void ICharacterMovementState_unk23() = 0;
    virtual void ICharacterMovementState_unk24() = 0;
    virtual void ICharacterMovementState_unk25() = 0;
    virtual void ICharacterMovementState_unk26() = 0;
    virtual void ICharacterMovementState_unk27() = 0;
    virtual void ICharacterMovementState_unk28() = 0;
    virtual void ICharacterMovementState_unk29() = 0;
    virtual void ICharacterMovementState_unk30() = 0;
    virtual void ICharacterMovementState_unk31() = 0;
    virtual void ICharacterMovementState_unk32() = 0;
    virtual void ICharacterMovementState_unk33() = 0;
    virtual void ICharacterMovementState_unk34() = 0;
    virtual void ICharacterMovementState_unk35() = 0;
    virtual void ICharacterMovementState_unk36() = 0;
    virtual void ICharacterMovementState_unk37() = 0;
    virtual void ICharacterMovementState_unk38() = 0;
    virtual void ICharacterMovementState_unk39() = 0;
    virtual void ICharacterMovementState_unk40() = 0;
    virtual void ICharacterMovementState_unk41() = 0;
    virtual void ICharacterMovementState_unk42() = 0;
    virtual void ICharacterMovementState_unk43() = 0;
    virtual void ICharacterMovementState_unk44() = 0;
    virtual void ICharacterMovementState_unk45() = 0;
    virtual void ICharacterMovementState_unk46() = 0;
    virtual void ICharacterMovementState_unk47() = 0;
    virtual void ICharacterMovementState_unk48() = 0;
    virtual void ICharacterMovementState_unk49() = 0;
    virtual void ICharacterMovementState_unk50() = 0;
    virtual void ICharacterMovementState_unk51() = 0;
    virtual void ICharacterMovementState_unk52() = 0;
    virtual void ICharacterMovementState_unk53() = 0;
    virtual void ICharacterMovementState_unk54() = 0;
    virtual void ICharacterMovementState_unk55() = 0;
    virtual void ICharacterMovementState_unk56() = 0;
    virtual void ICharacterMovementState_unk57() = 0;
    virtual void ICharacterMovementState_unk58() = 0;
    virtual void ICharacterMovementState_unk59() = 0;
    virtual void ICharacterMovementState_unk60() = 0;
    virtual void ICharacterMovementState_unk61() = 0;
    virtual void ICharacterMovementState_unk62() = 0;
};

class ICharacterCombatState :
    public IComponentInterface
{
public:
    virtual ~ICharacterCombatState() {}
    virtual void ICharacterCombatState_unk0() = 0;
    virtual void ICharacterCombatState_unk1() = 0;
    virtual void ICharacterCombatState_unk2() = 0;
    virtual void ICharacterCombatState_unk3() = 0;
    virtual void ICharacterCombatState_unk4() = 0;
    virtual void ICharacterCombatState_unk5() = 0;
    virtual void ICharacterCombatState_unk6() = 0;
    virtual void ICharacterCombatState_unk7() = 0;
    virtual void ICharacterCombatState_unk8() = 0;
    virtual void ICharacterCombatState_unk9() = 0;
    virtual void ICharacterCombatState_unk10() = 0;
    virtual void ICharacterCombatState_unk11() = 0;
    virtual void ICharacterCombatState_unk12() = 0;
    virtual void ICharacterCombatState_unk13() = 0;
    virtual void ICharacterCombatState_unk14() = 0;
    virtual void ICharacterCombatState_unk15() = 0;
    virtual void ICharacterCombatState_unk16() = 0;
    virtual void ICharacterCombatState_unk17() = 0;
    virtual void ICharacterCombatState_unk18() = 0;
    virtual void ICharacterCombatState_unk19() = 0;
    virtual void ICharacterCombatState_unk20() = 0;
    virtual void ICharacterCombatState_unk21() = 0;
    virtual void ICharacterCombatState_unk22() = 0;
    virtual void ICharacterCombatState_unk23() = 0;
    virtual void ICharacterCombatState_unk24() = 0;
    virtual void ICharacterCombatState_unk25() = 0;
    virtual void ICharacterCombatState_unk26() = 0;
    virtual void ICharacterCombatState_unk27() = 0;
    virtual void ICharacterCombatState_unk28() = 0;
    virtual void ICharacterCombatState_unk29() = 0;
};

class ICharacterCoreInventoryState :
    public IComponentInterface
{
public:
    virtual ~ICharacterCoreInventoryState() {}
    virtual void ICharacterCoreInventoryState_unk0() = 0;
    virtual void ICharacterCoreInventoryState_unk1() = 0;
    virtual void ICharacterCoreInventoryState_unk2() = 0;
    virtual void ICharacterCoreInventoryState_unk3() = 0;
    virtual void ICharacterCoreInventoryState_unk4() = 0;
    virtual void ICharacterCoreInventoryState_unk5() = 0;
    virtual void ICharacterCoreInventoryState_unk6() = 0;
};

class ICharacterInventoryState :
    public ICharacterCoreInventoryState
{
public:
    virtual ~ICharacterInventoryState() {}
    virtual void ICharacterInventoryState_unk0() = 0;
    virtual void ICharacterInventoryState_unk1() = 0;
    virtual void ICharacterInventoryState_unk2() = 0;
    virtual void ICharacterInventoryState_unk3() = 0;
    virtual void ICharacterInventoryState_unk4() = 0;
    virtual void ICharacterInventoryState_unk5() = 0;
    virtual void ICharacterInventoryState_unk6() = 0;
    virtual void ICharacterInventoryState_unk7() = 0;
    virtual void ICharacterInventoryState_unk8() = 0;
    virtual void ICharacterInventoryState_unk9() = 0;
    virtual void ICharacterInventoryState_unk10() = 0;
    virtual void ICharacterInventoryState_unk11() = 0;
    virtual void ICharacterInventoryState_unk12() = 0;
    virtual void ICharacterInventoryState_unk13() = 0;
    virtual void ICharacterInventoryState_unk14() = 0;
    virtual void ICharacterInventoryState_unk15() = 0;
    virtual void ICharacterInventoryState_unk16() = 0;
    virtual void ICharacterInventoryState_unk17() = 0;
    virtual void ICharacterInventoryState_unk18() = 0;
    virtual void ICharacterInventoryState_unk19() = 0;
    virtual void ICharacterInventoryState_unk20() = 0;
    virtual void ICharacterInventoryState_unk21() = 0;
    virtual void ICharacterInventoryState_unk22() = 0;
    virtual void ICharacterInventoryState_unk23() = 0;
    virtual void ICharacterInventoryState_unk24() = 0;
    virtual void ICharacterInventoryState_unk25() = 0;
    virtual void ICharacterInventoryState_unk26() = 0;
    virtual void ICharacterInventoryState_unk27() = 0;
    virtual void ICharacterInventoryState_unk28() = 0;
    virtual void ICharacterInventoryState_unk29() = 0;
    virtual void ICharacterInventoryState_unk30() = 0;
    virtual void ICharacterInventoryState_unk31() = 0;
    virtual void ICharacterInventoryState_unk32() = 0;
};

class ICharacterFriskingState :
    public IComponentInterface
{
public:
    virtual ~ICharacterFriskingState() {}
    virtual void ICharacterFriskingState_unk0() = 0;
    virtual void ICharacterFriskingState_unk1() = 0;
    virtual void ICharacterFriskingState_unk2() = 0;
    virtual void ICharacterFriskingState_unk3() = 0;
    virtual void ICharacterFriskingState_unk4() = 0;
    virtual void ICharacterFriskingState_unk5() = 0;
    virtual void ICharacterFriskingState_unk6() = 0;
    virtual void ICharacterFriskingState_unk7() = 0;
    virtual void ICharacterFriskingState_unk8() = 0;
    virtual void ICharacterFriskingState_unk9() = 0;
    virtual void ICharacterFriskingState_unk10() = 0;
    virtual void ICharacterFriskingState_unk11() = 0;
    virtual void ICharacterFriskingState_unk12() = 0;
    virtual void ICharacterFriskingState_unk13() = 0;
    virtual void ICharacterFriskingState_unk14() = 0;
    virtual void ICharacterFriskingState_unk15() = 0;
    virtual void ICharacterFriskingState_unk16() = 0;
    virtual void ICharacterFriskingState_unk17() = 0;
    virtual void ICharacterFriskingState_unk18() = 0;
    virtual void ICharacterFriskingState_unk19() = 0;
    virtual void ICharacterFriskingState_unk20() = 0;
    virtual void ICharacterFriskingState_unk21() = 0;
    virtual void ICharacterFriskingState_unk22() = 0;
    virtual void ICharacterFriskingState_unk23() = 0;
    virtual void ICharacterFriskingState_unk24() = 0;
    virtual void ICharacterFriskingState_unk25() = 0;
    virtual void ICharacterFriskingState_unk26() = 0;
    virtual void ICharacterFriskingState_unk27() = 0;
    virtual void ICharacterFriskingState_unk28() = 0;
    virtual void ICharacterFriskingState_unk29() = 0;
    virtual void ICharacterFriskingState_unk30() = 0;
};

class ICharacterShowItemState :
    public ICharacterCoreInventoryState
{
public:
    virtual ~ICharacterShowItemState() {}
    virtual void ICharacterShowItemState_unk0() = 0;
    virtual void ICharacterShowItemState_unk1() = 0;
    virtual void ICharacterShowItemState_unk2() = 0;
    virtual void ICharacterShowItemState_unk3() = 0;
    virtual void ICharacterShowItemState_unk4() = 0;
    virtual void ICharacterShowItemState_unk5() = 0;
    virtual void ICharacterShowItemState_unk6() = 0;
    virtual void ICharacterShowItemState_unk7() = 0;
};

class ICharacterArrestState :
    public ICharacterCoreInventoryState
{
public:
    virtual ~ICharacterArrestState() {}
    virtual void ICharacterArrestState_unk0() = 0;
    virtual void ICharacterArrestState_unk1() = 0;
    virtual void ICharacterArrestState_unk2() = 0;
    virtual void ICharacterArrestState_unk3() = 0;
};

class ICharacterIllegalInteractionsState :
    public IComponentInterface
{
public:
    virtual ~ICharacterIllegalInteractionsState() {}
    virtual void ICharacterIllegalInteractionsState_unk0() = 0;
    virtual void ICharacterIllegalInteractionsState_unk1() = 0;
    virtual void ICharacterIllegalInteractionsState_unk2() = 0;
    virtual void ICharacterIllegalInteractionsState_unk3() = 0;
    virtual void ICharacterIllegalInteractionsState_unk4() = 0;
    virtual void ICharacterIllegalInteractionsState_unk5() = 0;
};

class ICharacterLocationState :
    public IComponentInterface
{
public:
    virtual ~ICharacterLocationState() {}
    virtual void ICharacterLocationState_unk0() = 0;
    virtual void ICharacterLocationState_unk1() = 0;
    virtual void ICharacterLocationState_unk2() = 0;
    virtual void ICharacterLocationState_unk3() = 0;
    virtual void ICharacterLocationState_unk4() = 0;
};

class ICharacterCameraState :
    public IComponentInterface
{
public:
    virtual ~ICharacterCameraState() {}
    virtual void ICharacterCameraState_unk0() = 0;
};

class ZPhysicsSystemEntity;
class IChildNetworkEntity;
class ZItemPlacementConfigurationEntity;
class IVariationResourceEntity;
class ZBodyPartEntity;
class ZHeroGuideController;
class ZHeroIKController;
class ZCheatController;
class ZHeroInteractionController;
class ZHeroActorTagController;
class ZCharacter;
class ZHM5MainCamera;

class ZHM5Animator :
    public ZEntityImpl,
    public IBoneAnimator
{
public:
};

class ZHitman5 :
    public ZHM5BaseCharacter,
    public IFutureCameraState, // 720
    public ICharacterCollision, // 728
    public IInventoryOwner, // 736
    public IIKControllerOwner, // 744
    public IControllableCharacter, // 752
    public IHM5ActionEntityListener, // 760
    public ISavableEntity, // 768
    public IAIGameplayConcept, // 776
    public ICharacterMovementState, // 784
    public ICharacterCombatState, // 792
    public ICharacterInventoryState, // 800
    public ICharacterFriskingState, // 808
    public ICharacterShowItemState, // 816
    public ICharacterArrestState, // 824
    public ICharacterIllegalInteractionsState, // 832
    public ICharacterLocationState, // 840
    public ICharacterCameraState // 848
{
public:
    PAD(0x3B8); // 0x358
    ZRepositoryID m_InitialOutfitId; // 0x710
    ZEntityRef m_MorphemeEntityID; // 0x720
    ZEntityRef m_Animator; // 0x728
    TEntityRef<ZPhysicsSystemEntity> m_rRagdollEntity; // 0x730
    TEntityRef<IChildNetworkEntity> m_rDefaultPickupNetwork; // 0x740
    TEntityRef<ZItemPlacementConfigurationEntity> m_placementconfiguration; // 0x750
    bool m_bStreamableDisguiseGive; // 0x760
    TEntityRef<IVariationResourceEntity> m_DefaultWeaponVariationResource; // 0x768
    PAD(0x30); // 0x778
    ZGuid m_CharacterId; // 0x7A8
    PAD(0x78); // 0x7B8
    TEntityRef<ZBodyPartEntity> m_pVRHeadReplacement; // 0x830
    TEntityRef<ZBodyPartEntity> m_pVROldHeadReplacement; // 0x840
    ZRuntimeResourceID m_SeasonOneHead1; // 0x850
    ZRuntimeResourceID m_SeasonOneHead2; // 0x858
    ZRuntimeResourceID m_SeasonOneHead3; // 0x860
    ZRuntimeResourceID m_SeasonOneHead4; // 0x868
    ZRuntimeResourceID m_SeasonOneHead5; // 0x870
    PAD(0x4D0); // 0x878
    bool m_bIsInvincible; // 0xD48
    TEntityRef<ZHeroGuideController> m_pGuideController; // 0xD50
    TEntityRef<ZHeroIKController> m_pIKController; // 0xD60
    TEntityRef<ZCheatController> m_pCheatController; // 0xD70
    TEntityRef<ZHeroInteractionController> m_pInteractionController; // 0xD80
    TEntityRef<ZHeroActorTagController> m_pActorTagController; // 0xD90
    TEntityRef<ZCharacter> m_pCharacter; // 0xDA0
    PAD(0x398); // 0xDB0

    union
    {
        uint32_t m_flags; // 0x1148

        struct
        {
            bool m_bActivated : 1;
            bool m_bActivatingHitman : 1;
            bool m_bInEditMode : 1;
            bool m_bUnk3 : 1;
        };
    };

    PAD(0x1C); // 0x114C
    TEntityRef<ZHM5MainCamera> m_rMainCamera; // 0x1168
    PAD(0x98); // 0x1178
};

static_assert(offsetof(ZHitman5, m_InitialOutfitId) == 0x710);
static_assert(offsetof(ZHitman5, m_CharacterId) == 0x7A8);
static_assert(offsetof(ZHitman5, m_pVRHeadReplacement) == 0x830);
static_assert(offsetof(ZHitman5, m_bIsInvincible) == 0xD48);
static_assert(offsetof(ZHitman5, m_pCharacter) == 0xDA0);
static_assert(offsetof(ZHitman5, m_flags) == 0x1148);
static_assert(offsetof(ZHitman5, m_rMainCamera) == 0x1168);
static_assert(sizeof(ZHitman5) == 0x1210);
