#include <Editor.h>

#include "imgui_internal.h"
#include <cassert>
#include <imgui_stdlib.h>

#include <Glacier/EntityFactory.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZEntity.h>
#include <Glacier/ZFreeCamera.h>
#include <Glacier/ZComponentCreateInfo.h>
#include <Glacier/SExternalReferences.h>
#include <Glacier/ZEntityManager.h>

#include "IconsMaterialDesign.h"
#include "Logging.h"
#include "Util/StringUtils.h"
#include "Util/ImGuiUtils.h"

#include <shared_mutex>
#include <queue>
#include <map>
#include <utility>

class ZClothCharacterEntity;
class ZLinkedProxyEntity;

static void EraseEntityTreeNodeById(
    std::unordered_multimap<uint64_t, std::shared_ptr<EntityTreeNode>>& p_NodesById,
    const std::shared_ptr<EntityTreeNode>& p_Node
) {
    auto [s_It, s_End] = p_NodesById.equal_range(p_Node->EntityId);

    for (; s_It != s_End; ++s_It) {
        if (s_It->second == p_Node) {
            p_NodesById.erase(s_It);
            break;
        }
    }
}

void Editor::SetEntityTreeNode(
    std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>>& p_NodeMap,
    std::unordered_multimap<uint64_t, std::shared_ptr<EntityTreeNode>>& p_NodesById,
    const ZEntityRef p_Entity,
    const std::shared_ptr<EntityTreeNode>& p_Node
) {
    auto& s_MappedNode = p_NodeMap[p_Entity];

    if (s_MappedNode) {
        EraseEntityTreeNodeById(p_NodesById, s_MappedNode);
    }

    s_MappedNode = p_Node;
    p_NodesById.emplace(p_Node->EntityId, p_Node);
}

std::shared_ptr<EntityTreeNode> Editor::RemoveEntityTreeNode(const ZEntityRef p_Entity) {
    const auto s_NodeIt = m_CachedEntityTreeMap.find(p_Entity);

    if (s_NodeIt == m_CachedEntityTreeMap.end()) {
        return nullptr;
    }

    auto s_Node = std::move(s_NodeIt->second);
    m_CachedEntityTreeMap.erase(s_NodeIt);
    EraseEntityTreeNodeById(m_CachedEntityTreeNodesById, s_Node);

    return s_Node;
}

