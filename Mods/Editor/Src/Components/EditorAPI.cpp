#include <complex.h>
#include <Editor.h>
#include <functional>

#include "Logging.h"

#include <Glacier/EntityFactory.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZPhysics.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZResource.h>
#include <Glacier/SExternalReferences.h>

#include <ResourceLib_HM3.h>

#include <queue>
#include <utility>

#include "Glacier/ZRoom.h"


ZEntityRef Editor::FindEntity(EntitySelector p_Selector) {
    std::shared_lock s_Lock(m_CachedEntityTreeMutex);

    if (!m_CachedEntityTree) {
        return {};
    }

    // When TBLU hash is not set, we're selecting from spawned entities.
    if (!p_Selector.TbluHash.has_value()) {
        const auto s_Entity = m_SpawnedEntities.find(p_Selector.EntityId);

        if (s_Entity != m_SpawnedEntities.end()) {
            return s_Entity->second;
        }

        return {};
    }

    // Otherwise we're selecting from the entity tree.
    const ZRuntimeResourceID s_TBLU = p_Selector.TbluHash.value();

    // Create a queue and add the root to it.
    std::queue<std::shared_ptr<EntityTreeNode>> s_NodeQueue;
    s_NodeQueue.push(m_CachedEntityTree);

    // Keep track of the last node that matched the entity ID, but not the TBLU.
    std::shared_ptr<EntityTreeNode> s_IdMatchedNode = nullptr;

    // Keep iterating through the tree until we find the node we're looking for.
    while (!s_NodeQueue.empty()) {
        // Access the first node in the queue
        auto s_Node = s_NodeQueue.front();
        s_NodeQueue.pop();

        if (s_Node->EntityId == p_Selector.EntityId) {
            bool s_Matches = s_Node->BlueprintFactory == s_TBLU;

            // If the TBLU doesn't match, then check the owner entity.
            if (!s_Matches) {
                const auto s_OwningEntity = s_Node->Entity.GetOwningEntity();

                if (s_OwningEntity && s_OwningEntity.GetBlueprintFactory()) {
                    s_Matches = s_OwningEntity.GetBlueprintFactory()->m_ridResource == s_TBLU;
                }
            }

            // If it's not that either, try our own factory.
            if (!s_Matches) {
                const auto s_Factory = s_Node->Entity.GetBlueprintFactory();

                if (s_Factory) {
                    s_Matches = s_Factory->m_ridResource == s_TBLU;
                }
            }

            // Found the node we're looking for!
            if (s_Matches) {
                return s_Node->Entity;
            }

            // Otherwise, keep track of the last node that matched the entity ID.
            s_IdMatchedNode = s_Node;
        }

        // If not found, add children to the queue.
        for (auto& childPair : s_Node->Children) {
            s_NodeQueue.push(childPair.second);
        }
    }

    if (s_IdMatchedNode) {
        return s_IdMatchedNode->Entity;
    }

    return {};
}

