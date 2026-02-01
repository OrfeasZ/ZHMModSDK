#pragma once

#include "Enums.h"
#include "ZEntity.h"
#include "ZPrimitives.h"
#include "ZHM5BaseCharacter.h"
#include "ZResource.h"
#include "ZHM5GridManager.h"
#include "TSet.h"
#include "ZItem.h"
#include "ZCollision.h"

#include <Logging.h>

class ZCharacterTemplateAspect;
class ZCostumeFeatureCollection;
class ZAccessoryItemPool;
class ZItemRepositoryKeyEntity;
class ZOutfitProfessionEntity;
class ZAIVisionConfigurationEntity;
class ZHTNDomainEntity;
class ZCompiledBehaviorTree;
class ZSpatialEntity;
class ZKnowledge;
class ZAnimatedActor;
class ZGlobalOutfitKit;

class IActor {
public:
    virtual void IActor_unk0() = 0;
    virtual void RequestDisable() = 0;
    virtual void IActor_unk2() = 0;
    virtual void IActor_unk3() = 0;
    virtual void IActor_unk4() = 0;
    virtual void IActor_unk5() = 0;
    virtual void IActor_unk6() = 0;
    virtual void IActor_unk7() = 0;
    virtual void IActor_unk8() = 0;
    virtual void IActor_unk9() = 0;
    virtual bool IsDead() = 0;
    virtual bool IsAlive() = 0;
    virtual bool IsPacified() = 0;
    virtual void IActor_unk13() = 0;
    virtual void IActor_unk14() = 0;
    virtual void IActor_unk15() = 0;
    virtual void IActor_unk16() = 0;
    virtual void IActor_unk17() = 0;
    virtual void IActor_unk18() = 0;
    virtual void IActor_unk19() = 0;
    virtual ZKnowledge* Knowledge() = 0;
    virtual bool ItemPickup(
        const TEntityRef<IItem>& rItem, EAttachLocation eLocation, EGameTension eMaxTension,
        bool bLeftHand, bool bGiveItem
    ) = 0;
    virtual bool DropItem(const TEntityRef<IItem>& rItem, bool bAddToPhysicsWorld) = 0;
    virtual void IActor_unk23() = 0;
    virtual void IActor_unk24() = 0;
    virtual void ReleaseItem(const TEntityRef<IItem>& rItem, bool bAddToPhysicsWorld) = 0;
    virtual void IActor_unk26() = 0;
    virtual void IActor_unk27() = 0;
    virtual void IActor_unk28() = 0;
    virtual void IActor_unk29() = 0;
    virtual void IActor_unk30() = 0;
    virtual void IActor_unk31() = 0;
    virtual void IActor_unk32() = 0;
    virtual void IActor_unk33() = 0;
    virtual void IActor_unk34() = 0;
    virtual void DisposeOfItem(const TEntityRef<IItem>& rItem, bool bDisposeWeaponsAlso) = 0;
    virtual void IActor_unk36() = 0;
    virtual void IActor_unk37() = 0;
    virtual void IActor_unk38() = 0;
    virtual void IActor_unk39() = 0;
    virtual void IActor_unk40() = 0;
    virtual void IActor_unk41() = 0;
    virtual void IActor_unk42() = 0;
    virtual void IActor_unk43() = 0;
    virtual void IActor_unk44() = 0;
    virtual void IActor_unk45() = 0;
    virtual void IActor_unk46() = 0;
    virtual void IActor_unk47() = 0;
    virtual void IActor_unk48() = 0;
    virtual void IActor_unk49() = 0;
    virtual void IActor_unk50() = 0;
    virtual void IActor_unk51() = 0;
    virtual void IActor_unk52() = 0;
    virtual void IActor_unk53() = 0;
    virtual void IActor_unk54() = 0;
};

class IActorProxy :
        public IComponentInterface {
public:
    virtual ~IActorProxy() {}
    virtual void IActorProxy_unk0() = 0;
};

class ISequenceTarget {
public:
    virtual void ISequenceTarget_unk0() = 0;
    virtual void ISequenceTarget_unk1() = 0;
    virtual void ISequenceTarget_unk2() = 0;
};

class ISequenceAudioPlayer :
        public IComponentInterface {
public:
    virtual ~ISequenceAudioPlayer() {}
    virtual void ISequenceAudioPlayer_unk0() = 0;
    virtual void ISequenceAudioPlayer_unk1() = 0;
    virtual void ISequenceAudioPlayer_unk2() = 0;
    virtual void ISequenceAudioPlayer_unk3() = 0;
};

