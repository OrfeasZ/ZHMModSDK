#include "AssetPreview.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "Logging.h"
#include "IModSDK.h"
#include "IPluginInterface.h"
#include "Hooks.h"

#include <Functions.h>
#include <Globals.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZScene.h>
#include <Glacier/ZRoom.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/SExternalReferences.h>
#include <Glacier/ZSparseBitArray.h>

static const float4 c_AssetPreviewPosition(-999.f, -999.f, -999.f, 1.f);

// Cameras and lights face along -Z.
static SMatrix LookAt(const SVector3& p_From, const SVector3& p_To) {
    const SVector3 s_Back = (p_From - p_To).Normalized();
    const SVector3 s_Up = std::abs(s_Back.z) > 0.999f ? SVector3(0.f, 1.f, 0.f) : SVector3(0.f, 0.f, 1.f);
    const SVector3 s_Right = s_Up.Cross(s_Back).Normalized();
    const SVector3 s_CameraUp = s_Back.Cross(s_Right);

    return SMatrix(float4(s_Right), float4(s_CameraUp), float4(s_Back), float4(p_From, 1.f));
}

static ZRenderDestination* GetRenderDestination(const TEntityRef<ZRenderDestinationTextureEntity>& p_RT) {
    if (!p_RT) {
        return nullptr;
    }

    return reinterpret_cast<ZRenderDestination*>(p_RT.m_pInterfaceRef->GetRenderDestination());
}

AssetPreview::AssetPreview(EntityTreeDeleter& p_TreeDeleter) : m_TreeDeleter(p_TreeDeleter) {
    Hooks::ZRenderGraphNodeCamera_Traverse->AddDetour(this, &AssetPreview::ZRenderGraphNodeCamera_Traverse);
    Hooks::ZRenderGraphTraversal_TraverseSimple->AddDetour(this, &AssetPreview::ZRenderGraphTraversal_TraverseSimple);
    Hooks::CLightTile_DispatchShadeDeferred->AddDetour(this, &AssetPreview::CLightTile_DispatchShadeDeferred);
    Hooks::ZRoomManager_UnregisterRoom->AddDetour(this, &AssetPreview::ZRoomManager_UnregisterRoom);
}

AssetPreview::~AssetPreview() {
    Hooks::ZRenderGraphNodeCamera_Traverse->RemoveDetoursWithContext(this);
    Hooks::ZRenderGraphTraversal_TraverseSimple->RemoveDetoursWithContext(this);
    Hooks::CLightTile_DispatchShadeDeferred->RemoveDetoursWithContext(this);
    Hooks::ZRoomManager_UnregisterRoom->RemoveDetoursWithContext(this);
}

