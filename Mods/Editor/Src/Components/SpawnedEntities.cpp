#include <Editor.h>

#include <random>

#include "Logging.h"

#include <Globals.h>
#include <Glacier/EntityFactory.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZPhysics.h>
#include <Glacier/ZScene.h>
#include <Glacier/ZSpatialEntity.h>

static constexpr float c_AssetSpawnMissDistance = 5.f;

void Editor::AddSpawnedEntity(
    SpawnedEntity p_SpawnedEntity, const std::string& p_Name, std::optional<std::string> p_ClientId
) {
    const ZEntityRef s_Entity = p_SpawnedEntity.Entity;

    m_EntityTreeDeleter.OnTreeSpawned(p_SpawnedEntity.Tree);

    // Disable physics for spawned entities for a second, cause otherwise they fall through the map.
    for (const auto s_EntityObj : p_SpawnedEntity.Tree.Entities) {
        ZEntityRef s_TreeEntity(s_EntityObj);
        const auto s_PhysicsAspect = s_TreeEntity.QueryInterface<ZDynamicPhysicsAspect>();

        if (!s_PhysicsAspect || s_PhysicsAspect->m_bKinematic) {
            continue;
        }

        s_TreeEntity.SetProperty("m_bKinematic", true);
        p_SpawnedEntity.PhysicsFrozenEntities.push_back(s_TreeEntity);
    }

    {
        std::scoped_lock s_Lock(m_CachedEntityTreeMutex);

        m_EntityNames[s_Entity] = p_Name;

        m_SpawnedEntities.emplace(s_Entity, std::move(p_SpawnedEntity));

        if (m_CachedEntityTree && m_CachedEntityTreeMap.size() > 0) {
            UpdateEntityTree(m_CachedEntityTreeMap, m_CachedEntityTreeNodesById, { s_Entity }, true);
            ReparentSpawnedEntity(m_CachedEntityTreeMap, s_Entity);
        }
    }

    m_Server.OnEntitySpawned(s_Entity, std::move(p_ClientId));
}

// Needs m_CachedEntityTreeMutex.
ZEntityRef Editor::FindSpawnedRoot(const ZEntityRef p_Entity) {
    for (const auto& [s_Root, s_SpawnedEntity] : m_SpawnedEntities) {
        if (s_SpawnedEntity.Tree.Entities.contains(p_Entity.m_pObj)) {
            return s_Root;
        }
    }

    return {};
}

bool Editor::IsEntityInSpawnedTree(const ZEntityRef p_Entity) {
    std::shared_lock s_Lock(m_CachedEntityTreeMutex);

    return static_cast<bool>(FindSpawnedRoot(p_Entity));
}

void Editor::TrackSpawnedEntityReference(
    const ZEntityRef p_Entity, const uint32_t p_PropertyId, const ZObjectRef& p_Value
) {
    const ChangeKey s_Key { p_Entity, p_PropertyId };
    const ZEntityRef s_Target = GetEntityRefFromValue(p_Value);

    if (s_Target && !std::ranges::contains(m_SpawnedEntityReferences, s_Key) && IsEntityInSpawnedTree(s_Target)) {
        m_SpawnedEntityReferences.push_back(s_Key);
    }
}