void Editor::UpdateEntityTree(
    std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>>& p_NodeMap,
    std::unordered_multimap<uint64_t, std::shared_ptr<EntityTreeNode>>& p_NodesById,
    const std::vector<ZEntityRef>& p_Entities,
    const bool p_AreEntitiesDynamic
) {
    // Go through a first pass by creating all the nodes of the tree using a BFS
    // approach. We'll also opportunistically assign children nodes to parents we've
    // seen before. Then, as a second pass we'll go through and assign the remaining
    // children nodes to their parents.

    std::queue<std::pair<ZEntityBlueprintFactoryBase*, ZEntityRef>> s_NodeQueue;
    std::queue<std::shared_ptr<EntityTreeNode>> s_ParentlessNodes;

    for (const auto& s_Entity : p_Entities) {
        if (!s_Entity) {
            continue;
        }

        auto s_BpFactory = s_Entity.GetBlueprintFactory();

        if (!s_BpFactory) {
            continue;
        }

        s_NodeQueue.emplace(s_BpFactory, s_Entity);
    }

    const auto s_SceneEntity = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene.m_entityRef;
    const std::shared_ptr<EntityTreeNode> s_SceneNode = p_NodeMap[s_SceneEntity];
    const std::shared_ptr<EntityTreeNode> s_UnparentedEntitiesNode = s_SceneNode->Children.find("Unparented entities")->
        second;
    std::shared_ptr<EntityTreeNode> s_DynamicEntitiesNode;

    if (p_AreEntitiesDynamic) {
        s_DynamicEntitiesNode = s_SceneNode->Children.find("Dynamic entities")->second;
    }

    while (!s_NodeQueue.empty()) {
        // Pop the next factory and its root entity off the queue.
        auto [s_CurrentFactory, s_CurrentRoot] = s_NodeQueue.front();
        s_NodeQueue.pop();

        const auto s_SubEntityCount = s_CurrentFactory->GetSubEntitiesCount();
        const bool s_IsTemplateEntityBlueprintFactory = s_CurrentFactory->IsTemplateEntityBlueprintFactory();
        const bool s_IsAspectEntityBlueprintFactory = s_CurrentFactory->IsAspectEntityBlueprintFactory();

        // Go through each of its sub-entities and create nodes for them.
        if (s_SubEntityCount > 0) {
            for (int i = 0; i < s_SubEntityCount; ++i) {
                const ZEntityRef s_SubEntity = s_CurrentFactory->GetSubEntity(s_CurrentRoot.m_pObj, i);
                const auto s_SubEntityFactory = s_CurrentFactory->GetSubEntityBlueprint(i);

                if (!s_SubEntity.GetEntity() || !s_SubEntity->GetType()) {
                    continue;
                }

                // Skip the root entity of the referenced factory
                if (p_NodeMap.contains(s_SubEntity)) {
                    /**
                     * Enqueue sub-entities of the referenced factory to ensure they are processed
                     * even when the root entity is skipped
                     */
                    if (s_SubEntityFactory && s_SubEntityFactory->GetSubEntitiesCount() > 0) {
                        s_NodeQueue.emplace(s_SubEntityFactory, s_SubEntity);
                    }

                    continue;
                }

                const auto s_SubEntityID = s_SubEntity->GetType()->m_nEntityID;
                std::string s_EntityName = "<No name>";

                // If our current factory is a template factory, we can get the name of the entity from it.
                if (s_IsTemplateEntityBlueprintFactory) {
                    const auto s_TemplateBpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_CurrentFactory);

                    if (s_TemplateBpFactory->m_pTemplateEntityBlueprint) {
                        s_EntityName = s_TemplateBpFactory->m_pTemplateEntityBlueprint->subEntities[i].entityName;
                    }
                }
                else if (s_IsAspectEntityBlueprintFactory) {
                    const auto s_AspectEntityBlueprintFactory = reinterpret_cast<ZAspectEntityBlueprintFactory*>(
                        s_CurrentFactory);
                    const uint32_t s_AspectIndex = s_AspectEntityBlueprintFactory->m_aSubEntitiesLookUp[i].m_nAspectIdx;
                    const uint32_t s_SubEntityIndex = s_AspectEntityBlueprintFactory->m_aSubEntitiesLookUp[i].
                        m_nSubentityIdx;
                    const auto s_TemplateBpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(
                        s_AspectEntityBlueprintFactory->m_aBlueprintFactories[s_AspectIndex]
                    );

                    if (s_TemplateBpFactory->m_pTemplateEntityBlueprint) {
                        s_EntityName = s_TemplateBpFactory->m_pTemplateEntityBlueprint->subEntities[s_SubEntityIndex].
                            entityName;
                    }
                }

                if (const auto s_Name = m_EntityNames.find(s_SubEntity); s_Name != m_EntityNames.end()) {
                    s_EntityName = s_Name->second;
                }

                const uint64_t s_BaseKey = s_SubEntityID & 0xFFFFFFFFFFFC000F;
                const bool s_IsEntityIDGenerated = p_AreEntitiesDynamic && Globals::EntityManager->m_DynamicEntityIdToCount.
                    contains(s_BaseKey);

                // Format a human-readable name for the entity.
                const auto s_EntityTypeName = (*s_SubEntity->GetType()->m_pInterfaceData)[0].m_Type->GetTypeInfo()->
                    pszTypeName;
                const auto s_EntityHumanName = fmt::format(
                    "{} ({:016x}){}",
                    s_EntityName,
                    s_SubEntityID,
                    p_AreEntitiesDynamic ? (s_IsEntityIDGenerated ? " **" : " *") : ""
                );

                std::string s_ReferencedBlueprintFactoryType;

                if (s_SubEntityFactory->IsTemplateEntityBlueprintFactory()) {
                    s_ReferencedBlueprintFactoryType = "TBLU";
                }
                else if (s_SubEntityFactory->IsAspectEntityBlueprintFactory()) {
                    s_ReferencedBlueprintFactoryType = "ASEB";
                }
                else if (s_SubEntityFactory->IsCppEntityBlueprintFactory()) {
                    s_ReferencedBlueprintFactoryType = "CBLU";
                }
                else if (s_SubEntityFactory->IsExtendedCppEntityBlueprintFactory()) {
                    s_ReferencedBlueprintFactoryType = "ECPB";
                }
                else if (s_SubEntityFactory->IsUIControlBlueprintFactory()) {
                    s_ReferencedBlueprintFactoryType = "UICB";
                }
                else if (s_SubEntityFactory->IsRenderMaterialEntityBlueprintFactory()) {
                    s_ReferencedBlueprintFactoryType = "MATB";
                }
                else if (s_SubEntityFactory->IsBehaviorTreeEntityBlueprintFactory()) {
                    s_ReferencedBlueprintFactoryType = "AIBB";
                }
                else if (s_SubEntityFactory->IsAudioSwitchBlueprintFactory()) {
                    s_ReferencedBlueprintFactoryType = "WSWB";
                }
                else if (s_SubEntityFactory->IsAudioStateBlueprintFactory()) {
                    s_ReferencedBlueprintFactoryType = "WSGB";
                }

                // Add the node to the map.
                const auto s_SubEntityNode = std::make_shared<EntityTreeNode>(
                    s_EntityHumanName,
                    s_EntityTypeName,
                    s_SubEntityID,
                    s_CurrentFactory->m_ridResource,
                    s_IsTemplateEntityBlueprintFactory ? "TBLU" : "ASEB",
                    s_SubEntityFactory->m_ridResource,
                    s_ReferencedBlueprintFactoryType,
                    s_SubEntity,
                    p_AreEntitiesDynamic
                );

                const auto s_LogicalParent = s_SubEntity.GetLogicalParent();

                if (s_LogicalParent) {
                    auto s_ParentNode = p_NodeMap.find(s_LogicalParent);

                    if (s_ParentNode != p_NodeMap.end()) {
                        // If we have already seen the logical parent of this sub-entity, add it to the parent's children.
                        if (p_AreEntitiesDynamic && s_ParentNode->second == s_SceneNode) {
                            s_DynamicEntitiesNode->Children.insert({ s_EntityHumanName, s_SubEntityNode });
                            s_SubEntityNode->Parent = s_DynamicEntitiesNode;
                        }
                        else {
                            s_ParentNode->second->Children.insert({ s_EntityHumanName, s_SubEntityNode });
                            s_SubEntityNode->Parent = s_ParentNode->second;
                        }
                    }
                    else {
                        // Otherwise, add it to the parentless nodes queue.
                        s_ParentlessNodes.push(s_SubEntityNode);
                    }
                }
                else {
                    // If it has no logical parent, add it to the parentless nodes queue.
                    s_ParentlessNodes.push(s_SubEntityNode);
                }

                // If the sub-entity has a factory with more sub-entities, add it to the queue.
                if (s_SubEntityFactory && s_SubEntityFactory->GetSubEntitiesCount() > 0) {
                    s_NodeQueue.emplace(s_SubEntityFactory, s_SubEntity);
                }

                SetEntityTreeNode(p_NodeMap, p_NodesById, s_SubEntity, s_SubEntityNode);
            }
        }
        else if (p_AreEntitiesDynamic) {
            if (p_NodeMap.contains(s_CurrentRoot)) {
                continue;
            }

            const auto s_SubEntityID = s_CurrentRoot->GetType()->m_nEntityID;
            const uint64_t s_BaseKey = s_SubEntityID & 0xFFFFFFFFFFFC000F;
            const bool s_IsEntityIDGenerated = p_AreEntitiesDynamic && Globals::EntityManager->m_DynamicEntityIdToCount.
                contains(s_BaseKey);

            // Format a human-readable name for the entity.
            const auto s_EntityTypeName = (*s_CurrentRoot->GetType()->m_pInterfaceData)[0].m_Type->GetTypeInfo()->pszTypeName;
            const auto s_EntityHumanName = fmt::format(
                "{} ({:016x}{}){}", s_EntityTypeName, s_SubEntityID, "", p_AreEntitiesDynamic ? (s_IsEntityIDGenerated ? " **" : " *") : ""
            );

            std::string s_BlueprintFactoryType;

            if (s_CurrentFactory->IsTemplateEntityBlueprintFactory()) {
                s_BlueprintFactoryType = "TBLU";
            }
            else if (s_CurrentFactory->IsAspectEntityBlueprintFactory()) {
                s_BlueprintFactoryType = "ASEB";
            }
            else if (s_CurrentFactory->IsCppEntityBlueprintFactory()) {
                s_BlueprintFactoryType = "CBLU";
            }
            else if (s_CurrentFactory->IsExtendedCppEntityBlueprintFactory()) {
                s_BlueprintFactoryType = "ECPB";
            }
            else if (s_CurrentFactory->IsUIControlBlueprintFactory()) {
                s_BlueprintFactoryType = "UICB";
            }
            else if (s_CurrentFactory->IsRenderMaterialEntityBlueprintFactory()) {
                s_BlueprintFactoryType = "MATB";
            }
            else if (s_CurrentFactory->IsBehaviorTreeEntityBlueprintFactory()) {
                s_BlueprintFactoryType = "AIBB";
            }
            else if (s_CurrentFactory->IsAudioSwitchBlueprintFactory()) {
                s_BlueprintFactoryType = "WSWB";
            }
            else if (s_CurrentFactory->IsAudioStateBlueprintFactory()) {
                s_BlueprintFactoryType = "WSGB";
            }

            // Add the node to the map.
            const auto s_SubEntityNode = std::make_shared<EntityTreeNode>(
                s_EntityHumanName, s_EntityTypeName, s_SubEntityID, s_CurrentFactory->m_ridResource, s_BlueprintFactoryType, -1, "",
                s_CurrentRoot, p_AreEntitiesDynamic
            );

            const auto s_LogicalParent = s_CurrentRoot.GetLogicalParent();

            if (s_LogicalParent) {
                auto s_ParentNode = p_NodeMap.find(s_LogicalParent);

                if (s_ParentNode != p_NodeMap.end()) {
                    // If we have already seen the logical parent of this sub-entity, add it to the parent's children.
                    if (p_AreEntitiesDynamic && s_ParentNode->second == s_SceneNode) {
                        s_DynamicEntitiesNode->Children.insert({ s_EntityHumanName, s_SubEntityNode });
                        s_SubEntityNode->Parent = s_DynamicEntitiesNode;
                    }
                    else {
                        s_ParentNode->second->Children.insert({ s_EntityHumanName, s_SubEntityNode });
                        s_SubEntityNode->Parent = s_ParentNode->second;
                    }
                }
                else {
                    // Otherwise, add it to the parentless nodes queue.
                    s_ParentlessNodes.push(s_SubEntityNode);
                }
            }
            else {
                // If it has no logical parent, add it to the parentless nodes queue.
                s_ParentlessNodes.push(s_SubEntityNode);
            }

            SetEntityTreeNode(p_NodeMap, p_NodesById, s_CurrentRoot, s_SubEntityNode);
        }
    }

    // Go through the nodes and assign any remaining children to their parents.
    while (!s_ParentlessNodes.empty()) {
        const auto s_Node = s_ParentlessNodes.front();
        s_ParentlessNodes.pop();

        // Skip entities from second and later factories referenced by aspect factories
        if (s_Node->Entity &&
            *s_Node->Entity.m_pObj &&
            reinterpret_cast<intptr_t>(*s_Node->Entity.m_pObj) & 1
        ) {
            continue;
        }

        const auto s_LogicalParent = s_Node->Entity.GetLogicalParent();

        // If it has a logical parent and that parent is in the map, add it to the parent's children.
        if (s_LogicalParent) {
            auto s_ParentNode = p_NodeMap.find(s_LogicalParent);

            if (s_ParentNode != p_NodeMap.end()) {
                if (p_AreEntitiesDynamic && s_ParentNode->second == s_SceneNode) {
                    s_DynamicEntitiesNode->Children.insert({ s_Node->Name, s_Node });
                    s_Node->Parent = s_DynamicEntitiesNode;
                }
                else {
                    s_ParentNode->second->Children.insert({ s_Node->Name, s_Node });
                    s_Node->Parent = s_ParentNode->second;
                }

                continue;
            }
        }

        if (p_AreEntitiesDynamic) {
            s_DynamicEntitiesNode->Children.insert({ s_Node->Name, s_Node });
            s_Node->Parent = s_DynamicEntitiesNode;
        }
        else {
            // Otherwise, add it to the "Unparented entities" node.
            s_UnparentedEntitiesNode->Children.insert({ s_Node->Name, s_Node });
            s_Node->Parent = s_UnparentedEntitiesNode;
        }
    }

    m_EntityTreeRowsDirty = true;
}

