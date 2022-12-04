#pragma once

#include "Enums.h"
#include "ZEntity.h"
#include "ZPrimitives.h"
#include "ZHM5BaseCharacter.h"
#include "ZResource.h"

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

class ICharacterCollision :
	public IComponentInterface
{
public:
	virtual ~ICharacterCollision() {}
	virtual void ICharacterCollision_unk0() = 0;
};

class IActor
{
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
	virtual void IActor_unk21() = 0;
	virtual void IActor_unk22() = 0;
	virtual void IActor_unk23() = 0;
	virtual void IActor_unk24() = 0;
	virtual void IActor_unk25() = 0;
	virtual void IActor_unk26() = 0;
	virtual void IActor_unk27() = 0;
	virtual void IActor_unk28() = 0;
	virtual void IActor_unk29() = 0;
	virtual void IActor_unk30() = 0;
	virtual void IActor_unk31() = 0;
	virtual void IActor_unk32() = 0;
	virtual void IActor_unk33() = 0;
	virtual void IActor_unk34() = 0;
	virtual void IActor_unk35() = 0;
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
	public IComponentInterface
{
public:
	virtual ~IActorProxy() {}
	virtual void IActorProxy_unk0() = 0;
};

class ISequenceTarget
{
public:
	virtual void ISequenceTarget_unk0() = 0;
	virtual void ISequenceTarget_unk1() = 0;
	virtual void ISequenceTarget_unk2() = 0;
};

class ISequenceAudioPlayer :
	public IComponentInterface
{
public:
	virtual ~ISequenceAudioPlayer() {}
	virtual void ISequenceAudioPlayer_unk0() = 0;
	virtual void ISequenceAudioPlayer_unk1() = 0;
	virtual void ISequenceAudioPlayer_unk2() = 0;
	virtual void ISequenceAudioPlayer_unk3() = 0;
};

class ICrowdAIActor :
	public IComponentInterface
{
public:
	virtual ~ICrowdAIActor() {}
	virtual void ICrowdAIActor_unk0() = 0;
	virtual void ICrowdAIActor_unk1() = 0;
	virtual void ICrowdAIActor_unk2() = 0;
};

// Size = 0x1410
class ZActor :
	public ZHM5BaseCharacter,
	public ICharacterCollision,
	public IActor,
	public IActorProxy,
	public ISequenceTarget,
	public ISequenceAudioPlayer,
	public ICrowdAIActor
{
public:
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
	PAD(0xBC8); // 0x4E0
	ZAnimatedActor* m_pAnimatedActor; // 0x10A8
	PAD(0xA8);
	bool m_bUnk0 : 1; // 0x1158
	bool m_bUnk1 : 1;
	bool m_bUnk2 : 1;
	bool m_bUnk3 : 1;
	bool m_bUnk4 : 1;
	bool m_bUnk5 : 1;
	bool m_bIsBeingDragged : 1;
	bool m_bIsBeingDumped : 1;
	bool m_bUnk8 : 1; // 0x1159
	bool m_bUnk9 : 1;
	bool m_bUnk10 : 1;
	bool m_bUnk11 : 1;
	bool m_bUnk12 : 1;
	bool m_bUnk13 : 1;
	bool m_bUnk14 : 1;
	bool m_bUnk15 : 1;
	bool m_bUnk16 : 1; // 0x115A
	bool m_bUnk17 : 1;
	bool m_bUnk18 : 1;
	bool m_bUnk19 : 1;
	bool m_bUnk20 : 1;
	bool m_bUnk21 : 1;
	bool m_bUnk22 : 1;
	bool m_bUnk23 : 1;
	bool m_bUnk24 : 1; // 0x115B
	bool m_bUnk25 : 1;
	bool m_bUnk26 : 1;
	bool m_bUnk27 : 1;
	bool m_bUnk28 : 1;
	bool m_bUnk29 : 1;
	bool m_bUnk30 : 1;
	bool m_bUnk31 : 1;
	bool m_bUnk32 : 1; // 0x115C
	bool m_bUnk33 : 1;
	bool m_bUnk34 : 1;
	bool m_bUnk35 : 1;
	bool m_bBodyHidden : 1;
	bool m_bUnk37 : 1;
	bool m_bUnk38 : 1;
	bool m_bUnk39 : 1;
	PAD(0x2B8);

public:
	void PrintBitflags()
	{
		Logger::Debug("0:{} 1:{} 2:{} 3:{} 4:{} 5:{} 6:{} 7:{} 8:{} 9:{} 10:{} 11:{} 12:{} 13:{} 14:{} 15:{} 16:{} 17:{} 18:{} 19:{} 20:{} 21:{} 22:{} 23:{} 24:{} 25:{} 26:{} 27:{} 28:{} 29:{} 30:{} 31:{} 32:{} 33:{} 34:{} 35:{} 36:{} 37:{} 38:{} 39:{}",
			m_bUnk0, m_bUnk1, m_bUnk2, m_bUnk3, m_bUnk4, m_bUnk5, m_bIsBeingDragged, m_bIsBeingDumped, m_bUnk8, m_bUnk9,
			m_bUnk10, m_bUnk11, m_bUnk12, m_bUnk13, m_bUnk14, m_bUnk15, m_bUnk16, m_bUnk17, m_bUnk18, m_bUnk19,
			m_bUnk20, m_bUnk21, m_bUnk22, m_bUnk23, m_bUnk24, m_bUnk25, m_bUnk26, m_bUnk27, m_bUnk28, m_bUnk29,
			m_bUnk30, m_bUnk31, m_bUnk32, m_bUnk33, m_bUnk34, m_bUnk35, m_bBodyHidden, m_bUnk37, m_bUnk38, m_bUnk39
		);
	}
};

static_assert(offsetof(ZActor, m_OutfitRepositoryID) == 0x420);
static_assert(offsetof(ZActor, m_sActorName) == 0x488);
static_assert(offsetof(ZActor, m_DomainConfig) == 0x4D0);

class ZActorSavableHandler;

class ZActorManager :
	public IComponentInterface
{
public:
	virtual ~ZActorManager() {}


	/**
	 * Get an actor by their name
	 *
	 * Author: Andrew Pratt
	 * Param p_Name: Actor's name
	 * Returns: Pointer to actor, or nullptr if no actor with a matching name was found
	 */
	ZActor* GetActorByName(const ZString& p_Name)
	{
		for (int i = 0; i < *Globals::NextActorId; ++i)
		{
			auto* s_Actor = m_aActiveActors[i].m_pInterfaceRef;

			if (s_Actor->m_sActorName == p_Name)
				return s_Actor;
		}

		return nullptr;
	}

	/**
	 * Get an actor by their entity id
	 *
	 * Author: Andrew Pratt
	 * Param p_Id: Actor's entity id
	 * Returns: Pointer to actor, or nullptr if no actor with a matching name was found
	*/
	ZActor* GetActorById(uint64_t p_Id)
	{
		for (int i = 0; i < *Globals::NextActorId; ++i)
		{
			auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

			ZEntityRef s_EntRef;
			s_Actor->GetID(&s_EntRef);

			if ((*s_EntRef.m_pEntity)->m_nEntityId == p_Id)
				return s_Actor;
		}

		return nullptr;
	}


public:
#if CTT
	PAD(0x1F60);
	TEntityRef<ZActor> m_aActiveActors[1000]; // 0x1F68, ZActorManager destructor, last if
#else
	PAD(0x25A0);
	TEntityRef<ZActor> m_aActiveActors[1000]; // 0x25A8, ZActorManager destructor, last if
#endif
	/*PAD(0xAA20); // 0x5DE8
	ZActorSavableHandler* m_pSavableHandler; // 0x10808*/
};
