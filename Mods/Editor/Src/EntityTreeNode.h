#pragma once

#include <map>
#include <string>
#include <Glacier/ZResourceID.h>
#include <Glacier/ZEntity.h>

struct EntityTreeNode {
	std::string Name;
	uint64_t EntityId;
	ZRuntimeResourceID TBLU;
	ZEntityRef Entity;
	std::multimap<std::string, std::shared_ptr<EntityTreeNode>> Children;

	EntityTreeNode(const std::string& p_Name, uint64_t p_EntityId, ZRuntimeResourceID p_TBLU, ZEntityRef p_Ref)
		: Name(p_Name), EntityId(p_EntityId), TBLU(p_TBLU), Entity(p_Ref) {}
};