void Editor::UpdateEntities() {
    const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

    if (!s_SceneCtx
        || !s_SceneCtx->m_pScene
        || !s_SceneCtx->m_pScene.m_entityRef
        || s_SceneCtx->m_aLoadedBricks.size() == 0) {
        return;
    }

    const auto s_SceneEnt = s_SceneCtx->m_pScene.m_entityRef;

    std::vector<ZEntityRef> s_EntsToProcess;

    // Add all the brick nodes to the queue.
    for (const auto& s_Brick : s_SceneCtx->m_aLoadedBricks) {
        auto s_BrickEnt = s_Brick.m_EntityRef;

        if (!s_BrickEnt) {
            continue;
        }

        s_EntsToProcess.push_back(s_BrickEnt);
    }

    // Add all custom entities to the queue.
    for (const auto& s_SpawnedEntity : m_SpawnedEntities | std::views::values) {
        s_EntsToProcess.push_back(s_SpawnedEntity.Entity);
    }
    {
        std::unique_lock s_Lock(m_EntityRefToFactoryRuntimeResourceIDsMutex);

        m_EntityRefToFactoryRuntimeResourceIDs[s_SceneEnt] = {
            ResId<"	[modules:/zsceneentity.class].pc_entitytype">,
            s_SceneCtx->m_SceneConfig.m_ridSceneFactory
        };
    }

    auto s_SceneBlueprintFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_SceneCtx->m_SceneConfig.
        m_sceneBlueprint.GetResourceData());

    // Create the root scene node.
    auto s_SceneNode = std::make_shared<EntityTreeNode>(
        "Scene root",
        (*s_SceneEnt->GetType()->m_pInterfaceData)[0].m_Type->GetTypeInfo()->pszTypeName,
        s_SceneEnt->GetType()->m_nEntityID,
        s_SceneBlueprintFactory->m_ridResource,
        "TBLU",
        ResId<"[modules:/zsceneentity.class].pc_entityblueprint">,
        "CBLU",
        s_SceneEnt
    );

    auto s_UnparentedEntitiesNode = std::make_shared<EntityTreeNode>(
        "Unparented entities",
        "",
        -1,
        -1,
        "",
        -1,
        "",
        m_UnparentedEntitiesNodeEntityRef
    );

    s_SceneNode->Children.insert(std::make_pair(s_UnparentedEntitiesNode->Name, s_UnparentedEntitiesNode));
    s_UnparentedEntitiesNode->Parent = s_SceneNode;

    auto s_SpawnedEntitiesNode = std::make_shared<EntityTreeNode>(
        "Spawned entities",
        "",
        -1,
        -1,
        "",
        -1,
        "",
        m_SpawnedEntitiesNodeEntityRef
    );

    s_SceneNode->Children.insert(std::make_pair(s_SpawnedEntitiesNode->Name, s_SpawnedEntitiesNode));
    s_SpawnedEntitiesNode->Parent = s_SceneNode;

    std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>> s_NodeMap;
    std::unordered_multimap<uint64_t, std::shared_ptr<EntityTreeNode>> s_NodesById;
    SetEntityTreeNode(s_NodeMap, s_NodesById, s_SceneEnt, s_SceneNode);
    UpdateEntityTree(s_NodeMap, s_NodesById, s_EntsToProcess, false);

    AddDynamicEntitiesToEntityTree(s_SceneNode, s_NodeMap, s_NodesById);

    if (m_ReparentDynamicOutfitEntities) {
        ReparentDynamicOutfitEntities(s_NodeMap);
    }

    for (const auto& s_SpawnedEntity : m_SpawnedEntities | std::views::values) {
        ReparentSpawnedEntity(s_NodeMap, s_SpawnedEntity.Entity);
    }

    // Update the cached tree.
    m_CachedEntityTreeMutex.lock();
    m_CachedEntityTree.swap(s_SceneNode);
    m_CachedEntityTreeMap.swap(s_NodeMap);
    m_CachedEntityTreeNodesById.swap(s_NodesById);
    m_EntityTreeRowsDirty = true;
    m_CachedEntityTreeMutex.unlock();

    m_Server.OnEntityTreeRebuilt();
}

void Editor::AddDynamicEntitiesToEntityTree(
    const std::shared_ptr<EntityTreeNode>& p_SceneNode,
    std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>>& p_NodeMap,
    std::unordered_multimap<uint64_t, std::shared_ptr<EntityTreeNode>>& p_NodesById
) {
    auto s_DynamicEntitiesNode = std::make_shared<EntityTreeNode>(
        "Dynamic entities",
        "",
        -1,
        -1,
        "",
        -1,
        "",
        m_DynamicEntitiesNodeEntityRef
    );

    p_SceneNode->Children.insert(std::make_pair(s_DynamicEntitiesNode->Name, s_DynamicEntitiesNode));
    s_DynamicEntitiesNode->Parent = p_SceneNode;

    std::vector<ZEntityRef> s_DynamicEntities;
    {
        std::scoped_lock s_ScopedLock(m_DynamicEntitiesMutex);

        s_DynamicEntities.reserve(m_DynamicEntities.size());

        for (const auto& s_DynamicEntity : m_DynamicEntities) {
            s_DynamicEntities.push_back(s_DynamicEntity);
        }

        m_PendingDynamicEntities.clear();
    }

    if (!s_DynamicEntities.empty()) {
        UpdateEntityTree(p_NodeMap, p_NodesById, s_DynamicEntities, true);
    }
}

void Editor::ReparentDynamicOutfitEntities(
    std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>>& p_NodeMap
) {
    const auto s_SceneEntity = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene.m_entityRef;
    const std::shared_ptr<EntityTreeNode> s_SceneNode = p_NodeMap[s_SceneEntity];
    const std::shared_ptr<EntityTreeNode> s_DynamicEntitiesNode = s_SceneNode->Children.find("Dynamic entities")->
        second;

    std::vector<std::pair<std::shared_ptr<EntityTreeNode>, std::shared_ptr<EntityTreeNode>>> s_NodesToReparent;

    static STypeID* s_ClothCharacterEntityTypeID = (*Globals::TypeRegistry)->GetTypeID("ZClothCharacterEntity");
    static STypeID* s_LinkedProxyEntityTypeID = (*Globals::TypeRegistry)->GetTypeID("ZLinkedProxyEntity");

    for (const auto& [_, s_Node] : s_DynamicEntitiesNode->Children) {
        if (!s_Node->IsPendingDeletion.load(std::memory_order_acquire) &&
            s_Node->Entity && (
                s_Node->Entity.QueryInterface<ZClothCharacterEntity>(s_ClothCharacterEntityTypeID) ||
                s_Node->Entity.QueryInterface<ZLinkedProxyEntity>(s_LinkedProxyEntityTypeID)
                )) {
            ZEntityRef s_ParentRef = s_Node->Entity.GetProperty<TEntityRef<ZSpatialEntity>>("m_eidParent").Get().
                m_entityRef;

            if (!s_ParentRef) {
                continue;
            }

            auto s_ParentNodeIt = p_NodeMap.find(s_ParentRef);

            if (s_ParentNodeIt != p_NodeMap.end()) {
                s_NodesToReparent.emplace_back(s_ParentNodeIt->second, s_Node);
            }
        }
    }

    for (auto& [s_ParentNode, s_Node] : s_NodesToReparent) {
        s_DynamicEntitiesNode->Children.erase(s_Node->Name);

        s_ParentNode->Children.insert({ s_Node->Name, s_Node });
        s_Node->Parent = s_ParentNode;

        s_Node->Entity.SetLogicalParent(
            s_Node->Entity.GetProperty<TEntityRef<ZSpatialEntity>>("m_eidParent").Get().m_entityRef
        );
    }
}

bool Editor::IsSpecialEntityTreeNode(ZEntityRef p_Entity) const {
    return p_Entity == m_DynamicEntitiesNodeEntityRef ||
        p_Entity == m_UnparentedEntitiesNodeEntityRef ||
        p_Entity == m_SpawnedEntitiesNodeEntityRef;
}

bool Editor::HasVisibleChildren(const std::shared_ptr<EntityTreeNode>& p_Node) const {
    if (!p_Node || p_Node->Children.empty()) {
        return false;
    }

    if (!m_HasActiveFilters) {
        return true;
    }

    return std::any_of(
        p_Node->Children.begin(),
        p_Node->Children.end(),
        [this](const auto& s_Child) {
            return m_FilteredEntityTreeNodes.contains(s_Child.second.get());
        }
    );
}

Editor::EntityTreeNodeName Editor::SplitEntityTreeNodeName(const std::string_view p_NodeName) {
    // Node names end in " (entity ID)" and an optional dynamic entity marker.
    const size_t s_IdStart = p_NodeName.rfind(" (");
    const size_t s_IdEnd = p_NodeName.find(')', s_IdStart);

    if (s_IdStart == std::string_view::npos || s_IdEnd == std::string_view::npos) {
        return { p_NodeName };
    }

    return {
        p_NodeName.substr(0, s_IdStart),
        p_NodeName.substr(s_IdStart + 2, s_IdEnd - s_IdStart - 2),
        p_NodeName.substr(s_IdEnd + 1)
    };
}

bool Editor::ShouldOpenForScroll(const EntityTreeNode& p_Node) const {
    if (!m_ScrollToEntity || !m_SelectedEntity) {
        return false;
    }

    if (!IsSpecialEntityTreeNode(p_Node.Entity)) {
        return p_Node.Entity && m_SelectedEntity.IsAnyParent(p_Node.Entity);
    }

    for (const auto& [_, s_Child] : p_Node.Children) {
        if (s_Child->Entity == m_SelectedEntity ||
            (s_Child->Entity && m_SelectedEntity.IsAnyParent(s_Child->Entity))) {
            return true;
        }
    }

    return false;
}

