#pragma once

#include <map>
#include <string>
#include <Glacier/ZResourceID.h>
#include <Glacier/ZEntity.h>

#include "EntityNameCompare.h"

struct EntityTreeNode {
    std::string Name;
    std::string EntityType;
    uint64_t EntityId;
    ZRuntimeResourceID BlueprintFactory;
    std::string BlueprintFactoryType;
    ZRuntimeResourceID ReferencedBlueprintFactory;
    std::string ReferencedBlueprintFactoryType;
    ZEntityRef Entity;
    std::multimap<std::string, std::shared_ptr<EntityTreeNode>, EntityNameCompare> Children;
    std::vector<std::shared_ptr<EntityTreeNode>> Parents;
    bool IsDynamicEntity;
    std::atomic<bool> IsPendingDeletion = false;

    EntityTreeNode(
        const std::string& p_Name,
        const std::string& p_type,
        uint64_t p_EntityId,
        ZRuntimeResourceID p_BlueprintFactory,
        const std::string& p_BlueprintFactoryType,
        ZRuntimeResourceID p_ReferencedBlueprintFactory,
        const std::string& p_ReferencedBlueprintFactoryType,
        ZEntityRef p_Ref,
        bool p_IsDynamicEntity = false
    ) : Name(p_Name),
        EntityType(p_type),
        EntityId(p_EntityId),
        BlueprintFactory(p_BlueprintFactory),
        BlueprintFactoryType(p_BlueprintFactoryType),
        ReferencedBlueprintFactory(p_ReferencedBlueprintFactory),
        ReferencedBlueprintFactoryType(p_ReferencedBlueprintFactoryType),
        Entity(p_Ref),
        IsDynamicEntity(p_IsDynamicEntity) {}
};
