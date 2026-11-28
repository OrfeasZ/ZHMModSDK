#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "Hook.h"
#include "IRenderer.h"
#include "SpawnSafety.h"

#include "Glacier/ZEntity.h"
#include "Glacier/ZResource.h"
#include "Glacier/ZCameraEntity.h"
#include "Glacier/ZGeomEntity.h"
#include "Glacier/ZRender.h"
#include "Glacier/EntityFactory.h"

class ZRenderGraphNode;
class ZRenderTraversalContext;
class ZSparseBitArray;
class ZRenderDeviceContext;
class CLightTile;
class ZRoomManager;
class ZRoomEntity;

// Off-map camera, RT and lights that render only the preview, nothing else of the level.
class AssetPreview {
public:
    explicit AssetPreview(EntityTreeDeleter& p_TreeDeleter);
    ~AssetPreview();

    void Spawn();
    void Destroy();
    bool IsReady() const;

    bool SpawnPreview(IEntityFactory* p_Factory, const std::string& p_ItemName);
    void DestroyPreview();
    bool FramePreview(float p_Yaw);
    bool HasRenderables() const;

    bool IsPreviewInUse() const;

    bool ForceRenderRT();
    void StopForceRenderingRT();
    int GetForcedRenderFrameCount() const;

    ID3D12Resource* GetRTResource() const;
    const ImGuiTexture* GetRTTexture();

private:
    struct PreviewRenderable {
        ZSpatialEntity* Spatial;
        ZRenderableEntity* Renderable;

        bool IsFramed;
    };

    bool GetPreviewBounds(SVector3& p_Min, SVector3& p_Max) const;
    void PublishPreviewNodeIndices();

    DECLARE_DETOUR_WITH_CONTEXT(AssetPreview, void, ZRenderGraphNodeCamera_Traverse,
        ZRenderGraphNodeCamera* th, ZRenderGraphNode* pParent, ZRenderTraversalContext* pTraversalContext,
        ZRenderContext* pRenderContext, uint64_t a5
    );

    DECLARE_DETOUR_WITH_CONTEXT(AssetPreview, void, ZRenderGraphTraversal_TraverseSimple,
        ZRenderTraversalContext* pTraversalContext, uint32_t nSkipMask, const ZSparseBitArray& Nodes
    );

    DECLARE_DETOUR_WITH_CONTEXT(AssetPreview, void, CLightTile_DispatchShadeDeferred,
        CLightTile* th, ZRenderContext* pRenderContext, ZRenderDeviceContext* pDeviceContext, void* a4, void* a5,
        void* a6, void* a7, void* a8, void* a9, void* a10, void* a11, void* a12, void* a13, void* a14, int32_t a15,
        void* a16, const float4* vClearColor, bool bDisableLighting, bool bMergeDiffuse,
        int32_t nSelectedBoxReflectionId, bool bDisableBoxReflections, bool bEnableAtmosphericScatteringBackdrop,
        bool bIsBoxReflectionPass, int32_t a24
    );

    DECLARE_DETOUR_WITH_CONTEXT(AssetPreview, void, ZRoomManager_UnregisterRoom, ZRoomManager* th, ZRoomEntity* pRoom);

    // The rig uses these resources, so keep them referenced until it is deleted.
    ZEntityRef m_Rig {};
    std::optional<TResourcePtr<ZTemplateEntityFactory>> m_RigFactory;
    std::optional<TResourcePtr<ZTemplateEntityBlueprintFactory>> m_RigBlueprintFactory;
    TEntityRef<ZCameraEntity> m_Camera {};
    TEntityRef<ZRenderDestinationTextureEntity> m_RT {};
    ZEntityRef m_KeyLight {};
    ZEntityRef m_FillLight {};
    std::vector<ZRenderGraphNode*> m_ForcedNodes;
    int m_ForcedRenderFrameCount = 0;

    ZEntityRef m_PreviewEntity {};

    std::vector<PreviewRenderable> m_PreviewRenderables;
    std::vector<uint32_t> m_PreviewNodeIndicesScratch;

    SpawnedTree m_PreviewTree;
    EntityTreeDeleter& m_TreeDeleter;

    // Some entities create look-at triggers the tree walk doesn't reach, so we treat every
    // trigger registered after this snapshot as the preview's.
    std::unordered_set<ZEntityType**> m_LookAtTriggersBeforeSpawn;
    SVector3 m_FramedMin {};
    SVector3 m_FramedMax {};
    float m_FramedYaw = 0.f;

    ImGuiTexture m_RTTexture {};
    std::vector<uint64> m_NodeSetBits;

    std::atomic<uint32_t> m_CameraNodeIndex = -1;

    std::mutex m_PreviewNodeIndicesMutex;
    std::vector<uint32_t> m_PreviewNodeIndices;
};