// Returns what was cleared, so undoing the destroy can point the properties at the new tree.
std::vector<Editor::PropertyHistoryEntry> Editor::ClearReferencesToTree(const SpawnedTree& p_Tree) {
    std::vector<PropertyHistoryEntry> s_Cleared;
    std::vector<ChangeKey> s_Kept;

    for (const ChangeKey& s_Key : std::exchange(m_SpawnedEntityReferences, {})) {
        const ZEntityRef s_Entity = s_Key.Entity;

        if (p_Tree.Entities.contains(s_Entity.m_pObj) || !IsEntityInTree(s_Entity)) {
            continue;
        }

        const SPropertyData* s_Property = s_Entity->GetType()->FindProperty(*s_Key.PropertyId);

        if (!s_Property) {
            continue;
        }

        const ZObjectRef s_Value = GetPropertyValue(s_Entity, s_Property);
        const ZEntityRef s_Target = GetEntityRefFromValue(s_Value);

        if (!s_Target) {
            continue;
        }

        if (!p_Tree.Entities.contains(s_Target.m_pObj)) {
            if (IsEntityInSpawnedTree(s_Target)) {
                s_Kept.push_back(s_Key);
            }

            continue;
        }

        try {
            const PropertyHistoryEntry s_Reference {
                .Target = MakeHistoryTarget(s_Entity),
                .PropertyId = *s_Key.PropertyId,
                .OldValue = MakeHistoryValue(s_Value),
                .NewValue = { .Value = MakeEntityRefValue(s_Value.GetTypeID(), {}) },
            };

            SetPropertyValueInternal(
                s_Entity, s_Reference.PropertyId, s_Reference.NewValue.Value, std::nullopt, ChangeMode::Revert
            );
            s_Cleared.push_back(s_Reference);
        }
        catch (const std::exception& p_Exception) {
            Logger::Warn(
                "[Editor] Skipping the clear of property {:08x} on {:016x}: {}", *s_Key.PropertyId,
                s_Entity->GetType()->m_nEntityID, p_Exception.what()
            );
        }
    }

    m_SpawnedEntityReferences.insert(m_SpawnedEntityReferences.end(), s_Kept.begin(), s_Kept.end());

    return s_Cleared;
}

// Needs m_CachedEntityTreeMutex.
void Editor::ReparentSpawnedEntity(
    std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>>& p_NodeMap, ZEntityRef p_Entity
) {
    const auto s_SceneNode = p_NodeMap.find(Globals::Hitman5Module->m_pEntitySceneContext->m_pScene.m_entityRef);
    const auto s_Node = p_NodeMap.find(p_Entity);

    if (s_SceneNode == p_NodeMap.end() || s_Node == p_NodeMap.end()) {
        return;
    }

    const auto s_SpawnedEntitiesNode = s_SceneNode->second->Children.find("Spawned entities");

    if (s_SpawnedEntitiesNode == s_SceneNode->second->Children.end()) {
        return;
    }

    const auto& s_NewParent = s_SpawnedEntitiesNode->second;

    if (s_Node->second->Parent.lock() == s_NewParent) {
        return;
    }

    s_Node->second->RemoveFromParent();

    s_NewParent->Children.insert({ s_Node->second->Name, s_Node->second });
    s_Node->second->Parent = s_NewParent;
}

void Editor::OnAssetItemDropped(const AssetItem& p_Item, SVector2 p_ScreenPosition) {
    SVector3 s_World;
    SVector3 s_Direction;
    SDK()->ScreenToWorld(p_ScreenPosition, s_World, s_Direction);

    ZRayQueryOutput s_RayOutput {};
    SVector3 s_Position;

    if (RayCastCollision(s_World, s_Direction, s_RayOutput)) {
        s_Position = SVector3(s_RayOutput.m_vPosition.x, s_RayOutput.m_vPosition.y, s_RayOutput.m_vPosition.z);
    }
    else {
        s_Position = s_World + s_Direction * c_AssetSpawnMissDistance;
    }

    std::scoped_lock s_Lock(m_AssetSpawnRequestsMutex);

    m_AssetSpawnRequests.push_back({
        .ResourceId = *p_Item.ResId,
        .Path = p_Item.Path,
        .Name = std::string(GetAssetItemLabel(p_Item)),
        .Position = s_Position,
    });
}

