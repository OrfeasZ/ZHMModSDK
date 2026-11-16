#pragma once

#include <WinSock2.h>

#include <random>
#include <unordered_map>
#include <map>
#include <atomic>
#include <deque>
#include <list>
#include <shared_mutex>

#include <SimpleMath.h>

#include <directx/d3dx12.h>

#include <DirectXTex.h>

#include "Glacier/ZEntity.h"
#include "Glacier/ZInput.h"
#include "Glacier/ZFreeCamera.h"
#include "Glacier/EDebugChannel.h"
#include "Glacier/ZCurve.h"
#include "Glacier/ZAction.h"
#include "Glacier/ZSound.h"
#include "Glacier/ZSequence.h"
#include "Glacier/ZCover.h"
#include "Glacier/ZCollision.h"

#include "IPluginInterface.h"
#include "IconsMaterialDesign.h"
#include "ImGuizmo.h"
#include "EditorServer.h"
#include "EntityTreeNode.h"
#include "NavKit.h"
#include "Components/AssetBrowser.h"
#include "Components/SpawnSafety.h"
#include "Properties/Curve.h"
#include "Widgets.h"

struct QneTransform {
    SVector3 Position;
    SVector3 Rotation;
    SVector3 Scale;
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
    std::string GetEntityName(ZEntityRef p_Entity, bool p_WithID = true);
    ZEntityRef FindEntity(EntitySelector p_Selector);
    static std::string GetCollisionHash(auto p_SelectedEntity);
    void FindMeshes(
        bool p_OnlyCollidableMeshes, const std::function<void(std::vector<NavKitMeshEntity>&, std::map<std::string,
            NavKitMatiTextures>&, std
            ::map<std::string, std::vector<std::string>>&, bool)>& p_SendEntitiesCallback, const std::function<void()>&
        p_RebuiltCallback
    );
    std::vector<std::tuple<std::vector<std::string>, Quat, ZEntityRef>> FindEntitiesByType(
        const std::string& p_EntityType, const std::string& p_Hash
    );
    void RebuildEntityTree();
    static QneTransform MatrixToQneTransform(const SMatrix& p_Matrix);
    static int32_t GetEnumValue(const IEnumType* p_Type, const void* p_Data);
    static void SetEnumValue(const IEnumType* p_Type, void* p_Data, int32_t p_Value);

    void QueueTask(std::function<void()> p_Task);

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

    struct RayCastResult {
        ZEntityRef m_Entity;
        std::string m_TypeName;
        EDebugChannel m_DebugChannel;
        float m_Distance = FLT_MAX;
    };

    struct PinInfo {
        std::string m_Name;
        std::string m_Description;
    };

    struct PinLists {
        std::vector<PinInfo> m_InputPins;
        std::vector<PinInfo> m_OutputPins;
    };

    struct PropertyDeleter {
        STypeID* m_Type = nullptr;

        template <typename T>
        void operator()(T* p_Data) const {
            if (!p_Data) {
                return;
            }

            if (m_Type) {
                if (const auto* s_TypeInfo = m_Type->GetTypeInfo()) {
                    if (s_TypeInfo->m_pTypeFunctions) {
                        s_TypeInfo->m_pTypeFunctions->destruct(p_Data);
                    }
                }
            }

            (*Globals::MemoryManager)->m_pNormalAllocator->Free(p_Data);
        }
    };

    void ActivateCamera(ZEntityRef* m_CameraEntity);
    void DeactivateCamera();
    void CopyToClipboard(const std::string& p_String) const;
    void OnMouseDown(SVector2 p_Pos, bool p_FirstClick);
    bool RayCastCollision(const SVector3& p_World, const SVector3& p_Direction, ZRayQueryOutput& p_Output);

    void DrawEntityManipulator(bool p_HasFocus);
    void DrawEntityAABB(IRenderer* p_Renderer);

    void DrawEntityPropertiesWindow();

    struct EntityDetails {
        bool HasTreeNode = false;
        std::string Name;
        std::string Id;
        std::string TypeName;

        std::vector<std::pair<std::string, std::string>> Rows;

        std::string ReferencedTemplateFactoryType;
        ZRuntimeResourceID ReferencedTemplateFactory;
    };

    bool DrawEntityToolbar(ZEntityRef p_Entity, ZSpatialEntity* p_Spatial);
    EntityDetails CollectEntityDetails(ZEntityRef p_Entity);
    void DrawEntityName(const EntityDetails& p_Details);
    void DrawEntityDetails(const EntityDetails& p_Details);
    void DrawEntityKeywords(ZEntityRef p_Entity);
    void DrawEntityPropertyTable(ZEntityRef p_Entity, const EntityDetails& p_Details);
    void DrawEntityTransform(const SMatrix& p_Transform);
    void DrawEntityPins(ZEntityRef p_Entity);

