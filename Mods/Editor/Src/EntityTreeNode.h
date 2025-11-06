#pragma once

#include <map>
#include <string>
#include <Glacier/ZResourceID.h>
#include <Glacier/ZEntity.h>

struct EntityTreeNode {
    std::string Name;
    std::string EntityType;
    uint64_t EntityId;
    ZRuntimeResourceID TBLU;
    ZRuntimeResourceID ReferencedFactory;
    std::string ReferencedFactoryType;
    ZEntityRef Entity;
    std::multimap<std::string, std::shared_ptr<EntityTreeNode>> Children;
    bool IsDynamicEntity;
    std::atomic<bool> IsPendingDeletion = false;

    EntityTreeNode(
        const std::string& p_Name,
        const std::string& p_type,
        uint64_t p_EntityId,
        ZRuntimeResourceID p_TBLU,
        ZRuntimeResourceID p_ReferencedFactory,
        const std::string& p_ReferencedFactoryType,
        ZEntityRef p_Ref,
        bool p_IsDynamicEntity = false
    ) : 
        Name(p_Name),
        EntityType(p_type),
        EntityId(p_EntityId),
        TBLU(p_TBLU),
        ReferencedFactory(p_ReferencedFactory),
        ReferencedFactoryType(p_ReferencedFactoryType),
        Entity(p_Ref),
        IsDynamicEntity(p_IsDynamicEntity) {}
};
