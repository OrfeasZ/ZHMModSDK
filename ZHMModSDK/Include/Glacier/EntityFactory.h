#pragma once

#include "Reflection.h"
#include "ZResource.h"

// 0x00000001446A5720 (Size: 0x28)
class SEntityTemplatePropertyAlias
{
public:
    ZString sAliasName; // 0x0
    int32 entityID; // 0x10
    ZString sPropertyName; // 0x18
};

// 0x00000001446A5280 (Size: 0x20)
class SEntityTemplateReference
{
public:
    uint64 entityID; // 0x0
    int32 externalSceneIndex; // 0x8
    int32 entityIndex; // 0xC
    ZString exposedEntity; // 0x10
};

// 0x00000001446A53D8 (Size: 0x30)
class SEntityTemplateExposedEntity
{
public:
    ZString sName; // 0x0
    bool bIsArray; // 0x10
    TArray<SEntityTemplateReference> aTargets; // 0x18
};

// 0x00000001446A5498 (Size: 0x18)
class SEntityTemplateEntitySubset
{
public:
    TArray<int32> entities; // 0x0
};

// 0x00000001446A5738 (Size: 0xA8)
class STemplateBlueprintSubEntity
{
public:
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

// 0x00000001446A5510 (Size: 0x38)
class SEntityTemplatePinConnection
{
public:
    int32 fromID; // 0x0
    int32 toID; // 0x4
    ZString fromPinName; // 0x8
    ZString toPinName; // 0x18
    ZObjectRef constantPinValue; // 0x28
};

// 0x00000001446A56A8 (Size: 0x70)
class SExternalEntityTemplatePinConnection
{
public:
    SEntityTemplateReference fromEntity; // 0x0
    SEntityTemplateReference toEntity; // 0x20
    ZString fromPinName; // 0x40
    ZString toPinName; // 0x50
    ZObjectRef constantPinValue; // 0x60
};

// 0x00000001446A55B8 (Size: 0xC8)
class STemplateEntityBlueprint
{
public:
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

class ZEntityBlueprintFactoryBase : public IEntityBlueprintFactory
{
public:
    ZRuntimeResourceID m_ridResource;
    PAD(0x10);
    int32_t m_rootEntityIndex; // 0x20
    PAD(0x20); // 0x24
};

static_assert(offsetof(ZEntityBlueprintFactoryBase, m_rootEntityIndex) == 0x20);

class ZCompositeEntityBlueprintFactoryBase : public ZEntityBlueprintFactoryBase
{
public:
    TArray<int64_t> m_aSubEntityOffsets; // 0x48
};

static_assert(offsetof(ZCompositeEntityBlueprintFactoryBase, m_aSubEntityOffsets) == 0x48);

class ZTemplateEntityBlueprintFactory : public ZCompositeEntityBlueprintFactoryBase
{
public:
    PAD(0x140); // 0x60
    STemplateEntityBlueprint* m_pTemplateEntityBlueprint; // 0x1A0
};

static_assert(offsetof(ZTemplateEntityBlueprintFactory, m_pTemplateEntityBlueprint) == 0x1A0);

class ZTemplateEntityFactory : public IEntityFactory
{
public:
    PAD(0x30);
    TResourcePtr<ZTemplateEntityBlueprintFactory> m_blueprintResource; // 0x38
};

static_assert(offsetof(ZTemplateEntityFactory, m_blueprintResource) == 0x38);