    struct RecentPinFire {
        bool m_IsOutput;
        std::string m_Name;
        ZObjectRef m_Value;
        double m_FireTime;
    };

    void FireEntityPin(ZEntityRef p_Entity, bool p_IsOutput, const std::string& p_Name, const ZObjectRef& p_Value);

    std::optional<RecentPinFire> DrawRecentPinFires(ZEntityRef p_Entity);

    enum class EPropertyRowButton {
        Copy,
        Remove,
        None
    };

    struct PropertyRowResult {
        bool m_Changed = false;
        bool m_RemoveClicked = false;
        bool m_IsNameHovered = false;
    };

    bool BeginPropertyTable(const char* p_Id, float p_NameColumnWidth);
    static void EndPropertyTable();
    static void BeginPropertyTableRow();

    struct PropertyTableBlock {
        ImVec2 m_Min;
        float m_MaxX;
        float m_ContentWidth;
    };

    static PropertyTableBlock BeginPropertyTableBlock();
    static void EndPropertyTableBlock(const PropertyTableBlock& p_Block);

    PropertyRowResult DrawPropertyRow(
        const std::string& p_Id,
        const std::string& p_Name,
        const STypeID* p_TypeID,
        ZEntityRef p_Entity,
        SPropertyData* p_Property,
        void* p_Data,
        EPropertyRowButton p_Button
    );

    bool DrawEntityPropertyValue(const std::string& p_Id, const STypeID* p_TypeID, void* p_Data);

    bool DrawArrayElements(
        const std::string& p_Id,
        ZEntityRef p_Entity,
        SPropertyData* p_Property,
        void* p_Data,
        const IArrayType* p_ArrayType
    );

    void DrawEntityPinValue(const std::string& p_Id, SMatrix43* p_Value);


    void DrawDockSpace();
    void DrawTransformToolbar(const ImVec2& p_GameViewPos);
    void DrawTransformToolbarHandle(const ImVec2& p_GameViewPos);
    void DrawSettingsWindow(bool p_HasFocus);

    bool HasVisibleChildren(const std::shared_ptr<EntityTreeNode>& p_Node) const;
    bool ShouldOpenForScroll(const EntityTreeNode& p_Node) const;
    void DrawSpawnedEntityContextMenu(const EntityTreeNode& p_Node, bool p_IsSpawnedEntitiesNode);
    void RenderEntity(std::shared_ptr<EntityTreeNode> p_Node);

    struct EntityTreeNodeName {
        std::string_view Name;
        std::string_view Id;

        std::string_view Marker;
    };

    static EntityTreeNodeName SplitEntityTreeNodeName(std::string_view p_NodeName);
    void DrawEntityTreeWindow();
    void DrawEntityTreeToolbar();
    void ExportEntityTree();
    void DrawEntityTreeTable();
    void FilterEntityTree();
    bool FilterEntityTree(EntityTreeNode* p_Node, EntityTreeNode*& p_OutSingleMatchNode);
    void ClearFilters();
    std::shared_ptr<EntityTreeNode> FindMatchByIndex(
        const std::shared_ptr<EntityTreeNode>& p_Node,
        size_t p_TargetIndex,
        size_t& p_CurrentCounter
    );
    void UpdateEntities();
    void UpdateEntityTree(
        std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>>& p_NodeMap,
        const std::vector<ZEntityRef>& p_Entities,
        const bool p_AreEntitiesDynamic
    );
    void AddDynamicEntitiesToEntityTree(
        const std::shared_ptr<EntityTreeNode>& p_SceneNode,
        std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>>& p_NodeMap
    );
    void ReparentDynamicOutfitEntities(
        std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>>& p_NodeMap
    );
    void ReparentSpawnedEntity(
        std::unordered_map<ZEntityRef, std::shared_ptr<EntityTreeNode>>& p_NodeMap, ZEntityRef p_Entity
    );
    bool IsSpecialEntityTreeNode(ZEntityRef p_Entity) const;

    struct SpawnedEntity {
        ZEntityRef Entity;

        std::optional<TResourcePtr<ZTemplateEntityFactory>> Factory;

        SpawnedTree Tree;

        int DeleteReadyFrames = 0;
    };

    struct AssetSpawnRequest {
        ZRuntimeResourceID ResourceId;
        std::string Path;
        std::string Name;
        SVector3 Position;
    };

