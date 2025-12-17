#pragma once

#include <shared_mutex>
#include <random>
#include <unordered_map>
#include <map>
#include <set>

#include "IPluginInterface.h"

#include <Glacier/ZInput.h>
#include <Glacier/ZEntity.h>
#include <Glacier/ZResource.h>
#include "Glacier/ZScene.h"
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZPathfinder.h>

#include "NavPower.h"

class DebugMod : public IPluginInterface {
public:
    ~DebugMod() override;

    void Init() override;
    void OnEngineInitialized() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;
    void OnDraw3D(IRenderer* p_Renderer) override;
    void OnDepthDraw3D(IRenderer* p_Renderer) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    static void CopyToClipboard(const std::string& p_String);

private:
    void DrawOptions(bool p_HasFocus);
    void DrawPositionBox(bool p_HasFocus);

    void DrawReasoningGrid(IRenderer* p_Renderer);
    void DrawNavMesh(IRenderer* p_Renderer);
    void DrawObstacles(IRenderer* p_Renderer);

    void GenerateReasoningGridVertices();
    void GenerateVerticesForSmallQuads();
    void GenerateVerticesForLargeQuads();
    void GenerateVerticesForQuadBorderLines();
    void GenerateVerticesForNeighborConnectionLines();

    std::map<NavPower::Binary::Area*, uint32_t> GetAreaPointerToIndexMap();

    // Functions are adapted from OBJ Loader plugin: https://github.com/Bly7/OBJ-Loader/blob/master/Source/OBJ_Loader.h
    static void VertexTriangluation(const std::vector<SVector3>& vertices, std::vector<unsigned short>& indices);
    static float AngleBetween(const SVector3& a, const SVector3& b);
    static SVector3 ProjectionOnto(const SVector3& a, const SVector3& b);
    static bool AreOnSameSide(const SVector3& p1, const SVector3& p2, const SVector3& a, const SVector3& b);
    static SVector3 ComputeTriangleNormal(const SVector3& t1, const SVector3& t2, const SVector3& t3);
    static bool IsInTriangle(const SVector3& point, const SVector3& triangle1, const SVector3& triangle2, const SVector3& triangle3);

    static const char* CompiledBehaviorTypeToString(ECompiledBehaviorType p_Type);
    
    DECLARE_PLUGIN_DETOUR(DebugMod, bool, OnLoadScene, ZEntitySceneContext*, SSceneInitParameters&);
    DECLARE_PLUGIN_DETOUR(DebugMod, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene);

    DECLARE_PLUGIN_DETOUR(DebugMod, void, ZPFObstacleEntity_UpdateObstacle, ZPFObstacleEntity* th, uint32 nObstacleBlockageFlags, bool bEnabled, bool forceUpdate);

private:
    bool m_DebugMenuActive = false;
    bool m_PositionsMenuActive = false;
    bool m_RenderActorBoxes = false;
    bool m_RenderActorNames = false;
    bool m_RenderActorRepoIds = false;
    bool m_RenderActorBehaviors = false;

    bool m_DrawReasoningGrid = false;
    bool m_ShowVisibility = false;
    bool m_ShowLayers = false;
    bool m_ShowIndices = false;
    std::vector<Line> m_Lines;
    std::vector<Triangle> m_Triangles;

    bool m_DrawNavMesh = false;
    bool m_DrawPlannerAreas = true;
    bool m_DrawPlannerAreasSolid = true;
    bool m_ColorizeAreaUsageFlags = true;
    bool m_DrawObstacles = false;
    bool m_DrawDrawPlannerConnectivity = false;
    bool m_DrawAreaPenaltyMults = false;
    bool m_DrawGizmos = false;
    NavPower::NavMesh m_NavMesh;
    std::vector<uint8_t> m_NavpData;
    std::vector<std::vector<SVector3>> m_Vertices;
    std::vector<std::vector<unsigned short>> m_Indices;
    std::vector<Line> m_NavMeshLines;
    std::vector<Line> m_NavMeshConnectivityLines;
    std::unordered_map<IPFObstacleInternal*, uint64_t> m_ObstaclesToEntityIDs;
};

DECLARE_ZHM_PLUGIN(DebugMod)
