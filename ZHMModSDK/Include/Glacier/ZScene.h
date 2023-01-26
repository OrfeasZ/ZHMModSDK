#pragma once

#include "ZResourceID.h"
#include "Reflection.h"
#include "TArray.h"
#include "ZEntity.h"
#include "EntityFactory.h"

class ZEntityScope;

class IEntitySceneContext :
	public IComponentInterface
{
};

class ISceneEntity :
	public IComponentInterface
{	
};

class ZSceneData
{
public:
	ZString m_sceneName; // 0x00
	TArray<ZString> m_sceneBricks; // 0x10
	bool m_bStartScene; // 0x28
	bool m_unk0x29; // 0x29
	ZString m_type; // 0x30
	ZString m_codeNameHint; // 0x40
};

class ZSceneConfiguration
{
public:
	ZRuntimeResourceID m_ridSceneFactory; //0x0
	TArray<ZRuntimeResourceID> m_aAdditionalBrickFactoryRIDs; //0x8
	TResourcePtr<IEntityFactory> m_sceneFactoryResource; //0x20
	TArray<TResourcePtr<IEntityFactory>> m_aAdditionalBrickFactories; //0x28
	TResourcePtr<IEntityBlueprintFactory> m_sceneBlueprint; //0x40
	TArray<TResourcePtr<IEntityBlueprintFactory>> m_aAdditionalBrickBlueprints; //0x48
};

class ZBrickData
{
public:
	ZEntityRef entityRef;
	ZRuntimeResourceID runtimeResourceID;
	PAD(0x18);
};

class ZEntitySceneContext :
	public IEntitySceneContext
{
public:
	virtual ~ZEntitySceneContext() = 0;
	virtual void ZEntitySceneContext_unk0() = 0;
	virtual void ZEntitySceneContext_unk1() = 0;
	virtual void ZEntitySceneContext_unk2() = 0;
	virtual void ZEntitySceneContext_unk3() = 0;
	virtual void ZEntitySceneContext_unk4() = 0;
	virtual void ZEntitySceneContext_unk5() = 0;
	virtual void ZEntitySceneContext_unk6() = 0;
	virtual void ZEntitySceneContext_unk7() = 0;
	virtual void ZEntitySceneContext_unk8() = 0;
	virtual void ZEntitySceneContext_unk9() = 0;
	virtual void ZEntitySceneContext_unk10() = 0;
	virtual void ZEntitySceneContext_unk11() = 0;
	virtual void ZEntitySceneContext_unk12() = 0;
	virtual void ZEntitySceneContext_unk13() = 0;
	virtual void ZEntitySceneContext_unk14() = 0;
	virtual void ZEntitySceneContext_unk15() = 0;
	virtual void LoadScene(const ZSceneData& data) = 0;
	virtual void ZEntitySceneContext_unk16() = 0;
	virtual void ZEntitySceneContext_unk17() = 0;
	virtual void ZEntitySceneContext_unk18() = 0;
	virtual void ZEntitySceneContext_unk19() = 0;
	virtual void ZEntitySceneContext_unk20() = 0;
	virtual void ZEntitySceneContext_unk21() = 0;
	virtual void ZEntitySceneContext_unk22() = 0;
	virtual void ZEntitySceneContext_unk23() = 0;
	virtual void ZEntitySceneContext_unk24() = 0;
	virtual void ZEntitySceneContext_unk25() = 0;
	virtual void ZEntitySceneContext_unk26() = 0;

public:
	PAD(0x08);
	ZSceneData m_sceneData; //0x10
	ZSceneConfiguration m_SceneConfig; //0x60
	TArray<ZBrickData> m_aLoadedBricks; //0xC0
	TEntityRef<ISceneEntity> m_pScene; //0xD8
	ZEntityScope* m_pEntityScope; //0xE8
};
