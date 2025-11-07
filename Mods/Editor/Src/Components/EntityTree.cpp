#include <Editor.h>

#include "imgui_internal.h"

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

    const auto s_SceneEntity = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene.m_ref;
    const std::shared_ptr<EntityTreeNode> s_SceneNode = p_NodeMap[s_SceneEntity];
    const std::shared_ptr<EntityTreeNode> s_UnparentedEntitiesNode = s_SceneNode->Children.find("Unparented Entities")->second;
    std::shared_ptr<EntityTreeNode> s_DynamicEntitiesNode;

    if (p_AreEntitiesDynamic) {
        s_DynamicEntitiesNode = s_SceneNode->Children.find("Dynamic Entities")->second;
    }

    while (!s_NodeQueue.empty()) {
        // Pop the next factory and its root entity off the queue.
        auto [s_CurrentFactory, s_CurrentRoot] = s_NodeQueue.front();
        s_NodeQueue.pop();

        const auto s_SubEntityCount = s_CurrentFactory->GetSubEntitiesCount();
        const bool s_IsTemplateEntityBlueprintFactory = s_CurrentFactory->IsTemplateEntityBlueprintFactory();
        const bool s_IsAspectEntityBlueprintFactory = s_CurrentFactory->IsAspectEntityBlueprintFactory();

        // Go through each of its sub-entities and create nodes for them.
        for (int i = 0; i < s_SubEntityCount; ++i) {
            const ZEntityRef s_SubEntity = s_CurrentFactory->GetSubEntity(s_CurrentRoot.m_pEntity, i);
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
                s_NodeQueue.emplace(s_SubEntityFactory, s_SubEntity);

                continue;
            }

            const auto s_SubEntityId = s_SubEntity->GetType()->m_nEntityId;
            std::string s_EntityName = "<noname>";

            // If our current factory is a template factory, we can get the name of the entity from it.
            if (s_IsTemplateEntityBlueprintFactory) {
                const auto s_TemplateBpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_CurrentFactory);

                if (s_TemplateBpFactory->m_pTemplateEntityBlueprint) {
                    s_EntityName = s_TemplateBpFactory->m_pTemplateEntityBlueprint->subEntities[i].entityName;
                }
            }
            else if (s_IsAspectEntityBlueprintFactory) {
                const auto s_AspectEntityBlueprintFactory = reinterpret_cast<ZAspectEntityBlueprintFactory*>(s_CurrentFactory);
                const uint32_t s_AspectIndex = s_AspectEntityBlueprintFactory->m_aSubEntitiesLookUp[i].m_nAspectIdx;
                const uint32_t s_SubEntityIndex = s_AspectEntityBlueprintFactory->m_aSubEntitiesLookUp[i].m_nSubentityIdx;
                const auto s_TemplateBpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(
                    s_AspectEntityBlueprintFactory->m_aBlueprintFactories[s_AspectIndex]
                );

                if (s_TemplateBpFactory->m_pTemplateEntityBlueprint) {
                    s_EntityName = s_TemplateBpFactory->m_pTemplateEntityBlueprint->subEntities[s_SubEntityIndex].entityName;
                }
            }

            if (const auto s_Name = m_EntityNames.find(s_SubEntity); s_Name != m_EntityNames.end()) {
                s_EntityName = s_Name->second;
            }

            const uint64_t s_BaseKey = s_SubEntityId & 0xFFFFFFFFFFFC000F;
            const bool s_IsEntityIDGenerated = p_AreEntitiesDynamic && Globals::EntityManager->m_DynamicEntityIdToCount.contains(s_BaseKey);

            // Format a human-readable name for the entity.
            const auto s_EntityTypeName = (*s_SubEntity->GetType()->m_pInterfaces)[0].m_pTypeId->typeInfo()->
                m_pTypeName;
            const auto s_EntityHumanName = fmt::format(
                "{} ({:016x}){}",
                s_EntityName,
                s_SubEntityId,
                p_AreEntitiesDynamic ? (s_IsEntityIDGenerated ? " **" : " *") : ""
            );

            std::string s_ReferencedFactoryType;

            if (s_SubEntityFactory->IsTemplateEntityBlueprintFactory()) {
                s_ReferencedFactoryType = "TBLU";
            }

            if (s_SubEntityFactory->IsAspectEntityBlueprintFactory()) {
                s_ReferencedFactoryType = "ASEB";
            }

            if (s_SubEntityFactory->IsCppEntityBlueprintFactory()) {
                s_ReferencedFactoryType = "CBLU";
            }

            if (s_SubEntityFactory->IsExtendedCppEntityBlueprintFactory()) {
                s_ReferencedFactoryType = "ECPB";
            }

            if (s_SubEntityFactory->IsUIControlBlueprintFactory()) {
                s_ReferencedFactoryType = "UICB";
            }

            if (s_SubEntityFactory->IsRenderMaterialEntityBlueprintFactory()) {
                s_ReferencedFactoryType = "MATB";
            }

            if (s_SubEntityFactory->IsBehaviorTreeEntityBlueprintFactory()) {
                s_ReferencedFactoryType = "AIBB";
            }

            if (s_SubEntityFactory->IsAudioSwitchBlueprintFactory()) {
                s_ReferencedFactoryType = "WSWB";
            }

            if (s_SubEntityFactory->IsAudioStateBlueprintFactory()) {
                s_ReferencedFactoryType = "WSGB";
            }

            // Add the node to the map.
            const auto s_SubEntityNode = std::make_shared<EntityTreeNode>(
                s_EntityHumanName,
                s_EntityTypeName,
                s_SubEntityId,
                s_CurrentFactory->m_ridResource,
                s_SubEntityFactory->m_ridResource,
                s_ReferencedFactoryType,
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
                    }
                    else {
                        s_ParentNode->second->Children.insert({ s_EntityHumanName, s_SubEntityNode });
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

    // Go through the nodes and assign any remaining children to their parents.
    while (!s_ParentlessNodes.empty()) {
        const auto s_Node = s_ParentlessNodes.front();
        s_ParentlessNodes.pop();
        
        // Skip entities from second and later factories referenced by aspect factories
        if (s_Node->Entity &&
            *s_Node->Entity.m_pEntity &&
            reinterpret_cast<intptr_t>(*s_Node->Entity.m_pEntity) & 1
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
                }
                else {
                    s_ParentNode->second->Children.insert({ s_Node->Name, s_Node });
                }

                continue;
            }
        }

        if (p_AreEntitiesDynamic) {
            s_DynamicEntitiesNode->Children.insert({ s_Node->Name, s_Node });
        }
        else {
            // Otherwise, add it to the "Unparented Entities" node.
            s_UnparentedEntitiesNode->Children.insert({ s_Node->Name, s_Node });
        }
    }

    m_IsBuildingEntityTree = false;
}