void Editor::DrawSpawnedEntityContextMenu(const EntityTreeNode& p_Node, const bool p_IsSpawnedEntitiesNode) {
    if (!ImGui::BeginPopupContextItem("##SpawnedEntityMenu")) {
        return;
    }

    m_EntityTreeMenuNode = &p_Node;

    if (!p_IsSpawnedEntitiesNode && ImGui::MenuItem("Delete")) {
        OnDestroyEntity(p_Node.Entity, std::nullopt);
    }

    if (p_IsSpawnedEntitiesNode && ImGui::MenuItem("Delete all", nullptr, false, !p_Node.Children.empty())) {
        for (const auto& s_Child : p_Node.Children | std::views::values) {
            OnDestroyEntity(s_Child->Entity, std::nullopt);
        }
    }

    ImGui::EndPopup();
}

// Returns the row of the selected entity when p_OpenForScroll is set, otherwise -1.
int Editor::BuildEntityTreeRows(const bool p_OpenForScroll) {
    m_EntityTreeRows.clear();

    int s_SelectedRow = -1;

    AddEntityTreeRows(m_CachedEntityTree, 0, -1, p_OpenForScroll, s_SelectedRow);

    return s_SelectedRow;
}

void Editor::AddEntityTreeRows(
    const std::shared_ptr<EntityTreeNode>& p_Node,
    const int p_Depth,
    const int p_ParentRow,
    const bool p_OpenForScroll,
    int& p_SelectedRow
) {
    if (!p_Node) {
        return;
    }

    if (m_HasActiveFilters && !m_FilteredEntityTreeNodes.contains(p_Node.get())) {
        return;
    }

    const int s_RowIndex = static_cast<int>(m_EntityTreeRows.size());
    const bool s_HasVisibleChildren = HasVisibleChildren(p_Node);

    // Nodes after the selected one are not opened.
    if (p_OpenForScroll && p_SelectedRow < 0) {
        if (p_Node->Entity == m_SelectedEntity) {
            p_SelectedRow = s_RowIndex;
        }
        else if (ShouldOpenForScroll(*p_Node)) {
            m_OpenEntityTreeNodes.insert(p_Node.get());
        }
    }

    m_EntityTreeRows.push_back({ p_Node, p_Depth, p_ParentRow, -1, s_HasVisibleChildren });

    if (p_ParentRow >= 0) {
        m_EntityTreeRows[p_ParentRow].LastChildRow = s_RowIndex;
    }

    if (!s_HasVisibleChildren || !m_OpenEntityTreeNodes.contains(p_Node.get())) {
        return;
    }

    for (const auto& s_Child : p_Node->Children) {
        AddEntityTreeRows(s_Child.second, p_Depth + 1, s_RowIndex, p_OpenForScroll, p_SelectedRow);
    }
}

static float GetEntityTreeRowY(const int p_RowIndex, const ImGuiListClipper& p_Clipper) {
    return static_cast<float>(
        p_Clipper.StartPosY + p_Clipper.StartSeekOffsetY + p_RowIndex * static_cast<double>(p_Clipper.ItemsHeight)
    );
}

void Editor::RenderEntityTreeRows(const int p_SelectedRow, const float p_RootNameX) {
    ImGuiListClipper s_Clipper;
    s_Clipper.Begin(static_cast<int>(m_EntityTreeRows.size()));

    // `SetScrollHereY` aims at the row before the selected one.
    if (p_SelectedRow >= 0) {
        s_Clipper.IncludeItemsByIndex(std::max(0, p_SelectedRow - 1), p_SelectedRow + 1);
    }

    // The context menu closes when its row is not submitted.
    if (m_EntityTreeMenuNode) {
        const auto s_MenuRow = std::ranges::find_if(
            m_EntityTreeRows,
            [this](const EntityTreeRow& p_Row) {
                return p_Row.Node.get() == m_EntityTreeMenuNode;
            }
        );

        if (s_MenuRow != m_EntityTreeRows.end()) {
            s_Clipper.IncludeItemByIndex(static_cast<int>(s_MenuRow - m_EntityTreeRows.begin()));
        }

        m_EntityTreeMenuNode = nullptr;
    }

    std::vector<int> s_PushedRows;
    int s_LastSubmittedRow = -1;

    while (s_Clipper.Step()) {
        for (int i = s_Clipper.DisplayStart;
             i < s_Clipper.DisplayEnd && i < static_cast<int>(m_EntityTreeRows.size());
             ++i) {
            PushEntityTreeAncestors(i, s_PushedRows, s_LastSubmittedRow, s_Clipper, p_RootNameX);

            bool s_OpenStateChanged = false;

            if (RenderEntityRow(m_EntityTreeRows[i], s_OpenStateChanged)) {
                s_PushedRows.push_back(i);
            }

            // Rows up to i stay the same, so the pushed rows are still valid.
            if (s_OpenStateChanged) {
                BuildEntityTreeRows(false);
                s_Clipper.ItemsCount = static_cast<int>(m_EntityTreeRows.size());
            }

            s_LastSubmittedRow = i;
        }
    }

    while (!s_PushedRows.empty()) {
        PopEntityTreeRow(s_PushedRows, s_LastSubmittedRow, s_Clipper);
    }
}

// The clipper skips rows, so ancestors of a submitted row may not have been submitted.
// p_PushedRows[d] is the pushed row at depth d.
void Editor::PushEntityTreeAncestors(
    const int p_RowIndex,
    std::vector<int>& p_PushedRows,
    const int p_LastSubmittedRow,
    const ImGuiListClipper& p_Clipper,
    const float p_RootNameX
) {
    std::vector<int> s_MissingRows;
    int s_Ancestor = m_EntityTreeRows[p_RowIndex].ParentRow;

    while (s_Ancestor >= 0) {
        const int s_Depth = m_EntityTreeRows[s_Ancestor].Depth;

        if (s_Depth < static_cast<int>(p_PushedRows.size()) && p_PushedRows[s_Depth] == s_Ancestor) {
            break;
        }

        s_MissingRows.push_back(s_Ancestor);
        s_Ancestor = m_EntityTreeRows[s_Ancestor].ParentRow;
    }

    const size_t s_KeepCount = s_Ancestor >= 0 ? m_EntityTreeRows[s_Ancestor].Depth + 1 : 0;

    while (p_PushedRows.size() > s_KeepCount) {
        PopEntityTreeRow(p_PushedRows, p_LastSubmittedRow, p_Clipper);
    }

    for (const int s_MissingRow : s_MissingRows | std::views::reverse) {
        PushEntityTreeRow(s_MissingRow, p_Clipper, p_RootNameX);
        p_PushedRows.push_back(s_MissingRow);
    }
}

// Does what TreeNodeEx() does for an open node outside the clip rect, so ImGui still draws its tree lines.
void Editor::PushEntityTreeRow(const int p_RowIndex, const ImGuiListClipper& p_Clipper, const float p_RootNameX) {
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiWindow* s_Window = g.CurrentWindow;
    const EntityTreeRow& s_Row = m_EntityTreeRows[p_RowIndex];

    ImGui::PushID(s_Row.Node.get());

    const ImGuiID s_Id = ImGui::GetID("##EntityTreeNode");
    const float s_RowY = GetEntityTreeRowY(p_RowIndex, p_Clipper);

    if (g.Style.TreeLinesSize > 0.f && s_RowY < s_Window->ClipRect.Max.y) {
        const float s_NameX = p_RootNameX + s_Row.Depth * g.Style.IndentSpacing;

        ImGuiTreeNodeStackData s_Data {};
        s_Data.ID = s_Id;
        s_Data.TreeFlags = ImGuiTreeNodeFlags_DrawLinesToNodes;
        s_Data.ItemFlags = ImGuiItemFlags_None;
        s_Data.NavRect = ImRect(s_NameX, s_RowY, s_NameX, s_RowY + p_Clipper.ItemsHeight);
        s_Data.DrawLinesX1 = s_NameX + g.FontSize * 0.5f + g.Style.FramePadding.x;
        s_Data.DrawLinesToNodesY2 = -FLT_MAX;
        s_Data.DrawLinesTableColumn = 0;

        g.TreeNodeStack.push_back(s_Data);
        s_Window->DC.TreeHasStackDataDepthMask |= 1 << s_Window->DC.TreeDepth;
        s_Window->DC.TreeRecordsClippedNodesY2Mask |= 1 << s_Window->DC.TreeDepth;
    }

    ImGui::TreePushOverrideID(s_Id);
}