    struct PendingAssetSpawn {
        AssetSpawnRequest Request;
        TResourcePtr<ZTemplateEntityFactory> Factory;
        int LoadFrames = 0;
    };

    void AddSpawnedEntity(SpawnedEntity p_SpawnedEntity, const std::string& p_Name, std::optional<std::string> p_ClientId);
    void OnAssetItemDropped(const AssetItem& p_Item, SVector2 p_ScreenPosition);
    void UpdateAssetSpawns();
    void SpawnAssetItem(const PendingAssetSpawn& p_Spawn);
    bool IsReadyToDestroy(ZEntityRef p_Entity);

    void OnSelectEntity(ZEntityRef p_Entity, bool p_ShouldScrollToEntity, std::optional<std::string> p_ClientId);
    void OnDestroyEntity(ZEntityRef p_Entity, std::optional<std::string> p_ClientId);
    void DestroyEntityInternal(ZEntityRef p_Entity, std::optional<std::string> p_ClientId);
    void DestroyEntityNodeInternal(const std::shared_ptr<EntityTreeNode>& p_NodeToRemove, std::optional<std::string> p_ClientId);
    void OnEntityTransformChange(
        ZEntityRef p_Entity, SMatrix p_Transform, bool p_Relative, std::optional<std::string> p_ClientId
    );
    void RefreshPathfinderObstacle(ZEntityRef p_Entity, bool p_Force);
    void RefreshStaticPhysicsAndObstacles(ZEntityRef p_Root);
    void OnEntityNameChange(ZEntityRef p_Entity, const std::string& p_Name, std::optional<std::string> p_ClientId);
    void OnSetPropertyValue(
        ZEntityRef p_Entity, uint32_t p_PropertyId, const ZObjectRef& p_Value, std::optional<std::string> p_ClientId
    );
    void OnSignalEntityPin(
        ZEntityRef p_Entity,
        const std::string& p_Pin,
        bool p_Output,
        const ZObjectRef& p_Data = ZObjectRef()
    );
    void OnSignalEntityPin(
        ZEntityRef p_Entity,
        uint32_t p_PinId,
        bool p_Output,
        const ZObjectRef& p_Data = ZObjectRef()
    );

    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    void AddPendingDynamicEntities();
    void UpdatePinsDownload();

    void DrawPinTracerWindow();


    static void ToggleEditorServerEnabled();
    static void FindAlocAndPrimForZGeomEntityNode(
        std::vector<NavKitMeshEntity>& p_Entities,
        const std::shared_ptr<EntityTreeNode>& p_Node, const TArray<SInterfaceData>& p_Interfaces, const char*& p_EntityType, const
        std::unordered_map<std::string, std::string>& p_RoomNameToFolderName,
        std::map<std::string, NavKitMatiTextures>& p_MatiTextures,
        std::map<std::string, std::vector<std::string>>& p_PrimMatis, bool p_OnlyCollidableMeshes
    );
    static void FindAlocAndPrimForZPrimitiveProxyEntityNode(
        std::vector<NavKitMeshEntity>& p_Entities,
        const std::shared_ptr<EntityTreeNode>& p_Node, const TArray<SInterfaceData>& p_Interfaces, const char*& p_EntityType, const
        std::unordered_map<std::string, std::string>&
        roomNameToFolderName,
        std::map<std::string, NavKitMatiTextures>& p_MatiTextures,
        std::map<std::string, std::vector<std::string>>& p_PrimMatis, bool p_OnlyCollidableMeshes
    );

    static void DrawPropertyName(const std::string& p_Name, bool p_IsChild);
    static bool DrawVectorChildRow(const std::string& p_Id, const char* p_Label, const char* p_Suffix, float* p_Values);
    static bool DrawColorSwatch(const std::string& p_Id, float* p_Color, bool p_HasAlpha);
    static bool ColorComponentsProperty(const std::string& p_Id, float* p_Color, bool p_HasAlpha);
    static bool IsReadOnlyProperty(const SPropertyData* p_Property);
    static bool CanCopyPropertyValue(const IType* p_Type);
    std::string FormatPropertyValueForClipboard(const STypeID* p_TypeID, void* p_Data);
    std::string GetEnumValueName(const IEnumType* p_Type, const void* p_Data);

    // Properties
    void UnsupportedProperty();

    bool TEntityRefProperty(const std::string& p_Id, void* p_Data, const STypeID* p_TypeID);
    bool EntityRefProperty(const std::string& p_Id, ZEntityRef& p_EntityRef);

    void ZRepositoryIDProperty(void* p_Data);
    bool ZGuidProperty(const std::string& p_Id, void* p_Data);

