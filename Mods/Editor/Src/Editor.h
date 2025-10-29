#pragma once

#include <WinSock2.h>

#include <random>
#include <unordered_map>
#include <map>
#include <shared_mutex>

#include "IPluginInterface.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZInput.h"
#include "Glacier/ZFreeCamera.h"
#include "Glacier/EDebugChannel.h"

#include "ImGuizmo.h"
#include "EditorServer.h"
#include "EntityTreeNode.h"

struct QneTransform {
    SVector3 Position;
    SVector3 Rotation;
    SVector3 Scale;
};

struct AlignedDeleter {
    template <typename T>
    void operator()(T* ptr) const {
        if (ptr)
            (*Globals::MemoryManager)->m_pNormalAllocator->Free(ptr);
    }
};

class Editor : public IPluginInterface {
public:
    Editor();
    ~Editor() override;

    void Init() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;
    void OnDraw3D(IRenderer* p_Renderer) override;
    void OnDepthDraw3D(IRenderer* p_Renderer) override;
    void OnEngineInitialized() override;

public:
    void SelectEntity(EntitySelector p_Selector, std::optional<std::string> p_ClientId);
    void SetEntityTransform(
        EntitySelector p_Selector, SMatrix p_Transform, bool p_Relative, std::optional<std::string> p_ClientId
    );
    void SpawnQnEntity(
        const std::string& p_QnJson, uint64_t p_EntityId, std::string p_Name, std::optional<std::string> p_ClientId
    );
    void CreateEntityResources(const std::string& p_QnJson, std::optional<std::string> p_ClientId);
    void DestroyEntity(EntitySelector p_Selector, std::optional<std::string> p_ClientId);
    void SetEntityName(EntitySelector p_Selector, std::string p_Name, std::optional<std::string> p_ClientId);
    void SetEntityProperty(
        EntitySelector p_Selector, uint32_t p_PropertyId, std::string_view p_JsonValue,
        std::optional<std::string> p_ClientId
    );
    void SignalEntityPin(EntitySelector p_Selector, uint32_t p_PinId, bool p_Output);
    void LockEntityTree() { m_CachedEntityTreeMutex.lock_shared(); }
    std::shared_ptr<EntityTreeNode> GetEntityTree() { return m_CachedEntityTree; }
    void UnlockEntityTree() { m_CachedEntityTreeMutex.unlock_shared(); }
    ZEntityRef FindEntity(EntitySelector p_Selector);
    static std::string GetCollisionHash(auto p_SelectedEntity);
    void FindAlocs(
        const std::function<void(std::vector<std::tuple<std::vector<std::string>, Quat, ZEntityRef>>&, bool p_Done)>&
        p_SendEntitiesCallback, const std::function<void()>& p_RebuiltCallback
    );
    std::vector<std::tuple<std::vector<std::string>, Quat, ZEntityRef>> FindEntitiesByType(
        const std::string& p_EntityType, const std::string& p_Hash
    );
    void RebuildEntityTree();
    void LoadNavpAreas(simdjson::ondemand::array p_NavpAreas, int p_ChunkIndex);
    static QneTransform MatrixToQneTransform(const SMatrix& p_Matrix);

private:
    struct DebugEntity {
        std::string m_TypeName;
        ZEntityRef m_EntityRef;
        EDebugChannel m_DebugChannel;
        bool m_HasGizmo;
    };

    struct GizmoEntity : DebugEntity {
        ZRuntimeResourceID m_RuntimeResourceID;
        ZResourcePtr m_PrimResourcePtr;
        SVector4 m_Color;
        SMatrix m_Transform;
    };

    void SpawnCameras();
    void ActivateCamera(ZEntityRef* m_CameraEntity);
    void DeactivateCamera();
    void CopyToClipboard(const std::string& p_String) const;
    void OnMouseDown(SVector2 p_Pos, bool p_FirstClick);

    void DrawEntityManipulator(bool p_HasFocus);
    void DrawEntityAABB(IRenderer* p_Renderer);

    void DrawEntityProperties();

    void DrawLibrary();

    void DrawSettings(bool p_HasFocus);

    void RenderEntity(std::shared_ptr<EntityTreeNode> p_Node);
    void DrawEntityTree();
    void FilterEntityTree();
    bool FilterEntityTree(EntityTreeNode* p_Node);
    void UpdateEntities();
    void UpdateEntityTree(
        std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>>& p_NodeMap,
        const std::vector<ZEntityRef>& p_Entities
    );