std::string Editor::GetCollisionHash(auto p_SelectedEntity) {
    if (const auto s_EntityType = p_SelectedEntity->GetType(); s_EntityType && s_EntityType->m_pProperties01) {
        for (uint32_t i = 0; i < s_EntityType->m_pProperties01->size(); ++i) {
            const ZEntityProperty* s_Property = &s_EntityType->m_pProperties01->operator[](i);
            const auto* s_PropertyInfo = s_Property->m_pType->getPropertyInfo();

            if (!s_PropertyInfo || !s_PropertyInfo->m_pType)
                continue;

            const auto s_PropertyAddress =
                    reinterpret_cast<uintptr_t>(p_SelectedEntity.m_pEntity) + s_Property->m_nOffset;
            const uint16_t s_TypeSize = s_PropertyInfo->m_pType->typeInfo()->m_nTypeSize;
            const uint16_t s_TypeAlignment = s_PropertyInfo->m_pType->typeInfo()->m_nTypeAlignment;

            if (s_PropertyInfo->m_pType->typeInfo()->isResource() ||
                s_PropertyInfo->m_nPropertyID != s_Property->m_nPropertyId) {
                // Some properties don't have a name for some reason. Try to find using RL.
                if (const auto [s_data, s_size] =
                        HM3_GetPropertyName(s_Property->m_nPropertyId); s_size > 0) {
                    if (const auto s_COLLISION_RESOURCE_ID_PROPERTY_NAME = "m_CollisionResourceID"; std::string(
                        s_data, s_size
                    ) != s_COLLISION_RESOURCE_ID_PROPERTY_NAME) {
                        continue;
                    }
                    // Get the value of the property.
                    auto* s_Data = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
                        s_TypeSize, s_TypeAlignment
                    );

                    if (s_PropertyInfo->m_nFlags & E_HAS_GETTER_SETTER) {
                        s_PropertyInfo->get(
                            reinterpret_cast<void*>(s_PropertyAddress),
                            s_Data,
                            s_PropertyInfo->m_nOffset
                        );
                    }
                    else {
                        s_PropertyInfo->m_pType->typeInfo()->m_pTypeFunctions->copyConstruct(
                            s_Data,
                            reinterpret_cast<void*>(s_PropertyAddress)
                        );
                    }
                    const auto* s_Resource = static_cast<ZResourcePtr*>(s_Data);
                    std::string s_ResourceName = "null";

                    if (s_Resource && s_Resource->m_nResourceIndex.val >= 0) {
                        s_ResourceName = fmt::format(
                            "{:08X}{:08X}", s_Resource->GetResourceInfo().rid.m_IDHigh,
                            s_Resource->GetResourceInfo().rid.m_IDLow
                        );
                    }
                    (*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);

                    if (std::strcmp(s_ResourceName.c_str(), "") != 0 && s_ResourceName.c_str() != nullptr &&
                        std::strcmp(s_ResourceName.c_str(), "null") != 0) {
                        return s_ResourceName;
                    }
                }
            }
        }
    }
    return "";
}

template <typename T>
std::unique_ptr<T, AlignedDeleter> Editor::GetProperty(ZEntityRef p_Entity, const ZEntityProperty* p_Property) {
    const auto* s_PropertyInfo = p_Property->m_pType->getPropertyInfo();
    const auto s_PropertyAddress =
            reinterpret_cast<uintptr_t>(p_Entity.m_pEntity) + p_Property->m_nOffset;
    const uint16_t s_TypeSize = s_PropertyInfo->m_pType->typeInfo()->m_nTypeSize;
    const uint16_t s_TypeAlignment = s_PropertyInfo->m_pType->typeInfo()->m_nTypeAlignment;

    // Get the value of the property.
    T* s_Data =
            static_cast<T*>((*Globals::MemoryManager)->m_pNormalAllocator->
                                                       AllocateAligned(s_TypeSize, s_TypeAlignment));

    if (s_PropertyInfo->m_nFlags & E_HAS_GETTER_SETTER)
        s_PropertyInfo->get(reinterpret_cast<void*>(s_PropertyAddress), s_Data, s_PropertyInfo->m_nOffset);
    else
        s_PropertyInfo->m_pType->typeInfo()->m_pTypeFunctions->copyConstruct(
            s_Data, reinterpret_cast<void*>(s_PropertyAddress)
        );
    return std::unique_ptr<T, AlignedDeleter>(s_Data, AlignedDeleter());;
}

Quat Editor::GetQuatFromProperty(ZEntityRef p_Entity) {
    const std::string s_TransformPropertyName = "m_mTransform";
    const auto s_EntityType = p_Entity->GetType();

    for (uint32_t i = 0; i < s_EntityType->m_pProperties01->size(); ++i) {
        ZEntityProperty* s_Property = &s_EntityType->m_pProperties01->operator[](i);

        if (const auto* s_PropertyInfo = s_Property->m_pType->getPropertyInfo(); s_PropertyInfo->m_pType->typeInfo()->
            isResource() || s_PropertyInfo->m_nPropertyID != s_Property->m_nPropertyId) {
            // Some properties don't have a name for some reason. Try to find using RL.

            if (const auto [s_data, s_size] =
                    HM3_GetPropertyName(s_Property->m_nPropertyId); s_size > 0) {
                if (auto s_PropertyNameView = std::string_view(s_data, s_size);
                    s_PropertyNameView == s_TransformPropertyName) {
                    if (auto s_Data43 = GetProperty<SMatrix43>(p_Entity, s_Property)) {
                        auto s_Data = SMatrix(*s_Data43);
                        const auto s_Decomposed = s_Data.Decompose();
                        const auto s_Quat = s_Decomposed.Quaternion;
                        return s_Quat;
                    }
                }
            }
        }
        else if (s_PropertyInfo->m_pName && s_PropertyInfo->m_pName == s_TransformPropertyName) {
            if (auto s_Data43 = GetProperty<SMatrix43>(p_Entity, s_Property)) {
                auto s_Data = SMatrix(*s_Data43);
                const auto s_Decomposed = s_Data.Decompose();
                const auto s_Quat = s_Decomposed.Quaternion;
                return s_Quat;
            }
        }
    }
    return {};
}