    // Primitive properties.
    bool StringProperty(const std::string& p_Id, void* p_Data);
    bool BoolProperty(const std::string& p_Id, void* p_Data);

    struct ScalarPropertyType {
        std::string_view m_TypeName;
        const char* m_Tag;
        ImGuiDataType m_DataType;
        float m_Speed;
    };

    static const ScalarPropertyType* FindScalarPropertyType(std::string_view p_TypeName);
    bool EnumProperty(const std::string& p_Id, void* p_Data, const STypeID* p_TypeID);
    void LoadSmfEnumVariants();

    // Vector properties.
    bool SVector2Property(const std::string& p_Id, void* p_Data);
    bool SVector3Property(const std::string& p_Id, void* p_Data);
    bool SVector4Property(const std::string& p_Id, void* p_Data);

    bool SMatrix43Property(const std::string& p_Id, void* p_Data);

    template <typename T>
    static std::unique_ptr<T, PropertyDeleter> GetProperty(ZEntityRef p_Entity, const SPropertyData* p_Property);
    static Quat GetQuatFromProperty(ZEntityRef p_Entity);
    static Quat GetParentQuat(ZEntityRef p_Entity);
    std::pair<std::string, std::string> FindRoomForEntity(ZEntityRef p_Entity, const std::unordered_map<std::string, std::string>& p_RoomNameToFolderName);

    bool SColorRGBProperty(const std::string& p_Id, void* p_Data);
    bool SColorRGBAProperty(const std::string& p_Id, void* p_Data);

    void DrawResourceIdLink(const ZRuntimeResourceID& p_ResourceId) const;

    void ResourcePtrProperty(void* p_Data);
    void ZRuntimeResourceIDProperty(void* p_Data);

    bool ArrayProperty(void* p_Data, const STypeID* p_TypeID);

    bool ZGameTimeProperty(const std::string& p_Id, void* p_Data);

    static std::string FormatZCurveForClipboard(ZCurve* p_Curve);

    static SMatrix QneTransformToMatrix(const QneTransform& p_Transform);

    static std::string FormatFloat(float p_Value, bool p_Round, uint32_t p_Decimals);
    static std::string FormatMatrixAsRtJson(const SMatrix43& p_Matrix, bool p_Round, uint32_t p_Decimals);
    static std::string FormatQneTransformAsJson(const QneTransform& p_Transform, bool p_Round, uint32_t p_Decimals);

    std::string GetNameFromRepository(const ZRepositoryID& p_RepositoryID);

    void DrawItemsWindow(bool p_HasFocus);
    void DrawActorsWindow(bool p_HasFocus);
    void DrawDebugChannelsWindow(bool p_HasFocus);
    void DrawRoomsWindow(bool p_HasFocus);
    void DrawBoxReflectionsWindow(bool p_HasFocus);

    static void EquipOutfit(
        const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit, uint8_t p_CharSetIndex,
        const std::string& p_CharSetCharacterType, uint8_t p_OutfitVariationindex, ZActor* p_Actor
    );

    void ItemCreatedHandler(uint32 p_Ticket, TEntityRef<IItemBase> p_NewItem);
    void LoadRepositoryWeapons();

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
    bool DrawGizmo(GizmoEntity& p_GizmoEntity, IRenderer* p_Renderer);
    bool DrawShapes(const DebugEntity& p_DebugEntity, IRenderer* p_Renderer);
    void DrawShapes(ZSoundGateEntity* p_SoundGateEntity, IRenderer* p_Renderer);
    void DrawShapes(ZSequenceEntity* p_SequenceEntity, IRenderer* p_Renderer);
    void DrawShapes(ZCoverPlane* p_CoverPlane, IRenderer* p_Renderer);

    template <typename T>
    void DrawShapeOptions();

    void DrawShapeOptions(const std::string& p_TypeName, bool p_Disabled);

    void DrawDebugEntityID(
        ZSpatialEntity* p_Entity,
        IRenderer* p_Renderer
    );

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
        const ZRuntimeResourceID p_RuntimeResourceID,
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
    void DeleteDebugEntities(const std::shared_ptr<EntityTreeNode>& p_RootNode);
    EDebugChannel ConvertDrawLayerToDebugChannel(const ZDebugGizmoEntity_EDrawLayer p_DrawLayer);
    static bool EntityIDMatches(void* p_Interface, const uint64 p_EntityID);

    bool RayCastDebugEntities(const SVector3& p_WorldPosition, const SVector3& p_Direction);
    void RayCastGizmos(const SVector3& p_WorldPosition, const SVector3& p_Direction, RayCastResult& p_Result);
    void RayCastShapes(const SVector3& p_WorldPosition, const SVector3& p_Direction, RayCastResult& p_Result);