    void OnSelectEntity(ZEntityRef p_Entity, std::optional<std::string> p_ClientId);
    void OnDestroyEntity(ZEntityRef p_Entity, std::optional<std::string> p_ClientId);
    void DestroyEntityInternal(ZEntityRef p_Entity, std::optional<std::string> p_ClientId);
    void OnEntityTransformChange(
        ZEntityRef p_Entity, SMatrix p_Transform, bool p_Relative, std::optional<std::string> p_ClientId
    );
    void OnEntityNameChange(ZEntityRef p_Entity, const std::string& p_Name, std::optional<std::string> p_ClientId);
    void OnSetPropertyValue(
        ZEntityRef p_Entity, uint32_t p_PropertyId, const ZObjectRef& p_Value, std::optional<std::string> p_ClientId
    );
    void OnSignalEntityPin(ZEntityRef p_Entity, const std::string& p_Pin, bool p_Output);
    void OnSignalEntityPin(ZEntityRef p_Entity, uint32_t p_PinId, bool p_Output);

    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

    void DrawPinTracer();

    static bool ImGuiCopyWidget(const std::string& p_Id);

    static void ToggleEditorServerEnabled();
    static void FindAlocForZGeomEntityNode(
        std::vector<std::tuple<std::vector<std::string>, Quat, ZEntityRef>>& p_Entities,
        const std::shared_ptr<EntityTreeNode>& p_Node, const TArray<ZEntityInterface>& p_Interfaces, char*& p_EntityType
    );
    static void FindAlocForZPrimitiveProxyEntityNode(
        std::vector<std::tuple<std::vector<std::string>, Quat, ZEntityRef>>& entities,
        const std::shared_ptr<EntityTreeNode>& s_Node, const TArray<ZEntityInterface>& s_Interfaces, char*& s_EntityType
    );

    // Properties
    void UnsupportedProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void ZEntityRefProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void TEntityRefProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void ZRepositoryIDProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);

    // Primitive properties.
    void StringProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void BoolProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void Uint8Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void Uint16Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void Uint32Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void Uint64Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void Int8Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void Int16Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void Int32Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void Int64Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void Float32Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void Float64Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void EnumProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);

    // Vector properties.
    void SVector2Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void SVector3Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void SVector4Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);

    void SMatrix43Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);

    template <typename T>
    static std::unique_ptr<T, AlignedDeleter> GetProperty(ZEntityRef p_Entity, const ZEntityProperty* p_Property);
    static Quat GetQuatFromProperty(ZEntityRef p_Entity);
    static Quat GetParentQuat(ZEntityRef p_Entity);

    void SColorRGBProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);
    void SColorRGBAProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);

    void ResourceProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data);

    static SMatrix QneTransformToMatrix(const QneTransform& p_Transform);

    void DrawItems(bool p_HasFocus);
    void DrawActors(bool p_HasFocus);
    void DrawDebugChannels(bool p_HasFocus);

    static void EquipOutfit(
        const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit, uint8_t P_CharSetIndex,
        const std::string& P_CharSetCharacterType, uint8_t P_OutfitVariationindex, ZActor* p_Actor
    );

    void EnableTrackCam();
    void UpdateTrackCam() const;
    void DisableTrackCam();
    void GetPlayerCam();
    void GetTrackCam();
    void GetRenderDest();
    static void SetPlayerControlActive(bool active);

    void InitializeDebugChannels();
    void InitializeDebugEntityTypeIDs();
    void DrawDebugEntities(IRenderer* p_Renderer);
    void DrawGizmo(GizmoEntity& p_GizmoEntity, IRenderer* p_Renderer);
    void DrawShapes(const DebugEntity& p_DebugEntity, IRenderer* p_Renderer);
    void GetDebugEntities(const std::shared_ptr<EntityTreeNode>& p_EntityTreeNode);
    void AddDebugEntity(
        const ZEntityRef p_EntityRef,
        const std::string& p_TypeName,
        const EDebugChannel p_DebugChannel
    );
    void AddGizmoEntity(
        const ZEntityRef p_EntityRef,
        const std::string& p_TypeName,
        const EDebugChannel p_DebugChannel,
        const uint64_t p_RuntimeResourceID,
        const SVector4& p_Color = SVector4(1.f, 1.f, 1.f, 1.f),
        const SMatrix& p_Transform = SMatrix()
    );
    void AddGizmoEntity(
        const ZEntityRef p_EntityRef,
        const std::string& p_TypeName,
        const EDebugChannel p_DebugChannel,
        const std::string& p_PropertyName,
        const SVector4& p_Color = SVector4(1.f, 1.f, 1.f, 1.f),
        const SMatrix& p_Transform = SMatrix()
    );
    EDebugChannel ConvertDrawLayerToDebugChannel(const ZDebugGizmoEntity_EDrawLayer p_DrawLayer);
    static bool EntityIDMatches(void* p_Interface, const uint64 p_EntityID);
    bool RayCastGizmos(const SVector3& p_WorldPosition, const SVector3& p_Direction);