class ICrowdAIActor :
        public IComponentInterface {
public:
    virtual ~ICrowdAIActor() {}
    virtual void ICrowdAIActor_unk0() = 0;
    virtual void ICrowdAIActor_unk1() = 0;
    virtual void ICrowdAIActor_unk2() = 0;
};

class ZLinkedEntity;
class ZAttachmentHandler;
class IFirearm;

class ZActorInventoryHandler {
public:
    struct SPendingItemInfo {
        uint32 m_nTicket;
        TEntityRef<IItem> m_rItem;
        EAttachLocation m_eAttachLocation;
        EGameTension m_eMaxTension;
        bool m_bLeftHand;
        bool m_bWeapon;
    };

    virtual ~ZActorInventoryHandler() = 0;

    TArray<SPendingItemInfo> m_aPendingItems; // 0x8
    TEntityRef<ZActor> m_rActor; // 0x20
    TEntityRef<ZLinkedEntity> m_rLinkedEntity; // 0x30
    ZAttachmentHandler* m_pAttachmentHandler; // 0x40
    EGameTension m_eAttachedItemMaxTension; // 0x48
    bool m_bRequestItemAndVariantReset; // 0x4C
    TArray<TEntityRef<IItem>> m_aInventory; // 0x50
    PAD(0x20); // 0x68 (m_ActorItemEvent)
    bool m_bCarryingWeaponLeftHand; // 0x88
    PAD(0x17); // 0x89
    TEntityRef<ZHM5ItemWeapon> m_rMainWeapon; // 0xA0
    TEntityRef<ZHM5Item> m_rCarriedItem; // 0xB0
    PAD(0x10); // 0xC8
    TArray<TEntityRef<IItem>> m_aNotgivenItems; // 0xD0
    TArray<TEntityRef<IItem>> m_aActorAttachedItems; // 0xE8
    ZRepositoryID m_rWeaponID; // 0x100
};

/**
 * An NPC
 *
 * Size = 0x1410
 */