    bool RayCastShape(
        const DirectX::SimpleMath::Ray& p_Ray,
        ZSoundGateEntity* p_SoundGateEntity,
        float& p_Distance
    );
    bool RayCastShape(
        const DirectX::SimpleMath::Ray& p_Ray,
        ZSequenceEntity* p_SequenceEntity,
        float& p_Distance
    );
    bool RayCastShape(
        const DirectX::SimpleMath::Ray& p_Ray,
        ZCoverPlane* p_CoverPlane,
        float& p_Distance
    );

    bool RayCastTrajectoryTrack(
        const DirectX::SimpleMath::Ray& p_Ray,
        ZTrajectoryTrackBase* p_Track,
        ZEntityRef p_TargetEntity,
        float& p_Distance
    );
    template <typename TItem, typename EvaluateFn>
    bool RayCastTrajectoryItem(
        const DirectX::SimpleMath::Ray& p_Ray,
        TItem* p_Item,
        ZTrajectoryTrackBase* p_Track,
        ZEntityRef p_TargetEntity,
        float& p_Distance,
        EvaluateFn&& p_EvaluateFn
    );

    bool RayIntersectsTrajectorySegment(
        const DirectX::SimpleMath::Ray& p_Ray,
        const SVector3& p_Start,
        const SVector3& p_End,
        float p_Radius,
        float& p_Distance
    );
    bool RayIntersectsBox(
        const DirectX::SimpleMath::Ray& p_Ray,
        const SMatrix& p_Transform,
        const SVector3& p_Center,
        const SVector3& p_Extents,
        float& p_Distance
    );
    bool RayIntersectsSphere(
        const DirectX::SimpleMath::Ray& p_Ray,
        const SVector3& p_Position,
        float p_Radius,
        float& p_Distance
    );

    void DrawTrajectoryTrack(
        IRenderer* p_Renderer,
        ZTrajectoryTrackBase* p_Track,
        const SVector4& p_Color,
        ZEntityRef p_TargetEntity
    );
    template <typename TItem, typename EvaluateFn>
    void DrawTrajectoryItem(
        IRenderer* p_Renderer,
        TItem* p_Item,
        ZTrajectoryTrackBase* p_Track,
        const SVector4& p_Color,
        ZEntityRef p_TargetEntity,
        EvaluateFn&& p_EvaluateFn
    );
    static SVector3 EvaluateTrajectoryWorldPosition(
        ZMorphemeTrajectorySource* p_Item,
        ZTrajectoryTrackBase* p_Track,
        ZGameTime p_ItemTime,
        ZAnimationResource* p_AnimationResource,
        ZEntityRef p_TargetEntity
    );
    static SVector3 EvaluateTrajectoryWorldPosition(
        ZEulerAngleTrajectorySource* p_Item,
        ZTrajectoryTrackBase* p_Track,
        ZGameTime p_ItemTime,
        ZEntityRef p_TargetEntity
    );
    static SVector3 EvaluateTrajectoryWorldPosition(
        ZBezierSplineTrajectorySource* p_Item,
        ZTrajectoryTrackBase* p_Track,
        ZGameTime p_ItemTime,
        ZEntityRef p_TargetEntity
    );

    static bool IsActorTarget(ZActor* p_Actor);

    void ProcessTasks();

    std::vector<PinInfo> GetPins(ZEntityRef p_EntityRef, bool p_OutputPins);

    std::map<std::string, PinLists> ParsePinsJson(const std::string& p_PinsJson);

    bool UpdateBoxReflectionPreview(ZRenderGraphNodeBoxReflection* p_BoxReflectionGraphNode);
    bool UpdateBoxReflectionCubemapPreview(
        ZRenderTexture2D* p_SourceTexture,
        uint32_t p_CubeIndex,
        std::array<ScopedD3DRef<ID3D12Resource>, 6>& p_OutTextures,
        std::array<ImGuiTexture, 6>& p_OutImGuiTextures
    );
    void ClearBoxReflectionPreview();
    void DrawBoxReflectionCross(const std::array<ImGuiTexture, 6>& p_Textures, float p_FaceSize);

    static bool CreateBoxReflectionFaceTexture(
        const DirectX::Image& p_Image,
        ScopedD3DRef<ID3D12Resource>& p_OutTexture,
        ImGuiTexture& p_OutImGuiTexture
    );