void AssetPreview::Spawn() {
    if (m_Rig) {
        return;
    }

    static const std::string s_RigJson =
        R"(
{
    "tempHash": "00644fe9eb9feff5",
    "tbluHash": "005474211f99b411",
    "rootEntity": "fffffffffffffffe",
    "entities": {
        "fffffffffffffffe": {
            "parent": null,
            "name": "editor_data",
            "factory": "[modules:/zspatialentity.class].pc_entitytype",
            "blueprint": "[modules:/zspatialentity.class].pc_entityblueprint"
        },
        "feed678791f1b3e1": {
            "parent": "fffffffffffffffe",
            "name": "AssetPreviewKeyLight",
            "factory": "[modules:/zlightentity.class].pc_entitytype",
            "blueprint": "[modules:/zlightentity.class].pc_entityblueprint",
            "properties": {
                "m_mTransform": {
                    "type": "SMatrix43",
                    "value": {
                        "rotation": {
                            "x": 0,
                            "y": 0,
                            "z": 0
                        },
                        "position": {
                            "x": -40.105434,
                            "y": -29.001667,
                            "z": -998.0
                        }
                    }
                },
                "m_bCastShadows": {
                    "type": "bool",
                    "value": false
                },
                "m_fDiffusePower": {
                    "type": "float32",
                    "value": 1.5
                },
                "m_eRoomBehaviour": {
                    "type": "ZSpatialEntity.ERoomBehaviour",
                    "value": "ROOM_DYNAMIC"
                },
                "m_eidParent": {
                    "type": "SEntityTemplateReference",
                    "value": "fffffffffffffffe",
                    "postInit": true
                }
            }
        },
        "feed3b4c5d6e7f80": {
            "parent": "fffffffffffffffe",
            "name": "AssetPreviewFillLight",
            "factory": "[modules:/zlightentity.class].pc_entitytype",
            "blueprint": "[modules:/zlightentity.class].pc_entityblueprint",
            "properties": {
                "m_mTransform": {
                    "type": "SMatrix43",
                    "value": {
                        "rotation": {
                            "x": 0,
                            "y": 0,
                            "z": 0
                        },
                        "position": {
                            "x": -40.105434,
                            "y": -29.001667,
                            "z": -998.0
                        }
                    }
                },
                "m_bCastShadows": {
                    "type": "bool",
                    "value": false
                },
                "m_fDiffusePower": {
                    "type": "float32",
                    "value": 2
                },
                "m_eRoomBehaviour": {
                    "type": "ZSpatialEntity.ERoomBehaviour",
                    "value": "ROOM_DYNAMIC"
                },
                "m_eidParent": {
                    "type": "SEntityTemplateReference",
                    "value": "fffffffffffffffe",
                    "postInit": true
                }
            }
        },
        "feedbf5a41eb9c48": {
            "parent": "fffffffffffffffe",
            "name": "RenderDestinationTexture",
            "factory": "[modules:/zrenderdestinationtextureentity.class].pc_entitytype",
            "blueprint": "[modules:/zrenderdestinationtextureentity.class].pc_entityblueprint",
            "properties": {
                "m_aMultiSource": {
                    "type": "TArray<SEntityTemplateReference>",
                    "value": [
                        "feedb6fc4f5626ea"
                    ]
                },
                "m_nWidth": {
                    "type": "uint32",
                    "value": 512
                },
                "m_nHeight": {
                    "type": "uint32",
                    "value": 512
                },
                "m_bUseBGRA": {
                    "type": "bool",
                    "value": true
                },
                "m_bIsPIP": {
                    "type": "bool",
                    "value": false
                },
                "m_bDrawShadows": {
                    "type": "bool",
                    "value": true
                },
                "m_bDrawAtmosphericScattering": {
                    "type": "bool",
                    "value": false
                },
                "m_bDrawGates": {
                    "type": "bool",
                    "value": true
                },
                "m_nGateTraversalDepth": {
                    "type": "int32",
                    "value": 10000
                },
                "m_bForceVisible": {
                    "type": "bool",
                    "value": true
                },
                "m_eRoomBehaviour": {
                    "type": "ZSpatialEntity.ERoomBehaviour",
                    "value": "ROOM_DYNAMIC"
                }
            }
        },
        "feedb6fc4f5626ea": {
            "parent": "fffffffffffffffe",
            "name": "Camera",
            "factory": "[modules:/zcameraentity.class].pc_entitytype",
            "blueprint": "[modules:/zcameraentity.class].pc_entityblueprint",
            "properties": {
                "m_bAllowAutoCameraCuts": {
                    "type": "bool",
                    "value": false
                },
                "m_fNearZ": {
                    "type": "float32",
                    "value": 0.5
                },
                "m_vClearColor": {
                    "type": "SColorRGB",
                    "value": "#888888"
                },
                "m_fAspectWByH": {
                    "type": "float32",
                    "value": 1
                },
                "m_bAutoAspect": {
                    "type": "bool",
                    "value": false
                },
                "m_fFovYDeg": {
                    "type": "float32",
                    "value": 35
                },
                "m_fFarZ": {
                    "type": "float32",
                    "value": 250
                },
                "m_bIsUICamera": {
                    "type": "bool",
                    "value": true
                },
                "m_mTransform": {
                    "type": "SMatrix43",
                    "value": {
                        "rotation": {
                            "x": -73.77378164046733,
                            "y": 1.7075472925031877e-06,
                            "z": 90.61792971868923
                        },
                        "position": {
                            "x": -43.491676,
                            "y": -28.87086,
                            "z": 3.2203503
                        }
                    }
                },
                "m_bForceVisible": {
                    "type": "bool",
                    "value": true
                },
                "m_nPIPPriority": {
                    "type": "uint32",
                    "value": 0
                },
                "m_eRoomBehaviour": {
                    "type": "ZSpatialEntity.ERoomBehaviour",
                    "value": "ROOM_DYNAMIC"
                }
            }
        }
    },
    "propertyOverrides": [],
    "overrideDeletes": [],
    "pinConnectionOverrides": [],
    "pinConnectionOverrideDeletes": [],
    "externalScenes": [],
    "subType": "brick",
    "quickEntityVersion": 3.1,
    "extraFactoryDependencies": [],
    "extraBlueprintDependencies": [],
    "comments": []
}
)";

    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    auto& s_Factory = m_RigFactory.emplace();
    auto& s_BlueprintFactory = m_RigBlueprintFactory.emplace();

    if (!SDK()->LoadQnEntity(s_RigJson, s_BlueprintFactory, s_Factory)) {
        Logger::Error("[Editor] Failed to load the asset preview rig.");
        m_RigFactory.reset();
        m_RigBlueprintFactory.reset();
        return;
    }

    SExternalReferences s_ExternalRefs;
    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager,
        m_Rig,
        "SDKCam",
        s_Factory,
        s_Scene.m_entityRef,
        s_ExternalRefs,
        -1
    );

    const auto s_GetSubEntity = [&](uint64_t p_EntityId) -> ZEntityRef {
        const auto s_Index = s_BlueprintFactory.GetResource()->GetSubEntityIndex(p_EntityId);

        if (s_Index == -1) {
            return {};
        }

        return s_BlueprintFactory.GetResource()->GetSubEntity(m_Rig.m_pObj, s_Index);
    };

    m_Camera = TEntityRef<ZCameraEntity>(s_GetSubEntity(0xfeedb6fc4f5626ea));
    m_RT = TEntityRef<ZRenderDestinationTextureEntity>(s_GetSubEntity(0xfeedbf5a41eb9c48));
    m_KeyLight = s_GetSubEntity(0xfeed678791f1b3e1);
    m_FillLight = s_GetSubEntity(0xfeed3b4c5d6e7f80);

    if (!m_Camera || !m_RT) {
        Logger::Error("[Editor] Failed to get the asset preview camera or render destination texture entity.");
        return;
    }

    // The RT only renders when a client is visible, and lights only apply when the main camera
    // sees them. Registering the lights as clients forces both visible.
    for (const auto& s_Light : { m_KeyLight, m_FillLight }) {
        if (s_Light) {
            // AddClient takes ownership of the ref and clears it, so give it a copy.
            ZEntityRef s_Client = s_Light;
            m_RT.m_pInterfaceRef->AddClient(s_Client);
        }
    }

    m_RT.m_entityRef.SetProperty("m_bVisible", true);
    m_Camera.m_entityRef.SetProperty("m_bVisible", true);
}