Quat Editor::GetParentQuat(const ZEntityRef p_Entity) {
    const auto* s_Entity = p_Entity.QueryInterface<ZSpatialEntity>();
    std::vector<Quat> s_ParentQuats;
    while (s_Entity->m_eidParent != NULL) {
        const TEntityRef<ZSpatialEntity> s_EidParent = s_Entity->m_eidParent;
        s_Entity = s_EidParent.m_pInterfaceRef;
        s_ParentQuats.push_back(GetQuatFromProperty(s_EidParent.m_ref));
    }

    if (s_ParentQuats.empty()) {
        return {};
    }
    std::ranges::reverse(s_ParentQuats);
    auto s_QuatIter = s_ParentQuats.begin();
    Quat s_Quat = *s_QuatIter;
    while (s_QuatIter != s_ParentQuats.end()) {
        if (s_QuatIter != s_ParentQuats.begin()) {
            s_Quat = s_Quat * *s_QuatIter;
        }
        ++s_QuatIter;
    }
    return s_Quat;
}

std::string Editor::FindRoomForEntity(const ZEntityRef p_Entity) {
    std::shared_lock s_Lock(m_CachedEntityTreeMutex);
    const ZSpatialEntity* s_SpatialEntity = p_Entity.QueryInterface<ZSpatialEntity>();
    const uint16 s_RoomEntityIndex = Functions::ZRoomManager_GetRoomFromPoint->Call(*Globals::RoomManager, s_SpatialEntity->GetWorldMatrix().Pos);
    if (s_RoomEntityIndex == 65535) {
        return "No Room";
    }
    ZRoomEntity* s_RoomEntity = (*Globals::RoomManager)->m_RoomEntities[s_RoomEntityIndex];
    ZEntityRef s_RoomEntityRef;
    s_RoomEntity->GetID(s_RoomEntityRef);

    const std::shared_ptr<EntityTreeNode> s_EntityTreeNode = m_CachedEntityTreeMap[s_RoomEntityRef];
    return s_EntityTreeNode->Name;
}

