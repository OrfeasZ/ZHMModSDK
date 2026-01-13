#pragma once

#include "Reflection.h"
#include "ZResource.h"
#include "Enums.h"

struct SEntityTemplatePropertyAlias {
    ZString sAliasName; // 0x0
    int32 entityID; // 0x10
    ZString sPropertyName; // 0x18
};

struct SEntityTemplateReference {
    uint64 entityID; // 0x0
    int32 externalSceneIndex; // 0x8
    int32 entityIndex; // 0xC
    ZString exposedEntity; // 0x10
};

struct SEntityTemplateExposedEntity {
    ZString sName; // 0x0
    bool bIsArray; // 0x10
    TArray<SEntityTemplateReference> aTargets; // 0x18
};

struct SEntityTemplateEntitySubset {
    TArray<int32> entities; // 0x0
};

struct STemplateBlueprintSubEntity {
    SEntityTemplateReference logicalParent; // 0x0
    int32 entityTypeResourceIndex; // 0x20
    uint64 entityId; // 0x28
    bool editorOnly; // 0x30
    ZString entityName; // 0x38
    TArray<SEntityTemplatePropertyAlias> propertyAliases; // 0x48
    TArray<SEntityTemplateExposedEntity> exposedEntities; // 0x60
    TArray<TPair<ZString, int32>> exposedInterfaces; // 0x78
    TArray<TPair<ZString, SEntityTemplateEntitySubset>> entitySubsets; // 0x90
};

struct SEntityTemplatePinConnection {
    int32 fromID; // 0x0
    int32 toID; // 0x4
    ZString fromPinName; // 0x8
    ZString toPinName; // 0x18
    ZObjectRef constantPinValue; // 0x28
};

struct SExternalEntityTemplatePinConnection {
    SEntityTemplateReference fromEntity; // 0x0
    SEntityTemplateReference toEntity; // 0x20
    ZString fromPinName; // 0x40
    ZString toPinName; // 0x50
    ZObjectRef constantPinValue; // 0x60
};

struct STemplateEntityBlueprint {
    int32 subType; // 0x0
    int32 rootEntityIndex; // 0x4
    TArray<STemplateBlueprintSubEntity> subEntities; // 0x8
    TArray<int32> externalSceneTypeIndicesInResourceHeader; // 0x20
    TArray<SEntityTemplatePinConnection> pinConnections; // 0x38
    TArray<SEntityTemplatePinConnection> inputPinForwardings; // 0x50
    TArray<SEntityTemplatePinConnection> outputPinForwardings; // 0x68
    TArray<SEntityTemplateReference> overrideDeletes; // 0x80
    TArray<SExternalEntityTemplatePinConnection> pinConnectionOverrides; // 0x98
    TArray<SExternalEntityTemplatePinConnection> pinConnectionOverrideDeletes; // 0xB0
};

struct SEntityTemplateProperty {
    uint32 nPropertyID; // 0x0
    ZObjectRef value; // 0x8
};

struct SEntityTemplatePlatformSpecificProperty {
    SEntityTemplateProperty propertyValue; // 0x0
    EVirtualPlatformID platform; // 0x18
    bool postInit; // 0x1C
};

struct STemplateFactorySubEntity {
    SEntityTemplateReference logicalParent; // 0x0
    int32 entityTypeResourceIndex; // 0x20
    TArray<SEntityTemplateProperty> propertyValues; // 0x28
    TArray<SEntityTemplateProperty> postInitPropertyValues; // 0x40
    TArray<SEntityTemplatePlatformSpecificProperty> platformSpecificPropertyValues; // 0x58
};

struct SEntityTemplatePropertyOverride {
    SEntityTemplateReference propertyOwner; // 0x0
    SEntityTemplateProperty propertyValue; // 0x20
};

struct STemplateEntityFactory {
    int32 subType; // 0x0
    int32 blueprintIndexInResourceHeader; // 0x4
    int32 rootEntityIndex; // 0x8
    TArray<STemplateFactorySubEntity> subEntities; // 0x10
    TArray<SEntityTemplatePropertyOverride> propertyOverrides; // 0x28
    TArray<int32> externalSceneTypeIndicesInResourceHeader; // 0x40
};

class ZEntityBlueprintFactoryBase : public IEntityBlueprintFactory {
public:
    ZRuntimeResourceID m_ridResource; // 0x8
    PAD(0x10); // 0x10
    int32_t m_rootEntityIndex; // 0x20
    PAD(0x0C); // 0x24
    TArray<IEntityBlueprintFactory*> m_aBlueprintFactories; // 0x30
};