    static bool ExportAllBoxReflectionCubemaps(
        const std::filesystem::path& p_OutputFolder,
        bool p_Diffuse
    );
    static bool ExportBoxReflectionCubemap(
        const DirectX::ScratchImage& p_CapturedImage,
        uint32_t p_CubeIndex,
        const std::filesystem::path& p_OutputFilePath
    );
    static bool ExportBoxReflectionCubemap(
        ID3D12Resource* p_Resource,
        uint32_t p_CubeIndex,
        const std::filesystem::path& p_OutputFilePath
    );
    static bool GetBoxReflectionTexture(
        ZRenderGraphNodeBoxReflection* p_BoxReflectionGraphNode,
        bool p_Diffuse,
        ZRenderTexture2D*& p_OutTexture,
        uint32_t& p_OutCubeIndex
    );
    static std::filesystem::path GetBoxReflectionExportPath(
        const ZRenderGraphNodeBoxReflection* p_BoxReflection,
        const std::filesystem::path& p_OutputFolder,
        bool p_Diffuse
    );

    static bool GenerateBoxReflectionCacheResource(const std::filesystem::path& p_OutputFolder);
    static size_t CalculateCubemapSize(const DirectX::ScratchImage& p_Image, uint32_t p_CubeIndex);

private:
    DECLARE_PLUGIN_DETOUR(Editor, bool, OnLoadScene, ZEntitySceneContext*, SSceneInitParameters&);
    DECLARE_PLUGIN_DETOUR(Editor, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene);
    DECLARE_PLUGIN_DETOUR(
        Editor, ZTemplateEntityBlueprintFactory*, ZTemplateEntityBlueprintFactory_ctor,
        ZTemplateEntityBlueprintFactory* th, STemplateEntityBlueprint* pTemplateEntityBlueprint,
        ZResourcePending& ResourcePending
    );
    DECLARE_PLUGIN_DETOUR(Editor, bool, OnInputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data);
    DECLARE_PLUGIN_DETOUR(Editor, bool, OnOutputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data);

    DECLARE_PLUGIN_DETOUR(Editor, ZEntityRef*, ZEntityManager_NewUninitializedEntity,
        ZEntityManager* th,
        ZEntityRef& result,
        const ZString& sDebugName,
        IEntityFactory* pEntityFactory,
        const ZEntityRef& logicalParent,
        uint64_t entityID,
        const SExternalReferences& externalRefs,
        bool unk0
    );
    DECLARE_PLUGIN_DETOUR(Editor, void, ZEntityManager_DeleteEntity,
        ZEntityManager* th,
        const ZEntityRef& entityRef,
        const SExternalReferences& externalRefs
    );

    DECLARE_PLUGIN_DETOUR(Editor, void, ZTemplateEntityFactory_ConfigureEntity,
        ZTemplateEntityFactory* th,
        ZEntityType** pEntity,
        const SExternalReferences& externalRefs,
        uint8_t* unk0
    );

    DECLARE_PLUGIN_DETOUR(Editor, bool, ZExtendedCppEntityTypeInstaller_Install,
        ZExtendedCppEntityTypeInstaller* th,
        ZResourcePending& ResourcePending
    );

    DECLARE_PLUGIN_DETOUR(Editor, void, ZResourceManager_UninstallResource,
        ZResourceManager* th, ZResourceIndex index
    );

private:
    enum class EntityHighlightMode {
        Lines,
        LinesAndTriangles
    };

    enum EntityViewMode {
        All,
        ScenesAndBricks,
        DynamicEntities
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
        SoundGateEntity,
        SequenceEntity,
        Count
    };

    enum class DebugDrawMode {
        SelectedChannelsAndTypes,
        SelectedEntity,
        All
    };

    struct BoxReflectionPreview {
        int32_t m_BoxReflectionId = SIZE_MAX;

        std::array<ScopedD3DRef<ID3D12Resource>, 6> m_Textures;
        std::array<ImGuiTexture, 6> m_ImGuiTextures;

        std::array<ScopedD3DRef<ID3D12Resource>, 6> m_DiffuseTextures;
        std::array<ImGuiTexture, 6> m_DiffuseImGuiTextures;
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
    bool m_RoundCopiedMatrixValues = false;
    int32_t m_CopyDecimalPlaces = 3;
    double m_SnapValue = 1.0;
    double m_AngleSnapValue = 90.0;
    double m_ScaleSnapValue = 1.0;

    ImVec2 m_TransformToolbarOffset { 8.f, 8.f };

    bool m_SettingsVisible = false;
    bool m_ResetDockLayout = false;
    bool m_EditorWindowsVisible = true;

    size_t m_SelectedBrickIndex = 0;
    ZEntityRef m_SelectedEntity;
    bool m_ScrollToEntity = false;