void AssetPreview::Destroy() {
    if (!m_Rig) {
        return;
    }

    StopForceRenderingRT();
    m_KeyLight = {};
    m_FillLight = {};
    m_Camera = {};
    m_RT = {};
    SDK()->DestroyImGuiTextureSRV(m_RTTexture);
    Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, m_Rig, {});
    m_Rig = {};
    m_RigFactory.reset();
    m_RigBlueprintFactory.reset();
}

bool AssetPreview::IsReady() const {
    return m_Camera && m_RT;
}

ID3D12Resource* AssetPreview::GetRTResource() const {
    const auto s_RT = GetRenderDestination(m_RT);

    return s_RT && s_RT->m_pTexture2D ? s_RT->m_pTexture2D->m_pResource : nullptr;
}

const ImGuiTexture* AssetPreview::GetRTTexture() {
    if (!m_RTTexture.id) {
        if (const auto s_Resource = GetRTResource()) {
            SDK()->CreateImGuiTextureSRV(s_Resource, m_RTTexture);
        }
    }

    return m_RTTexture.id ? &m_RTTexture : nullptr;
}

bool AssetPreview::SpawnPreview(IEntityFactory* p_Factory, const std::string& p_ItemName) {
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene || !m_Camera || !p_Factory) {
        return false;
    }

    if (!CanSpawnTemplate(p_Factory, p_ItemName)) {
        return false;
    }

    m_LookAtTriggersBeforeSpawn = GetRegisteredLookAtTriggers();

    // Preview rooms and gates reorder the level's room indices on the next rebuild, which lights
    // the editor camera as another room until restart. Take them out before the rebuild.
    const auto s_RoomManager = *Globals::RoomManager;
    const size_t s_RoomCount = s_RoomManager->m_RoomEntities.size();
    const size_t s_ChangedGateCount = s_RoomManager->m_ChangedGates.size();
    const int32_t s_PendingRoomUpdate = s_RoomManager->m_nPendingUpdate;

    SExternalReferences s_ExternalRefs;
    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager,
        m_PreviewEntity,
        "AssetBrowser",
        p_Factory,
        s_Scene.m_entityRef,
        s_ExternalRefs,
        -1
    );

    const size_t s_AddedRooms = s_RoomManager->m_RoomEntities.size() - s_RoomCount;
    const size_t s_AddedGates = s_RoomManager->m_ChangedGates.size() - s_ChangedGateCount;

    for (size_t i = 0; i < s_AddedRooms; ++i) {
        s_RoomManager->m_RoomEntities.pop_back();
    }

    for (size_t i = 0; i < s_AddedGates; ++i) {
        s_RoomManager->m_ChangedGates.pop_back();
    }

    s_RoomManager->m_nPendingUpdate = s_PendingRoomUpdate;

    if (!m_PreviewEntity) {
        Logger::Error("[Editor] Failed to spawn asset {}.", p_ItemName);
        return false;
    }

    if (const auto s_SpatialEntity = m_PreviewEntity.QueryInterface<ZSpatialEntity>()) {
        m_PreviewEntity.SetProperty("m_eRoomBehaviour", ZSpatialEntity::ERoomBehaviour::ROOM_DYNAMIC);

        SMatrix s_Transform;
        s_Transform.Trans = c_AssetPreviewPosition;
        s_SpatialEntity->SetObjectToWorldMatrixFromEditor(s_Transform);
    }

    // Lights report their whole volume as bounds and render materials report empty bounds.
    // Volume and global lights don't implement ILightEntity.
    static constexpr std::string_view c_UnframedTypes[] = {
        "ILightEntity",
        "IRenderMaterialEntity",
        "ZVolumeLightEntity",
        "ZGlobalLightEntity",
    };

    std::vector<STypeID*> s_UnframedTypeIds;

    for (const auto s_TypeName : c_UnframedTypes) {
        if (const auto s_TypeId = (*Globals::TypeRegistry)->GetTypeID(s_TypeName)) {
            s_UnframedTypeIds.push_back(s_TypeId);
        }
    }

    m_PreviewTree = CollectSpawnedTree(m_PreviewEntity);
    m_TreeDeleter.OnTreeSpawned(m_PreviewTree);

    for (const auto s_EntityObj : m_PreviewTree.Entities) {
        ZEntityRef s_Entity(s_EntityObj);

        // Stop physics props from falling.
        s_Entity.SetProperty("m_bKinematic", true);

        if (const auto s_Renderable = s_Entity.QueryInterface<ZRenderableEntity>()) {
            const bool s_IsFramed = std::ranges::none_of(s_UnframedTypeIds, [&](STypeID* p_TypeId) {
                return s_Entity.QueryInterface(p_TypeId) != nullptr;
            });

            m_PreviewRenderables.push_back({ s_Entity.QueryInterface<ZSpatialEntity>(), s_Renderable, s_IsFramed });
        }
    }

    // A root node makes the camera skip room and gate traversal, so it can see the off-map
    // preview. The TraverseSimple detour limits what it draws.
    m_Camera.m_entityRef.SetProperty("m_pRootNode", m_PreviewEntity);

    return true;
}