void Editor::PopEntityTreeRow(
    std::vector<int>& p_PushedRows,
    const int p_LastSubmittedRow,
    const ImGuiListClipper& p_Clipper
) {
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiWindow* s_Window = g.CurrentWindow;
    const EntityTreeRow& s_Row = m_EntityTreeRows[p_PushedRows.back()];
    const ImU32 s_DepthMask = 1 << (s_Window->DC.TreeDepth - 1);

    // ImGui only extends the tree line to children that were submitted.
    if (s_Row.LastChildRow > p_LastSubmittedRow && (s_Window->DC.TreeHasStackDataDepthMask & s_DepthMask)) {
        ImGuiTreeNodeStackData& s_Data = g.TreeNodeStack.back();

        s_Data.DrawLinesToNodesY2 = ImMax(
            s_Data.DrawLinesToNodesY2,
            GetEntityTreeRowY(s_Row.LastChildRow, p_Clipper)
        );
    }

    ImGui::TreePop();
    ImGui::PopID();

    p_PushedRows.pop_back();
}

// Returns true when the row stays pushed for its children.
bool Editor::RenderEntityRow(const EntityTreeRow& p_Row, bool& p_OpenStateChanged) {
    const auto& s_Node = p_Row.Node;

    const bool s_IsPendingDeletion = s_Node->IsPendingDeletion.load(std::memory_order_acquire);

    ImGui::PushID(s_Node.get());

    const auto s_Entity = s_Node->Entity;
    const auto& s_EntityType = s_Node->EntityType;
    const auto& s_EntityName = s_Node->Name;
    const auto s_IsSelected = s_Entity == m_SelectedEntity;

    ImGuiTreeNodeFlags s_Flags = ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_DrawLinesToNodes;

    if (!p_Row.HasVisibleChildren) {
        s_Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (s_IsSelected) {
        s_Flags |= ImGuiTreeNodeFlags_Selected;

        if (m_ScrollToEntity) {
            ImGui::SetScrollHereY();
            m_ScrollToEntity = false;
        }
    }

    if (s_IsPendingDeletion) {
        p_OpenStateChanged = m_OpenEntityTreeNodes.erase(s_Node.get()) > 0;

        ImGui::PushStyleColor(ImGuiCol_Text, c_SecondaryTextColor);
        ImGui::BeginDisabled();
    }

    const bool s_IsDirectMatch = m_HasActiveFilters && s_Node->IsDirectMatch && !s_IsPendingDeletion;

    if (s_IsDirectMatch) {
        ImGui::PushStyleColor(ImGuiCol_Text, c_HighlightTextColor);
    }

    ImGui::SetNextItemOpen(
        m_OpenEntityTreeNodes.contains(s_Node.get()),
        ImGuiCond_Always
    );

    const auto s_Parent = s_Node->Parent.lock();
    const bool s_IsSpawnedEntitiesNode = s_Entity == m_SpawnedEntitiesNodeEntityRef;
    const bool s_IsSpawnedEntity = s_Parent && s_Parent->Entity == m_SpawnedEntitiesNodeEntityRef;

    const auto [s_DisplayName, s_DisplayId, s_DynamicMarker] = SplitEntityTreeNodeName(s_EntityName);
    const std::string_view s_Marker = s_IsSpawnedEntity ? " +" : s_DynamicMarker;

    std::string s_Label(s_DisplayName);

    if (s_DisplayName == "Scene") {
        ZString s_ResourceID;

        if (SDK()->GetResourceID(s_Node->EntityId, s_ResourceID)) {
            const std::string_view s_Path = s_ResourceID.c_str();
            const size_t s_Start = s_Path.find_last_of("/\\");
            const size_t s_End = s_Path.find(']', s_Start == std::string_view::npos ? 0 : s_Start);

            s_Label += " - ";
            s_Label += s_Path.substr(
                s_Start == std::string_view::npos ? 0 : s_Start + 1,
                s_End == std::string_view::npos ?
                std::string_view::npos : s_End - (s_Start == std::string_view::npos ? 0 : s_Start + 1)
            );
        }
    }

    ImGui::TableNextRow(ImGuiTableRowFlags_None, ScaledSize(17.f));
    ImGui::TableSetColumnIndex(0);

    const float s_NameStartX = ImGui::GetCursorPosX();

    ImGui::SetCursorPosX(s_NameStartX - m_EntityTreeNameScroll);

    auto s_Open = ImGui::TreeNodeEx(
        "##EntityTreeNode",
        s_Flags | ImGuiTreeNodeFlags_SpanAllColumns,
        "%.*s%.*s",
        static_cast<int>(s_Label.size()),
        s_Label.data(),
        static_cast<int>(s_Marker.size()),
        s_Marker.data()
    );

    if (ImGui::IsItemVisible()) {
        const float s_NameWidth =
            ImGui::CalcTextSize(s_Label.data(), s_Label.data() + s_Label.size()).x +
            ImGui::CalcTextSize(s_Marker.data(), s_Marker.data() + s_Marker.size()).x;

        m_EntityTreeNameWidth = std::max(
            m_EntityTreeNameWidth, s_NameStartX + ImGui::GetTreeNodeToLabelSpacing() + s_NameWidth
        );
    }

    if (ImGui::IsItemToggledOpen()) {
        p_OpenStateChanged = true;

        if (s_Open) {
            m_OpenEntityTreeNodes.insert(s_Node.get());
        }
        else {
            m_OpenEntityTreeNodes.erase(s_Node.get());
        }
    }

    if (s_IsDirectMatch) {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", s_EntityType.c_str());
    }

    if (!s_IsPendingDeletion) {
        if (ImGui::IsItemFocused() && !s_IsSelected) {
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Space))
                OnSelectEntity(s_Entity, false, std::nullopt);
        }

        if (ImGui::IsItemClicked()) {
            OnSelectEntity(s_Entity, false, std::nullopt);
        }

        if (s_IsSpawnedEntitiesNode || s_IsSpawnedEntity) {
            DrawSpawnedEntityContextMenu(*s_Node, s_IsSpawnedEntitiesNode);
        }
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::TextColored(
        s_IsSelected ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : c_MutedTextColor,
        "%.*s",
        static_cast<int>(s_DisplayId.size()),
        s_DisplayId.data()
    );

    if (s_IsPendingDeletion) {
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
    }

    if (s_Open && !(s_Flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
        return true;
    }

    ImGui::PopID();

    return false;
}

void Editor::FilterEntityTree() {
    m_FilteredEntityTreeNodes.clear();
    m_EntityTreeRowsDirty = true;
    m_TotalMatchCount = 0;

    if (!m_CachedEntityTree) {
        m_HasActiveFilters = false;
        m_CurrentEntitySearchResultIndex = 0;
        return;
    }

    m_HasActiveSearch = !m_EntityIDSearchInput.empty() ||
        !m_EntityTypeSearchInput.empty() ||
        !m_EntityNameSearchInput.empty();

    m_HasActiveFilters = m_HasActiveSearch ||
        m_EntityViewMode != EntityViewMode::All;

    if (!m_HasActiveFilters) {
        m_CurrentEntitySearchResultIndex = 0;
        m_LastEntityViewMode = m_EntityViewMode;
        return;
    }

    EntityTreeNode* s_SingleMatchNode = nullptr;

    FilterEntityTree(m_CachedEntityTree.get(), s_SingleMatchNode);

    if (m_FilteredEntityTreeNodes.empty()) {
        m_FilteredEntityTreeNodes.insert(m_CachedEntityTree.get());
    }

    m_CurrentEntitySearchResultIndex = 0;

    if (m_TotalMatchCount == 1 && s_SingleMatchNode) {
        OnSelectEntity(s_SingleMatchNode->Entity, true, std::nullopt);
    }

    m_LastEntityViewMode = m_EntityViewMode;
}

bool Editor::FilterEntityTree(EntityTreeNode* p_Node, EntityTreeNode*& p_OutSingleMatchNode) {
    if (!p_Node || p_Node->IsPendingDeletion.load(std::memory_order_acquire)) {
        return false;
    }

    if (m_EntityViewMode == EntityViewMode::ScenesAndBricks &&
        (p_Node->IsDynamicEntity || p_Node->Entity == m_DynamicEntitiesNodeEntityRef)) {
        p_Node->IsDirectMatch = false;
        return false;
    }

    const bool s_PassesViewMode =
        m_EntityViewMode == EntityViewMode::All ||
        m_EntityViewMode == EntityViewMode::ScenesAndBricks ||
        (m_EntityViewMode == EntityViewMode::DynamicEntities && p_Node->IsDynamicEntity);

    bool s_Matches = false;

    if (s_PassesViewMode && m_HasActiveSearch) {
        bool s_MatchesID = true;
        bool s_MatchesType = true;
        bool s_MatchesName = true;

        if (!IsSpecialEntityTreeNode(p_Node->Entity)) {
            if (!m_EntityIDSearchInput.empty()) {
                const uint64_t s_EntityID = std::strtoull(m_EntityIDSearchInput.c_str(), nullptr, 16);
                s_MatchesID = p_Node->EntityId == s_EntityID;
            }

            if (!m_EntityTypeSearchInput.empty()) {
                s_MatchesType = p_Node->Entity.HasInterface(m_EntityTypeSearchInput);
            }
        }

        if (!m_EntityNameSearchInput.empty()) {
            s_MatchesName = Util::StringUtils::FindSubstring(p_Node->Name.c_str(), m_EntityNameSearchInput);
        }

        s_Matches = s_MatchesID && s_MatchesType && s_MatchesName;

        // Special root containers match only if explicitly queried by name
        if (s_Matches && IsSpecialEntityTreeNode(p_Node->Entity)) {
            if (m_EntityNameSearchInput.empty() ||
                !Util::StringUtils::FindSubstring(p_Node->Name.c_str(), m_EntityNameSearchInput)) {
                s_Matches = false;
            }
        }
    }

    p_Node->IsDirectMatch = s_Matches;

    if (s_Matches) {
        m_TotalMatchCount++;
        p_OutSingleMatchNode = p_Node;
    }

    bool s_ChildMatches = false;

    for (auto& [_, child] : p_Node->Children) {
        if (FilterEntityTree(child.get(), p_OutSingleMatchNode)) {
            s_ChildMatches = true;
        }
    }

    if ((!m_HasActiveSearch && s_PassesViewMode) || s_Matches || s_ChildMatches) {
        m_FilteredEntityTreeNodes.insert(p_Node);
        return true;
    }

    return false;
}

// Needs m_CachedEntityTreeMutex.
void Editor::ClearFilters() {
    m_EntityIDSearchInput.clear();
    m_EntityTypeSearchInput.clear();
    m_EntityNameSearchInput.clear();

    m_EntityViewMode = EntityViewMode::All;
    m_LastEntityViewMode = EntityViewMode::All;

    m_FilteredEntityTreeNodes.clear();
    m_TotalMatchCount = 0;
    m_CurrentEntitySearchResultIndex = 0;
    m_HasActiveSearch = false;
    m_HasActiveFilters = false;
    m_EntityTreeRowsDirty = true;
}

std::shared_ptr<EntityTreeNode> Editor::FindMatchByIndex(
    const std::shared_ptr<EntityTreeNode>& p_Node,
    size_t p_TargetIndex,
    size_t& p_CurrentCounter
) {
    if (!p_Node || p_Node->IsPendingDeletion.load(std::memory_order_acquire)) {
        return nullptr;
    }

    if (p_Node->IsDirectMatch) {
        if (p_CurrentCounter == p_TargetIndex) {
            return p_Node;
        }

        p_CurrentCounter++;
    }

    for (const auto& [_, s_Child] : p_Node->Children) {
        if (auto s_Match = FindMatchByIndex(s_Child, p_TargetIndex, p_CurrentCounter)) {
            return s_Match;
        }
    }

    return nullptr;
}

// Needs m_CachedEntityTreeMutex.
void Editor::DrawEntityTreeWindow() {
    BeginPanelWindow(c_EntityTreeWindowName, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_Docking);

    const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

    if (s_SceneCtx && s_SceneCtx->m_pScene && s_SceneCtx->m_aLoadedBricks.size() > 0) {
        PushFrameHeight(20.f, 3.f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ScaledSize(5.f), ScaledSize(4.f)));

        BeginPanelPadding("##EntityTreeToolbar");

        DrawEntityTreeToolbar();

        EndPanelPadding();

        DrawEntityTreeTable();

        ImGui::PopStyleVar(2);

        /*const std::string s_PreviewLabel = fmt::format(
            "{:016X}",
            s_SceneCtx->m_aLoadedBricks[m_SelectedBrickIndex].runtimeResourceID.GetID());

        if (ImGui::BeginCombo(ICON_MD_GRID_VIEW " Bricks", s_PreviewLabel.c_str())) {
            for (int i = 0; i < s_SceneCtx->m_aLoadedBricks.size(); ++i) {
                const auto& s_Brick = s_SceneCtx->m_aLoadedBricks[i];
                const auto s_Selected = i == m_SelectedBrickIndex;

                if (ImGui::Selectable(
                    fmt::format("{:016X}", s_Brick.runtimeResourceID.GetID()).c_str(),
                    s_Selected
                ))
                    m_SelectedBrickIndex = i;

                if (s_Selected) ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        if (s_SceneCtx->m_aLoadedBricks.size() > 0) {
            RenderBrick(
                s_SceneCtx->m_aLoadedBricks[m_SelectedBrickIndex].entityRef,
                m_SelectedEntity
            );
        }*/
    }
    else {
        ImGui::Text("No scene loaded.");
    }

    ImGui::End();

    if (m_ScrollToEntity) {
        m_ScrollToEntity = false;
    }
}