void Editor::UpdateEntities() {
    const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

    if (!s_SceneCtx
        || !s_SceneCtx->m_pScene
        || !s_SceneCtx->m_pScene.m_ref
        || s_SceneCtx->m_aLoadedBricks.size() == 0) {
        return;
    }

    const auto s_SceneEnt = s_SceneCtx->m_pScene.m_ref;

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

    auto s_SceneFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_SceneEnt.GetBlueprintFactory());

    if (s_SceneEnt.GetOwningEntity()) {
        s_SceneFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_SceneEnt.GetOwningEntity().
            GetBlueprintFactory());
    }

    // Create the root scene node.
    auto s_SceneNode = std::make_shared<EntityTreeNode>(
        "Scene Root",
        (*s_SceneEnt->GetType()->m_pInterfaces)[0].m_pTypeId->typeInfo()->m_pTypeName,
        s_SceneEnt->GetType()->m_nEntityId,
        s_SceneFactory->m_ridResource,
        -1,
        "",
        s_SceneEnt
    );

    auto s_UnparentedEntitiesNode = std::make_shared<EntityTreeNode>(
        "Unparented Entities",
        "",
        -1,
        -1,
        -1,
        "",
        m_UnparentedEntitiesNodeEntityRef
    );

    s_SceneNode->Children.insert(std::make_pair(s_UnparentedEntitiesNode->Name, s_UnparentedEntitiesNode));

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
        "Dynamic Entities",
        "",
        -1,
        -1,
        -1,
        "",
        m_DynamicEntitiesNodeEntityRef
    );

    p_SceneNode->Children.insert(std::make_pair(s_DynamicEntitiesNode->Name, s_DynamicEntitiesNode));

    std::vector<ZEntityRef> s_DynamicEntities;

    {
        std::scoped_lock s_ScopedLock(m_DynamicEntitiesMutex);

        s_DynamicEntities.reserve(m_DynamicEntities.size());

        for (const auto& ref : m_DynamicEntities) {
            s_DynamicEntities.push_back(ref);
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
    const auto s_SceneEntity = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene.m_ref;
    const std::shared_ptr<EntityTreeNode> s_SceneNode = p_NodeMap[s_SceneEntity];
    const std::shared_ptr<EntityTreeNode> s_DynamicEntitiesNode = s_SceneNode->Children.find("Dynamic Entities")->second;

    std::vector<std::pair<std::shared_ptr<EntityTreeNode>, std::shared_ptr<EntityTreeNode>>> s_NodesToReparent;

    static STypeID* s_ClothCharacterEntityTypeID = (*Globals::TypeRegistry)->GetTypeID("ZClothCharacterEntity");
    static STypeID* s_LinkedProxyEntityTypeID = (*Globals::TypeRegistry)->GetTypeID("ZLinkedProxyEntity");

    for (const auto& [_, s_Node] : s_DynamicEntitiesNode->Children) {
        if (!s_Node->IsPendingDeletion &&
            s_Node->Entity && (
                s_Node->Entity.QueryInterface<ZClothCharacterEntity>(s_ClothCharacterEntityTypeID) ||
                s_Node->Entity && s_Node->Entity.QueryInterface<ZLinkedProxyEntity>(s_LinkedProxyEntityTypeID)
        )) {
            ZEntityRef s_ParentRef = s_Node->Entity.GetProperty<TEntityRef<ZSpatialEntity>>("m_eidParent").Get().m_ref;

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

        s_Node->Entity.SetLogicalParent(
            s_Node->Entity.GetProperty<TEntityRef<ZSpatialEntity>>("m_eidParent").Get().m_ref
        );
    }
}

void Editor::RenderEntity(std::shared_ptr<EntityTreeNode> p_Node) {
    if (!p_Node) return;

    if (
        (!m_EntityIdSearchInput.empty() ||
        !m_EntityTypeSearchInput.empty() ||
        !m_EntityNameSearchInput.empty() ||
        m_EntityViewMode != EntityViewMode::All) &&
        !m_FilteredEntityTreeNodes.contains(p_Node.get())
    ) {
        return;
    }

    ImGui::PushID(p_Node.get());

    const auto s_Entity = p_Node->Entity;
    const auto s_EntityType = p_Node->EntityType;
    const auto s_EntityName = p_Node->Name;
    const auto s_IsSelected = s_Entity == m_SelectedEntity;

    ImGuiTreeNodeFlags s_Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanAvailWidth;

    if (p_Node->Children.empty()) {
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
        else if (s_Entity == m_DynamicEntitiesNodeEntityRef ||
            s_Entity == m_UnparentedEntitiesNodeEntityRef
        ) {
            for (const auto& [_, s_Child] : p_Node->Children) {
                if (s_Child->Entity == m_SelectedEntity ||
                    (s_Child->Entity && m_SelectedEntity.IsAnyParent(s_Child->Entity)))
                {
                    s_ShouldExpandNode = true;
                    break;
                }
            }
        }

        if (s_ShouldExpandNode) {
            ImGui::SetNextItemOpen(true);
        }
    }

    if (p_Node->IsPendingDeletion) {
        ImGui::SetNextItemOpen(false);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::BeginDisabled();
    }

    auto s_Open = ImGui::TreeNodeEx(
        s_EntityName.c_str(),
        s_Flags
    );

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", s_EntityType.c_str());
    }

    if (!p_Node->IsPendingDeletion) {
        if (ImGui::IsItemFocused() && !s_IsSelected) {
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Space))
                OnSelectEntity(s_Entity, false, std::nullopt);
        }

        if (ImGui::IsItemClicked()) {
            OnSelectEntity(s_Entity, false, std::nullopt);
        }
    }

    if (s_Open) {
        for (const auto& s_Child : p_Node->Children) {
            RenderEntity(s_Child.second);
        }

        if (!(s_Flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
            ImGui::TreePop();
        }
    }

    if (p_Node->IsPendingDeletion) {
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
    }

    ImGui::PopID();
}

