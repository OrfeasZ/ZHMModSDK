#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "Glacier/ZEntity.h"

class ZEntityBlueprintFactoryBase;
class ZTemplateEntityFactory;
class ZItemSpawner;

struct SpawnedTree {
    std::unordered_set<ZEntityType**> Entities;
    std::vector<ZItemSpawner*> ItemSpawners;
};

void ForEachEntityInTree(const ZEntityRef& p_Root, const std::function<void(ZEntityRef)>& p_Callback);
std::optional<std::string_view> FindUnspawnablePath(std::string_view p_Path);
bool CanSpawnTemplate(IEntityFactory* p_Factory, const std::string& p_ItemName);
std::unordered_set<ZEntityType**> GetRegisteredLookAtTriggers();
size_t UnregisterLookAtTriggersIf(const std::function<bool(ZEntityType**)>& p_ShouldUnregister);
SpawnedTree CollectSpawnedTree(const ZEntityRef& p_Root);
bool IsTreeInUse(const SpawnedTree& p_Tree);

class EntityTreeDeleter {
public:
    void Delete(const ZEntityRef& p_Root, const SpawnedTree& p_Tree);
    void Update();
    void OnTreeSpawned(const SpawnedTree& p_Tree);
    bool IsInSpawnedTree(ZEntityType** p_Entity);

private:
    std::mutex m_Mutex;
    std::unordered_set<ZEntityType**> m_SpawnedTreeEntities;
    std::unordered_set<ZEntityType**> m_DeletedTreeEntities;
    int m_DeletedTreeCheckFrames = 0;
};
