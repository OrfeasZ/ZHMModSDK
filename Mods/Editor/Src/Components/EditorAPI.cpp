#include <Editor.h>

#include "Logging.h"

#include <Glacier/EntityFactory.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZPhysics.h>

#include <ResourceLib_HM3.h>

#include <queue>
#include <utility>
#include <spdlog/fmt/ostr.h>

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

std::string Editor::getCollisionHash(auto s_SelectedEntity) {
	const auto s_EntityType = s_SelectedEntity->GetType();
	std::string s_AlocHash = "";
	if (s_EntityType && s_EntityType->m_pProperties01) {
		for (uint32_t i = 0; i < s_EntityType->m_pProperties01->size(); ++i) {
			ZEntityProperty* s_Property = &s_EntityType->m_pProperties01->operator[](i);
			const auto* s_PropertyInfo = s_Property->m_pType->getPropertyInfo();

			if (!s_PropertyInfo || !s_PropertyInfo->m_pType)
				continue;

			const auto s_PropertyAddress = reinterpret_cast<uintptr_t>(s_SelectedEntity.m_pEntity) + s_Property->m_nOffset;
			const uint16_t s_TypeSize = s_PropertyInfo->m_pType->typeInfo()->m_nTypeSize;
			const uint16_t s_TypeAlignment = s_PropertyInfo->m_pType->typeInfo()->m_nTypeAlignment;

			// Get the value of the property.
			auto* s_Data = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(s_TypeSize, s_TypeAlignment);

			if (s_PropertyInfo->m_nFlags & EPropertyInfoFlags::E_HAS_GETTER_SETTER) {
				s_PropertyInfo->get(
				    reinterpret_cast<void*>(s_PropertyAddress),
				    s_Data,
				    s_PropertyInfo->m_nOffset);
			} else {
				s_PropertyInfo->m_pType->typeInfo()->m_pTypeFunctions->copyConstruct(
				    s_Data,
				    reinterpret_cast<void*>(s_PropertyAddress));
			}

			const std::string s_TypeName = s_PropertyInfo->m_pType->typeInfo()->m_pTypeName;
			const std::string s_InputId = std::format("##Property{}", i);

			const char* s_COLLISION_RESOURCE_ID_PROPERTY_NAME = "m_CollisionResourceID";
			
			if (s_PropertyInfo->m_pType->typeInfo()->isResource() || s_PropertyInfo->m_nPropertyID != s_Property->m_nPropertyId) {
				// Some properties don't have a name for some reason. Try to find using RL.
				const auto s_PropertyNameView = HM3_GetPropertyName(s_Property->m_nPropertyId);

				if (s_PropertyNameView.Size > 0) {
					if (std::string(s_PropertyNameView.Data, s_PropertyNameView
						.Size) != s_COLLISION_RESOURCE_ID_PROPERTY_NAME) {
						continue;
					} else {
						Logger::Info("Property Name: {}", std::string(s_PropertyNameView.Data, s_PropertyNameView.Size).c_str());
						auto* s_Resource = static_cast<ZResourcePtr*>(s_Data);
						std::string s_ResourceName = "null";

						if (s_Resource && s_Resource->m_nResourceIndex >= 0) {
							s_ResourceName = fmt::format("{:08X}{:08X}", s_Resource->GetResourceInfo().rid.m_IDHigh, s_Resource->GetResourceInfo().rid.m_IDLow);
						}

						Logger::Info("Found ALOC Resource: {}", s_ResourceName.c_str());
						if (s_ResourceName.c_str() != "" && s_ResourceName.c_str() != NULL && s_ResourceName.c_str() != "null") { 
							return s_ResourceName.c_str();
						}
					}
					
				}
			}
		}
	}

	return "";
}