void AssetPreview::DestroyPreview() {
    if (!m_PreviewEntity) {
        return;
    }

    if (m_Camera) {
        m_Camera.m_entityRef.SetProperty("m_pRootNode", ZEntityRef());
    }

    m_TreeDeleter.Delete(m_PreviewEntity, m_PreviewTree);
    m_PreviewEntity = {};
    m_PreviewTree = {};

    // The game never deletes look-at triggers the tree walk doesn't reach, so unregister every
    // new one, except those of trees spawned in the level meanwhile.
    UnregisterLookAtTriggersIf([&](ZEntityType** p_Trigger) {
        return !m_LookAtTriggersBeforeSpawn.contains(p_Trigger) && !m_TreeDeleter.IsInSpawnedTree(p_Trigger);
    });

    m_PreviewRenderables.clear();

    {
        std::scoped_lock s_Lock(m_PreviewNodeIndicesMutex);
        m_PreviewNodeIndices.clear();
    }

    m_FramedMin = {};
    m_FramedMax = {};
}

bool AssetPreview::HasRenderables() const {
    return std::ranges::any_of(m_PreviewRenderables, &PreviewRenderable::IsFramed);
}

bool AssetPreview::IsPreviewInUse() const {
    return IsTreeInUse(m_PreviewTree);
}