class ZActor :
        public ZHM5BaseCharacter,
        public ICharacterCollision,
        public IActor,
        public IActorProxy,
        public ISequenceTarget,
        public ISequenceAudioPlayer,
        public ICrowdAIActor {
public:
    /**
     * Get the NPC's name, like "Robert Knox"
     */
    ZHMSDK_API ZString GetActorName() const;

    PAD(0x100); // 0x300
    bool m_bStartEnabled; // 0x400
    TEntityRef<ZCharacterTemplateAspect> m_rCharacter; // 0x408
    bool m_bBlockDisguisePickup; // 0x418
    ZRepositoryID m_OutfitRepositoryID; // 0x420
    int32 m_nOutfitCharset; // 0x430
    int32 m_nOutfitVariation; // 0x434
    TEntityRef<ZCostumeFeatureCollection> m_pCostumeFeatures; // 0x438
    TEntityRef<ZAccessoryItemPool> m_pAccessoryItemPool; // 0x448
    TArray<TEntityRef<ZItemRepositoryKeyEntity>> m_InventoryItemKeys; // 0x458
    TArray<TEntityRef<ZOutfitProfessionEntity>> m_aEnforcedOutfits; // 0x470
    ZString m_sActorName; // 0x488
    EActorGroup m_eActorGroup; // 0x498
    TResourcePtr<ZCompiledBehaviorTree> m_pCompiledBehaviorTree; // 0x49C
    EActorVoiceVariation m_eRequiredVoiceVariation; // 0x4A4
    TResourcePtr<ZSpatialEntity> m_pBodybagResource; // 0x4A8
    bool m_bWeaponUnholstered; // 0x4B0
    int32 m_nWeaponIndex; // 0x4B4
    int32 m_nGrenadeIndex; // 0x4B8
    bool m_bIsGrenadeDroppable; // 0x4BC
    bool m_bEnableOutfitModifiers; // 0x4BD
    TEntityRef<ZAIVisionConfigurationEntity> m_AgentVisionConfiguration; // 0x4C0
    TEntityRef<ZHTNDomainEntity> m_DomainConfig; // 0x4D0
    bool m_bDisableBumpAnimations; // 0x4E0
    PAD(0x3F); // 0x4E1
    ZActorInventoryHandler* m_pInventoryHandler; // 0x520
    PAD(0xAC0); // 0x528
    TEntityRef<ZGlobalOutfitKit> m_rOutfit; //0xFE8
    PAD(0xB0); // 0xFF8
    ZAnimatedActor* m_pAnimatedActor; // 0x10A8
    PAD(0xB8);
    bool m_bActorActivated : 1; // 0x1168
    bool m_bEnabled : 1;
    bool m_bAlive : 1;
    bool m_bUnk3 : 1;
    bool m_bUnk4 : 1;
    bool m_bUnk5 : 1;
    bool m_bIsBeingDragged : 1;
    bool m_bIsBeingDumped : 1;
    bool m_bHasAIIcon : 1; // 0x1169
    bool m_bUnk9 : 1;
    bool m_bUnk10 : 1;
    bool m_bUnk11 : 1;
    bool m_bUnk12 : 1;
    bool m_bUnk13 : 1;
    bool m_bUnk14 : 1;
    bool m_bUnk15 : 1;

    /**
     * Seems to determine whether the actor has a red glow in instinct mode (and probably does other things too?)
     *
     * Note that the instinct glow doesn't update immediately upon changing this - certain actions, such as bumping into
     * an actor, will force it to update its instinct glow. You can use the `SetGlowType` pin to manually update it.
     */
    bool m_bContractTarget : 1; // 0x116A

    /**
     * When true, the actor shows up as a target on the map
     *
     * Probably does other things too?
     */
    bool m_bContractTargetLive : 1;

    bool m_bContractTargetHidden : 1;
    bool m_bIsNamedNPC : 1;
    bool m_bNude : 1;
    bool m_bIsActiveEnforcer : 1;
    bool m_bIsPotentialEnforcer : 1;
    bool m_bIsDynamicEnforcer : 1;
    bool m_bCrowdCharacter : 1; // 0x116B
    bool m_bActiveSentry : 1;
    bool m_bUnk26 : 1;
    bool m_bUnk27 : 1;
    bool m_bUnk28 : 1;
    bool m_bUnk29 : 1;
    bool m_bUnk30 : 1;
    bool m_bUnk31 : 1;
    bool m_bUnk32 : 1; // 0x116C
    bool m_bUnk33 : 1;
    bool m_bUnk34 : 1;
    bool m_bUnk35 : 1;
    bool m_bBodyHidden : 1;
    bool m_bUnk37 : 1;
    bool m_bUnk38 : 1;
    bool m_bUnk39 : 1;
    bool m_bUnk40 : 1; // 0x116D
    bool m_bUnk41 : 1;
    bool m_bUnk42 : 1;
    bool m_bUnk43 : 1;
    bool m_bUnk44 : 1;
    bool m_bUnk45 : 1;
    bool m_bUnk46 : 1;
    bool m_bHasClothOutfit : 1;
    PAD(0x1E);
    int32_t m_nActorRuntimeId; // 0x118C
    PAD(0x2B0);

public:
    void PrintBitflags() {
        Logger::Debug(
            "0:{} 1:{} 2:{} 3:{} 4:{} 5:{} 6:{} 7:{} 8:{} 9:{} 10:{} 11:{} 12:{} 13:{} 14:{} 15:{} 16:{} 17:{} 18:{} 19:{} 20:{} 21:{} 22:{} 23:{} 24:{} 25:{} 26:{} 27:{} 28:{} 29:{} 30:{} 31:{} 32:{} 33:{} 34:{} 35:{} 36:{} 37:{} 38:{} 39:{}",
            m_bActorActivated, m_bEnabled, m_bAlive, m_bUnk3, m_bUnk4, m_bUnk5, m_bIsBeingDragged, m_bIsBeingDumped, m_bHasAIIcon,
            m_bUnk9,
            m_bUnk10, m_bUnk11, m_bUnk12, m_bUnk13, m_bUnk14, m_bUnk15, m_bContractTarget, m_bContractTargetLive, m_bContractTargetHidden, m_bIsNamedNPC,
            m_bNude, m_bIsActiveEnforcer, m_bIsPotentialEnforcer, m_bIsDynamicEnforcer, m_bCrowdCharacter, m_bActiveSentry, m_bUnk26, m_bUnk27, m_bUnk28, m_bUnk29,
            m_bUnk30, m_bUnk31, m_bUnk32, m_bUnk33, m_bUnk34, m_bUnk35, m_bBodyHidden, m_bUnk37, m_bUnk38, m_bUnk39
        );
    }
};

