#include <Editor.h>

#include "imgui_internal.h"
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

void Editor::UpdateEntityTree(
    std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>>& p_NodeMap,
    const std::vector<ZEntityRef>& p_Entities,
    const bool p_AreEntitiesDynamic
) {
    if (m_IsBuildingEntityTree.exchange(true)) {
        return;
    }

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

                p_NodeMap[s_SubEntity] = s_SubEntityNode;
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

            p_NodeMap[s_CurrentRoot] = s_SubEntityNode;
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

    m_IsBuildingEntityTree = false;
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
    for (const auto& s_Entity : m_SpawnedEntities | std::views::values) {
        s_EntsToProcess.push_back(s_Entity);
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

    std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>> s_NodeMap;
    s_NodeMap.emplace(s_SceneEnt, s_SceneNode);
    UpdateEntityTree(s_NodeMap, s_EntsToProcess, false);

    AddDynamicEntitiesToEntityTree(s_SceneNode, s_NodeMap);

    if (m_ReparentDynamicOutfitEntities) {
        ReparentDynamicOutfitEntities(s_NodeMap);
    }

    // Update the cached tree.
    m_CachedEntityTreeMutex.lock();
    m_CachedEntityTree = std::move(s_SceneNode);
    m_CachedEntityTreeMap = std::move(s_NodeMap);
    m_CachedEntityTreeMutex.unlock();

    m_Server.OnEntityTreeRebuilt();
}

void Editor::AddDynamicEntitiesToEntityTree(
    const std::shared_ptr<EntityTreeNode>& p_SceneNode,
    std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>>& p_NodeMap
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
        UpdateEntityTree(p_NodeMap, s_DynamicEntities, true);
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
    return p_Entity == m_DynamicEntitiesNodeEntityRef || p_Entity == m_UnparentedEntitiesNodeEntityRef;
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

void Editor::RenderEntity(std::shared_ptr<EntityTreeNode> p_Node) {
    if (!p_Node) {
        return;
    }

    const bool s_IsPendingDeletion = p_Node->IsPendingDeletion.load(std::memory_order_acquire);

    if (m_HasActiveFilters && !m_FilteredEntityTreeNodes.contains(p_Node.get())) {
        return;
    }

    ImGui::PushID(p_Node.get());

    const auto s_Entity = p_Node->Entity;
    const auto s_EntityType = p_Node->EntityType;
    const auto s_EntityName = p_Node->Name;
    const auto s_IsSelected = s_Entity == m_SelectedEntity;

    ImGuiTreeNodeFlags s_Flags = ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_DrawLinesToNodes;

    const bool s_HasVisibleChildren = HasVisibleChildren(p_Node);

    if (!s_HasVisibleChildren) {
        s_Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (s_IsSelected) {
        s_Flags |= ImGuiTreeNodeFlags_Selected;

        if (m_ScrollToEntity) {
            ImGui::SetScrollHereY();
            m_ScrollToEntity = false;
        }
    }
    else if (m_ScrollToEntity && m_SelectedEntity) {
        bool s_ShouldExpandNode = false;

        if (s_Entity && m_SelectedEntity.IsAnyParent(s_Entity)) {
            s_ShouldExpandNode = true;
        }
        else if (IsSpecialEntityTreeNode(s_Entity)) {
            for (const auto& [_, s_Child] : p_Node->Children) {
                if (s_Child->Entity == m_SelectedEntity ||
                    (s_Child->Entity && m_SelectedEntity.IsAnyParent(s_Child->Entity))) {
                    s_ShouldExpandNode = true;
                    break;
                }
            }
        }

        if (s_ShouldExpandNode) {
            m_OpenEntityTreeNodes.insert(p_Node.get());
            ImGui::SetNextItemOpen(true);
        }
    }

    if (s_IsPendingDeletion) {
        m_OpenEntityTreeNodes.erase(p_Node.get());

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::BeginDisabled();
    }

    const bool s_IsDirectMatch = m_HasActiveFilters && p_Node->IsDirectMatch && !s_IsPendingDeletion;

    if (s_IsDirectMatch) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(0xFF8AF0FE)); // Tailwind yellow-200
    }

    ImGui::SetNextItemOpen(
        m_OpenEntityTreeNodes.contains(p_Node.get()),
        ImGuiCond_Always
    );

    auto s_Open = ImGui::TreeNodeEx(
        s_EntityName.c_str(),
        s_Flags
    );

    if (ImGui::IsItemToggledOpen()) {
        if (s_Open) {
            m_OpenEntityTreeNodes.insert(p_Node.get());
        }
        else {
            m_OpenEntityTreeNodes.erase(p_Node.get());
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
    }

    if (s_Open) {
        if (s_HasVisibleChildren) {
            for (const auto& s_Child : p_Node->Children) {
                RenderEntity(s_Child.second);
            }
        }

        if (!(s_Flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
            ImGui::TreePop();
        }
    }

    if (s_IsPendingDeletion) {
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
    }

    ImGui::PopID();
}

void Editor::FilterEntityTree() {
    m_FilteredEntityTreeNodes.clear();
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

void Editor::ClearFilters() {
    std::scoped_lock s_Lock(m_CachedEntityTreeMutex);

    m_EntityIDSearchInput.clear();
    m_EntityTypeSearchInput.clear();
    m_EntityNameSearchInput.clear();

    m_EntityViewMode = EntityViewMode::All;
    m_LastEntityViewMode = EntityViewMode::All;

    m_FilteredEntityTreeNodes.clear();
    m_TotalMatchCount = 0;
    m_CurrentEntitySearchResultIndex = 0;
    m_HasActiveFilters = false;
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

void Editor::DrawEntityTreeWindow() {
    ImGui::SetNextWindowPos({ 0, 110 }, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({ 615, ImGui::GetIO().DisplaySize.y - 110 }, ImGuiCond_FirstUseEver);
    ImGui::Begin(ICON_MD_CATEGORY " Entities", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

    const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

    if (s_SceneCtx && s_SceneCtx->m_pScene && s_SceneCtx->m_aLoadedBricks.size() > 0) {
        const ImGuiStyle& s_Style = ImGui::GetStyle();

        const float s_PreviousButtonWidth =
            ImGui::CalcTextSize(ICON_MD_ARROW_BACK " Previous").x + s_Style.FramePadding.x * 2;

        const std::string s_ResultCount = fmt::format(
            "{} / {}",
            m_TotalMatchCount == 0 ? 0 : m_CurrentEntitySearchResultIndex + 1,
            m_TotalMatchCount
        );
        const float s_ResultCountWidth = ImGui::CalcTextSize(s_ResultCount.c_str()).x;

        const float s_NextButtonWidth =
            ImGui::CalcTextSize("Next " ICON_MD_ARROW_FORWARD).x + s_Style.FramePadding.x * 2;

        const float s_ClearFiltersButtonWidth =
            ImGui::CalcTextSize(ICON_MD_CLEAR " Clear filters").x + s_Style.FramePadding.x * 2;

        const float s_FilterControlsWidth =
            s_PreviousButtonWidth +
            s_ResultCountWidth +
            s_NextButtonWidth +
            s_ClearFiltersButtonWidth +
            s_Style.ItemSpacing.x * 3;

        ImGui::SetNextItemWidth(s_FilterControlsWidth);

        if (ImGui::InputText(
            ICON_MD_SEARCH " Search by ID",
            &m_EntityIDSearchInput,
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal |
            ImGuiInputTextFlags_CharsNoBlank
        )) {
            FilterEntityTree();
        }

        ImGui::SetNextItemWidth(s_FilterControlsWidth);

        if (ImGui::InputText(
            ICON_MD_SEARCH " Search by name",
            &m_EntityNameSearchInput,
            ImGuiInputTextFlags_EnterReturnsTrue
        )) {
            FilterEntityTree();
        }

        ImGui::SetNextItemWidth(s_FilterControlsWidth);

        Util::ImGuiUtils::InputWithAutocomplete(
            ICON_MD_SEARCH " Search by type##EntityTypesPopup",
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

        ImGui::SetNextItemWidth(s_FilterControlsWidth);

        if (ImGui::BeginCombo("Entity view mode", m_EntityViewModes[m_EntityViewMode].c_str())) {
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

        const bool s_HasSearchResults = m_TotalMatchCount > 0;

        // Clamp index in case matches shrank while searching
        if (s_HasSearchResults && m_CurrentEntitySearchResultIndex >= m_TotalMatchCount) {
            m_CurrentEntitySearchResultIndex = 0;
        }

        ImGui::BeginDisabled(!s_HasSearchResults);

        if (ImGui::Button(ICON_MD_ARROW_BACK " Previous")) {
            if (s_HasSearchResults) {
                if (m_CurrentEntitySearchResultIndex == 0) {
                    m_CurrentEntitySearchResultIndex = m_TotalMatchCount - 1;
                }
                else {
                    --m_CurrentEntitySearchResultIndex;
                }

                size_t s_Counter = 0;
                if (auto s_Node = FindMatchByIndex(m_CachedEntityTree, m_CurrentEntitySearchResultIndex, s_Counter)) {
                    OnSelectEntity(s_Node->Entity, true, std::nullopt);
                }
            }
        }

        ImGui::SameLine();

        ImGui::Text(
            "%zu / %zu",
            s_HasSearchResults ? m_CurrentEntitySearchResultIndex + 1 : 0,
            m_TotalMatchCount
        );

        ImGui::SameLine();

        if (ImGui::Button("Next " ICON_MD_ARROW_FORWARD)) {
            if (s_HasSearchResults) {
                m_CurrentEntitySearchResultIndex = (m_CurrentEntitySearchResultIndex + 1) % m_TotalMatchCount;

                size_t s_Counter = 0;
                if (auto s_Node = FindMatchByIndex(m_CachedEntityTree, m_CurrentEntitySearchResultIndex, s_Counter)) {
                    OnSelectEntity(s_Node->Entity, true, std::nullopt);
                }
            }
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button(ICON_MD_CLEAR " Clear filters")) {
            ClearFilters();
        }

        ImGui::Spacing();

        bool s_HasEntityTree;

        {
            std::shared_lock s_Lock(m_CachedEntityTreeMutex);
            s_HasEntityTree = m_CachedEntityTree != nullptr;
        }

        if (ImGui::Button(
            s_HasEntityTree
            ? ICON_MD_CONSTRUCTION " Rebuild entity tree"
            : ICON_MD_CONSTRUCTION " Build entity tree"
        )) {
            UpdateEntities();
        }

        ImGui::SameLine();

        {
            std::shared_lock s_Lock(m_CachedEntityTreeMutex);

            ImGui::BeginDisabled(!m_CachedEntityTree);

            if (ImGui::Button(ICON_MD_UNFOLD_LESS " Collapse tree")) {
                m_OpenEntityTreeNodes.clear();
            }

            ImGui::SameLine();

            if (ImGui::Button(ICON_MD_SAVE_ALT " Export entity tree")) {
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

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Outputs the current entity tree as a text file (entity_tree.txt - found in the Retail folder of the game)."
                );
            }

            ImGui::EndDisabled();

            if (ImGui::BeginChild(
                "EntityTree",
                ImVec2(0, 0),
                ImGuiChildFlags_None,
                ImGuiWindowFlags_HorizontalScrollbar
            )) {
                const bool s_HasNoResults = m_HasActiveSearch && m_TotalMatchCount == 0;

                if (s_HasNoResults) {
                    ImGui::TextColored(ImVec4(1.0f, 0.27f, 0.27f, 1.0f), "No results found.");
                }
                else if (m_CachedEntityTree) {
                    RenderEntity(m_CachedEntityTree);
                }
                else {
                    ImGui::Text("No entities loaded. Build the entity tree to load them.");
                }
            }

            ImGui::EndChild();
        }

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

void Editor::OnSelectEntity(
    ZEntityRef p_Entity,
    bool p_ShouldScrollToEntity,
    const std::optional<std::string> p_ClientId
) {
    if (m_SelectedEntity.m_pObj == p_Entity.m_pObj) {
        return;
    }

    m_ScrollToEntity = p_ShouldScrollToEntity && p_Entity.GetEntity() != nullptr;

    if (!IsSpecialEntityTreeNode(p_Entity)) {
        m_Server.OnEntitySelected(p_Entity, std::move(p_ClientId));
    }

    m_SelectedEntity = p_Entity;

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
    m_EntityDestructionMutex.lock();
    m_EntitiesToDestroy.push_back({ p_Entity, std::move(p_ClientId) });
    m_EntityDestructionMutex.unlock();
}

void Editor::DestroyEntityInternal(ZEntityRef p_Entity, std::optional<std::string> p_ClientId) {
    m_CachedEntityTreeMutex.lock();

    m_EntityNames.erase(p_Entity);
    m_SpawnedEntities.erase(p_Entity->GetType()->m_nEntityID);

    if (m_SelectedEntity == p_Entity) {
        m_SelectedEntity = {};
    }

    if (m_SelectedGizmoEntity == p_Entity) {
        m_SelectedGizmoEntity = {};
    }

    // Remove from the tree.
    const auto s_EntityIter = m_CachedEntityTreeMap.find(p_Entity);

    if (s_EntityIter != m_CachedEntityTreeMap.end()) {
        const auto s_NodeToRemove = s_EntityIter->second;
        m_CachedEntityTreeMap.erase(s_EntityIter);

        // If a child of this node is selected, deselect it (non-recursive).
        std::queue<std::shared_ptr<EntityTreeNode>> s_ChildrenQueue;

        for (auto& s_Child : s_NodeToRemove->Children) {
            s_ChildrenQueue.push(s_Child.second);
        }

        m_OpenEntityTreeNodes.erase(s_NodeToRemove.get());
        m_FilteredEntityTreeNodes.erase(s_NodeToRemove.get());

        while (!s_ChildrenQueue.empty()) {
            auto s_CurrentChild = s_ChildrenQueue.front();

            m_OpenEntityTreeNodes.erase(s_CurrentChild.get());
            m_FilteredEntityTreeNodes.erase(s_CurrentChild.get());

            if (m_SelectedEntity == s_CurrentChild->Entity) {
                m_SelectedEntity = {};
            }

            if (m_SelectedGizmoEntity == s_CurrentChild->Entity) {
                m_SelectedGizmoEntity = {};
            }

            for (auto& s_Child : s_ChildrenQueue.front()->Children) {
                s_ChildrenQueue.push(s_Child.second);
            }

            s_ChildrenQueue.pop();
        }

        // Remove it from the children of it's parent.
        if (auto s_Parent = s_NodeToRemove->Parent.lock()) {
            for (auto it = s_Parent->Children.begin(); it != s_Parent->Children.end(); ++it) {
                if (it->second == s_NodeToRemove) {
                    s_Parent->Children.erase(it);
                    break;
                }
            }
        }
    }

    m_CachedEntityTreeMutex.unlock();

    m_Server.OnEntityDestroying(p_Entity->GetType()->m_nEntityID, std::move(p_ClientId));
    Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, p_Entity, {});
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
        m_CachedEntityTreeMap.erase(p_NodeToRemove->Entity);
        m_EntityNames.erase(p_NodeToRemove->Entity);

        if (const auto p_Type = p_NodeToRemove->Entity->GetType()) {
            m_SpawnedEntities.erase(p_Type->m_nEntityID);
        }

        if (m_SelectedEntity == p_NodeToRemove->Entity) {
            m_SelectedEntity = {};
        }

        if (m_SelectedGizmoEntity == p_NodeToRemove->Entity) {
            m_SelectedGizmoEntity = {};
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

        m_OpenEntityTreeNodes.erase(s_CurrentChild.get());
        m_FilteredEntityTreeNodes.erase(s_CurrentChild.get());

        if (m_SelectedEntity == s_CurrentChild->Entity) {
            m_SelectedEntity = {};
        }

        if (m_SelectedGizmoEntity == s_CurrentChild->Entity) {
            m_SelectedGizmoEntity = {};
        }

        for (auto& s_Child : s_CurrentChild->Children) {
            s_ChildrenQueue.push(s_Child.second);
        }
    }

    if (auto s_Parent = p_NodeToRemove->Parent.lock()) {
        for (auto it = s_Parent->Children.begin(); it != s_Parent->Children.end(); ++it) {
            if (it->second == p_NodeToRemove) {
                s_Parent->Children.erase(it);
                break;
            }
        }
    }

    m_Server.OnEntityDestroying(s_EntityId, std::move(p_ClientId));
}

void Editor::OnEntityNameChange(ZEntityRef p_Entity, const std::string& p_Name, std::optional<std::string> p_ClientId) {
    m_CachedEntityTreeMutex.lock();
    m_EntityNames[p_Entity] = p_Name;
    m_CachedEntityTreeMutex.unlock();

    m_Server.OnEntityNameChanged(p_Entity, std::move(p_ClientId));
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

    if (m_CachedEntityTree && !m_IsBuildingEntityTree.load()) {
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
    if (m_SelectedEntity == entityRef) {
        m_SelectedEntity = nullptr;
    }

    if (m_SelectedGizmoEntity == entityRef) {
        m_SelectedGizmoEntity = nullptr;
    }

    std::shared_ptr<EntityTreeNode> s_NodeToRemove;

    {
        std::shared_lock s_Lock(m_CachedEntityTreeMutex);

        if (m_CachedEntityTree && !m_IsBuildingEntityTree.load()) {
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

    m_EntityRefToFactoryRuntimeResourceIDs.erase(entityRef);

    return { HookAction::Continue() };
}