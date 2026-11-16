#pragma once

#include <map>
#include <memory>
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
    std::weak_ptr<EntityTreeNode> Parent;
    bool IsDynamicEntity;
    std::atomic<bool> IsPendingDeletion { false };
    bool IsDirectMatch = false;

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
        IsDynamicEntity(p_IsDynamicEntity) {
    }

    void RemoveFromParent() {
        const auto s_Parent = Parent.lock();

        if (!s_Parent) {
            return;
        }

        auto [s_It, s_End] = s_Parent->Children.equal_range(Name);

        for (; s_It != s_End; ++s_It) {
            if (s_It->second.get() == this) {
                s_Parent->Children.erase(s_It);
                return;
            }
        }
    }
};