#include <Editor.h>

#include <Glacier/EntityFactory.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZEntity.h>

#include "IconsMaterialDesign.h"
#include "Logging.h"

#include <shared_mutex>
#include <queue>
#include <map>
#include <utility>

void Editor::UpdateEntities() {
	const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

	if (!s_SceneCtx
	    || !s_SceneCtx->m_pScene
	    || !s_SceneCtx->m_pScene.m_ref
	    || s_SceneCtx->m_aLoadedBricks.size() == 0) {
		return;
	}

	const auto s_SceneEnt = s_SceneCtx->m_pScene.m_ref;

	// Go through a first pass by creating all the nodes of the tree using a BFS
	// approach. We'll also opportunistically assign children nodes to parents we've
	// seen before. Then, as a second pass we'll go through and assign the remaining
	// children nodes to their parents.

	std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>> s_NodeMap;
	std::queue<std::pair<ZEntityBlueprintFactoryBase*, ZEntityRef>> s_NodeQueue;
	std::queue<std::shared_ptr<EntityTreeNode>> s_ParentlessNodes;

	// Add all the brick nodes to the queue.
	for (const auto& s_Brick: s_SceneCtx->m_aLoadedBricks) {
		auto s_BrickEnt = s_Brick.entityRef;

		if (!s_BrickEnt) {
			continue;
		}

		const auto s_BpFactory = s_BrickEnt.GetBlueprintFactory();

		if (!s_BpFactory) {
			continue;
		}

		s_NodeQueue.emplace(s_BpFactory, s_BrickEnt);
	}

	// Create the root scene node.
	auto s_SceneNode = std::make_shared<EntityTreeNode>("Scene Root", 0, 0, s_SceneEnt);
	s_NodeMap.emplace(s_SceneEnt, s_SceneNode);
	s_NodeMap.emplace(ZEntityRef(), s_SceneNode);

	while (!s_NodeQueue.empty()) {
		// Pop the next factory and its root entity off the queue.
		auto [s_CurrentFactory, s_CurrentRoot] = s_NodeQueue.front();
		s_NodeQueue.pop();

		const auto s_SubEntityCount = s_CurrentFactory->GetSubEntitiesCount();
		const bool s_IsTemplateFactory = s_CurrentFactory->IsTemplateEntityBlueprintFactory();

		// Go through each of its sub-entities and create nodes for them.
		for (int i = 0; i < s_SubEntityCount; ++i) {
			const ZEntityRef s_SubEntity = s_CurrentFactory->GetSubEntity(s_CurrentRoot.m_pEntity, i);
			const auto s_SubEntityFactory = s_CurrentFactory->GetSubEntityBlueprint(i);

			if (!s_SubEntity.GetEntity() || !s_SubEntity->GetType()) {
				continue;
			}

			// If this sub-entity has already been added to the tree, skip it.
			if (s_NodeMap.contains(s_SubEntity)) {
				continue;
			}

			const auto s_SubEntityId = s_SubEntity->GetType()->m_nEntityId;
			std::string s_EntityName = "<noname>";

			// If our current factory is a template factory, we can get the name of the entity from it.
			if (s_IsTemplateFactory) {
				const auto s_TemplateBpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_CurrentFactory);

				if (s_TemplateBpFactory->m_pTemplateEntityBlueprint) {
					s_EntityName = s_TemplateBpFactory->m_pTemplateEntityBlueprint->subEntities[i].entityName;
				}
			}

			// Format a human-readable name for the entity.
			const auto s_EntityTypeName = s_SubEntity->GetType()->m_pInterfaces->operator[](0).m_pTypeId->typeInfo()->m_pTypeName;
			const auto s_EntityHumanName = fmt::format(
				"{}::{} ({:08x})",
				s_EntityName,
				s_EntityTypeName,
				s_SubEntityId
			);

			// Add the node to the map.
			const auto s_SubEntityNode = std::make_shared<EntityTreeNode>(
				s_EntityHumanName,
				s_SubEntityId,
				s_CurrentFactory->m_ridResource,
				s_SubEntity
			);

			const auto s_LogicalParent = s_SubEntity.GetLogicalParent();

			if (s_LogicalParent) {
				auto s_ParentNode = s_NodeMap.find(s_LogicalParent);

				if (s_ParentNode != s_NodeMap.end()) {
					// If we have already seen the logical parent of this sub-entity, add it to the parent's children.
					s_ParentNode->second->Children.insert({ s_EntityHumanName, s_SubEntityNode });
				}
				else {
					// Otherwise, add it to the parentless nodes queue.
					s_ParentlessNodes.push(s_SubEntityNode);
				}
			} else {
				// If it has no logical parent, add it to the parentless nodes queue.
				s_ParentlessNodes.push(s_SubEntityNode);
			}

			// If the sub-entity has a factory with more sub-entities, add it to the queue.
			if (s_SubEntityFactory && s_SubEntityFactory->GetSubEntitiesCount() > 0) {
				s_NodeQueue.emplace(s_SubEntityFactory, s_SubEntity);
			}

			s_NodeMap[s_SubEntity] = s_SubEntityNode;
		}
	}

	// Go through the nodes and assign any remaining children to their parents.
	while (!s_ParentlessNodes.empty()) {
		const auto s_Node = s_ParentlessNodes.front();
		s_ParentlessNodes.pop();

		const auto s_LogicalParent = s_Node->Entity.GetLogicalParent();

		// If it has a logical parent and that parent is in the map, add it to the parent's children.
		if (s_LogicalParent) {
			auto s_ParentNode = s_NodeMap.find(s_LogicalParent);

			if (s_ParentNode != s_NodeMap.end()) {
				s_ParentNode->second->Children.insert({ s_Node->Name, s_Node });
				continue;
			}
		}

		// Otherwise, add it to the root node.
		s_NodeMap[ZEntityRef()] = s_Node;
	}

	// Update the cached tree.
	m_CachedEntityTreeMutex.lock();
	m_CachedEntityTree = std::move(s_SceneNode);
	m_CachedEntityTreeMutex.unlock();

	m_Server.OnEntityTreeRebuilt();
}

