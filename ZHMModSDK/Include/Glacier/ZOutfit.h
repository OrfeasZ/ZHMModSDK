#pragma once

#include "ZEntity.h"
#include "ZResource.h"
#include "ZRepository.h"

class ZHM5CCProfile;
class ZHuntProfile;
class ZNPCAnimationSetDefinition;
class ZActorDynamicTemplateHolder;
class ZKeywordEntity;
class ZTextLine;

class ZOutfitVariation : public ZEntityImpl
{
public:
	ZRuntimeResourceID m_Outfit;
};

class ZCharsetCharacterType : public ZEntityImpl
{
public:
	TArray<TEntityRef<ZOutfitVariation>> m_aVariations;
};

class ZOutfitVariationCollection : public ZEntityImpl
{
public:
	TArray<TEntityRef<ZCharsetCharacterType>> m_aCharacters; // 0x18
};

struct SBodyPartDamageMultipliers
{
	float32 m_fHeadDamageMultiplier; // 0x0
	float32 m_fFaceDamageMultiplier; // 0x4
	float32 m_fArmDamageMultiplier; // 0x8
	float32 m_fLArmDamageScalar; // 0xC
	float32 m_fRArmDamageScalar; // 0x10
	float32 m_fHandDamageMultiplier; // 0x14
	float32 m_fLHandDamageScalar; // 0x18
	float32 m_fRHandDamageScalar; // 0x1C
	float32 m_fLegDamageMultiplier; // 0x20
	float32 m_fLLegDamageScalar; // 0x24
	float32 m_fRLegDamageScalar; // 0x28
	float32 m_fTorsoDamageMultiplier; // 0x2C
	bool m_bApplyLeftRightScalars; // 0x30
};

class ZGlobalOutfitKit : public ZEntityImpl
{
public:
	TArray<EActorVoiceVariation> m_rDefaultVoiceVariations; // 0x18
	PAD(0x20);
	ZString m_sCommonName; // 0x50
	ZString m_sTitle; // 0x60
	TResourcePtr<ZTextLine> m_rNameTextResource; // 0x70
	TResourcePtr<ZTextLine> m_rDescriptionTextResource; // 0x78
	TEntityRef<ZGlobalOutfitKit> m_pParentOutfit; // 0x80
	TArray<TEntityRef<ZOutfitVariationCollection>> m_aCharSets; // 0x90
	bool m_bIsFemale; // 0xA8
	bool m_bHeroDisguiseAvailable; // 0xA9
	bool m_bAllowRadioAct; // 0xAA
	float32 m_fHitPoints; // 0xAC
	SBodyPartDamageMultipliers m_DamageMultipliers; // 0xB0
	EActorCCPreset m_eActorCCClass; // 0xE4
	TArray<ZString> m_CharacterStatLabelList; // 0xE8
	EHM5SoundFootwearType m_eSoundFootwearType; // 0x100
	bool m_bCanCallForHelp; // 0x104
	bool m_bPreferredAccidentInvestigator; // 0x105
	bool m_bAccidentShy; // 0x106
	EActorType m_eActorType; // 0x108
	EActorRank m_eActorRank; // 0x10C
	TEntityRef<ZHM5CCProfile> m_rCCProfile; // 0x110
	TEntityRef<ZHuntProfile> m_pHuntProfile; // 0x120
	TArray<TEntityRef<ZItemRepositoryKeyEntity>> m_AvailableWeaponKeys; // 0x130
	TArray<TEntityRef<ZItemRepositoryKeyEntity>> m_AvailableGrenadeKeys; // 0x148
	TArray<TEntityRef<ZNPCAnimationSetDefinition>> m_aAnimationSets; // 0x160
	TArray<TEntityRef<ZActorDynamicTemplateHolder>> m_aDynamicTemplates; // 0x178
	EOutfitAICategory m_eOutfitAICategory; // 0x190
	EOutfitType m_eOutfitType; // 0x194
	int32 m_nArmorRating; // 0x198
	bool m_bWeaponsAllowed; // 0x19C
	bool m_bAuthorityFigure; // 0x19D
	bool m_bIsHitmanSuit; // 0x19E
	TArray<TEntityRef<ZItemRepositoryKeyEntity>> m_aAllowedWeaponKeys; // 0x1A0
	TArray<TEntityRef<ZKeywordEntity>> m_aAllowedWeaponKeywords; // 0x1B8
	TArray<TEntityRef<ZItemRepositoryKeyEntity>> m_aWeaponsAllowedExceptionKeys; // 0x1D0
	TArray<TEntityRef<ZKeywordEntity>> m_aWeaponsAllowedExceptionKeywords; // 0x1E8
	ZRepositoryID m_sId; // 0x200
};