void Editor::FindAlocAndPrimForZGeomEntityNode(
    std::vector<std::tuple<std::vector<std::pair<std::string, std::string>>, Quat, std::string, ZEntityRef>>& p_Entities,
    const std::shared_ptr<EntityTreeNode>& p_Node, const TArray<ZEntityInterface>& p_Interfaces, char*& p_EntityType
) {
    const ZGeomEntity* s_GeomEntity = p_Node->Entity.QueryInterface<ZGeomEntity>();
    std::string s_Id = std::format("{:016x}", p_Node->Entity->GetType()->m_nEntityId);
    std::string s_TbluHashString =
            std::format("<{:08X}{:08X}>", p_Node->BlueprintFactory.m_IDHigh, p_Node->BlueprintFactory.m_IDLow);

    if (ZResourceIndex s_ResourceIndex(s_GeomEntity->m_ResourceID.m_nResourceIndex);
        s_ResourceIndex.val != -1) {
        TArray<unsigned char> s_Flags;
        TArray<ZResourceIndex> s_Indices;
        std::vector<std::pair<std::string, std::string>> s_AlocAndPrimHashes;
        Functions::ZResourceContainer_GetResourceReferences->Call(
            *Globals::ResourceContainer, s_ResourceIndex, s_Indices, s_Flags
        );
        const auto s_PrimResourceInfo = (*Globals::ResourceContainer)->
                m_resources[s_GeomEntity->m_ResourceID.m_nResourceIndex.val];
        const auto s_PrimHash = s_PrimResourceInfo.rid.GetID();
        std::string s_PrimHashString {std::format("{:016X}", s_PrimHash)};
        for (ZResourceIndex s_CurrentResourceIndex : s_Indices) {
            if (s_CurrentResourceIndex.val < 0) {
                continue;
            }
            if (const auto s_ReferenceResourceInfo = (*Globals::ResourceContainer)->m_resources[
                s_CurrentResourceIndex.val]; s_ReferenceResourceInfo.resourceType == 'ALOC') {
                const auto s_AlocHash = s_ReferenceResourceInfo.rid.GetID();
                std::string s_AlocHashString {std::format("{:016X}", s_AlocHash)};
                std::pair s_AlocAndPrimHashStrings {s_AlocHashString, s_PrimHashString};
                s_AlocAndPrimHashes.push_back(s_AlocAndPrimHashStrings);
                Logger::Debug(
                    "Found ALOC. ID: {} TBLU: {} ALOC: {} PRIM: {}",
                    s_Id, s_TbluHashString, s_AlocAndPrimHashStrings.first, s_AlocAndPrimHashStrings.second
                );
            }
        }
        if (std::string s_collision_ioi_string = GetCollisionHash(p_Node->Entity);
            !s_collision_ioi_string.empty() && s_collision_ioi_string != "null") {
            bool s_Skip = false;
            for (auto s_Interface : p_Interfaces) {
                if (s_Interface.m_pTypeId->typeInfo() != nullptr) {
                    p_EntityType = s_Interface.m_pTypeId->typeInfo()->m_pTypeName;
                    if (strcmp(p_EntityType, "ZPureWaterAspect") == 0) {
                        s_Skip = true;
                        break;
                    }
                }
            }
            if (!s_Skip) {
                Logger::Debug(
                    "Found ALOC. ID: {} TBLU: {} PRIM: {} ALOC: {}", s_Id, s_TbluHashString,
                    s_PrimHashString, s_collision_ioi_string
                );
                s_AlocAndPrimHashes.emplace_back(s_collision_ioi_string, s_PrimHashString);
                Quat s_EntityQuat = GetQuatFromProperty(p_Node->Entity);
                Quat s_ParentQuat = GetParentQuat(p_Node->Entity);

                Quat s_CombinedQuat;
                s_CombinedQuat = s_ParentQuat * s_EntityQuat;
                std::string s_RoomName = Plugin()->FindRoomForEntity(p_Node->Entity);
                auto s_Entity =
                        std::make_tuple(
                            s_AlocAndPrimHashes,
                            s_CombinedQuat,
                            s_RoomName,
                            p_Node->Entity
                        );
                p_Entities.push_back(s_Entity);
            }
        }
    }
}

void Editor::FindAlocAndPrimForZPrimitiveProxyEntityNode(
    std::vector<std::tuple<std::vector<std::pair<std::string, std::string>>, Quat, std::string, ZEntityRef>>& entities,
    const std::shared_ptr<EntityTreeNode>& s_Node, const TArray<ZEntityInterface>& s_Interfaces, char*& s_EntityType
) {
    std::string s_Id = std::format("{:016x}", s_Node->Entity->GetType()->m_nEntityId);
    std::string s_HashString =
            std::format("<{:08X}{:08X}>", s_Node->BlueprintFactory.m_IDHigh, s_Node->BlueprintFactory.m_IDLow);
    // TODO: Check if the prim hash is needed here

    if (std::string s_collision_ioi_string = GetCollisionHash(s_Node->Entity);
        !s_collision_ioi_string.empty() && s_collision_ioi_string != "null") {
        bool s_Skip = false;
        for (auto s_Interface : s_Interfaces) {
            if (s_Interface.m_pTypeId->typeInfo() != nullptr) {
                s_EntityType = s_Interface.m_pTypeId->typeInfo()->m_pTypeName;
                if (strcmp(s_EntityType, "ZPureWaterAspect") == 0) {
                    s_Skip = true;
                    break;
                }
            }
        }
        if (!s_Skip) {
            std::vector<std::pair<std::string, std::string>> s_AlocHashes;
            Logger::Debug(
                "Found ALOC. ID: {} TBLU: {} ALOC: {}", s_Id, s_HashString, s_collision_ioi_string
            );
            s_AlocHashes.emplace_back(s_collision_ioi_string, "");
            Quat s_EntityQuat = GetQuatFromProperty(s_Node->Entity);
            Quat s_ParentQuat = GetParentQuat(s_Node->Entity);

            Quat s_CombinedQuat;
            s_CombinedQuat = s_ParentQuat * s_EntityQuat;
            std::string s_RoomName = Plugin()->FindRoomForEntity(s_Node->Entity);
            auto s_Entity =
                    std::make_tuple(
                        s_AlocHashes,
                        s_CombinedQuat,
                        s_RoomName,
                        s_Node->Entity
                    );
            entities.push_back(s_Entity);
        }
    }
}