void Editor::DrawEntityTreeToolbar() {
    const float s_ButtonSize = ScaledSize(20.f);
    const float s_Spacing = ImGui::GetStyle().ItemSpacing.x;

    ImGui::SetNextItemWidth(ScaledSize(70.f));

    if (ImGui::BeginCombo("##EntityViewMode", m_EntityViewModes[m_EntityViewMode].c_str())) {
        for (int i = 0; i < m_EntityViewModes.size(); ++i) {
            const bool s_IsSelected = m_EntityViewMode == static_cast<EntityViewMode>(i);

            if (ImGui::Selectable(m_EntityViewModes[i].c_str(), s_IsSelected)) {
                m_EntityViewMode = static_cast<EntityViewMode>(i);

                FilterEntityTree();
            }

            if (s_IsSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Entity view mode");
    }

    ImGui::SameLine();

    const bool s_HasSearchResults = m_TotalMatchCount > 0;

    // Clamp index in case matches shrank while searching
    if (s_HasSearchResults && m_CurrentEntitySearchResultIndex >= m_TotalMatchCount) {
        m_CurrentEntitySearchResultIndex = 0;
    }

    const std::string s_ResultCount = fmt::format(
        "{} / {}",
        s_HasSearchResults ? m_CurrentEntitySearchResultIndex + 1 : 0,
        m_TotalMatchCount
    );

    // 7 buttons, the match count, a separator and the spacing between them.
    const float s_ControlsWidth = s_ButtonSize * 7.f + ImGui::CalcTextSize(s_ResultCount.c_str()).x +
        ScaledSize(1.f) + s_Spacing * 9.f;

    ImGui::SetNextItemWidth(std::max(ScaledSize(40.f), ImGui::GetContentRegionAvail().x - s_ControlsWidth));

    Util::ImGuiUtils::InputWithAutocomplete(
        "##EntityTypesPopup",
        m_EntityTypeSearchInput,
        m_ClassNames,
        [](const std::string& p_ClassName) -> const std::string& {
            return p_ClassName;
        },
        [](const std::string& p_ClassName) -> const std::string& {
            return p_ClassName;
        },
        [&](const std::string&, const std::string&, const auto&) {
            FilterEntityTree();
        }
    );

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Filter by type");
    }

    ImGui::SameLine();

    ImGui::BeginDisabled(!s_HasSearchResults);

    bool s_ShouldSelectMatch = false;

    if (IconButton(ICON_MD_ARROW_BACK, "Previous match")) {
        if (m_CurrentEntitySearchResultIndex == 0) {
            m_CurrentEntitySearchResultIndex = m_TotalMatchCount - 1;
        }
        else {
            --m_CurrentEntitySearchResultIndex;
        }

        s_ShouldSelectMatch = true;
    }

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(s_ResultCount.c_str());
    ImGui::SameLine();

    if (IconButton(ICON_MD_ARROW_FORWARD, "Next match")) {
        m_CurrentEntitySearchResultIndex = (m_CurrentEntitySearchResultIndex + 1) % m_TotalMatchCount;
        s_ShouldSelectMatch = true;
    }

    ImGui::EndDisabled();

    if (s_ShouldSelectMatch) {
        size_t s_Counter = 0;

        if (auto s_Node = FindMatchByIndex(m_CachedEntityTree, m_CurrentEntitySearchResultIndex, s_Counter)) {
            OnSelectEntity(s_Node->Entity, true, std::nullopt);
        }
    }

    ImGui::SameLine();

    DrawToolbarSeparator();
    ImGui::SameLine();

    if (IconButton(ICON_MD_CLEAR, "Clear filters")) {
        ClearFilters();
    }

    ImGui::SameLine();

    if (IconButton(ICON_MD_CONSTRUCTION, m_CachedEntityTree ? "Rebuild entity tree" : "Build entity tree")) {
        QueueTask([this] { UpdateEntities(); });
    }

    ImGui::SameLine();

    ImGui::BeginDisabled(!m_CachedEntityTree);

    if (IconButton(ICON_MD_UNFOLD_LESS, "Collapse tree")) {
        m_OpenEntityTreeNodes.clear();
        m_EntityTreeRowsDirty = true;
    }

    ImGui::SameLine();

    if (IconButton(
        ICON_MD_SAVE_ALT,
        "Export entity tree. Outputs the current entity tree as a text file (entity_tree.txt - found in the "
        "Retail folder of the game)."
    )) {
        ExportEntityTree();
    }

    ImGui::EndDisabled();
    ImGui::SameLine();

    if (IconButton(ICON_MD_TAG, m_ShowEntityIds ? "Hide entity IDs" : "Show entity IDs", m_ShowEntityIds)) {
        m_ShowEntityIds = !m_ShowEntityIds;
        SetSettingBool("general", "show_entity_ids", m_ShowEntityIds);
    }
}

// Needs m_CachedEntityTreeMutex.
void Editor::ExportEntityTree() {
    std::ofstream s_FileOut("entity_tree.txt");

    if (s_FileOut.is_open()) {
        auto s_OutputNode = [&](auto& self, const std::shared_ptr<EntityTreeNode>& p_Node, uint32_t p_Depth) -> void {
            if (!p_Node || p_Node->IsPendingDeletion.load(std::memory_order_acquire)) {
                return;
            }

            if (m_HasActiveFilters && !m_FilteredEntityTreeNodes.contains(p_Node.get())) {
                return;
            }

            const bool s_HasVisibleChildren = HasVisibleChildren(p_Node);

            const std::string s_Prefix = p_Depth != 0
                ? std::string(p_Depth * 2, ' ') + (s_HasVisibleChildren ? "> " : "- ")
                : "";

            s_FileOut << s_Prefix << p_Node->Name << std::endl;

            if (!s_HasVisibleChildren) {
                return;
            }

            for (const auto& [_, s_Child] : p_Node->Children) {
                self(self, s_Child, p_Depth + 1);
            }
            };

        if (m_CachedEntityTree) {
            s_OutputNode(s_OutputNode, m_CachedEntityTree, 0);
        }
    }
}

void Editor::DrawEntityTreeTable() {
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, c_ListRowHoverColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, c_AlternateRowColor);
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, c_SectionHeaderColor);
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, c_SectionBorderColor);
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, c_RowBorderColor);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(ScaledSize(3.f), ScaledSize(1.f)));
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, ScaledSize(14.f));

    // Tables can't scroll a single column, so the name column gets its own scrollbar.
    const float s_ScrollbarHeight = ImGui::GetStyle().ScrollbarSize;

    if (ImGui::BeginTable(
        "##EntityTree",
        2,
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable,
        ImVec2(0.f, -s_ScrollbarHeight)
    )) {
        ImGui::TableSetupScrollFreeze(0, 2);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, ScaledSize(132.f));
        ImGui::TableSetColumnEnabled(1, m_ShowEntityIds);

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers, ScaledSize(20.f));

        for (int i = 0; i < 2; ++i) {
            ImGui::TableSetColumnIndex(i);
            ImGui::AlignTextToFramePadding();
            ImGui::TableHeader(ImGui::TableGetColumnName(i));
        }

        ImGui::TableNextRow(ImGuiTableRowFlags_None, ScaledSize(24.f));
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_WindowBg));

        ImGui::TableSetColumnIndex(0);

        const float s_NameColumnScreenX = ImGui::GetCursorScreenPos().x;
        const float s_NameColumnWidth = ImGui::GetContentRegionAvail().x;
        const float s_NameContentWidth = m_EntityTreeNameWidth - ImGui::GetCursorPosX();

        m_EntityTreeNameWidth = 0.f;

        if (ImGui::IsWindowHovered() && ImGui::GetIO().KeyShift) {
            m_EntityTreeNameScroll -= ImGui::GetIO().MouseWheel * ScaledSize(40.f);
        }

        ImGui::SetNextItemWidth(-FLT_MIN);

        if (ImGui::InputTextWithHint(
            "##EntityNameFilter",
            "Filter name",
            &m_EntityNameSearchInput,
            ImGuiInputTextFlags_EnterReturnsTrue
        )) {
            FilterEntityTree();
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-FLT_MIN);

        if (ImGui::InputTextWithHint(
            "##EntityIdFilter",
            "Filter ID",
            &m_EntityIDSearchInput,
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal |
            ImGuiInputTextFlags_CharsNoBlank
        )) {
            FilterEntityTree();
        }

        PushFrameHeight(15.f, 3.f);

        const bool s_HasNoResults = m_HasActiveSearch && m_TotalMatchCount == 0;

        if (s_HasNoResults) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(c_ErrorTextColor, "No results found.");
        }
        else if (m_CachedEntityTree) {
            const bool s_RowsDirty = std::exchange(m_EntityTreeRowsDirty, false);
            int s_SelectedRow = -1;

            if (s_RowsDirty || m_ScrollToEntity) {
                s_SelectedRow = BuildEntityTreeRows(m_ScrollToEntity);
            }

            RenderEntityTreeRows(s_SelectedRow, s_NameColumnScreenX - m_EntityTreeNameScroll);
        }
        else {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted("No entities loaded. Build the entity tree to load them.");
        }

        ImGui::PopStyleVar();

        ImGui::EndTable();

        const float s_ScrollbarY = ImGui::GetItemRectMax().y;
        ImS64 s_Scroll = static_cast<ImS64>(m_EntityTreeNameScroll);

        ImGui::ScrollbarEx(
            ImRect(
                s_NameColumnScreenX,
                s_ScrollbarY,
                s_NameColumnScreenX + s_NameColumnWidth,
                s_ScrollbarY + s_ScrollbarHeight
            ),
            ImGui::GetID("##EntityTreeNameScroll"),
            ImGuiAxis_X,
            &s_Scroll,
            static_cast<ImS64>(s_NameColumnWidth),
            static_cast<ImS64>(s_NameContentWidth)
        );

        m_EntityTreeNameScroll = std::clamp(
            static_cast<float>(s_Scroll), 0.f, std::max(0.f, s_NameContentWidth - s_NameColumnWidth)
        );
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(7);
}

