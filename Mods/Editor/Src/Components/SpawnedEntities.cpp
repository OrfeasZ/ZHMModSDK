#include <Editor.h>

#include <random>

#include "Logging.h"

#include <Functions.h>
#include <Globals.h>
#include <Glacier/EntityFactory.h>
#include <Glacier/SExternalReferences.h>
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
            UpdateEntityTree(m_CachedEntityTreeMap, { s_Entity }, true);
            ReparentSpawnedEntity(m_CachedEntityTreeMap, s_Entity);
        }
    }

    m_Server.OnEntitySpawned(s_Entity, std::move(p_ClientId));
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
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        return;
    }

    const auto& s_Request = p_Spawn.Request;
    const auto s_Factory = p_Spawn.Factory.GetResource();
    const std::string s_ItemName = fmt::format("{} ({})", s_Request.Path, s_Request.ResourceId);

    if (!CanSpawnTemplate(s_Factory, s_ItemName)) {
        return;
    }

    static std::mt19937_64 s_Random(std::random_device {}());

    ZEntityRef s_Entity;
    SExternalReferences s_ExternalRefs;

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager,
        s_Entity,
        s_Request.Name,
        s_Factory,
        s_Scene.m_entityRef,
        s_ExternalRefs,
        s_Random()
    );

    if (!s_Entity) {
        Logger::Error("[Editor] Failed to spawn asset {}.", s_ItemName);
        return;
    }

    if (const auto s_SpatialEntity = s_Entity.QueryInterface<ZSpatialEntity>()) {
        s_Entity.SetProperty("m_eRoomBehaviour", ZSpatialEntity::ERoomBehaviour::ROOM_DYNAMIC);

        SMatrix s_Transform;
        s_Transform.Trans = float4(s_Request.Position, 1.f);
        s_SpatialEntity->SetObjectToWorldMatrixFromEditor(s_Transform);
    }

    RefreshStaticPhysicsAndObstacles(s_Entity);

    SpawnedEntity s_SpawnedEntity { .Entity = s_Entity, .Tree = CollectSpawnedTree(s_Entity) };
    s_SpawnedEntity.Factory.emplace(p_Spawn.Factory);

    Logger::Info(
        "[Editor] Spawned asset {} with id {:016x} at ({}, {}, {}).",
        s_ItemName,
        s_Entity->GetType()->m_nEntityID,
        s_Request.Position.x,
        s_Request.Position.y,
        s_Request.Position.z
    );

    AddSpawnedEntity(std::move(s_SpawnedEntity), s_Request.Name, std::nullopt);
    OnSelectEntity(s_Entity, true, std::nullopt);
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
bool Editor::IsReadyToDestroy(ZEntityRef p_Entity) {
    std::scoped_lock s_Lock(m_CachedEntityTreeMutex);

    const auto s_It = m_SpawnedEntities.find(p_Entity);

    if (s_It == m_SpawnedEntities.end()) {
        return true;
    }

    auto& s_SpawnedEntity = s_It->second;
    s_SpawnedEntity.DeleteReadyFrames = IsTreeInUse(s_SpawnedEntity.Tree) ? 0 : s_SpawnedEntity.DeleteReadyFrames + 1;

    return s_SpawnedEntity.DeleteReadyFrames >= 2;
}