static SVector3 Min(const SVector3& a, const SVector3& b) {
    return SVector3((std::min)(a.x, b.x), (std::min)(a.y, b.y), (std::min)(a.z, b.z));
}

static SVector3 Max(const SVector3& a, const SVector3& b) {
    return SVector3((std::max)(a.x, b.x), (std::max)(a.y, b.y), (std::max)(a.z, b.z));
}

bool AssetPreview::GetPreviewBounds(SVector3& p_Min, SVector3& p_Max) const {
    struct WorldBounds {
        SVector3 Min;
        SVector3 Max;
    };

    std::vector<WorldBounds> s_Bounds;

    for (const auto& s_Renderable : m_PreviewRenderables) {
        if (!s_Renderable.Spatial || !s_Renderable.IsFramed) {
            continue;
        }

        float4 s_LocalMin, s_LocalMax;
        s_Renderable.Spatial->CalculateBounds(s_LocalMin, s_LocalMax, 1, 0);

        // Empty bounds pull the framing towards the entity, often at the world origin.
        const bool s_IsEmpty =
            (s_LocalMax.x <= s_LocalMin.x && s_LocalMax.y <= s_LocalMin.y && s_LocalMax.z <= s_LocalMin.z) ||
            s_LocalMax.x < s_LocalMin.x || s_LocalMax.y < s_LocalMin.y || s_LocalMax.z < s_LocalMin.z;

        if (s_IsEmpty) {
            continue;
        }

        const auto s_Transform = s_Renderable.Spatial->GetObjectToWorldMatrix();
        auto& s_World = s_Bounds.emplace_back();

        for (int i = 0; i < 8; ++i) {
            const float4 s_Corner(
                (i & 1) ? s_LocalMax.x : s_LocalMin.x,
                (i & 2) ? s_LocalMax.y : s_LocalMin.y,
                (i & 4) ? s_LocalMax.z : s_LocalMin.z,
                1.f
            );

            const SVector3 s_Point = s_Transform.WVectorTransform(s_Corner);

            if (i == 0) {
                s_World = { s_Point, s_Point };
                continue;
            }

            s_World.Min = Min(s_World.Min, s_Point);
            s_World.Max = Max(s_World.Max, s_Point);
        }
    }

    if (s_Bounds.empty()) {
        return false;
    }

    const auto s_Diagonal = [](const WorldBounds& p_Bounds) {
        return (p_Bounds.Max - p_Bounds.Min).Length();
    };

    const auto s_Center = [](const WorldBounds& p_Bounds) {
        return (p_Bounds.Min + p_Bounds.Max) * 0.5f;
    };

    const auto& s_Largest = *std::ranges::max_element(s_Bounds, {}, s_Diagonal);
    const float s_LargestDiagonal = s_Diagonal(s_Largest);

    p_Min = s_Largest.Min;
    p_Max = s_Largest.Max;

    for (const auto& s_World : s_Bounds) {
        // Small parts on bones the preview doesn't pose, like jewelry, can end up metres away
        // and would shrink the item to a dot.
        const bool s_IsStrayPart =
            s_Diagonal(s_World) < s_LargestDiagonal * 0.25f &&
            (s_Center(s_World) - s_Center(s_Largest)).Length() > s_LargestDiagonal;

        if (s_IsStrayPart) {
            continue;
        }

        p_Min = Min(p_Min, s_World.Min);
        p_Max = Max(p_Max, s_World.Max);
    }

    return true;
}