// Needs m_CachedEntityTreeMutex.
void Editor::OnSelectEntity(
    ZEntityRef p_Entity,
    bool p_ShouldScrollToEntity,
    const std::optional<std::string> p_ClientId
) {
    // An entity reference property is waiting for a pick, so the click fills it instead.
    if (!m_EntityRefPickTarget.empty()) {
        if (p_Entity && !p_ClientId && m_SelectedEntity) {
            m_PickedEntityRef = p_Entity;
            return;
        }

        m_EntityRefPickTarget.clear();
    }

    QueueTask([this, p_Entity, p_ShouldScrollToEntity, p_ClientId] {
        SelectEntityInternal(p_Entity, p_ShouldScrollToEntity, p_ClientId);
    });
}

void Editor::SelectEntityInternal(
    ZEntityRef p_Entity,
    bool p_ShouldScrollToEntity,
    std::optional<std::string> p_ClientId
) {
    assert(std::this_thread::get_id() == m_UpdateThreadId);

    if (m_SelectedEntity.m_pObj == p_Entity.m_pObj) {
        return;
    }

    if (p_Entity && !IsSpecialEntityTreeNode(p_Entity) && !IsEntityInTree(p_Entity)) {
        return;
    }

    ZRenderMaterialInstance* s_MaterialInstance = p_Entity && !IsSpecialEntityTreeNode(p_Entity)
        ? LoadSelectedMaterialInstance(p_Entity)
        : nullptr;

    {
        std::scoped_lock s_Lock(m_CachedEntityTreeMutex);

        m_ScrollToEntity = p_ShouldScrollToEntity && p_Entity.GetEntity() != nullptr;
        m_SelectedEntity = p_Entity;
        m_SelectedMaterialInstance = s_MaterialInstance;

        if (m_SelectActorOnMouseClick) {
            ZActor* s_Actor = nullptr;
            ZEntityRef logicalParent = m_SelectedEntity.GetLogicalParent();

            if (logicalParent) {
                s_Actor = logicalParent.QueryInterface<ZActor>();
            }

            if (s_Actor) {
                m_SelectedActor = s_Actor;
                m_ScrollToActor = true;
                m_GlobalOutfitKit = {};
            }
        }
    }

    if (!IsSpecialEntityTreeNode(p_Entity)) {
        m_Server.OnEntitySelected(p_Entity, std::move(p_ClientId));
    }

    if (!m_SelectionForFreeCameraEditorStyleEntity) {
        m_SelectionForFreeCameraEditorStyleEntity = reinterpret_cast<ZSelectionForFreeCameraEditorStyleEntity*>(calloc(
            1,
            sizeof(ZSelectionForFreeCameraEditorStyleEntity)
        ));

        TEntityRef<ZSelectionForFreeCameraEditorStyleEntity> s_EntityRef;
        s_EntityRef.m_pInterfaceRef = m_SelectionForFreeCameraEditorStyleEntity;

        Globals::Selections->push_back(s_EntityRef);
    }

    if (m_SelectionForFreeCameraEditorStyleEntity && !IsSpecialEntityTreeNode(p_Entity)) {
        m_SelectionForFreeCameraEditorStyleEntity->m_selection.clear();

        if (p_Entity) {
            m_SelectionForFreeCameraEditorStyleEntity->m_selection.push_back(p_Entity);
        }
    }
}