void Editor::FindMeshes(
    const std::function<void(
        std::vector<std::tuple<std::vector<std::pair<std::string, std::string>>, Quat, std::string, ZEntityRef>>&, bool p_Done
    )>& p_SendEntitiesCallback,
    const std::function<void()>& p_RebuiltCallback
) {
    if (!m_CachedEntityTree) {
        p_RebuiltCallback();
        RebuildEntityTree();
    }
    std::shared_lock s_Lock(m_CachedEntityTreeMutex);

    std::vector<std::tuple<std::vector<std::pair<std::string, std::string>>, Quat, std::string, ZEntityRef>> entities;

    // Create a queue and add the root to it.
    std::queue<std::pair<std::shared_ptr<EntityTreeNode>, std::shared_ptr<EntityTreeNode>>> s_NodeQueue;
    s_NodeQueue.emplace(std::shared_ptr<EntityTreeNode>(), m_CachedEntityTree);
    // Keep iterating through the tree until we find all the ZGeomEntities.
    while (!s_NodeQueue.empty()) {
        // Send batches of 10 entities at a time so the client can start processing
        if (entities.size() >= 10) {
            p_SendEntitiesCallback(entities, false);
            // Once a batch has been sent, clear the entities vectory to reduce memory usage
            entities.clear();
        }
        // Access the first node in the queue
        auto s_Parent = s_NodeQueue.front().first;
        auto s_Node = s_NodeQueue.front().second;
        s_NodeQueue.pop();
        const auto& s_Interfaces = *s_Node->Entity.GetEntity()->GetType()->m_pInterfaces;
        const auto typeInfo = s_Interfaces[0].m_pTypeId->typeInfo();
        if (typeInfo == nullptr) {
            continue;
        }
        if (char* s_EntityType = typeInfo->m_pTypeName; strcmp(s_EntityType, "ZGeomEntity") == 0) {
            FindAlocAndPrimForZGeomEntityNode(entities, s_Node, s_Interfaces, s_EntityType);
        }
        else if (strcmp(s_EntityType, "ZPrimitiveProxyEntity") == 0) {
            FindAlocAndPrimForZPrimitiveProxyEntityNode(entities, s_Node, s_Interfaces, s_EntityType);
        }

        // Add children to the queue.
        for (auto& node : s_Node->Children | std::views::values) {
            if (node->Entity == m_DynamicEntitiesNodeEntityRef ||
                node->Entity == m_UnparentedEntitiesNodeEntityRef ||
                node->IsDynamicEntity ||
                node->IsPendingDeletion) {
                continue;
            }
            std::string s_ChildId = std::format("{:016x}", node->Entity->GetType()->m_nEntityId);
            s_NodeQueue.push(std::pair {s_Node, node});
        }
    }
    p_SendEntitiesCallback(entities, true);
    entities.clear();
}

std::vector<std::tuple<std::vector<std::string>, Quat, ZEntityRef>> Editor::FindEntitiesByType(
    const std::string& p_EntityType, const std::string& p_Hash
) {
    std::shared_lock s_Lock(m_CachedEntityTreeMutex);
    if (!m_CachedEntityTree) {
        return {};
    }
    std::vector<std::tuple<std::vector<std::string>, Quat, ZEntityRef>> entities;

    Logger::Info("Getting {} Entities.", p_EntityType);
    // Create a queue and add the root to it.
    std::queue<std::shared_ptr<EntityTreeNode>> s_NodeQueue;
    s_NodeQueue.push(m_CachedEntityTree);

    // Keep iterating through the tree until we find the nodes we're looking for.
    while (!s_NodeQueue.empty()) {
        // Access the first node in the queue
        auto s_Node = s_NodeQueue.front();
        s_NodeQueue.pop();
        if (s_Node->Entity == m_DynamicEntitiesNodeEntityRef ||
           s_Node->Entity == m_UnparentedEntitiesNodeEntityRef ||
           s_Node->IsDynamicEntity ||
           s_Node->IsPendingDeletion) {
            continue;
        }
        const auto& s_Interfaces = *s_Node->Entity.GetEntity()->GetType()->m_pInterfaces;

        if (char* s_EntityType = s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName;
            strcmp(s_EntityType, p_EntityType.c_str()) == 0) {
            Quat s_EntityQuat = GetQuatFromProperty(s_Node->Entity);
            Quat s_ParentQuat = GetParentQuat(s_Node->Entity);

            Quat s_CombinedQuat;
            s_CombinedQuat = s_ParentQuat * s_EntityQuat;
            std::tuple<std::vector<std::string>, Quat, ZEntityRef> s_Entity =
                    std::make_tuple(
                        std::vector {p_Hash},
                        s_CombinedQuat,
                        s_Node->Entity
                    );

            entities.push_back(s_Entity);
        }

        // Add children to the queue.
        for (auto& node : s_Node->Children | std::views::values) {
            s_NodeQueue.push(node);
        }
    }

    return entities;
}

