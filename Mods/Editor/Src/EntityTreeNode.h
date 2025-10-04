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
    ZEntityRef Entity;
    std::multimap<std::string, std::shared_ptr<EntityTreeNode>> Children;

    EntityTreeNode(
        const std::string& p_Name, const std::string& p_type, uint64_t p_EntityId, ZRuntimeResourceID p_TBLU,
        ZEntityRef p_Ref
    )
        : Name(p_Name), EntityType(p_type), EntityId(p_EntityId), TBLU(p_TBLU), Entity(p_Ref) {}
};