    EntityHighlightMode m_EntityHighlightMode = EntityHighlightMode::Lines;

    EntityViewMode m_EntityViewMode = EntityViewMode::All;
    EntityViewMode m_LastEntityViewMode = EntityViewMode::All;
    const std::vector<std::string> m_EntityViewModes = { "All", "Scenes/bricks", "Dynamic entities" };

    std::string m_EntityIDSearchInput;
    std::string m_EntityTypeSearchInput;
    std::string m_EntityNameSearchInput;
    std::unordered_set<EntityTreeNode*> m_FilteredEntityTreeNodes;
    size_t m_CurrentEntitySearchResultIndex = 0;
    size_t m_TotalMatchCount = 0;
    bool m_HasActiveFilters = false;
    bool m_HasActiveSearch = false;

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
    std::unordered_set<EntityTreeNode*> m_OpenEntityTreeNodes;

    std::unordered_map<ZEntityRef, SpawnedEntity> m_SpawnedEntities;
    EntityTreeDeleter m_EntityTreeDeleter;

    std::mutex m_AssetSpawnRequestsMutex;
    std::vector<AssetSpawnRequest> m_AssetSpawnRequests;

    // TResourcePtr's assignment doesn't count references, so these must not move.
    std::list<PendingAssetSpawn> m_PendingAssetSpawns;
    std::unordered_map<ZEntityRef, std::string> m_EntityNames;

    std::mutex m_EntityDestructionMutex;
    std::vector<std::tuple<ZEntityRef, std::optional<std::string>>> m_EntitiesToDestroy;

    std::atomic_bool m_IsBuildingEntityTree = false;
    std::mutex m_DynamicEntitiesMutex;
    std::unordered_set<ZEntityRef> m_DynamicEntities;
    std::mutex m_PendingDynamicEntitiesMutex;
    std::unordered_set<ZEntityRef> m_PendingDynamicEntities;

    EditorServer m_Server;

    bool m_ShowItemsWindow = false;
    bool m_ShowActorsWindow = false;
    bool m_ShowDebugChannelsWindow = false;
    bool m_ShowRoomsWindow = false;
    bool m_ShowBoxReflectionsWindow = false;
    ZActor* m_SelectedActor = nullptr;
    TEntityRef<ZGlobalOutfitKit> m_GlobalOutfitKit = {};
    const std::vector<std::string> m_CharSetCharacterTypes = { "Actor", "Nude", "HeroA" };
    bool m_ShowAliveActors = false;
    bool m_ShowCivilians = true;
    bool m_ShowGuards = true;
    bool m_ShowMaleActors = false;
    bool m_ShowFemaleActors = false;
    bool m_ShowTargets = false;
    bool m_ShowActiveEnforcers = false;
    bool m_ShowPotentialEnforcers = false;
    bool m_ShowDynamicEnforcers = false;
    bool m_ShowCrowdCharacters = false;
    bool m_ShowActiveSentries = false;
    bool m_ShowActorsWithClothOutfit = false;
    bool m_SelectActorOnMouseClick = false;
    bool m_ScrollToActor = false;

    std::vector<std::pair<ZRepositoryID, std::string>> m_RepositoryWeapons;
    TEntityRef<IItem> m_ItemToRemove{};
    bool m_RemoveItemFromInventory = false;

    ZActor* m_ActorTracked = nullptr;
    bool m_TrackCamActive = false;
    ZEntityRef m_PlayerCam = nullptr;
    TEntityRef<ZCameraEntity> m_TrackCam{};
    TEntityRef<IRenderDestinationEntity> m_RenderDest{};

    ZHM5Action* m_SelectedAction = nullptr;

    std::unordered_map<ZEntityRef, std::vector<std::unique_ptr<DebugEntity>>> m_EntityRefToDebugEntities;
    std::shared_mutex m_DebugEntitiesMutex;
    std::vector<std::pair<std::string, EDebugChannel>> m_DebugChannels;
    std::unordered_map<std::string, std::vector<std::string>> m_DebugChannelNameToTypeNames;
    std::unordered_map<EDebugChannel, uint32> m_DebugChannelToDebugEntityCount;
    std::unordered_map<EDebugChannel, std::unordered_map<std::string, uint32_t>>
        m_DebugChannelToTypeNameToDebugEntityCount;
    std::unordered_map<EDebugChannel, bool> m_DebugChannelToGizmoState;
    std::unordered_map<EDebugChannel, bool> m_DebugChannelToShapeState;
    std::unordered_map<EDebugChannel, std::unordered_map<std::string, bool>> m_DebugChannelToTypeNameToGizmoState;
    std::unordered_map<EDebugChannel, std::unordered_map<std::string, bool>> m_DebugChannelToTypeNameToShapeState;
    std::vector<STypeID*> m_DebugEntityTypeIds;