static_assert(offsetof(ZActor, m_OutfitRepositoryID) == 0x420);
static_assert(offsetof(ZActor, m_sActorName) == 0x488);
static_assert(offsetof(ZActor, m_DomainConfig) == 0x4D0);
static_assert(offsetof(ZActor, m_nActorRuntimeId) == 0x118C);

class ZAIStateChangeService;
class ZAIModifierService;
class ZAIModifierService;
class ZDynamicEnforcerService;
class ZCombatService;
class ZVIPService;
class ZCollisionService;
class ZCrowdService;
class ZPerceptibleCrowdService;
class ZAnimationService;
class ZGetHelpService;
class ZManhuntService;
class ZIslandService;
class ZAILegalService;
class ZSocialNetworkService;
class ZTargetTrackingService;
class ZAISoundEventService;
class ZSniperChallengeService;
class ZAIService;
class ZSharedVisibilitySensor2;
class ZSharedSoundSensor;
class ZSharedHitmanSensor;
class ZSharedDeadBodySensor;
class ZCollisionSensor;
class ZSocialSensor;
class ZDangerSensor;
class ZDisguiseSensor;
class ZShootTargetSensor;
class ZSentrySensor;
class ZPrivacySensor;
class ZInferenceSensor;
class ZSuspiciousMovementSensor;
class ZLockdownManager;
class ZGameStatsManager;
class ZActorSavableHandler;
class ZPointOfInterestEntity;
class ZItemStashEntity;
class ZOutfitProviderEntity;
class ZImpactConfigEntity;
class IAsyncRayHandle;
class ZActorEventEmitter;

struct SActorEventEmitterState {
};

class ZCombatManager {
public:
    PAD(0x80);
};

class ZSituationManager {
public:
    PAD(0x75890);
};

class ZActorDialogManager {
public:
    PAD(0x11A0);
};

class ZCombatDialogManager {
public:
    PAD(0x8);
};

class ZDialogGesturesManager {
public:
    PAD(0x20);
};