void Editor::OnDestroyEntity(ZEntityRef p_Entity, std::optional<std::string> p_ClientId) {
    QueueTask([this, p_Entity, p_ClientId] {
        DestroyEntityInternal(p_Entity, p_ClientId, ChangeMode::Commit);
    });
}

void Editor::DestroyEntityInternal(ZEntityRef p_Entity, std::optional<std::string> p_ClientId, const ChangeMode p_Mode) {
    assert(std::this_thread::get_id() == m_UpdateThreadId);

    // The game can only delete the entities it created as a whole, so the editor only deletes
    // spawned entities. A second delete of the same entity can also still be in the queue.
    const auto s_It = m_SpawnedEntities.find(p_Entity);

    if (s_It == m_SpawnedEntities.end()) {
        return;
    }

    if (p_Mode == ChangeMode::Commit) {
        PushHistory({ .Change = DestroyHistoryEntry { MakeSpawnInfo(s_It->second) }, .ClientId = p_ClientId });
    }

    m_Server.OnEntityDestroying(p_Entity->GetType()->m_nEntityID, std::move(p_ClientId));

    // Spawned entities wait until their tree can be deleted. We take them out of the spawned
    // entities immediately, so a spawn with the same entity ID doesn't find this one.
    // Constructed in place, because assigning a TResourcePtr doesn't count the reference.
    std::optional<SpawnedEntity> s_SpawnedEntity;

    {
        std::scoped_lock s_Lock(m_CachedEntityTreeMutex);

        m_EntityNames.erase(p_Entity);
        s_SpawnedEntity.emplace(std::move(m_SpawnedEntities.extract(s_It).mapped()));

        RemoveEntityFromTree(p_Entity);
    }

    std::scoped_lock s_Lock(m_EntityDestructionMutex);
    m_EntitiesToDestroy.push_back(std::move(*s_SpawnedEntity));
}

// Needs m_CachedEntityTreeMutex locked exclusively.
void Editor::RemoveEntityFromTree(ZEntityRef p_Entity) {
    if (m_SelectedEntity == p_Entity) {
        m_SelectedEntity = {};
    }

    if (m_SelectedDebugEntity == p_Entity) {
        m_SelectedDebugEntity = {};
    }

    // Remove from the tree.
    if (const auto s_NodeToRemove = RemoveEntityTreeNode(p_Entity)) {

        // If a child of this node is selected, deselect it (non-recursive).
        std::queue<std::shared_ptr<EntityTreeNode>> s_ChildrenQueue;

        for (auto& s_Child : s_NodeToRemove->Children) {
            s_ChildrenQueue.push(s_Child.second);
        }

        m_OpenEntityTreeNodes.erase(s_NodeToRemove.get());
        m_FilteredEntityTreeNodes.erase(s_NodeToRemove.get());

        while (!s_ChildrenQueue.empty()) {
            auto s_CurrentChild = s_ChildrenQueue.front();

            // Queued changes check the map, so they skip the children too.
            RemoveEntityTreeNode(s_CurrentChild->Entity);
            m_OpenEntityTreeNodes.erase(s_CurrentChild.get());
            m_FilteredEntityTreeNodes.erase(s_CurrentChild.get());

            if (m_SelectedEntity == s_CurrentChild->Entity) {
                m_SelectedEntity = {};
            }

            if (m_SelectedDebugEntity == s_CurrentChild->Entity) {
                m_SelectedDebugEntity = {};
            }

            for (auto& s_Child : s_CurrentChild->Children) {
                s_ChildrenQueue.push(s_Child.second);
            }

            s_ChildrenQueue.pop();
        }

        s_NodeToRemove->RemoveFromParent();
        m_EntityTreeRowsDirty = true;
    }
}

void Editor::DestroyEntityNodeInternal(
    const std::shared_ptr<EntityTreeNode>& p_NodeToRemove,
    std::optional<std::string> p_ClientId
) {
    if (!p_NodeToRemove) {
        return;
    }

    const uint64_t s_EntityId = p_NodeToRemove->EntityId;

    std::scoped_lock s_Lock(m_CachedEntityTreeMutex);

    m_OpenEntityTreeNodes.erase(p_NodeToRemove.get());
    m_FilteredEntityTreeNodes.erase(p_NodeToRemove.get());

    if (p_NodeToRemove->Entity) {
        RemoveEntityTreeNode(p_NodeToRemove->Entity);
        m_EntityNames.erase(p_NodeToRemove->Entity);
        m_SpawnedEntities.erase(p_NodeToRemove->Entity);

        if (m_SelectedEntity == p_NodeToRemove->Entity) {
            m_SelectedEntity = {};
        }

        if (m_SelectedDebugEntity == p_NodeToRemove->Entity) {
            m_SelectedDebugEntity = {};
        }
    }

    // If a child of this node is selected, deselect it (non-recursive).
    std::queue<std::shared_ptr<EntityTreeNode>> s_ChildrenQueue;

    for (auto& s_Child : p_NodeToRemove->Children) {
        s_ChildrenQueue.push(s_Child.second);
    }

    while (!s_ChildrenQueue.empty()) {
        auto s_CurrentChild = s_ChildrenQueue.front();
        s_ChildrenQueue.pop();

        // Queued changes check the map, so they skip the children too.
        RemoveEntityTreeNode(s_CurrentChild->Entity);
        m_OpenEntityTreeNodes.erase(s_CurrentChild.get());
        m_FilteredEntityTreeNodes.erase(s_CurrentChild.get());

        if (m_SelectedEntity == s_CurrentChild->Entity) {
            m_SelectedEntity = {};
        }

        if (m_SelectedDebugEntity == s_CurrentChild->Entity) {
            m_SelectedDebugEntity = {};
        }

        for (auto& s_Child : s_CurrentChild->Children) {
            s_ChildrenQueue.push(s_Child.second);
        }
    }

    p_NodeToRemove->RemoveFromParent();
    m_EntityTreeRowsDirty = true;

    m_Server.OnEntityDestroying(s_EntityId, std::move(p_ClientId));
}

DEFINE_PLUGIN_DETOUR(
    Editor,
    ZEntityRef*,
    ZEntityManager_NewUninitializedEntity,
    ZEntityManager* th,
    ZEntityRef& result,
    const ZString& sDebugName,
    IEntityFactory* pEntityFactory,
    const ZEntityRef& logicalParent,
    uint64_t entityID,
    const SExternalReferences& externalRefs,
    bool unk0
) {
    ZEntityRef* s_EntityRef = p_Hook->CallOriginal(
        th,
        result,
        sDebugName,
        pEntityFactory,
        logicalParent,
        entityID,
        externalRefs,
        unk0
    );

    {
        std::scoped_lock lock(m_DynamicEntitiesMutex);
        m_DynamicEntities.insert(result);
    }

    if (m_CachedEntityTree) {
        std::scoped_lock lock(m_PendingDynamicEntitiesMutex);
        m_PendingDynamicEntities.insert(result);
    }

    return { HookAction::Return(), s_EntityRef };
}

DEFINE_PLUGIN_DETOUR(
    Editor,
    void,
    ZEntityManager_DeleteEntity,
    ZEntityManager* th,
    const ZEntityRef& entityRef,
    const SExternalReferences& externalRefs
) {
    std::shared_ptr<EntityTreeNode> s_NodeToRemove;

    {
        std::scoped_lock s_Lock(m_CachedEntityTreeMutex);

        if (m_SelectedEntity == entityRef) {
            m_SelectedEntity = nullptr;
        }

        if (m_SelectedDebugEntity == entityRef) {
            m_SelectedDebugEntity = nullptr;
        }

        if (m_CachedEntityTree) {
            auto it = m_CachedEntityTreeMap.find(entityRef);

            if (it != m_CachedEntityTreeMap.end()) {
                s_NodeToRemove = it->second;
            }
        }
    }

    if (s_NodeToRemove) {
        s_NodeToRemove->IsPendingDeletion.store(true, std::memory_order_release);

        DeleteDebugEntities(s_NodeToRemove);
        DestroyEntityNodeInternal(s_NodeToRemove, std::nullopt);
    }

    {
        std::scoped_lock lock(m_DynamicEntitiesMutex);
        m_DynamicEntities.erase(entityRef);
    }

    {
        std::scoped_lock lock(m_PendingDynamicEntitiesMutex);
        m_PendingDynamicEntities.erase(entityRef);
    }

    {
        std::unique_lock s_Lock(m_EntityRefToFactoryRuntimeResourceIDsMutex);
        m_EntityRefToFactoryRuntimeResourceIDs.erase(entityRef);
    }

    return { HookAction::Continue() };
}