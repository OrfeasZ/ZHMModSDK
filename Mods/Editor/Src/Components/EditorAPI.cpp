#include <Editor.h>

#include "Logging.h"

#include <Glacier/EntityFactory.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZPhysics.h>

#include <ResourceLib_HM3.h>

#include <queue>
#include <utility>

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
			bool s_Matches = s_Node->TBLU == s_TBLU;

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

void Editor::SelectEntity(EntitySelector p_Selector, std::optional<std::string> p_ClientId) {
	auto s_Entity = FindEntity(p_Selector);

	if (s_Entity) {
		OnSelectEntity(s_Entity, std::move(p_ClientId));
	} else {
		throw std::runtime_error("Could not find entity for the given selector.");
	}
}

void Editor::SetEntityTransform(EntitySelector p_Selector, SMatrix p_Transform, bool p_Relative, std::optional<std::string> p_ClientId) {
	if (const auto s_Entity = FindEntity(p_Selector)) {
		OnEntityTransformChange(s_Entity, p_Transform, p_Relative, std::move(p_ClientId));
	} else {
		throw std::runtime_error("Could not find entity for the given selector.");
	}
}

void Editor::SpawnEntity(ZRuntimeResourceID p_Template, uint64_t p_EntityId, std::string p_Name, std::optional<std::string> p_ClientId) {

}

void Editor::DestroyEntity(EntitySelector p_Selector, std::optional<std::string> p_ClientId) {

}

void Editor::SetEntityName(EntitySelector p_Selector, std::string p_Name, std::optional<std::string> p_ClientId) {
	if (const auto s_Entity = FindEntity(p_Selector)) {
		OnEntityNameChange(s_Entity, p_Name, std::move(p_ClientId));
	} else {
		throw std::runtime_error("Could not find entity for the given selector.");
	}
}

void Editor::SetEntityProperty(EntitySelector p_Selector, uint32_t p_PropertyId, std::string_view p_JsonValue, std::optional<std::string> p_ClientId) {
	if (const auto s_Entity = FindEntity(p_Selector)) {
		auto s_EntityType = s_Entity->GetType();
		auto s_Property = s_EntityType->FindProperty(p_PropertyId);

		if (!s_Property) {
			throw std::runtime_error("Could not find property for the given ID.");
		}

		if (!s_Property->m_pType || !s_Property->m_pType->getPropertyInfo() || !s_Property->m_pType->getPropertyInfo()->m_pType) {
			throw std::runtime_error("Unable to set this property because its type information is missing from the game.");
		}

		const auto s_PropertyInfo = s_Property->m_pType->getPropertyInfo();

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

		OnSetPropertyValue(s_Entity, p_PropertyId, ZObjectRef(s_PropertyInfo->m_pType, s_Data), std::move(p_ClientId));
		(*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);
	} else {
		throw std::runtime_error("Could not find entity for the given selector.");
	}
}

void Editor::SignalEntityPin(EntitySelector p_Selector, uint32_t p_PinId, bool p_Output) {
	if (const auto s_Entity = FindEntity(p_Selector)) {
		OnSignalEntityPin(s_Entity, p_PinId, p_Output);
	} else {
		throw std::runtime_error("Could not find entity for the given selector.");
	}
}

void Editor::RebuildEntityTree() {
	UpdateEntities();
}