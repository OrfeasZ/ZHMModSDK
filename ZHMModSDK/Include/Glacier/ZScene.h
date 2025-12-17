#pragma once

#include "ZResourceID.h"
#include "Reflection.h"
#include "TArray.h"
#include "ZEntity.h"
#include "EntityFactory.h"

class ZEntityScope;

class IEntitySceneContext :
        public IComponentInterface {};

class ISceneEntity :
        public IComponentInterface {};

struct SSceneInitParameters {
    ZString m_SceneResource; // 0x00
    TArray<ZString> m_aAdditionalBrickResources; // 0x10
    bool m_bStartGame; // 0x28
    bool m_Unk0; // 0x29
    ZString m_Type; // 0x30
    ZString m_CodeNameHint; // 0x40
};

class ZSceneConfiguration {
public:
    ZRuntimeResourceID m_ridSceneFactory; //0x0
    TArray<ZRuntimeResourceID> m_aAdditionalBrickFactoryRIDs; //0x8
    TResourcePtr<IEntityFactory> m_sceneFactoryResource; //0x20
    TArray<TResourcePtr<IEntityFactory>> m_aAdditionalBrickFactories; //0x28
    TResourcePtr<IEntityBlueprintFactory> m_sceneBlueprint; //0x40
    TArray<TResourcePtr<IEntityBlueprintFactory>> m_aAdditionalBrickBlueprints; //0x48
};

struct SBrickAllocationInfo {
    ZEntityRef m_EntityRef;
    ZRuntimeResourceID m_RuntimeResourceID;
    PAD(0x18);
};

enum class ESceneLoadingStage {
    eLoading_Start = 0,
    eLoading_SceneStopped = 1,
    eLoading_SceneDeleted = 2,
    eLoading_AssetsLoaded = 3,
    eLoading_SceneAllocated = 4,
    eLoading_SceneStarted = 5,
    eLoading_ScenePrecaching = 6,
    eLoading_SceneActivated = 7,
    eLoading_ScenePlaying = 8,
    eLoading_MAX = 9
};

class ZEntitySceneContext :
        public IEntitySceneContext {
public:
    virtual ~ZEntitySceneContext() = 0;
    virtual void ZEntitySceneContext_unk5() = 0;
    virtual void ClearScene(bool) = 0;
    virtual void CreateScene(bool bResetScene) = 0;
    virtual void ZEntitySceneContext_unk8() = 0;
    virtual void ZEntitySceneContext_unk9() = 0;
    virtual void ZEntitySceneContext_unk10() = 0;
    virtual void ZEntitySceneContext_unk11() = 0;
    virtual void CreateTransformParentScene() = 0;
    virtual ZEntityRef* GetTransformParentScene(ZEntityRef& result) = 0;
    virtual void ZEntitySceneContext_unk14() = 0;
    virtual void ZEntitySceneContext_unk15() = 0;
    virtual void ZEntitySceneContext_unk16() = 0;
    virtual void LoadScene(const SSceneInitParameters& parameters) = 0;
    virtual const TArray<SBrickAllocationInfo>& GetLoadedBricks() = 0;
    virtual const SSceneInitParameters& GetSceneInitParameters() = 0;
    virtual void ZEntitySceneContext_unk20() = 0;
    virtual void ZEntitySceneContext_unk21() = 0;
    virtual void ZEntitySceneContext_unk22() = 0;
    virtual void ZEntitySceneContext_unk23() = 0;
    virtual void ZEntitySceneContext_unk24() = 0;
    virtual void ZEntitySceneContext_unk25() = 0;
    virtual void SetLoadingStage(ESceneLoadingStage stage) = 0;
    virtual ESceneLoadingStage GetLoadingStage() = 0;
    virtual float GetLoadingProgress() = 0;

public:
    PAD(0x08);
    SSceneInitParameters m_SceneInitParameters; //0x10
    ZSceneConfiguration m_SceneConfig; //0x60
    TArray<SBrickAllocationInfo> m_aLoadedBricks; //0xC0
    TEntityRef<ISceneEntity> m_pScene; //0xD8
    ZEntityScope* m_pEntityScope; //0xE8
    PAD(0x88);
    ESceneLoadingStage m_LoadingStage; // 0x178
};