void Editor::FilterEntityTree() {
    m_FilteredEntityTreeNodes.clear();
    m_DirectEntityTreeNodeMatches.clear();

    if (!m_CachedEntityTree) {
        return;
    }

    if (m_EntityIdSearchInput.empty() &&
        m_EntityTypeSearchInput.empty() &&
        m_EntityNameSearchInput.empty() &&
        m_EntityViewMode == EntityViewMode::All &&
        m_LastEntityViewMode == EntityViewMode::All) {
        return;
    }

    FilterEntityTree(m_CachedEntityTree.get());

    if (m_FilteredEntityTreeNodes.empty()) {
        m_FilteredEntityTreeNodes.insert(m_CachedEntityTree.get());
    }

    if (m_DirectEntityTreeNodeMatches.size() == 1) {
        const EntityTreeNode* s_EntityTreeNode = *m_DirectEntityTreeNodeMatches.begin();

        OnSelectEntity(s_EntityTreeNode->Entity, true, std::nullopt);
    }

    m_LastEntityViewMode = m_EntityViewMode;
}

bool Editor::FilterEntityTree(EntityTreeNode* p_Node) {
    if (!p_Node) {
        return false;
    }

    if (m_EntityViewMode == EntityViewMode::ScenesAndBricks &&
        (p_Node->IsDynamicEntity || p_Node->Entity == m_DynamicEntitiesNodeEntityRef)
    ) {
        return false;
    }
    
    bool s_Matches = false;

    if (m_EntityViewMode == EntityViewMode::All ||
        m_EntityViewMode == EntityViewMode::ScenesAndBricks ||
        m_EntityViewMode == EntityViewMode::DynamicEntities && p_Node->IsDynamicEntity
    ) {
        bool s_MatchesId = true;
        bool s_MatchesType = true;
        bool s_MatchesName = true;

        if (p_Node->Entity != m_DynamicEntitiesNodeEntityRef &&
            p_Node->Entity != m_UnparentedEntitiesNodeEntityRef
        ) {
            if (!m_EntityIdSearchInput.empty()) {
                const uint64_t id = std::strtoull(m_EntityIdSearchInput.c_str(), nullptr, 16);

                s_MatchesId = p_Node->EntityId == id;
            }

            if (!m_EntityTypeSearchInput.empty()) {
                s_MatchesType = p_Node->Entity.HasInterface(m_EntityTypeSearchInput);
            }
        }

        if (!m_EntityNameSearchInput.empty()) {
            s_MatchesName = Util::StringUtils::FindSubstring(p_Node->Name.c_str(), m_EntityNameSearchInput);
        }

        s_Matches = s_MatchesId && s_MatchesType && s_MatchesName;
    }

    bool s_ChildMatches = false;

    for (auto& child : p_Node->Children) {
        if (FilterEntityTree(child.second.get())) {
            s_ChildMatches = true;
        }
    }

    if (s_Matches) {
        m_DirectEntityTreeNodeMatches.push_back(p_Node);
    }

    if (s_Matches || s_ChildMatches) {
        m_FilteredEntityTreeNodes.insert(p_Node);

        return true;
    }

    return false;
}