void Editor::RenderEntity(std::shared_ptr<EntityTreeNode> p_Node) {
	if (!p_Node) return;

	const auto s_Entity = p_Node->Entity;
	const auto s_EntityName = p_Node->Name;
	const auto s_IsSelected = s_Entity == m_SelectedEntity;

	ImGuiTreeNodeFlags s_Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

	if (p_Node->Children.empty()) {
		s_Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	if (s_IsSelected) {
		s_Flags |= ImGuiTreeNodeFlags_Selected;

		if (m_ShouldScrollToEntity) {
			ImGui::SetScrollHereY();
			m_ShouldScrollToEntity = false;
		}
	}
	else if (m_ShouldScrollToEntity && m_SelectedEntity && m_SelectedEntity.IsAnyParent(s_Entity)) {
		ImGui::SetNextTreeNodeOpen(true);
	}

	auto s_Open = ImGui::TreeNodeEx(
		s_EntityName.c_str(),
		s_Flags
	);

	if (ImGui::IsItemClicked()) {
		OnSelectEntity(s_Entity, std::nullopt);
	}

	if (s_Open) {
		for (const auto& s_Child: p_Node->Children) {
			RenderEntity(s_Child.second);
		}

		if (!(s_Flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
			ImGui::TreePop();
		}
	}
}

bool Editor::SearchForEntityById(ZTemplateEntityBlueprintFactory* p_BrickFactory, ZEntityRef p_BrickEntity, uint64_t p_EntityId) {
	if (!p_BrickFactory || !p_BrickEntity) return false;

	const auto s_EntIndex = p_BrickFactory->GetSubEntityIndex(p_EntityId);

	if (s_EntIndex != -1) {
		OnSelectEntity(p_BrickFactory->GetSubEntity(p_BrickEntity.m_pEntity, s_EntIndex), std::nullopt);
		return true;
	}

	for (int i = 0; i < p_BrickFactory->GetSubEntitiesCount(); ++i) {
		const auto& s_SubEntity = p_BrickFactory->GetSubEntity(p_BrickEntity.m_pEntity, i);

		if (!s_SubEntity) continue;

		const auto s_SubFactory = p_BrickFactory->GetSubEntityBlueprint(i);

		if (s_SubFactory &&
		    s_SubFactory->GetSubEntitiesCount() >
		    0) {
			if (SearchForEntityById(
				reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_SubFactory),
				s_SubEntity,
				p_EntityId
			))
				return true;
		}
	}

	return false;
}

bool Editor::SearchForEntityByType(
	ZTemplateEntityBlueprintFactory* p_BrickFactory,
	ZEntityRef p_BrickEntity,
	const std::string& p_TypeName
) {
	if (!p_BrickFactory || !p_BrickEntity) return false;

	for (int i = 0; i < p_BrickFactory->GetSubEntitiesCount(); ++i) {
		const auto& s_SubEntity = p_BrickFactory->GetSubEntity(p_BrickEntity.m_pEntity, i);

		if (!s_SubEntity) continue;

		ZEntityRef s_Ref = s_SubEntity;

		if (s_Ref.HasInterface(p_TypeName)) {
			OnSelectEntity(s_Ref, std::nullopt);
			return true;
		}

		const auto s_SubFactory = p_BrickFactory->GetSubEntityBlueprint(i);

		if (s_SubFactory &&
		    s_SubFactory->GetSubEntitiesCount() >
		    0) {
			if (SearchForEntityByType(
				reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_SubFactory),
				s_SubEntity,
				p_TypeName
			))
				return true;
		}
	}

	return false;
}

