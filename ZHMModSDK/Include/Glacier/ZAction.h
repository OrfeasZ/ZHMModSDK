#pragma once

#include "ZEntity.h"
#include "ZResource.h"

class ZGeomEntity;
class ZBoxVolumeEntity;
class ZSubaction;
class ZTextListData;
class ZIllegalActionEntity;
class IEnableConditionListener;

class ZHM5Action : public ZEntityImpl
{
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

class ZHM5ActionManager : IComponentInterface
{
public:
	TArray<ZHM5Action*> m_Actions;
};