class ZActorManager :
    public IComponentInterface {
public:
    virtual ~ZActorManager() {}

    ZActor* GetActorByName(const ZString& p_Name) const {
        for (const auto& s_Entry : m_activatedActors) {
            ZActor* s_Actor = s_Entry.m_pInterfaceRef;

            if (s_Actor && s_Actor->m_sActorName == p_Name) {
                return s_Actor;
            }
        }

        return nullptr;
    }

    ZActor* GetActorById(uint64_t p_Id) const {
        for (const auto& s_Entry : Globals::ActorManager->m_activatedActors) {
            ZActor* s_Actor = s_Entry.m_pInterfaceRef;

            if (!s_Actor) {
                continue;
            }

            ZEntityRef entRef;

            s_Actor->GetID(entRef);

            if (entRef->GetType()->m_nEntityID == p_Id) {
                return s_Actor;
            }
        }

        return nullptr;
    }

public:
    TEntityRef<ZActor> m_aActors[500]; // 0x8
    TArray<int> m_aFreeActorRuntimeIds; // 0x1F48
    bool m_bLockActorLists; // 0x1F60
    TMaxArray<TEntityRef<ZActor>, 500> m_activatedActors; // 0x1F68
    TMaxArray<int, 500> m_aActivatedActorIds; // 0x3EB0
    TMaxArray<TEntityRef<ZActor>, 500> m_enabledActors; // 0x4688
    TMaxArray<TEntityRef<ZActor>, 500> m_disabledActors; // 0x65D0
    TArray<TEntityRef<ZActor>> m_aOutgoingEnabledActors; // 0x8518
    TMaxArray<int, 500> m_aEnabledActorIds; // 0x8530
    bool m_bLockEnabledActorId; // 0x8D04
    TMaxArray<TEntityRef<ZActor>, 400> m_aliveActors; // 0x8D08
    TMaxArray<int, 500> m_aAliveActorIds; // 0xA610
    TMaxArray<ZGridNodeRef, 500> m_aAliveActorGridNodes; // 0xADE8
    TMaxArray<TEntityRef<ZActor>, 400> m_aliveHm5Characters; // 0xCD30
    TMaxArray<int, 400> m_aliveHm5CharacterIds; // 0xE638
    PAD(0x88); // 0xEC80
    bool m_bDisableAIBehavior; // 0xED08
    bool m_bDisableAIBehaviorLast; // 0xED09
    TArray<TEntityRef<ZActor>> m_Unk0; // 0xED10
    TMaxArray<TEntityRef<ZActor>, 400> m_aliveActorsByDistanceToHM; // 0xED28
    TArray<TEntityRef<ZActor>> m_SpawnedActors; // 0x10630
    TArray<TEntityRef<ZActor>> m_aActiveLookAtActors;// 0x10648
    ZAIStateChangeService* m_pStateChangeService; // 0x10660
    ZAIModifierService* m_pAIModifierService; // 0x10668
    ZDynamicEnforcerService* m_pDynamicEnforcerService; // 0x10670
    ZCombatService* m_pCombatService; // 0x10678
    ZVIPService* m_pVIPService; // 0x10680
    ZCollisionService* m_pCollisionService; // 0x10688
    ZCrowdService* m_pCrowdService; // 0x10690
    ZPerceptibleCrowdService* m_pPerceptibleCrowdService; // 0x10698
    ZAnimationService* m_pAnimationService; // 0x106A0
    ZGetHelpService* m_pGetHelpService; // 0x106A8
    ZManhuntService* m_pManhuntService; // 0x106B0
    ZIslandService* m_pIslandService; // 0x106B8
    ZAILegalService* m_pAILegalService; // 0x106C0
    ZSocialNetworkService* m_pSocialNetworkService; // 0x106C8
    ZTargetTrackingService* m_pTargetTrackingService; // 0x106D0
    ZAISoundEventService* m_pAISoundEventService;  // 0x106D8
    ZSniperChallengeService* m_pSniperChallengeService; // 0x106E0
    TArray<ZAIService*> m_aServices; // 0x106E8
    ZSharedVisibilitySensor2* m_pVisibilitySensor; // 0x10700
    ZSharedSoundSensor* m_pSoundSensor; // 0x10708
    ZSharedHitmanSensor* m_pHitmanSensor; // 0x10710
    ZSharedDeadBodySensor* m_pDeadBodySensor; // 0x10718
    ZCollisionSensor* m_pCollisionSensor; // 0x10720
    ZSocialSensor* m_pSocialSensor; // 0x10728
    ZDangerSensor* m_pDangerSensor; // 0x10730
    ZDisguiseSensor* m_pDisguiseSensor; // 0x10738
    ZShootTargetSensor* m_pShootTargetSensor; // 0x10740
    ZSentrySensor* m_pSentrySensor; // 0x10748
    ZPrivacySensor* m_pPrivacySensor; // 0x10750
    ZInferenceSensor* m_pInferenceSensor; // 0x10758
    ZSuspiciousMovementSensor* m_pSuspiciousMovementSensor; // 0x10760
    ZLockdownManager* m_pLockdownManager; // 0x10768
    ZCombatManager m_CombatManager; // 0x10770
    ZGameStatsManager* m_pGameStats; // 0x107F0
    PAD(0x10); // 0x107F8
    ZActorSavableHandler* m_pSavableHandler; // 0x10808
    ZSituationManager m_situationManager; // 0x10810
    ZActorDialogManager m_actorDialogManager; // 0x860A0
    ZCombatDialogManager m_combatDialogManager; // 0x87240
    ZDialogGesturesManager m_dialogGesturesManager; // 0x87248
    TArray<SActorEventEmitterState> m_actorEventEmitters; // 0x87268
    TMaxArray<TEntityRef<ZPointOfInterestEntity>, 64> m_RegisteredPOI; // 0x87280
    TSet<TEntityRef<ZActor>> m_GuardsSneakedBy; // 0x87688
    TArray<ZEntityRef> m_RegisteredLinkedItems; // 0x876B0
    PAD(0xC0); // 0x876C8
    TSet<TEntityRef<ZActor>> m_CurrentDyingActors; // 0x87788
    TArray<TEntityRef<ZActor>> m_aTargetList; // 0x877B0
    TArray<TEntityRef<ZActor>> m_aCollateralTargetList; // 0x877C8
    TArray<TEntityRef<ZCharacterTemplateAspect>> m_aFakeTargetList; // 0x877E0
    TArray<TEntityRef<ZItemStashEntity>> m_aItemStashList; // 0x877F8
    TArray<TEntityRef<ZOutfitProviderEntity>> m_aOutfitProviderList; // 0x87810
    TEntityRef<ZImpactConfigEntity> m_rImpactConfig; // 0x87828
};