// Renderables get their index a few frames after spawning, so we check every frame.
void AssetPreview::PublishPreviewNodeIndices() {
    m_PreviewNodeIndicesScratch.clear();

    const auto s_AddRenderable = [&](const ZRenderableEntity* p_Renderable) {
        if (p_Renderable && p_Renderable->m_nRenderBaseIndex != static_cast<uint32_t>(-1)) {
            m_PreviewNodeIndicesScratch.push_back(p_Renderable->m_nRenderBaseIndex);
        }
    };

    s_AddRenderable(m_KeyLight.QueryInterface<ZRenderableEntity>());
    s_AddRenderable(m_FillLight.QueryInterface<ZRenderableEntity>());

    for (const auto& s_Renderable : m_PreviewRenderables) {
        s_AddRenderable(s_Renderable.Renderable);
    }

    // Only this thread writes m_PreviewNodeIndices, so it can be compared without the lock.
    if (m_PreviewNodeIndices == m_PreviewNodeIndicesScratch) {
        return;
    }

    std::scoped_lock s_Lock(m_PreviewNodeIndicesMutex);
    m_PreviewNodeIndices = m_PreviewNodeIndicesScratch;
}

// Parts of an item can report their bounds a few frames after spawning, so re-frame whenever
// they change.
bool AssetPreview::FramePreview(const float p_Yaw) {
    PublishPreviewNodeIndices();

    if (!m_PreviewEntity) {
        return false;
    }

    SVector3 s_Min, s_Max;

    if (!GetPreviewBounds(s_Min, s_Max)) {
        return false;
    }

    const auto s_IsSame = [](const SVector3& a, const SVector3& b) {
        constexpr float c_Epsilon = 0.0001f;
        return std::abs(a.x - b.x) < c_Epsilon && std::abs(a.y - b.y) < c_Epsilon && std::abs(a.z - b.z) < c_Epsilon;
    };

    const bool s_BoundsChanged =
        !s_IsSame(s_Min, m_FramedMin) || !s_IsSame(s_Max, m_FramedMax);

    if (!s_BoundsChanged && p_Yaw == m_FramedYaw) {
        return false;
    }

    const SVector3 s_Center = (s_Min + s_Max) * 0.5f;
    const float s_Radius = ((s_Max - s_Min) * 0.5f).Length();

    if (s_Radius < 0.001f) {
        return false;
    }

    m_FramedMin = s_Min;
    m_FramedMax = s_Max;
    m_FramedYaw = p_Yaw;

    const float s_YawSin = std::sin(p_Yaw);
    const float s_YawCos = std::cos(p_Yaw);

    const auto s_Orbit = [&](const SVector3& p_Vector) {
        return SVector3(
            p_Vector.x * s_YawCos - p_Vector.y * s_YawSin,
            p_Vector.x * s_YawSin + p_Vector.y * s_YawCos,
            p_Vector.z
        );
    };

    // Look at the item along +Y, pitched down by about 27 degrees.
    const SVector3 s_Forward = s_Orbit(SVector3(0.f, 0.894427f, -0.447214f));
    const SVector3 s_Right = s_Forward.Cross(SVector3(0.f, 0.f, 1.f)).Normalized();
    const SVector3 s_Up = s_Right.Cross(s_Forward);

    // The RT is square, so the FOV is the same on both axes.
    constexpr float c_FovYDeg = 35.f;
    constexpr float c_Margin = 1.1f;
    const float s_TanHalfFov = std::tan(c_FovYDeg * 0.5f * std::numbers::pi_v<float> / 180.f) / c_Margin;

    float s_Distance = 0.f;

    for (int i = 0; i < 8; ++i) {
        const SVector3 s_Corner(
            (i & 1) ? s_Max.x : s_Min.x,
            (i & 2) ? s_Max.y : s_Min.y,
            (i & 4) ? s_Max.z : s_Min.z
        );

        const SVector3 s_Offset = s_Corner - s_Center;
        const float s_Extent = (std::max)(std::abs(s_Offset.Dot(s_Right)), std::abs(s_Offset.Dot(s_Up)));

        s_Distance = (std::max)(s_Distance, s_Extent / s_TanHalfFov - s_Offset.Dot(s_Forward));
    }

    m_Camera.m_pInterfaceRef->SetObjectToWorldMatrixFromEditor(LookAt(s_Center - s_Forward * s_Distance, s_Center));

    if (s_BoundsChanged) {
        m_Camera.m_entityRef.SetProperty("m_fNearZ", (std::max)(0.01f, s_Distance - s_Radius * 1.5f));
        m_Camera.m_entityRef.SetProperty("m_fFarZ", s_Distance + s_Radius * 2.f + 1.f);
    }

    const auto s_PlaceLight = [&](ZEntityRef p_Light, const SVector3& p_Offset) {
        if (!p_Light) {
            return;
        }

        if (const auto s_LightSpatial = p_Light.QueryInterface<ZSpatialEntity>()) {
            s_LightSpatial->SetObjectToWorldMatrixFromEditor(LookAt(s_Center + s_Orbit(p_Offset) * s_Distance, s_Center));
        }

        if (s_BoundsChanged) {
            p_Light.SetProperty("m_vFarAttenuation", SVector2(s_Distance * 2.f, s_Distance * 4.f));
        }
    };

    s_PlaceLight(m_KeyLight, SVector3(-0.5f, -0.8f, 0.9f));
    s_PlaceLight(m_FillLight, SVector3(0.9f, -0.4f, 0.1f));

    return s_BoundsChanged;
}