static_assert(offsetof(ZEntityBlueprintFactoryBase, m_rootEntityIndex) == 0x20);

class ZCompositeEntityBlueprintFactoryBase : public ZEntityBlueprintFactoryBase {
public:
    TArray<int64_t> m_aSubEntityOffsets; // 0x48
};

static_assert(offsetof(ZCompositeEntityBlueprintFactoryBase, m_aSubEntityOffsets) == 0x48);

class ZTemplateEntityBlueprintFactory : public ZCompositeEntityBlueprintFactoryBase {
public:
    PAD(0x140); // 0x60
    STemplateEntityBlueprint* m_pTemplateEntityBlueprint; // 0x1A0
};

static_assert(offsetof(ZTemplateEntityBlueprintFactory, m_pTemplateEntityBlueprint) == 0x1A0);

class ZAspectEntityBlueprintFactory : public ZCompositeEntityBlueprintFactoryBase {
public:
    struct SAspectedSubentityEntry {
        uint32 m_nAspectIdx; // 0x0
        uint32 m_nSubentityIdx; // 0x4
    };

    PAD(0x30); // 0x60
    ZEntityType* m_pFactoryEntityType; // 0x90
    TArray<SAspectedSubentityEntry> m_aSubEntitiesLookUp; // 0x98
    THashMap<uint64, int32, TDefaultHashMapPolicy<uint64>> m_aSubEntityIndexMap; // 0xB0
};

class ZTemplateEntityFactory : public IEntityFactory {
public:
    STemplateEntityFactory* m_pResourceData; // 0x8
    bool m_bHasCalculatedPropertyValues; // 0x10
    int32 m_rootEntityIndex; // 0x14
    TArray<IEntityFactory*> m_pFactories; // 0x18
    ZRuntimeResourceID m_ridResource; // 0x30
    TResourcePtr<ZTemplateEntityBlueprintFactory> m_blueprintResource; // 0x38
};

static_assert(offsetof(ZTemplateEntityFactory, m_blueprintResource) == 0x38);

class ZAspectEntityFactory : public IEntityFactory {
public:
    TArray<TResourcePtr<IEntityFactory>> m_factoryResources; // 0x8
    TResourcePtr<ZAspectEntityBlueprintFactory> m_blueprintResource; // 0x20
    ZRuntimeResourceID m_ridResource; // 0x28
};

class ZCppEntityBlueprintFactory;

class ZCppEntityFactory : public IEntityFactory {
public:
    PAD(0x48); // 0x8
    TResourcePtr<ZCppEntityBlueprintFactory> m_blueprintResource; // 0x50
    ZRuntimeResourceID m_ridResource; // 0x58
};

class ZExtendedCppEntityFactory : public IEntityFactory {
public:
    TResourcePtr<IEntityFactory> m_pCppEntityFactory; // 0x8
    TResourcePtr<IEntityBlueprintFactory> m_pBlueprintFactory; // 0x10
};

class ZUIControlEntityFactory : public IEntityFactory {
public:
    TResourcePtr<IEntityFactory> m_pCppEntityFactory; // 0x8
    TResourcePtr<IEntityBlueprintFactory> m_pBlueprintFactory; // 0x10
    ZRuntimeResourceID m_ridResource; // 0x18
};

class ZRenderMaterialEntityFactory : public IEntityFactory {
public:
    PAD(0xA8);
    TResourcePtr<IEntityFactory> m_pMaterialEntityFactory; // 0xB0
    TResourcePtr<IEntityBlueprintFactory> m_blueprintResource; // 0xB8
    ZRuntimeResourceID m_ridResource; // 0xC0
    ZRuntimeResourceID m_ridMaterialInstance; // 0xC8
};

class ZBehaviorTreeEntityFactory : public IEntityFactory {
public:
    TResourcePtr<IEntityFactory> m_pParentEntityFactory; // 0x8
    TResourcePtr<IEntityBlueprintFactory> m_blueprintResource; // 0x10
    ZRuntimeResourceID m_ridResource; // 0x18
};

class ZAudioSwitchEntityFactory : public IEntityFactory {
public:
    TResourcePtr<IEntityFactory> m_pParentEntityFactory; // 0x8
    TResourcePtr<IEntityBlueprintFactory> m_pBlueprintResource; // 0x10
    ZRuntimeResourceID m_ridResource; // 0x18
};

class ZAudioStateEntityFactory : public IEntityFactory {
public:
    TResourcePtr<IEntityFactory> m_pParentEntityFactory; // 0x8
    TResourcePtr<IEntityBlueprintFactory> m_pBlueprintResource; // 0x10
    ZRuntimeResourceID m_ridResource; // 0x18
};