void Editor::SelectEntity(EntitySelector p_Selector, std::optional<std::string> p_ClientId) {
    auto s_Entity = FindEntity(p_Selector);

    if (s_Entity) {
        OnSelectEntity(s_Entity, true, std::move(p_ClientId));
    }
    else {
        throw std::runtime_error("Could not find entity for the given selector.");
    }
}

void Editor::SetEntityTransform(
    EntitySelector p_Selector, SMatrix p_Transform, bool p_Relative, std::optional<std::string> p_ClientId
) {
    if (const auto s_Entity = FindEntity(p_Selector)) {
        OnEntityTransformChange(s_Entity, p_Transform, p_Relative, std::move(p_ClientId));
    }
    else {
        throw std::runtime_error("Could not find entity for the given selector.");
    }
}

void Editor::SpawnQnEntity(
    const std::string& p_QnJson, uint64_t p_EntityId, std::string p_Name, std::optional<std::string> p_ClientId
) {
    auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        throw std::runtime_error("Scene is not yet loaded. Cannot spawn entities.");
    }

    // If entity is already spawned, destroy it first.
    const auto s_SpawnedEntity = m_SpawnedEntities.find(p_EntityId);

    if (s_SpawnedEntity != m_SpawnedEntities.end()) {
        OnDestroyEntity(s_SpawnedEntity->second, p_ClientId);
    }

    TResourcePtr<ZTemplateEntityBlueprintFactory> s_BpFactory;
    TResourcePtr<ZTemplateEntityFactory> s_Factory;

    if (!SDK()->LoadQnEntity(p_QnJson, s_BpFactory, s_Factory)) {
        throw std::runtime_error("Failed to load entity from QN JSON.");
    }

    ZEntityRef s_SpawnedEnt;
    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager, s_SpawnedEnt, p_Name, s_Factory, s_Scene.m_ref, {}, p_EntityId
    );

    if (!s_SpawnedEnt) {
        throw std::runtime_error("Could not spawn entity.");
    }

    Logger::Info(
        "Spawned entity from rid {} with id {}!", s_Factory.GetResourceInfo().rid,
        s_SpawnedEnt->GetType()->m_nEntityId
    );

    m_CachedEntityTreeMutex.lock();

    m_EntityNames[s_SpawnedEnt] = p_Name;
    m_SpawnedEntities[p_EntityId] = s_SpawnedEnt;

    if (m_CachedEntityTree && m_CachedEntityTreeMap.size() > 0) {
        UpdateEntityTree(m_CachedEntityTreeMap, {s_SpawnedEnt}, true);
    }

    m_CachedEntityTreeMutex.unlock();

    m_Server.OnEntitySpawned(s_SpawnedEnt, std::move(p_ClientId));
}

void Editor::CreateEntityResources(const std::string& p_QnJson, std::optional<std::string> p_ClientId) {
    TResourcePtr<ZTemplateEntityBlueprintFactory> s_BpFactory;
    TResourcePtr<ZTemplateEntityFactory> s_Factory;

    if (!SDK()->LoadQnEntity(p_QnJson, s_BpFactory, s_Factory)) {
        throw std::runtime_error("Failed to load entity from QN JSON.");
    }
}

void Editor::DestroyEntity(EntitySelector p_Selector, std::optional<std::string> p_ClientId) {
    if (const auto s_Entity = FindEntity(p_Selector)) {
        OnDestroyEntity(s_Entity, std::move(p_ClientId));
    }
    else {
        throw std::runtime_error("Could not find entity for the given selector.");
    }
}