// An offscreen RT only renders when one of its clients is visible to the main camera, so
// force the client nodes visible.
bool AssetPreview::ForceRenderRT() {
    if (!m_ForcedNodes.empty()) {
        ++m_ForcedRenderFrameCount;
        return true;
    }

    const auto s_NodeIndex = m_RT.m_pInterfaceRef->m_nRenderBaseIndex;

    if (s_NodeIndex == static_cast<uint32>(-1)) {
        return false;
    }

    const auto s_Node = (*Globals::RenderGraphNodes)[s_NodeIndex].m_pNode;

    if (!s_Node || s_Node->m_nType != ZRenderGraphNode::DESTINATION) {
        return false;
    }

    const auto s_DestinationNode = static_cast<ZRenderGraphNodeDestination*>(s_Node);

    for (const auto s_ClientNode : s_DestinationNode->m_aClientNodes) {
        Globals::RenderGraphManager->m_aForcedVisibleNodes.push_back(s_ClientNode);
        m_ForcedNodes.push_back(s_ClientNode);
    }

    if (m_ForcedNodes.empty()) {
        return false;
    }

    m_CameraNodeIndex = m_Camera.m_pInterfaceRef->m_nRenderBaseIndex;

    return true;
}

int AssetPreview::GetForcedRenderFrameCount() const {
    return m_ForcedRenderFrameCount;
}

void AssetPreview::StopForceRenderingRT() {
    m_CameraNodeIndex = -1;
    m_ForcedRenderFrameCount = 0;

    if (m_ForcedNodes.empty()) {
        return;
    }

    auto& s_ForcedNodes = Globals::RenderGraphManager->m_aForcedVisibleNodes;

    std::vector<ZRenderGraphNode*> s_OtherNodes;

    for (const auto s_Node : s_ForcedNodes) {
        if (!std::ranges::contains(m_ForcedNodes, s_Node)) {
            s_OtherNodes.push_back(s_Node);
        }
    }

    s_ForcedNodes.clear();

    for (const auto s_Node : s_OtherNodes) {
        s_ForcedNodes.push_back(s_Node);
    }

    m_ForcedNodes.clear();
}

// Traversal and shading both run inside ZRenderGraphNodeCamera::Traverse on this thread.
static thread_local ZRenderGraphNodeCamera* s_TraversingEditorCamera = nullptr;

DEFINE_DETOUR_WITH_CONTEXT(
    AssetPreview, void, ZRenderGraphNodeCamera_Traverse,
    ZRenderGraphNodeCamera* th, ZRenderGraphNode* pParent, ZRenderTraversalContext* pTraversalContext,
    ZRenderContext* pRenderContext, uint64_t a5
) {
    const uint32_t s_CameraNodeIndex = m_CameraNodeIndex;

    if (s_CameraNodeIndex == static_cast<uint32_t>(-1) ||
        reinterpret_cast<void*>((*Globals::RenderGraphNodes)[s_CameraNodeIndex].m_pNode) != th) {
        return HookResult<void>(HookAction::Continue());
    }

    s_TraversingEditorCamera = th;
    p_Hook->CallOriginal(th, pParent, pTraversalContext, pRenderContext, a5);
    s_TraversingEditorCamera = nullptr;

    return HookResult<void>(HookAction::Return());
}

