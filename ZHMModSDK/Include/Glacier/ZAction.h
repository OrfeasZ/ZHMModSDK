#pragma once

#include "ZEntity.h"
#include "ZResource.h"
#include "ZItem.h"

class ZGeomEntity;
class ZBoxVolumeEntity;
class ZTextListData;
class ZIllegalActionEntity;
class IEnableConditionListener;
class ZSpatialEntity;
class IEventConsumerCollection;
class IConditionalTextLine;
class IUpdateableFloat;
class ZTextLine;
class IChildNetworkEntity;
class IBoolCondition;

class IUIAction : public IComponentInterface {
};

class IAnimlayerSubactionInterface : public IComponentInterface {
};

class ZSubaction :
    public ZEntityImpl,
    public IUIAction,
    public IAnimlayerSubactionInterface,
    public IBoolConditionListener {
public:
    ZEntityRef m_Object; // 0x30
    TArray<TEntityRef<ZGeomEntity>> m_aUIHighlightGeom; // 0x38
    TArray<TEntityRef<ZGeomEntity>> m_aIgnoreCollisionGeom; // 0x50
    bool m_bUseAnimlayer; // 0x68
    float32 m_nAnimlayerTime; // 0x6C
    ZString m_sAnimationEvent; // 0x70
    bool m_bPlayFastSwipeOutsideLocomotion; // 0x80
    bool m_bPlaySwipeIfThereIsNoAnim; // 0x81
    TEntityRef<IChildNetworkEntity> m_rAnimationNetwork; // 0x88
    ZString m_rAnimationNetworkName; // 0x98
    TEntityRef<IEventConsumerCollection> m_pEventConsumerCollection; // 0xA8
    TEntityRef<ZSpatialEntity> m_rContextLocation; // 0xB8
    TEntityRef<ZSpatialEntity> m_rContextObjectSpatial; // 0xC8
    bool m_bCanBeOperatedFromAnyAngle; // 0xD8
    bool m_bCanBeOperatedWithItemsInHand; // 0xD9
    bool m_bCanBeOperatedWithItemInLeftHand; // 0xDA
    bool m_bHolsterNonWeaponItemsInHand; // 0xDB
    bool m_bEquipRequiredItem; // 0xDC
    bool m_bRestorePreSubactionItems; // 0xDD
    bool m_bAlignFreeAttacherToContextObject; // 0xDE
    bool m_bAlignFreeAttacherToInteractionPoint; // 0xDF
    float32 m_fAdditionalAlignRadius; // 0xE0
    bool m_bOverrideInteractionRange; // 0xE4
    float32 m_fOverrideInteractionRangeValue; // 0xE8
    bool m_bOverrideInteractionRangeVR; // 0xEC
    float32 m_fOverrideInteractionRangeValueVR; // 0xF0
    bool m_bIgnoreCollision; // 0xF4
    bool m_bUseRightHandItemGrip; // 0xF5
    bool m_bOneItemPerSubaction; // 0xF6
    bool m_bInstinctModeOnly; // 0xF7
    TEntityRef<IBoolCondition> m_rUsableCondition; // 0xF8
    TEntityRef<IBoolCondition> m_rVisibleCondition; // 0x108
    TEntityRef<IBoolCondition> m_rReadyCondition; // 0x118
    TEntityRef<ZIllegalActionEntity> m_rIllegalActionEntity; // 0x128
    bool m_bShowUnusableAsGrayedInteraction; // 0x138
    ZEntityRef m_rKeywords; // 0x140
    TEntityRef<ZItemRepositoryKeyEntity> m_RequiredItem; // 0x148
    TEntityRef<IBoolCondition> m_rCompleteCondition; // 0x158
    TEntityRef<IBoolCondition> m_rAbortCondition; // 0x168
    bool m_bInterruptibleAction; // 0x178
    float32 m_fInterruptibleActionDuration; // 0x17C
    EHM5GameInputFlag m_eInputAction; // 0x180
    EActionType m_eActionType; // 0x184
    bool m_bShowPromptInWorld; // 0x188
    bool m_bShouldPauseWorld; // 0x189
    ZString m_sPromptText; // 0x190
    ZString m_sPromptTextPassive; // 0x1A0
    ZString m_sPromptDescriptionText; // 0x1B0
    ZString m_sPromptDescriptionPassiveText; // 0x1C0
    TResourcePtr<ZTextLine> m_rPromptTextResource; // 0x1D0
    TResourcePtr<ZTextLine> m_rPromptTextPassiveResource; // 0x1D8
    TResourcePtr<ZTextLine> m_rPromptDescriptionTextResource; // 0x1E0
    TResourcePtr<ZTextLine> m_rPromptDescriptionTextPassiveResource; // 0x1E8
    TEntityRef<ZSpatialEntity> m_rVrPromptLocationOverride; // 0x1F0
    TArray<TEntityRef<IConditionalTextLine>> m_aPromptText; // 0x200
    TArray<TEntityRef<IConditionalTextLine>> m_aPromptTextPassive; // 0x218
    TArray<TEntityRef<IConditionalTextLine>> m_aPromptDescriptionText; // 0x230
    TArray<TEntityRef<IConditionalTextLine>> m_aPromptDescriptionTextPassive; // 0x248
    PAD(0x10);
    TResourcePtr<ZTextLine> m_rNonExecutableDescriptionResource; // 0x270
    bool m_bApplyPromptDescriptionText; // 0x278
    bool m_bIgnoreEmptyPromptDescriptionText; // 0x279
    TArray<TEntityRef<ZSubaction>> m_aSubactions; // 0x280
    TEntityRef<IUpdateableFloat> m_vrMotionProgress; // 0x298
    EUIActionGroupIcon m_eActionGroupIcon; // 0x2A8
    bool m_bIsInventorySubactionGroup; // 0x2AC
    bool m_bUseItemIllegalityInInventorySubactionGroup; // 0x2AD
    ZEntityRef m_rUsableConditionKeywordsA; // 0x2B0
    EKeywordSetBehavior m_eUsableConditionKeywordsBehaviorA; // 0x2B8
    ZEntityRef m_rUsableConditionKeywordsB; // 0x2C0
    EKeywordSetBehavior m_eUsableConditionKeywordsBehaviorB; // 0x2C8
    bool m_bUsableConditionUseAllEquippableItems; // 0x2CC
    bool m_bVRNoAlignment; // 0x2CD
    bool m_bVRForceSwipeAnimation; // 0x2CE
    bool m_bVRHideAnimation; // 0x2CF
    bool m_bVRAllowAlternativeInput; // 0x2D0
    PAD(0x50);
};