void Editor::DrawEntityTree() {
	ImGui::SetNextWindowPos({ 0, 110 }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({ 700, ImGui::GetIO().DisplaySize.y - 110 }, ImGuiCond_FirstUseEver);
	ImGui::Begin(ICON_MD_CATEGORY " Entities", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

	const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

	if (s_SceneCtx && s_SceneCtx->m_pScene && s_SceneCtx->m_aLoadedBricks.size() > 0) {
		static char s_EntitySearchInput[17] = {};
		if (ImGui::InputText(
			ICON_MD_SEARCH " Search by ID",
			s_EntitySearchInput,
			IM_ARRAYSIZE(s_EntitySearchInput),
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsNoBlank
		)) {
			const auto s_EntityId = std::strtoull(s_EntitySearchInput, nullptr, 16);

			for (int i = 0; i < s_SceneCtx->m_aLoadedBricks.size(); ++i) {
				const auto& s_Brick = s_SceneCtx->m_aLoadedBricks[i];
				const auto s_BpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_Brick.entityRef.GetBlueprintFactory());

				if (SearchForEntityById(s_BpFactory, s_Brick.entityRef, s_EntityId)) {
					Logger::Debug("Found entity in brick {} (idx = {}).", s_Brick.runtimeResourceID, i);
					m_SelectedBrickIndex = i;
					break;
				}
			}
		}

		static char s_EntityTypeSearchInput[2048] = {};
		if (ImGui::InputText(
			ICON_MD_SEARCH " Search by type",
			s_EntityTypeSearchInput,
			IM_ARRAYSIZE(s_EntityTypeSearchInput),
			ImGuiInputTextFlags_EnterReturnsTrue
		)) {
			for (int i = 0; i < s_SceneCtx->m_aLoadedBricks.size(); ++i) {
				const auto& s_Brick = s_SceneCtx->m_aLoadedBricks[i];
				const auto s_BpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_Brick.entityRef.GetBlueprintFactory());

				if (SearchForEntityByType(s_BpFactory, s_Brick.entityRef, s_EntityTypeSearchInput)) {
					Logger::Debug("Found entity in brick {} (idx = {}).", s_Brick.runtimeResourceID, i);
					m_SelectedBrickIndex = i;
					break;
				}
			}
		}

		if (ImGui::Button("Rebuild entity tree")) {
			UpdateEntities();
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

	if (m_ShouldScrollToEntity) {
		m_ShouldScrollToEntity = false;
	}
}

void Editor::OnSelectEntity(ZEntityRef p_Entity, std::optional<std::string> p_ClientId) {
	const bool s_DifferentEntity = m_SelectedEntity != p_Entity;

	m_SelectedEntity = p_Entity;
	m_ShouldScrollToEntity = p_Entity.GetEntity() != nullptr;

	if (s_DifferentEntity) {
		m_Server.OnEntitySelected(p_Entity, std::move(p_ClientId));
	}
}

void Editor::OnEntityNameChange(ZEntityRef p_Entity, const std::string& p_Name, std::optional<std::string> p_ClientId) {
	m_CachedEntityTreeMutex.lock();
	m_EntityNames[p_Entity] = p_Name;
	m_CachedEntityTreeMutex.unlock();

	m_Server.OnEntityNameChanged(p_Entity, std::move(p_ClientId));
}