void Editor::SetEntityName(EntitySelector p_Selector, std::string p_Name, std::optional<std::string> p_ClientId) {
    if (const auto s_Entity = FindEntity(p_Selector)) {
        OnEntityNameChange(s_Entity, p_Name, std::move(p_ClientId));
    }
    else {
        throw std::runtime_error("Could not find entity for the given selector.");
    }
}

void Editor::SetEntityProperty(
    EntitySelector p_Selector, uint32_t p_PropertyId, std::string_view p_JsonValue,
    std::optional<std::string> p_ClientId
) {
    if (const auto s_Entity = FindEntity(p_Selector)) {
        auto s_EntityType = s_Entity->GetType();
        auto s_Property = s_EntityType->FindProperty(p_PropertyId);

        if (!s_Property) {
            throw std::runtime_error("Could not find property for the given ID.");
        }

        if (!s_Property->m_pType || !s_Property->m_pType->getPropertyInfo() || !s_Property->m_pType->getPropertyInfo()->
            m_pType) {
            throw std::runtime_error(
                "Unable to set this property because its type information is missing from the game."
            );
        }

        const auto s_PropertyInfo = s_Property->m_pType->getPropertyInfo();

        if (s_PropertyInfo->m_pType->typeInfo()->isEntity()) {
            if (p_JsonValue == "null") {
                auto s_EntityRefObj = ZObjectRef::From<ZEntityRef>({});
                s_EntityRefObj.UNSAFE_SetType(s_PropertyInfo->m_pType);

                OnSetPropertyValue(
                    s_Entity, p_PropertyId, s_EntityRefObj, std::move(p_ClientId)
                );
            }
            else {
                // Parse EntitySelector
                simdjson::ondemand::parser s_Parser;
                const auto s_EntitySelectorJson = simdjson::padded_string(p_JsonValue);
                simdjson::ondemand::document s_EntitySelectorMsg = s_Parser.iterate(s_EntitySelectorJson);

                const auto s_EntitySelector = EditorServer::ReadEntitySelector(s_EntitySelectorMsg);

                if (const auto s_TargetEntity = FindEntity(s_EntitySelector)) {
                    auto s_EntityRefObj = ZObjectRef::From<TEntityRef<ZEntityImpl>>(
                        TEntityRef<ZEntityImpl>(s_TargetEntity)
                    );
                    s_EntityRefObj.UNSAFE_SetType(s_PropertyInfo->m_pType);

                    OnSetPropertyValue(
                        s_Entity, p_PropertyId, s_EntityRefObj, std::move(p_ClientId)
                    );
                }
                else {
                    throw std::runtime_error("Could not find entity for the given selector.");
                }
            }
        }
        else {
            const uint16_t s_TypeSize = s_PropertyInfo->m_pType->typeInfo()->m_nTypeSize;
            const uint16_t s_TypeAlignment = s_PropertyInfo->m_pType->typeInfo()->m_nTypeAlignment;
            const std::string s_TypeName = s_PropertyInfo->m_pType->typeInfo()->m_pTypeName;

            void* s_Data = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
                s_TypeSize,
                s_TypeAlignment
            );

            const bool s_Success = HM3_JsonToGameStruct(
                s_TypeName.c_str(),
                p_JsonValue.data(),
                p_JsonValue.size(),
                s_Data,
                s_TypeSize
            );

            if (!s_Success) {
                (*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);
                throw std::runtime_error("Unable to convert JSON to game struct.");
            }

            ZObjectRef s_DataObj;
            s_DataObj.UNSAFE_Assign(s_PropertyInfo->m_pType, s_Data);

            OnSetPropertyValue(
                s_Entity, p_PropertyId, s_DataObj, std::move(p_ClientId)
            );
        }
    }
    else {
        throw std::runtime_error("Could not find entity for the given selector.");
    }
}

void Editor::SignalEntityPin(EntitySelector p_Selector, uint32_t p_PinId, bool p_Output) {
    if (const auto s_Entity = FindEntity(p_Selector)) {
        OnSignalEntityPin(s_Entity, p_PinId, p_Output);
    }
    else {
        throw std::runtime_error("Could not find entity for the given selector.");
    }
}

void Editor::RebuildEntityTree() {
    UpdateEntities();
}