void Editor::UpdateAssetSpawns() {
    std::vector<AssetSpawnRequest> s_Requests;

    {
        std::scoped_lock s_Lock(m_AssetSpawnRequestsMutex);
        s_Requests.swap(m_AssetSpawnRequests);
    }

    for (auto& s_Request : s_Requests) {
        auto& s_Spawn = m_PendingAssetSpawns.emplace_back();
        s_Spawn.Request = std::move(s_Request);
        m_AssetBrowser.LoadFactory(s_Spawn.Request.ResourceId, s_Spawn.Factory);
    }

    for (auto s_It = m_PendingAssetSpawns.begin(); s_It != m_PendingAssetSpawns.end();) {
        const auto s_Status = AssetBrowser::GetFactoryStatus(s_It->Factory);

        if (s_Status == RESOURCE_STATUS_VALID) {
            SpawnAssetItem(*s_It);
        }
        else if (s_Status != RESOURCE_STATUS_FAILED && ++s_It->LoadFrames < 600) {
            ++s_It;
            continue;
        }
        else {
            Logger::Warn(
                "[Editor] Could not load asset {} ({}) to spawn it: resource status {}, {} frame(s).",
                s_It->Request.Path,
                s_It->Request.ResourceId,
                static_cast<int>(s_Status),
                s_It->LoadFrames
            );
        }

        s_It = m_PendingAssetSpawns.erase(s_It);
    }
}

void Editor::SpawnAssetItem(const PendingAssetSpawn& p_Spawn) {
    const auto& s_Request = p_Spawn.Request;
    const auto s_Factory = p_Spawn.Factory.GetResource();
    const std::string s_ItemName = fmt::format("{} ({})", s_Request.Path, s_Request.ResourceId);

    if (!CanSpawnTemplate(s_Factory, s_ItemName)) {
        return;
    }

    static std::mt19937_64 s_Random(std::random_device {}());

    SpawnInfo s_Spawn {
        .Factory = p_Spawn.Factory,
        .Name = s_Request.Name,
        .EntityId = s_Random(),
        .IsAsset = true,
    };
    s_Spawn.Transform.Trans = float4(s_Request.Position, 1.f);

    const ZEntityRef s_Entity = SpawnEntityInternal(s_Spawn);

    if (!s_Entity) {
        Logger::Error("[Editor] Failed to spawn asset {}.", s_ItemName);
        return;
    }

    Logger::Info(
        "[Editor] Spawned asset {} with id {:016x} at ({}, {}, {}).",
        s_ItemName,
        s_Entity->GetType()->m_nEntityID,
        s_Request.Position.x,
        s_Request.Position.y,
        s_Request.Position.z
    );

    PushHistory({ .Change = SpawnHistoryEntry { std::move(s_Spawn) } });
    SelectEntityInternal(s_Entity, true, std::nullopt);
}

void Editor::UpdatePhysicsFreezes(double p_DeltaSeconds) {
    std::shared_lock s_Lock(m_CachedEntityTreeMutex);

    for (auto& s_SpawnedEntity : m_SpawnedEntities | std::views::values) {
        if (s_SpawnedEntity.PhysicsFrozenEntities.empty()) {
            continue;
        }

        s_SpawnedEntity.PhysicsFreezeSeconds -= p_DeltaSeconds;

        if (s_SpawnedEntity.PhysicsFreezeSeconds > 0.0) {
            continue;
        }

        for (auto& s_Entity : s_SpawnedEntity.PhysicsFrozenEntities) {
            s_Entity.SetProperty("m_bKinematic", false);
        }

        s_SpawnedEntity.PhysicsFrozenEntities.clear();
    }
}

// Wait one more frame after the tree is no longer in use, because an item spawner's item is
// still pending registration with ZDistanceConditionManager in the frame it's created.
bool Editor::IsReadyToDestroy(SpawnedEntity& p_SpawnedEntity) {
    p_SpawnedEntity.DeleteReadyFrames = IsTreeInUse(p_SpawnedEntity.Tree) ? 0 : p_SpawnedEntity.DeleteReadyFrames + 1;

    return p_SpawnedEntity.DeleteReadyFrames >= 2;
}

void Editor::UpdateEntitiesToDestroy() {
    std::scoped_lock s_Lock(m_EntityDestructionMutex);

    std::erase_if(m_EntitiesToDestroy, [&](SpawnedEntity& p_SpawnedEntity) {
        if (!IsReadyToDestroy(p_SpawnedEntity)) {
            return false;
        }

        m_EntityTreeDeleter.Delete(p_SpawnedEntity.Entity, p_SpawnedEntity.Tree);
        return true;
    });
}