void Editor::DrawEntityTree() {
    ImGui::SetNextWindowPos({0, 110}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({615, ImGui::GetIO().DisplaySize.y - 110}, ImGuiCond_FirstUseEver);
    ImGui::Begin(ICON_MD_CATEGORY " Entities", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

    const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

    if (s_SceneCtx && s_SceneCtx->m_pScene && s_SceneCtx->m_aLoadedBricks.size() > 0) {
        static char s_EntitySearchInput[17] = {};
        if (ImGui::InputText(
            ICON_MD_SEARCH " Search by ID",
            s_EntitySearchInput,
            IM_ARRAYSIZE(s_EntitySearchInput),
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal |
            ImGuiInputTextFlags_CharsNoBlank
        )) {
            m_EntityIdSearchInput = s_EntitySearchInput;

            FilterEntityTree();
        }

        static char s_EntityTypeSearchInput[2048] = {};
        const bool s_IsInputTextEnterPressed = ImGui::InputText(
            ICON_MD_SEARCH " Search by type", s_EntityTypeSearchInput, sizeof(s_EntityTypeSearchInput),
            ImGuiInputTextFlags_EnterReturnsTrue
        );
        const bool s_IsInputTextActive = ImGui::IsItemActive();

        if (ImGui::IsItemActivated()) {
            ImGui::OpenPopup("##popup");
        }

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

        if (ImGui::BeginPopup(
            "##popup",
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_ChildWindow
        )) {
            ZTypeRegistry* typeRegistry = *Globals::TypeRegistry;
            std::vector<std::string> typeNames;

            typeNames.reserve(typeRegistry->m_types.size());

            for (auto& pair : typeRegistry->m_types) {
                if (!pair.second->typeInfo()->isClass())
                    continue;

                if (!Util::StringUtils::FindSubstring(pair.first.c_str(), s_EntityTypeSearchInput))
                    continue;

                typeNames.push_back(pair.first.c_str());
            }

            std::sort(typeNames.begin(), typeNames.end());

            for (auto& typeName : typeNames) {
                if (ImGui::Selectable(typeName.c_str())) {
                    ImGui::ClearActiveID();
                    strcpy_s(s_EntityTypeSearchInput, typeName.c_str());

                    m_EntityTypeSearchInput = s_EntityTypeSearchInput;

                    FilterEntityTree();
                }
            }

            if (s_IsInputTextEnterPressed || (!s_IsInputTextActive && !ImGui::IsWindowFocused())) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        static char s_EntityNameSearchInput[2048] = {};

        if (ImGui::InputText(
            ICON_MD_SEARCH " Search by name",
            s_EntityNameSearchInput,
            IM_ARRAYSIZE(s_EntityNameSearchInput),
            ImGuiInputTextFlags_EnterReturnsTrue
        )) {
            m_EntityNameSearchInput = s_EntityNameSearchInput;

            FilterEntityTree();
        }

        if (ImGui::BeginCombo("Entity View Mode", m_EntityViewModes[m_EntityViewMode].c_str())) {
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

        if (ImGui::Button(ICON_MD_CLEAR " Clear Filters")) {
            m_EntityIdSearchInput.clear();
            m_EntityTypeSearchInput.clear();
            m_EntityNameSearchInput.clear();

            memset(s_EntitySearchInput, 0, sizeof(s_EntitySearchInput));
            memset(s_EntityTypeSearchInput, 0, sizeof(s_EntityTypeSearchInput));
            memset(s_EntityNameSearchInput, 0, sizeof(s_EntityNameSearchInput));

            m_EntityViewMode = EntityViewMode::All;
            m_LastEntityViewMode = EntityViewMode::All;

            m_FilteredEntityTreeNodes.clear();
            m_DirectEntityTreeNodeMatches.clear();
        }

        if (ImGui::Button(ICON_MD_CONSTRUCTION " Rebuild entity tree")) {
            UpdateEntities();
        }

        if (!m_EntityIdSearchInput.empty() ||
            !m_EntityTypeSearchInput.empty() ||
            !m_EntityNameSearchInput.empty()) {

            if (!m_FilteredEntityTreeNodes.empty() &&
                m_FilteredEntityTreeNodes.size() == 1 &&
                *m_FilteredEntityTreeNodes.begin() == m_CachedEntityTree.get()) {
                ImGui::TextColored(ImVec4(1.f, 69.f / 255.f, 69.f / 255.f, 1.f), "No results found.");
                ImGui::End();

                return;
            }
        }

        m_CachedEntityTreeMutex.lock_shared();

        if (m_CachedEntityTree) {
            RenderEntity(m_CachedEntityTree);
        }
        else {
            ImGui::Text("No entities loaded. You may want to press the 'Rebuild entity tree' button.");
        }

        m_CachedEntityTreeMutex.unlock_shared();

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

void Editor::OnSelectEntity(ZEntityRef p_Entity, bool p_ShouldScrollToEntity, const std::optional<std::string> p_ClientId) {
    const bool s_DifferentEntity = m_SelectedEntity.m_pEntity != p_Entity.m_pEntity;

    m_ScrollToEntity = p_ShouldScrollToEntity && p_Entity.GetEntity() != nullptr;

    if (s_DifferentEntity) {
        if (p_Entity != m_DynamicEntitiesNodeEntityRef &&
            p_Entity != m_UnparentedEntitiesNodeEntityRef
        ) {
            m_Server.OnEntitySelected(p_Entity, std::move(p_ClientId));
        }

        m_SelectedEntity = p_Entity;
    }
    else {
        m_SelectedEntity = nullptr; //Unselect it
    }

    if (!m_SelectionForFreeCameraEditorStyleEntity) {
        m_SelectionForFreeCameraEditorStyleEntity = reinterpret_cast<ZSelectionForFreeCameraEditorStyleEntity*>(calloc(
            1, sizeof(ZSelectionForFreeCameraEditorStyleEntity)
        ));

        TEntityRef<ZSelectionForFreeCameraEditorStyleEntity> s_EntityRef;
        s_EntityRef.m_pInterfaceRef = m_SelectionForFreeCameraEditorStyleEntity;

        Globals::Selections->push_back(s_EntityRef);
    }

    if (m_SelectionForFreeCameraEditorStyleEntity) {
        if (p_Entity != m_DynamicEntitiesNodeEntityRef &&
            p_Entity != m_UnparentedEntitiesNodeEntityRef
        ) {
            m_SelectionForFreeCameraEditorStyleEntity->m_selection.clear();
            m_SelectionForFreeCameraEditorStyleEntity->m_selection.push_back(p_Entity);
        }
    }
}

void Editor::OnDestroyEntity(ZEntityRef p_Entity, std::optional<std::string> p_ClientId) {
    m_EntityDestructionMutex.lock();
    m_EntitiesToDestroy.push_back({p_Entity, std::move(p_ClientId)});
    m_EntityDestructionMutex.unlock();
}


void Editor::DestroyEntityInternal(ZEntityRef p_Entity, std::optional<std::string> p_ClientId) {
    m_CachedEntityTreeMutex.lock();

    m_EntityNames.erase(p_Entity);
    m_SpawnedEntities.erase(p_Entity->GetType()->m_nEntityId);

    if (m_SelectedEntity == p_Entity) {
        m_SelectedEntity = {};
    }

    // Remove from the tree.
    const auto s_EntityIter = m_CachedEntityTreeMap.find(p_Entity);

    if (s_EntityIter != m_CachedEntityTreeMap.end()) {
        const auto s_NodeToRemove = s_EntityIter->second;
        m_CachedEntityTreeMap.erase(s_EntityIter);

        // Walk the tree and remove any references to this entity non-recursively.
        if (m_CachedEntityTree) {
            // Use a queue to traverse the tree non-recursively (BFS).
            std::queue<std::shared_ptr<EntityTreeNode>> s_NodeQueue;
            s_NodeQueue.push(m_CachedEntityTree);

            while (!s_NodeQueue.empty()) {
                auto s_CurrentNode = s_NodeQueue.front();
                s_NodeQueue.pop();

                // Check if any of this node's children is the node we want to remove.
                for (auto it = s_CurrentNode->Children.begin(); it != s_CurrentNode->Children.end();) {
                    if (it->second == s_NodeToRemove) {
                        // Remove this child from the parent's children.
                        it = s_CurrentNode->Children.erase(it);
                    }
                    else {
                        // Add this child to the queue for further processing.
                        s_NodeQueue.push(it->second);
                        ++it;
                    }
                }
            }
        }
    }

    m_CachedEntityTreeMutex.unlock();

    m_Server.OnEntityDestroying(p_Entity->GetType()->m_nEntityId, std::move(p_ClientId));
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

    std::scoped_lock lock(m_CachedEntityTreeMutex);

    if (p_NodeToRemove->Entity) {
        m_EntityNames.erase(p_NodeToRemove->Entity);
        if (const auto p_Type = p_NodeToRemove->Entity->GetType()) {
            m_SpawnedEntities.erase(p_Type->m_nEntityId);
        }

        if (m_SelectedEntity == p_NodeToRemove->Entity) {
            m_SelectedEntity = {};
        }
    }

    for (auto it = m_CachedEntityTreeMap.begin(); it != m_CachedEntityTreeMap.end();) {
        if (it->second == p_NodeToRemove) {
            it = m_CachedEntityTreeMap.erase(it);
        }
        else {
            ++it;
        }
    }

    if (m_CachedEntityTree) {
        std::queue<std::shared_ptr<EntityTreeNode>> s_NodeQueue;
        s_NodeQueue.push(m_CachedEntityTree);

        while (!s_NodeQueue.empty()) {
            auto s_CurrentNode = s_NodeQueue.front();
            s_NodeQueue.pop();

            for (auto it = s_CurrentNode->Children.begin(); it != s_CurrentNode->Children.end();) {
                if (it->second == p_NodeToRemove) {
                    it = s_CurrentNode->Children.erase(it);
                }
                else {
                    s_NodeQueue.push(it->second);
                    ++it;
                }
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

DEFINE_PLUGIN_DETOUR(Editor, ZEntityRef*, ZEntityManager_NewUninitializedEntity,
    ZEntityManager* th,
    ZEntityRef& result,
    const ZString& sDebugName,
    IEntityFactory* pEntityFactory,
    const ZEntityRef& logicalParent,
    uint64_t entityID,
    const SExternalReferences& externalRefs,
    bool unk0
) {
    ZEntityRef* s_EntityRef = p_Hook->CallOriginal(th, result, sDebugName, pEntityFactory, logicalParent, entityID, externalRefs, unk0);

    {
        std::scoped_lock lock(m_DynamicEntitiesMutex);

        m_DynamicEntities.insert(result);
    }

    if (m_CachedEntityTree && !m_IsBuildingEntityTree.load()) {
        std::scoped_lock lock(m_PendingDynamicEntitiesMutex);

        m_PendingDynamicEntities.push_back(result);
    }

    return HookResult<ZEntityRef*>(HookAction::Return(), s_EntityRef);
}

DEFINE_PLUGIN_DETOUR(Editor, void, ZEntityManager_DeleteEntity, ZEntityManager* th, const ZEntityRef& entityRef, const SExternalReferences& externalRefs) {
    if (m_SelectedEntity == entityRef) {
        m_SelectedEntity = nullptr;
    }
    
    if (m_CachedEntityTree && !m_IsBuildingEntityTree.load()) {
        auto it = m_CachedEntityTreeMap.find(entityRef);

        if (it != m_CachedEntityTreeMap.end()) {
            it->second->IsPendingDeletion = true;

            //std::scoped_lock s_ScopedLock(m_PendingNodeDeletionsMutex);

            //m_PendingNodeDeletions.push_back(it->second);
        }
    }

    {
        std::scoped_lock lock(m_DynamicEntitiesMutex);

        m_DynamicEntities.erase(entityRef);
    }

    p_Hook->CallOriginal(th, entityRef, externalRefs);

    return HookResult<void>(HookAction::Return());
}