class ZHM5Action : public ZEntityImpl {
public:
    PAD(0x18);
    ZEntityRef m_Object; // 0x30
    TArray<TEntityRef<ZGeomEntity>> m_aUIHighlightGeom; // 0x38
    TArray<TEntityRef<ZGeomEntity>> m_aIgnoreCollisionGeom; // 0x50
    TEntityRef<ZBoxVolumeEntity> m_rActivateBoxVolume; // 0x68
    ZString m_sActionName; // 0x78
    bool m_bVisible; // 0x88
    bool m_bAlwaysDirty; // 0x89
    bool m_bIgnoreCollision; // 0x8A
    bool m_bIgnoreOcclusionSetup; // 0x8B
    bool m_bRequiresHitmanFacing; // 0x8C
    bool m_bRequiresHitmanInFront; // 0x8D
    bool m_bWorldAlignPrompt; // 0x8E
    bool m_bWorldAlignRotateHorizontalOnly; // 0x8F
    ZEntityRef m_Listener; // 0x90
    TArray<TEntityRef<ZSubaction>> m_aSubactions; // 0x98
    ZString m_sDefaultItemName; // 0xB0
    TResourcePtr<ZTextListData> m_pTextListResource; // 0xC0
    ZString m_sExitTextID; // 0xC8
    TEntityRef<ZSpatialEntity> m_3dPromptPosition; // 0xD8
    TArray<TEntityRef<ZSpatialEntity>> m_aPromptPositions; // 0xE8
    TEntityRef<ZIllegalActionEntity> m_rIllegalActionEntity; // 0x100
    TArray<TEntityRef<IEnableConditionListener>> m_aNearHeroEnableConditionListeners; // 0x110
    PAD(0x20);
    EActionType m_eActionType;
};

class ZHM5ActionManager : IComponentInterface {
public:
    TArray<ZHM5Action*> m_Actions;
};