std::vector<std::pair<std::string, ZEntityRef>> Editor::FindPrims() {
	std::shared_lock s_Lock(m_CachedEntityTreeMutex);

	if (!m_CachedEntityTree) {
		return {};
	}
	std::vector<std::pair<std::string, ZEntityRef>> entities;

	// Create a queue and add the root to it.
	std::queue<std::pair<std::shared_ptr<EntityTreeNode>, std::shared_ptr<EntityTreeNode>>> s_NodeQueue;
	s_NodeQueue.push(std::pair(std::shared_ptr<EntityTreeNode>(), m_CachedEntityTree));
	const char* s_GEOMENTITY_TYPE = "ZGeomEntity";
	const char* s_PURE_WATER_TYPE = "ZPureWaterAspect";
	std::vector<std::string> s_selectorPrimHashes;

	// Keep iterating through the tree until we find all the prims.
	while (!s_NodeQueue.empty()) {
		// Access the first node in the queue
		auto s_Parent = s_NodeQueue.front().first;
		auto s_Node = s_NodeQueue.front().second;
		s_NodeQueue.pop();
		

		const auto& s_Interfaces = *s_Node->Entity.GetEntity()->GetType()->m_pInterfaces;
		char* s_EntityType = s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName;
		if (strcmp(s_EntityType, s_GEOMENTITY_TYPE) == 0) {
			if (const ZGeomEntity* s_GeomEntity = s_Node->Entity.QueryInterface<ZGeomEntity>()) {
				if (s_GeomEntity->m_ResourceID.m_nResourceIndex != -1) {
					std::string s_collision_ioi_string = getCollisionHash(s_Node->Entity);
					if (!s_collision_ioi_string.empty() && s_collision_ioi_string != "null") {
						std::string s_HashString = std::format("Node TBLU <{:08X}{:08X}>", s_Node->TBLU.m_IDHigh, s_Node->TBLU.m_IDLow);
						const auto s_PrimResourceInfo = (*Globals::ResourceContainer)->m_resources[s_GeomEntity->m_ResourceID.m_nResourceIndex];
						const auto s_PrimHash = s_PrimResourceInfo.rid.GetID();
						std::string s_PrimHashString{std::format("{:016X}", s_PrimHash)};
						bool s_Skip = false;
						for (auto s_Interface: s_Interfaces) {
							if (s_Interface.m_pTypeId->typeInfo() != NULL) {
								char* s_EntityType = s_Interface.m_pTypeId->typeInfo()->m_pTypeName;
								if (strcmp(s_EntityType, s_PURE_WATER_TYPE) == 0) {
									s_Skip = true;
									Logger::Info("Skipping PRIM with ZPureWaterAspect type: '{}' '{}' '{}'", s_HashString, s_PrimHashString, s_collision_ioi_string);
									break;
								}
							}
						}
						if (!s_Skip) {
							Logger::Info("Found PRIM with collision: '{}' '{}' '{}'", s_HashString, s_PrimHashString, s_collision_ioi_string);

							entities.push_back(std::pair<std::string, ZEntityRef>{s_collision_ioi_string, s_Node->Entity});
						}
					}
				}
			}
		}

		// Add children to the queue.
		for (auto& childPair: s_Node->Children) {
			s_NodeQueue.push(std::pair(s_Node, childPair.second));
		}
	}

	return entities;
}

std::vector<std::pair<std::string, ZEntityRef>> Editor::FindPfBoxEntities() {
	std::shared_lock s_Lock(m_CachedEntityTreeMutex);

	if (!m_CachedEntityTree) {
		return {};
	}
	std::vector<std::pair<std::string, ZEntityRef>> entities;
	const char* s_PFBOXENTITY_TYPE = "ZPFBoxEntity";

	Logger::Info("Getting PfBoxEntities:");
	// Create a queue and add the root to it.
	std::queue<std::shared_ptr<EntityTreeNode>> s_NodeQueue;
	s_NodeQueue.push(m_CachedEntityTree);

	// Keep iterating through the tree until we find the nodes we're looking for.
	while (!s_NodeQueue.empty()) {
		// Access the first node in the queue
		auto s_Node = s_NodeQueue.front();
		s_NodeQueue.pop();
		const auto& s_Interfaces = *s_Node->Entity.GetEntity()->GetType()->m_pInterfaces;
		char* s_EntityType = s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName;

		if (strcmp(s_EntityType, s_PFBOXENTITY_TYPE) == 0) {
			Logger::Info("Found PfBoxEntity: 00724CDE424AFE76");
			entities.push_back(std::pair<std::string, ZEntityRef>{"00724CDE424AFE76", s_Node->Entity});
		}

		// Add children to the queue.
		for (auto& childPair: s_Node->Children) {
			s_NodeQueue.push(childPair.second);
		}
	}

	return entities;
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

		if (s_PropertyInfo->m_pType->typeInfo()->isEntity()) {
			if (p_JsonValue == "null") {
				TEntityRef<ZEntityImpl> s_EntityRef;
				OnSetPropertyValue(s_Entity, p_PropertyId, ZObjectRef(s_PropertyInfo->m_pType, &s_EntityRef), std::move(p_ClientId));
			} else {
				// Parse EntitySelector
				simdjson::ondemand::parser s_Parser;
				const auto s_EntitySelectorJson = simdjson::padded_string(p_JsonValue);
				simdjson::ondemand::document s_EntitySelectorMsg = s_Parser.iterate(s_EntitySelectorJson);

				const auto s_EntitySelector = EditorServer::ReadEntitySelector(s_EntitySelectorMsg);

				if (const auto s_TargetEntity = FindEntity(s_EntitySelector)) {
					TEntityRef<ZEntityImpl> s_EntityRef(s_TargetEntity);
					OnSetPropertyValue(s_Entity, p_PropertyId, ZObjectRef(s_PropertyInfo->m_pType, &s_EntityRef), std::move(p_ClientId));
				} else {
					throw std::runtime_error("Could not find entity for the given selector.");
				}
			}
		} else {
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
		}
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

void Editor::LoadNavpAreas(simdjson::ondemand::array p_NavpAreas, int p_ChunkIndex) {
	Logger::Info("Loading Navp areas");	

	if (p_ChunkIndex == 0) {
		m_NavpAreas.clear();
	}
	for (simdjson::ondemand::array s_NavpArea: p_NavpAreas) {
		std::vector<SVector3> s_Area;
		for (simdjson::ondemand::array s_NavpPoint: s_NavpArea) {
			std::vector<double> s_Point;
			for (double coord: s_NavpPoint) {
				s_Point.push_back(coord);
			}
			SVector3 point{(float) s_Point[0], (float) s_Point[1], (float) s_Point[2]};

			s_Area.push_back(point);
		}
		m_NavpAreas.push_back(s_Area);
	}
}