    // Enum type name -> (ModID:VariantName, value), from the SMF deployment summary.
    std::unordered_map<std::string, std::vector<std::pair<std::string, int64_t>>> m_SmfEnumVariants;

    bool m_DrawGizmos = true;
    DebugDrawMode m_GizmoDrawMode = DebugDrawMode::SelectedChannelsAndTypes;

    bool m_DrawShapes = false;
    DebugDrawMode m_ShapeDrawMode = DebugDrawMode::SelectedChannelsAndTypes;

    bool m_DrawEntityIDs = false;

    ZEntityRef m_SelectedDebugEntity = nullptr;

    bool m_DrawInvalidCoverOnNPCError = true;
    bool m_DrawHeroGuidesSolid = false;
    bool m_AlwaysDrawDebugBoxForCollisionValidityCheck = false;
    bool m_DrawPushDebug = false;
    bool m_DrawSafeZones = true;
    bool m_DrawNPCFlags = true;
    bool m_DrawNPCValidation = true;

    bool m_ShowSoundOcclusion = false;

    AssetBrowser m_AssetBrowser;

    bool m_ReparentDynamicOutfitEntities = true;

    inline static ZEntityRef m_DynamicEntitiesNodeEntityRef = ZEntityRef(reinterpret_cast<ZEntityType**>(0x1));
    inline static ZEntityRef m_UnparentedEntitiesNodeEntityRef = ZEntityRef(reinterpret_cast<ZEntityType**>(0x2));
    inline static ZEntityRef m_SpawnedEntitiesNodeEntityRef = ZEntityRef(reinterpret_cast<ZEntityType**>(0x3));

    std::unordered_map<ZEntityRef, std::pair<ZRuntimeResourceID, ZRuntimeResourceID>> m_EntityRefToFactoryRuntimeResourceIDs;
    std::unordered_map<ZExtendedCppEntityFactory*, ZRuntimeResourceID> m_ExtendedCppEntityFactoryToRuntimeResourceID;
    std::unordered_map<ZRuntimeResourceID, ZExtendedCppEntityFactory*> m_RuntimeResourceIDToExtendedCppEntityFactory;
    std::shared_mutex m_EntityRefToFactoryRuntimeResourceIDsMutex;
    std::mutex m_ExtendedCppEntityFactoryResourceMapsMutex;

    std::vector<ZEntityRef> m_SortedRoomEntities;
    bool m_ShowOnlyVisibleRooms = false;
    bool m_ShowOnlyVisibleGates = false;

    int m_PropertyTableTreeDepth = 0;

    static constexpr const char* c_EntityTreeWindowName = ICON_MD_CATEGORY " Entities";
    static constexpr const char* c_EntityPropertiesWindowName = ICON_MD_TUNE " Entity properties";
    static constexpr const char* c_AssetBrowserWindowName = ICON_MD_LIBRARY_BOOKS " Asset browser";
    bool m_UseLocalTransforms = false;
    bool m_ShowEntityIds = true;

    float m_EntityTreeNameScroll = 0.f;
    float m_EntityTreeNameWidth = 0.f;

    std::string m_EntityRefPickTarget;
    std::optional<ZEntityRef> m_PickedEntityRef;

    std::mutex m_TaskMutex;
    std::vector<std::function<void()>> m_TaskQueue;

    std::map<std::string, PinLists> m_ClassToInputAndOutputPins;
    std::vector<std::pair<std::string, STypeID*>> m_PinDataTypes;
    ZObjectRef m_InputPinValue;
    ZObjectRef m_OutputPinValue;

    bool m_IsFiringOutputPin = false;
    std::unordered_map<ZEntityRef, std::deque<RecentPinFire>> m_RecentPinFires;
    std::unordered_map<std::string, STypeID*> m_LastPinArgumentTypes;

    std::atomic<bool> m_ShouldClearRecentPinFires = false;

    CurveEditor m_CurveEditor;

    std::vector<std::string> m_ClassNames;

    TResourcePtr<ZTemplateEntityFactory> m_RepositoryResource;

    ZRenderGraphNodeBoxReflection* m_SelectedBoxReflectionGraphNode = nullptr;
    BoxReflectionPreview m_BoxReflectionPreview;

    bool m_EnableBoxReflectionCache = true;

    std::string m_BoxReflectionOutputFolder;
};

DECLARE_ZHM_PLUGIN(Editor)