// With a root node set, the editor camera frustum-tests every base node in the scene, so
// replace that set with only the preview's nodes and our lights.
DEFINE_DETOUR_WITH_CONTEXT(
    AssetPreview, void, ZRenderGraphTraversal_TraverseSimple,
    ZRenderTraversalContext* pTraversalContext, uint32_t nSkipMask, const ZSparseBitArray& Nodes
) {
    if (!s_TraversingEditorCamera || s_TraversingEditorCamera->m_RootNodes.size() == 0) {
        return HookResult<void>(HookAction::Continue());
    }

    // Same layout as the game's: one stencil bit per 128 nodes, then one bit per node.
    // The size must match the original, because the traversal sizes its outputs from it.
    const uint32_t s_Size = Nodes.m_nSize;
    const uint32_t s_StencilWordCount = (s_Size >> 13) + 1;

    m_NodeSetBits.assign(s_StencilWordCount + (s_Size + 63) / 64, 0);

    ZSparseBitArray s_PreviewNodes {};
    s_PreviewNodes.m_pStencil = m_NodeSetBits.data();
    s_PreviewNodes.m_pMask = m_NodeSetBits.data() + s_StencilWordCount;
    s_PreviewNodes.m_nSize = s_Size;
    s_PreviewNodes.m_nStencilSize = s_Size >> 7;
    s_PreviewNodes.m_nAllocatedSize = s_Size;

    // Our indices come from the game thread and can refer to nodes the render thread already
    // freed, so only take nodes the game's own set contains.
    {
        std::scoped_lock s_Lock(m_PreviewNodeIndicesMutex);

        for (const auto s_NodeIndex : m_PreviewNodeIndices) {
            if (s_NodeIndex < s_Size && Nodes.Test(s_NodeIndex)) {
                s_PreviewNodes.Set(s_NodeIndex);
            }
        }
    }

    p_Hook->CallOriginal(pTraversalContext, nSkipMask, s_PreviewNodes);

    return HookResult<void>(HookAction::Return());
}

// Empty pixels get the level's sky cubemap instead of m_vClearColor when the level has a sun
// and an atmosphere. Turn that off for the editor camera.
DEFINE_DETOUR_WITH_CONTEXT(
    AssetPreview, void, CLightTile_DispatchShadeDeferred,
    CLightTile* th, ZRenderContext* pRenderContext, ZRenderDeviceContext* pDeviceContext, void* a4, void* a5,
    void* a6, void* a7, void* a8, void* a9, void* a10, void* a11, void* a12, void* a13, void* a14, int32_t a15,
    void* a16, const float4* vClearColor, bool bDisableLighting, bool bMergeDiffuse,
    int32_t nSelectedBoxReflectionId, bool bDisableBoxReflections, bool bEnableAtmosphericScatteringBackdrop,
    bool bIsBoxReflectionPass, int32_t a24
) {
    if (!s_TraversingEditorCamera) {
        return HookResult<void>(HookAction::Continue());
    }

    p_Hook->CallOriginal(
        th, pRenderContext, pDeviceContext, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16,
        vClearColor, bDisableLighting, bMergeDiffuse, nSelectedBoxReflectionId, bDisableBoxReflections, false,
        bIsBoxReflectionPass, a24
    );

    return HookResult<void>(HookAction::Return());
}

// For a room that isn't registered, like a preview room, the game removes the last room
// instead and writes past the end of the list.
DEFINE_DETOUR_WITH_CONTEXT(AssetPreview, void, ZRoomManager_UnregisterRoom, ZRoomManager* th, ZRoomEntity* pRoom) {
    if (std::ranges::find(th->m_RoomEntities, pRoom) == th->m_RoomEntities.end()) {
        return HookResult<void>(HookAction::Return());
    }

    return HookResult<void>(HookAction::Continue());
}