private:
    DECLARE_PLUGIN_DETOUR(Editor, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);
    DECLARE_PLUGIN_DETOUR(Editor, void, OnClearScene, ZEntitySceneContext* th, bool forReload);
    DECLARE_PLUGIN_DETOUR(
        Editor, ZTemplateEntityBlueprintFactory*, ZTemplateEntityBlueprintFactory_ctor,
        ZTemplateEntityBlueprintFactory* th, STemplateEntityBlueprint* pTemplateEntityBlueprint,
        ZResourcePending& ResourcePending
    );
    DECLARE_PLUGIN_DETOUR(Editor, bool, OnInputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data);
    DECLARE_PLUGIN_DETOUR(Editor, bool, OnOutputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data);

private:
    enum class EntityHighlightMode {
        Lines,
        LinesAndTriangles
    };

    enum DebugEntityTypeName {
        CoverPlane,
        GuideLadder,
        GuideWindow,
        PFBoxEntity,
        PFObstacleEntity,
        PFSeedPoint,
        DebugGizmoEntity,
        PureWaveModifierEntity,
        LightEntity,
        DarkLightEntity,
        BoxReflectionEntity,
        CubemapProbeEntity,
        VolumeLightEntity,
        ParticleEmitterBoxEntity,
        ParticleEmitterEmitterEntity,
        ParticleEmitterMeshEntity,
        ParticleEmitterPointEntity,
        ParticleGlobalAttractorEntity,
        ParticleKillVolumeEntity,
        GateEntity,
        OccluderEntity,
        DecalsSpawnEntity,
        StaticDecalEntity,
        LiquidTrailEntity,
        CrowdActorGroupEntity,
        CrowdActorGroupFocalPointEntity,
        CrowdEntity,
        ManualActorEntity,
        SplineCrowdFlowEntity,
        BoxShapeAspect,
        CapsuleShapeAspect,
        SphereShapeAspect,
        WindEntity,
        AISoundConnector,
        AISoundConnectorTarget,
        AISoundModifierVolume,
        AIVisionBlockerPlane,
        AgitatedGuardPointEntity,
        AgitatedWaypointEntity,
        CombatActEntity,
        LookAtEntity,
        OutfitProviderEntity,
        PointOfInterestEntity,
        ActBehaviorEntity,
        SpawnPointEntity,
        BystanderPointEntity,
        WaypointEntity,
        PatrolBehaviorEntity,
        PostfilterAreaBoxEntity,
        FogBoxEntity,
        BoxVolumeEntity,
        SphereVolumeEntity,
        VolumeShapeEntity,
        CameraEntity,
        OrientationEntity,
        ScatterContainerEntity,
        TrailShapeEntity,
        SplineEntity,
        SplineControlPointEntity,
        AudioEmitterSpatialAspect,
        AudioEmitterVolumetricAspect,
        ClothWireEntity,
        Count
    };

    bool m_raycastLogging; // Mainly used for the raycasting logs

    bool m_CameraActive = false;
    ZEntityRef m_OriginalCam;

    ZSelectionForFreeCameraEditorStyleEntity* m_SelectionForFreeCameraEditorStyleEntity = nullptr;

    bool m_HoldingMouse = false;
    bool m_UseSnap = false;
    bool m_UseAngleSnap = false;
    bool m_UseScaleSnap = false;
    bool m_UseQneTransforms = false;
    double m_SnapValue = 1.0;
    double m_AngleSnapValue = 90.0;
    double m_ScaleSnapValue = 1.0;

    bool m_SettingsVisible = false;
    bool m_EditorWindowsVisible = true;

    float4 m_From;
    float4 m_To;
    float4 m_Hit;
    float4 m_Normal;

    size_t m_SelectedBrickIndex = 0;
    ZEntityRef m_SelectedEntity;
    bool m_ShouldScrollToEntity = false;

    EntityHighlightMode m_EntityHighlightMode = EntityHighlightMode::Lines;

    std::string m_EntityIdSearchInput;
    std::string m_EntityTypeSearchInput;
    std::string m_EntityNameSearchInput;
    std::unordered_set<EntityTreeNode*> m_FilteredEntityTreeNodes;
    std::vector<EntityTreeNode*> m_DirectEntityTreeNodeMatches;

    std::vector<std::vector<SVector3>> m_NavpAreas;

    ImGuizmo::OPERATION m_GizmoMode = ImGuizmo::OPERATION::TRANSLATE;
    ImGuizmo::MODE m_GizmoSpace = ImGuizmo::MODE::WORLD;

    ZInputAction m_RaycastAction = "SkipLoadingAction"; // space
    ZInputAction m_DeleteAction = "TemporaryCamSpeedMultiplier0"; // shift

    struct PinFireInfo {
        std::chrono::time_point<std::chrono::system_clock> m_FireTime;
    };

    std::unordered_map<uint32_t, PinFireInfo> m_FiredInputPins = {};
    std::unordered_map<uint32_t, PinFireInfo> m_FiredOutputPins = {};

    SOCKET m_QneSocket = INVALID_SOCKET;
    bool m_ConnectedToQne = false;
    float m_QneConnectionTimer = 999.f; // Set to a high number so we connect on startup.
    sockaddr_in m_QneAddress = {};

    std::shared_mutex m_CachedEntityTreeMutex;
    std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>> m_CachedEntityTreeMap;
    std::shared_ptr<EntityTreeNode> m_CachedEntityTree;

    std::unordered_map<uint64_t, ZEntityRef> m_SpawnedEntities;
    std::unordered_map<ZEntityRef, std::string> m_EntityNames;

    std::mutex m_EntityDestructionMutex;
    std::vector<std::tuple<ZEntityRef, std::optional<std::string>>> m_EntitiesToDestroy;

    EditorServer m_Server;

    bool m_ItemsMenuActive = false;
    bool m_ActorsMenuActive = false;
    bool m_DebugChannelsMenuActive = false;

    ZActor* m_CurrentlySelectedActor = nullptr;
    const std::vector<std::string> m_CharSetCharacterTypes = {"Actor", "Nude", "HeroA"};

    ZActor* m_ActorTracked = nullptr;
    bool m_TrackCamActive = false;
    ZEntityRef m_PlayerCam = nullptr;
    TEntityRef<ZCameraEntity> m_TrackCam {};
    TEntityRef<IRenderDestinationEntity> m_RenderDest {};

    std::vector<std::unique_ptr<DebugEntity>> m_DebugEntities;
    std::vector<std::pair<std::string, EDebugChannel>> m_DebugChannels;
    std::unordered_map<std::string, std::vector<std::string>> m_DebugChannelNameToTypeNames;
    std::unordered_map<EDebugChannel, uint32> m_DebugChannelToDebugEntityCount;
    std::unordered_map<EDebugChannel, std::unordered_map<std::string, uint32_t>>
    m_DebugChannelToTypeNameToDebugEntityCount;
    std::unordered_map<EDebugChannel, bool> m_DebugChannelToVisibility;
    std::unordered_map<EDebugChannel, std::unordered_map<std::string, bool>> m_DebugChannelToTypeNameToVisibility;
    std::vector<STypeID*> m_DebugEntityTypeIds;
    bool m_DrawGizmos = true;
    bool m_DrawAllGizmos = false;
    bool m_DrawShapes = false;
    GizmoEntity* m_SelectedGizmoEntity = nullptr;

    bool m_DrawCoverInvalidOnNPCErrors = true;
    bool m_DrawHeroGuidesSolid = false;
    bool m_AlwaysDrawDebugBoxForColiValidityCheck = false;
    bool m_DrawPushDebug = false;
    bool m_DrawSafeZones = true;

    ZEntityRef m_EditorData {};
    TEntityRef<ZCameraEntity> m_EditorCamera {};
    TEntityRef<ZRenderDestinationTextureEntity> m_EditorCameraRT {};
};

DECLARE_ZHM_PLUGIN(Editor)
