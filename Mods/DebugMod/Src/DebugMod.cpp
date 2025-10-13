#include "DebugMod.h"
#include "Hooks.h"
#include "Logging.h"

#include <winhttp.h>
#include <numbers>
#include <filesystem>

#include <imgui_internal.h>

#include <IconsMaterialDesign.h>

#include <Glacier/ZScene.h>
#include <Glacier/ZActor.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZKnowledge.h>
#include <Glacier/ZPathfinder.h>
#include <Glacier/SReasoningGrid.h>
#include <Glacier/ZGridManager.h>
#include <Glacier/ZHM5GridManager.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZColor.h>

#include <Functions.h>
#include <Globals.h>

DebugMod::~DebugMod() {
    const ZMemberDelegate<DebugMod, void(const SGameUpdateEvent&)> s_Delegate(this, &DebugMod::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void DebugMod::Init() {
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &DebugMod::OnLoadScene);
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &DebugMod::OnClearScene);

    Hooks::ZPFObstacleEntity_UpdateObstacle->AddDetour(this, &DebugMod::ZPFObstacleEntity_UpdateObstacle);
}

void DebugMod::OnEngineInitialized() {
    const ZMemberDelegate<DebugMod, void(const SGameUpdateEvent&)> s_Delegate(this, &DebugMod::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void DebugMod::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {}

void DebugMod::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_BUILD " DEBUG MENU")) {
        m_DebugMenuActive = !m_DebugMenuActive;
    }

    if (ImGui::Button(ICON_MD_PLACE " POSITIONS MENU")) {
        m_PositionsMenuActive = !m_PositionsMenuActive;
    }
}

void DebugMod::OnDrawUI(bool p_HasFocus) {
    DrawOptions(p_HasFocus);
    DrawPositionBox(p_HasFocus);
}

void DebugMod::DrawOptions(const bool p_HasFocus) {
    if (!p_HasFocus || !m_DebugMenuActive) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("DEBUG MENU", &m_DebugMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing) {
        if (ImGui::CollapsingHeader("Actors")) {
            ImGui::Checkbox("Render Actor position boxes", &m_RenderActorBoxes);
            ImGui::Checkbox("Render Actor names", &m_RenderActorNames);
            ImGui::Checkbox("Render Actor repository IDs", &m_RenderActorRepoIds);
            ImGui::Checkbox("Render Actor behaviors", &m_RenderActorBehaviors);
        }

        if (ImGui::CollapsingHeader("Reasoning Grid")) {
            if (ImGui::Checkbox("Draw Reasoning Grid", &m_DrawReasoningGrid)) {
                if (m_Triangles.size() == 0) {
                    GenerateReasoningGridVertices();
                }
            }

            ImGui::Checkbox("Show Visibility", &m_ShowVisibility);
            ImGui::Checkbox("Show Layers", &m_ShowLayers);
            ImGui::Checkbox("Show Indices", &m_ShowIndices);
        }

        if (ImGui::CollapsingHeader("Guide Path Finder")) {
            if (ImGui::Checkbox("Draw Nav Mesh", &m_DrawNavMesh)) {
                if (m_NavMesh.m_areas.size() == 0) {
                    static const SVector4 s_LineColor = SVector4(0.f, 1.f, 0.f, 1.f);
                    static const SVector4 s_AdjacentLineColor = SVector4(1.f, 1.f, 1.f, 1.f);

                    const uintptr_t s_NavpData = reinterpret_cast<uintptr_t>(Globals::Pathfinder->m_NavPowerResources[0]
                        .m_pNavpowerResource);
                    const uint32_t s_NavpDataSize = Globals::Pathfinder->m_NavPowerResources[0].m_nNavpowerResourceSize;

                    m_NavpData.resize(s_NavpDataSize);

                    std::memcpy(m_NavpData.data(), reinterpret_cast<void*>(s_NavpData), s_NavpDataSize);

                    m_NavMesh.read(reinterpret_cast<uintptr_t>(m_NavpData.data()), s_NavpDataSize);

                    m_Vertices.resize(m_NavMesh.m_areas.size());
                    m_Indices.resize(m_NavMesh.m_areas.size());
                    m_NavMeshLines.reserve(m_NavMesh.m_areas.size() * 3);
                    m_NavMeshConnectivityLines.reserve(m_NavMesh.m_areas.size() * 3);

                    std::map<NavPower::Binary::Area*, uint32_t> s_AreaPointerToIndexMap = GetAreaPointerToIndexMap();

                    for (size_t i = 0; i < m_NavMesh.m_areas.size(); ++i) {
                        const size_t s_VertexCount = m_NavMesh.m_areas[i].m_edges.size();

                        m_Vertices[i].reserve(s_VertexCount);

                        const SVector3 s_Centroid = m_NavMesh.m_areas[i].CalculateCentroid();

                        for (size_t j = 0; j < s_VertexCount; ++j) {
                            m_Vertices[i].push_back(m_NavMesh.m_areas[i].m_edges[j]->m_pos);

                            const size_t s_NextIndex = (j + 1) % s_VertexCount;
                            Line& s_Line = m_NavMeshLines.emplace_back();

                            s_Line.start = m_NavMesh.m_areas[i].m_edges[j]->m_pos;
                            s_Line.startColor = s_LineColor;

                            s_Line.end = m_NavMesh.m_areas[i].m_edges[s_NextIndex]->m_pos;
                            s_Line.endColor = s_LineColor;

                            NavPower::Binary::Area* s_AdjArea = m_NavMesh.m_areas[i].m_edges[j]->m_pAdjArea;

                            if (s_AdjArea) {
                                const uint32_t s_AdjacentAreaIndex = s_AreaPointerToIndexMap[s_AdjArea];
                                NavPower::Area& s_AdjacentArea = m_NavMesh.m_areas[s_AdjacentAreaIndex - 1];
                                const SVector3 s_AdjacentCentroid = s_AdjacentArea.CalculateCentroid();

                                Line& s_ConnLine = m_NavMeshConnectivityLines.emplace_back();
                                s_ConnLine.start = s_Centroid;
                                s_ConnLine.startColor = s_AdjacentLineColor;
                                s_ConnLine.end = s_AdjacentCentroid;
                                s_ConnLine.endColor = s_AdjacentLineColor;
                            }
                        }

                        VertexTriangluation(m_Vertices[i], m_Indices[i]);
                    }
                }
            }

            ImGui::Checkbox("Draw Planner Areas", &m_DrawPlannerAreas);
            ImGui::Checkbox("Draw Planner Areas Solid", &m_DrawPlannerAreasSolid);
            ImGui::Checkbox("Colorize Area Usage Flags", &m_ColorizeAreaUsageFlags);
            ImGui::Checkbox("Draw Obstacles", &m_DrawObstacles);
            ImGui::Checkbox("Draw Planner Connectivity", &m_DrawDrawPlannerConnectivity);
            ImGui::Checkbox("Draw Area Penalty Multipliers", &m_DrawAreaPenaltyMults);
        }
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

void DebugMod::CopyToClipboard(const std::string& p_String) {
    if (!OpenClipboard(nullptr))
        return;

    EmptyClipboard();

    const auto s_GlobalData = GlobalAlloc(GMEM_MOVEABLE, p_String.size() + 1);

    if (!s_GlobalData) {
        CloseClipboard();
        return;
    }

    const auto s_GlobalDataPtr = GlobalLock(s_GlobalData);

    if (!s_GlobalDataPtr) {
        CloseClipboard();
        GlobalFree(s_GlobalData);
        return;
    }

    memset(s_GlobalDataPtr, 0, p_String.size() + 1);
    memcpy(s_GlobalDataPtr, p_String.c_str(), p_String.size());

    GlobalUnlock(s_GlobalData);

    SetClipboardData(CF_TEXT, s_GlobalData);
    CloseClipboard();
}

void DebugMod::OnDraw3D(IRenderer* p_Renderer) {}

void DebugMod::OnDepthDraw3D(IRenderer* p_Renderer) {
    if (m_DrawReasoningGrid) {
        DrawReasoningGrid(p_Renderer);
    }

    if (m_DrawNavMesh) {
        DrawNavMesh(p_Renderer);
    }

    if (m_DrawObstacles) {
        DrawObstacles(p_Renderer);
    }

    if (m_RenderActorBoxes || m_RenderActorNames || m_RenderActorRepoIds || m_RenderActorBehaviors) {
        for (size_t i = 0; i < *Globals::NextActorId; ++i) {
            auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

            ZEntityRef s_Ref;
            s_Actor->GetID(&s_Ref);

            auto* s_SpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();
            auto s_ActorTransform = s_SpatialEntity->GetWorldMatrix();

            float4 s_Min, s_Max;

            s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

            if (m_RenderActorBoxes) {
                p_Renderer->DrawOBB3D(
                    SVector3(s_Min.x, s_Min.y, s_Min.z),
                    SVector3(s_Max.x, s_Max.y, s_Max.z),
                    s_ActorTransform,
                    SVector4(1.f, 0.f, 0.f, 1.f)
                );
            }
            else {
                const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

                if (!s_CurrentCamera) {
                    return;
                }

                auto s_CameraTransform = s_CurrentCamera->GetWorldMatrix();

                const float4 s_Center = (s_Min + s_Max) * 0.5f;
                const float4 s_Extents = (s_Max - s_Min) * 0.5f;

                float4 s_LocalPosition = s_Center + float4(0.f, 0.f, s_Extents.z, 0.f);
                float4 s_WorldPosition = s_ActorTransform * s_LocalPosition;

                s_WorldPosition.z -= 0.5f;
                s_CameraTransform.Trans = s_WorldPosition;

                std::string s_Text;

                if (m_RenderActorNames) {
                    s_Text += s_Actor->m_sActorName.c_str();
                }

                if (m_RenderActorRepoIds) {
                    auto* s_RepoEntity = s_Ref.QueryInterface<ZRepositoryItemEntity>();

                    if (s_Text.length() > 0) {
                        s_Text += "\n\n";
                    }

                    s_Text += s_RepoEntity->m_sId.ToString().c_str();
                }

                if (m_RenderActorBehaviors) {
                    const SBehaviorBase* s_BehaviorBase = Globals::BehaviorService->m_aKnowledgeData[i].
                            m_pCurrentBehavior;

                    if (s_BehaviorBase) {
                        const ECompiledBehaviorType s_CompiledBehaviorType = static_cast<ECompiledBehaviorType>(
                            s_BehaviorBase->m_Type);

                        if (s_Text.length() > 0) {
                            s_Text += "\n\n";
                        }

                        s_Text += BehaviorToString(s_CompiledBehaviorType);
                    }
                }

                p_Renderer->DrawText3D(
                    s_Text.c_str(),
                    s_CameraTransform,
                    SVector4(1.f, 1.f, 0.f, 1.f),
                    0.1f,
                    TextAlignment::Center
                );
            }
        }
    }
}

void DebugMod::DrawReasoningGrid(IRenderer* p_Renderer) {
    const SReasoningGrid* s_ReasoningGrid = *Globals::ActiveGrid;
    const size_t s_WaypointCount = s_ReasoningGrid->m_WaypointList.size();

    for (size_t i = 0; i < s_WaypointCount * 2; ++i) {
        p_Renderer->DrawTriangle3D(
            m_Triangles[i].vertexPosition1, m_Triangles[i].vertexColor1,
            m_Triangles[i].vertexPosition2, m_Triangles[i].vertexColor2,
            m_Triangles[i].vertexPosition3, m_Triangles[i].vertexColor3
        );
    }

    const ZGridNodeRef& s_HitmanNode = Globals::HM5GridManager->m_HitmanNode;
    const size_t s_StartIndex = s_WaypointCount * 2;

    static const SVector4 s_SelectedNodeVertexColor = SVector4(0.f, 1.f, 1.f, 0.43922f);
    static const SVector4 s_LargeQuadVertexColor = SVector4(0.33333f, 0.f, 1.f, 0.43922f);

    for (size_t i = s_StartIndex; i < m_Triangles.size(); ++i) {
        const unsigned short s_WaypointIndex = static_cast<unsigned short>((i - s_WaypointCount * 2) / 2);

        if (s_ReasoningGrid->GetNode(s_WaypointIndex) == s_HitmanNode.GetNode()) {
            m_Triangles[i].vertexColor1 = s_SelectedNodeVertexColor;
            m_Triangles[i].vertexColor2 = s_SelectedNodeVertexColor;
            m_Triangles[i].vertexColor3 = s_SelectedNodeVertexColor;
        }
        else {
            if (m_ShowVisibility) {
                float s_Rating = 0.f;

                if (s_HitmanNode.CheckVisibility(s_WaypointIndex, true, false)) {
                    s_Rating = 1.f;
                }
                else if (s_HitmanNode.CheckVisibility(s_WaypointIndex, false, false)) {
                    s_Rating = 0.5f;
                }

                const unsigned int s_HeatMapColor = ((*Globals::GridManager)->GetHeatmapColorFromRating(s_Rating) &
                    0xFFFFFF) + 0x70000000;
                const SVector4 s_VertexColor = ZColor::UnpackUnsigned(s_HeatMapColor);

                m_Triangles[i].vertexColor1 = s_VertexColor;
                m_Triangles[i].vertexColor2 = s_VertexColor;
                m_Triangles[i].vertexColor3 = s_VertexColor;
            }
            else if (m_ShowLayers) {
                const unsigned int s_LayerIndex = static_cast<unsigned int>(s_ReasoningGrid->m_WaypointList[
                    s_WaypointIndex].nLayerIndex);
                const unsigned int s_Color = (s_LayerIndex << 6) | 0xC0000000;
                const SVector4 s_VertexColor = ZColor::UnpackUnsigned(s_Color);

                m_Triangles[i].vertexColor1 = s_VertexColor;
                m_Triangles[i].vertexColor2 = s_VertexColor;
                m_Triangles[i].vertexColor3 = s_VertexColor;
            }
            else {
                m_Triangles[i].vertexColor1 = s_LargeQuadVertexColor;
                m_Triangles[i].vertexColor2 = s_LargeQuadVertexColor;
                m_Triangles[i].vertexColor3 = s_LargeQuadVertexColor;
            }
        }

        p_Renderer->DrawTriangle3D(
            m_Triangles[i].vertexPosition1, m_Triangles[i].vertexColor1,
            m_Triangles[i].vertexPosition2, m_Triangles[i].vertexColor2,
            m_Triangles[i].vertexPosition3, m_Triangles[i].vertexColor3
        );
    }

    for (size_t i = 0; i < m_Lines.size(); ++i) {
        p_Renderer->DrawLine3D(m_Lines[i].start, m_Lines[i].end, m_Lines[i].startColor, m_Lines[i].endColor);
    }

    if (m_ShowIndices) {
        const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

        if (!s_CurrentCamera) {
            return;
        }

        SMatrix s_WorldMatrix = s_CurrentCamera->GetWorldMatrix();
        const size_t s_WaypointCount = s_ReasoningGrid->m_WaypointList.size();

        std::swap(s_WorldMatrix.YAxis, s_WorldMatrix.ZAxis);

        static const SVector4 s_Color = SVector4(0.f, 0.f, 0.f, 1.f);
        static const float s_Scale = 0.2f;

        for (size_t i = 0; i < s_WaypointCount; ++i) {
            float4 s_WorldPosition = s_ReasoningGrid->m_WaypointList[i].vPos;

            s_WorldPosition.z += 0.5f;
            s_WorldMatrix.Trans = s_WorldPosition;

            const std::string s_Text = std::to_string(i);

            p_Renderer->DrawText3D(s_Text.c_str(), s_WorldMatrix, s_Color, s_Scale);
        }
    }
}

void DebugMod::DrawNavMesh(IRenderer* p_Renderer) {
    static const SVector4 s_GreenTriangleColor = SVector4(0.19608f, 0.80392f, 0.19608f, 0.49804f);
    static const SVector4 s_YellowTriangleColor = SVector4(1.f, 1.f, 0.f, 0.49804f);

    if (m_DrawPlannerAreasSolid) {
        for (size_t i = 0; i < m_NavMesh.m_areas.size(); ++i) {
            if (m_ColorizeAreaUsageFlags && m_NavMesh.m_areas[i].m_area->m_usageFlags ==
                NavPower::AreaUsageFlags::AREA_STEPS) {
                p_Renderer->DrawMesh(m_Vertices[i], m_Indices[i], s_YellowTriangleColor);
            }
            else {
                p_Renderer->DrawMesh(m_Vertices[i], m_Indices[i], s_GreenTriangleColor);
            }
        }
    }

    if (m_DrawPlannerAreas) {
        for (size_t i = 0; i < m_NavMeshLines.size(); ++i) {
            p_Renderer->DrawLine3D(
                m_NavMeshLines[i].start, m_NavMeshLines[i].end, m_NavMeshLines[i].startColor, m_NavMeshLines[i].endColor
            );
        }
    }

    if (m_DrawDrawPlannerConnectivity) {
        for (size_t i = 0; i < m_NavMeshConnectivityLines.size(); ++i) {
            p_Renderer->DrawLine3D(
                m_NavMeshConnectivityLines[i].start, m_NavMeshConnectivityLines[i].end,
                m_NavMeshConnectivityLines[i].startColor, m_NavMeshConnectivityLines[i].endColor
            );
        }
    }

    if (m_DrawAreaPenaltyMults) {
        const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

        if (!s_CurrentCamera) {
            return;
        }

        SMatrix s_WorldMatrix = s_CurrentCamera->GetWorldMatrix();

        std::swap(s_WorldMatrix.YAxis, s_WorldMatrix.ZAxis);

        static const SVector4 s_Color = SVector4(1.f, 1.f, 1.f, 1.f);
        static const float s_Scale = 0.2f;

        for (size_t i = 0; i < m_NavMesh.m_areas.size(); ++i) {
            SVector3 s_WorldPosition = m_NavMesh.m_areas[i].m_area->m_pos;

            const DirectX::XMVECTOR s_WorldPosition2 = DirectX::XMVectorSet(
                s_WorldPosition.x, s_WorldPosition.y, s_WorldPosition.z, 1.0f
            );

            s_WorldPosition.z += 2.f;
            s_WorldMatrix.Trans = float4(s_WorldPosition.x, s_WorldPosition.y, s_WorldPosition.z, 1.0f);

            std::string s_Text;

            if (!m_NavMesh.m_areas[i].m_area->m_flags.IsImpassable() || m_NavMesh.m_areas[i].m_area->m_flags.
                ApplyObCostWhenFlagsDontMatch()) {
                const uint32_t obCostMult = m_NavMesh.m_areas[i].m_area->m_flags.GetObCostMult();
                const uint32_t staticCostMult = m_NavMesh.m_areas[i].m_area->m_flags.GetStaticCostMult();
                const uint32_t costMult = obCostMult > staticCostMult ? obCostMult : staticCostMult;

                s_Text = std::to_string(costMult);
            }
            else {
                s_Text = "---";
            }

            p_Renderer->DrawText3D(s_Text.c_str(), s_WorldMatrix, s_Color, s_Scale);
        }
    }
}

void DebugMod::DrawObstacles(IRenderer* p_Renderer) {
    ZPFObstacleManagerDeprecated* s_ObstacleManagerDeprecated = static_cast<ZPFObstacleManagerDeprecated*>(
        Globals::Pathfinder->m_obstacleManager);

    for (size_t i = 0; i < s_ObstacleManagerDeprecated->m_obstacles.size(); ++i) {
        const SVector4 s_Color = SVector4(1.f, 1.f, 0.f, 0.29804f);
        const SMatrix s_Transform = s_ObstacleManagerDeprecated->m_obstacles[i].GetTransform();
        const float4 s_HalfSize = s_ObstacleManagerDeprecated->m_obstacles[i].GetHalfSize();
        const SVector3 s_MinBound = -s_HalfSize;
        const SVector3 s_MaxBound = s_HalfSize;

        p_Renderer->DrawBoundingQuads3D(s_MinBound, s_MaxBound, s_Transform, s_Color);
    }

    for (size_t i = 0; i < s_ObstacleManagerDeprecated->m_obstacles.size(); ++i) {
        const SVector4 s_Color = SVector4(1.f, 1.f, 0.f, 1.f);
        const SMatrix s_Transform = s_ObstacleManagerDeprecated->m_obstacles[i].GetTransform();
        const float4 s_HalfSize = s_ObstacleManagerDeprecated->m_obstacles[i].GetHalfSize();

        const SVector3 s_MinBound = SVector3(-s_HalfSize.x, -s_HalfSize.y, -s_HalfSize.z);
        const SVector3 s_MaxBound = SVector3(s_HalfSize.x, s_HalfSize.y, s_HalfSize.z);

        p_Renderer->DrawOBB3D(s_MinBound, s_MaxBound, s_Transform, s_Color);
    }

    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

    if (!s_CurrentCamera) {
        return;
    }

    SMatrix s_WorldMatrix = s_CurrentCamera->GetWorldMatrix();

    std::swap(s_WorldMatrix.YAxis, s_WorldMatrix.ZAxis);

    static const SVector4 s_Color = SVector4(1.f, 1.f, 1.f, 1.f);
    static const float s_Scale = 0.3f;

    for (size_t i = 0; i < s_ObstacleManagerDeprecated->m_obstacles.size(); ++i) {
        ZPFObstacleManagerDeprecated::ZPFObstacleInternalDep* s_PFObstacleInternalDep = (
            ZPFObstacleManagerDeprecated::ZPFObstacleInternalDep*) (s_ObstacleManagerDeprecated->m_obstacles[i].
                                                                    m_internal.GetTarget());
        const SMatrix s_Transform = s_ObstacleManagerDeprecated->m_obstacles[i].GetTransform();
        const float4 s_HalfSize = s_ObstacleManagerDeprecated->m_obstacles[i].GetHalfSize();
        float4 s_TopCenter = s_Transform.Trans + s_Transform.ZAxis * (s_HalfSize.z + 0.5f);
        s_TopCenter.z += 2.0f;

        s_WorldMatrix.Trans = s_TopCenter;

        const std::string s_Text = fmt::format(
            "Entity ID: {:08x}\nObstacle Flags: {:04x}\nPenalty: {}",
            m_ObstaclesToEntityIDs[s_ObstacleManagerDeprecated->m_obstacles[i].m_internal.GetTarget()],
            s_PFObstacleInternalDep->m_obstacleDef.m_blockageFlags,
            s_PFObstacleInternalDep->m_obstacleDef.m_penalty
        );

        p_Renderer->DrawText3D(s_Text.c_str(), s_WorldMatrix, s_Color, s_Scale);
    }
}

void DebugMod::GenerateReasoningGridVertices() {
    GenerateVerticesForSmallQuads();
    GenerateVerticesForLargeQuads();
    GenerateVerticesForQuadBorderLines();
    GenerateVerticesForNeighborConnectionLines();
}

void DebugMod::GenerateVerticesForSmallQuads() {
    const SReasoningGrid* s_ReasoningGrid = *Globals::ActiveGrid;
    const size_t s_WaypointCount = s_ReasoningGrid->m_WaypointList.size();

    m_Triangles.reserve(m_Triangles.size() + s_WaypointCount * 2);

    for (size_t i = 0; i < s_WaypointCount; ++i) {
        const float s_HalfBaseLength = 0.1f;
        const float4& s_VertexPostion = s_ReasoningGrid->m_WaypointList[i].vPos;
        const SVector4 s_VertexColor = SVector4(0.f, 0.33333f, 1.f, 0.62745f);
        Triangle& s_Triangle1 = m_Triangles.emplace_back();
        Triangle& s_Triangle2 = m_Triangles.emplace_back();

        s_Triangle1.vertexPosition1 = {
            s_VertexPostion.x - s_HalfBaseLength,
            s_VertexPostion.y - s_HalfBaseLength,
            s_VertexPostion.z + s_HalfBaseLength
        };

        s_Triangle1.vertexPosition2 = {
            s_VertexPostion.x + s_HalfBaseLength,
            s_VertexPostion.y - s_HalfBaseLength,
            s_VertexPostion.z + s_HalfBaseLength
        };

        s_Triangle1.vertexPosition3 = {
            s_VertexPostion.x - s_HalfBaseLength,
            s_VertexPostion.y + s_HalfBaseLength,
            s_VertexPostion.z + s_HalfBaseLength
        };

        s_Triangle1.vertexColor1 = s_VertexColor;
        s_Triangle1.vertexColor2 = s_VertexColor;
        s_Triangle1.vertexColor3 = s_VertexColor;

        s_Triangle2.vertexPosition1 = {
            s_VertexPostion.x + s_HalfBaseLength,
            s_VertexPostion.y - s_HalfBaseLength,
            s_VertexPostion.z + s_HalfBaseLength
        };

        s_Triangle2.vertexPosition2 = {
            s_VertexPostion.x + s_HalfBaseLength,
            s_VertexPostion.y + s_HalfBaseLength,
            s_VertexPostion.z + s_HalfBaseLength
        };

        s_Triangle2.vertexPosition3 = {
            s_VertexPostion.x - s_HalfBaseLength,
            s_VertexPostion.y + s_HalfBaseLength,
            s_VertexPostion.z + s_HalfBaseLength
        };

        s_Triangle2.vertexColor1 = s_VertexColor;
        s_Triangle2.vertexColor2 = s_VertexColor;
        s_Triangle2.vertexColor3 = s_VertexColor;
    }
}

void DebugMod::GenerateVerticesForLargeQuads() {
    const SReasoningGrid* s_ReasoningGrid = *Globals::ActiveGrid;
    size_t s_WaypointCount = s_ReasoningGrid->m_WaypointList.size();

    m_Triangles.reserve(m_Triangles.size() + s_WaypointCount * 2);

    for (uint32_t w = 0; w < s_WaypointCount; ++w) {
        const float s_ZOffset = 0.05f;
        float4 s_VertexPostion = s_ReasoningGrid->m_WaypointList[w].vPos;
        const SVector4 s_VertexColor = SVector4(0.33333f, 0.f, 1.f, 0.43922f);
        Triangle& s_Triangle1 = m_Triangles.emplace_back();
        Triangle& s_Triangle2 = m_Triangles.emplace_back();

        s_VertexPostion.z += s_ZOffset;
        s_VertexPostion = (*Globals::GridManager)->GetCellUpperLeft(s_VertexPostion, s_ReasoningGrid->m_Properties);

        s_Triangle1.vertexPosition1 = {
            s_VertexPostion.x,
            s_VertexPostion.y,
            s_VertexPostion.z
        };

        s_Triangle1.vertexPosition2 = {
            s_VertexPostion.x + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.y,
            s_VertexPostion.z
        };

        s_Triangle1.vertexPosition3 = {
            s_VertexPostion.x + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.y + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.z
        };

        s_Triangle1.vertexColor1 = s_VertexColor;
        s_Triangle1.vertexColor2 = s_VertexColor;
        s_Triangle1.vertexColor3 = s_VertexColor;

        s_Triangle2.vertexPosition1 = {
            s_VertexPostion.x,
            s_VertexPostion.y,
            s_VertexPostion.z
        };

        s_Triangle2.vertexPosition2 = {
            s_VertexPostion.x + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.y + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.z
        };

        s_Triangle2.vertexPosition3 = {
            s_VertexPostion.x,
            s_VertexPostion.y + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.z
        };

        s_Triangle2.vertexColor1 = s_VertexColor;
        s_Triangle2.vertexColor2 = s_VertexColor;
        s_Triangle2.vertexColor3 = s_VertexColor;
    }
}

void DebugMod::GenerateVerticesForQuadBorderLines() {
    const SReasoningGrid* s_ReasoningGrid = *Globals::ActiveGrid;
    const size_t s_WaypointCount = s_ReasoningGrid->m_WaypointList.size();

    m_Lines.reserve(m_Lines.size() + s_WaypointCount * 4);

    for (size_t i = 0; i < s_WaypointCount; ++i) {
        Line& s_TopBorder = m_Lines.emplace_back();
        Line& s_RightBorder = m_Lines.emplace_back();
        Line& s_BottomBorder = m_Lines.emplace_back();
        Line& s_LeftBorder = m_Lines.emplace_back();
        float4 s_VertexPostion = s_ReasoningGrid->m_WaypointList[i].vPos;
        const SVector4 s_VertexColor = SVector4(0.f, 0.f, 0.f, 0.43922f);
        const float s_ZOffset = 0.075f;

        s_VertexPostion.z += s_ZOffset;
        s_VertexPostion = (*Globals::GridManager)->GetCellUpperLeft(s_VertexPostion, s_ReasoningGrid->m_Properties);

        s_TopBorder.start = {
            s_VertexPostion.x,
            s_VertexPostion.y,
            s_VertexPostion.z
        };

        s_TopBorder.end = {
            s_VertexPostion.x + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.y,
            s_VertexPostion.z
        };

        s_TopBorder.startColor = s_VertexColor;
        s_TopBorder.endColor = s_VertexColor;

        s_RightBorder.start = {
            s_VertexPostion.x + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.y,
            s_VertexPostion.z
        };

        s_RightBorder.end = {
            s_VertexPostion.x + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.y + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.z
        };

        s_RightBorder.startColor = s_VertexColor;
        s_RightBorder.endColor = s_VertexColor;

        s_BottomBorder.start = {
            s_VertexPostion.x + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.y + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.z
        };

        s_BottomBorder.end = {
            s_VertexPostion.x,
            s_VertexPostion.y + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.z
        };

        s_BottomBorder.startColor = s_VertexColor;
        s_BottomBorder.endColor = s_VertexColor;

        s_LeftBorder.start = {
            s_VertexPostion.x,
            s_VertexPostion.y + s_ReasoningGrid->m_Properties.fGridSpacing,
            s_VertexPostion.z
        };

        s_LeftBorder.end = {
            s_VertexPostion.x,
            s_VertexPostion.y,
            s_VertexPostion.z
        };

        s_LeftBorder.startColor = s_VertexColor;
        s_LeftBorder.endColor = s_VertexColor;
    }
}

void DebugMod::GenerateVerticesForNeighborConnectionLines() {
    const SReasoningGrid* s_ReasoningGrid = *Globals::ActiveGrid;
    const size_t s_WaypointCount = s_ReasoningGrid->m_WaypointList.size();

    m_Lines.reserve(m_Lines.size() + s_WaypointCount * 4);

    for (size_t i = 0; i < s_WaypointCount; ++i) {
        float4 s_VertexPostion1 = s_ReasoningGrid->m_WaypointList[i].vPos;
        const SVector4 s_VertexColor = SVector4(0.f, 0.33333f, 1.f, 0.62745f);
        const float s_ZOffset = 0.1f;

        s_VertexPostion1.z += s_ZOffset;

        short s_NeighborIndex = 0;
        int j = 4;

        while (j != 0) {
            if (s_ReasoningGrid->m_WaypointList[i].Neighbors[s_NeighborIndex] != -1) {
                Line& s_Line = m_Lines.emplace_back();
                const short s_Neighbor = s_ReasoningGrid->m_WaypointList[i].Neighbors[s_NeighborIndex];
                float4 s_VertexPostion2 = s_ReasoningGrid->m_WaypointList[s_Neighbor].vPos;

                s_VertexPostion2.z += s_ZOffset;

                s_Line.start = {
                    s_VertexPostion1.x,
                    s_VertexPostion1.y,
                    s_VertexPostion1.z
                };

                s_Line.end = {
                    s_VertexPostion2.x,
                    s_VertexPostion2.y,
                    s_VertexPostion2.z
                };

                s_Line.startColor = s_VertexColor;
                s_Line.endColor = s_VertexColor;
            }

            ++s_NeighborIndex;
            --j;
        }
    }
}

std::map<NavPower::Binary::Area*, uint32_t> DebugMod::GetAreaPointerToIndexMap() {
    std::map<NavPower::Binary::Area*, uint32_t> s_AreaPointerToIndexMap;
    uint32_t s_AreaIndex = 1;

    for (NavPower::Area area : m_NavMesh.m_areas) {
        s_AreaPointerToIndexMap.emplace(area.m_area, s_AreaIndex);

        s_AreaIndex++;
    }

    return s_AreaPointerToIndexMap;
}

float DebugMod::AngleBetween(const SVector3& a, const SVector3& b) {
    float angle = SVector3::DotProduct(a, b);
    angle /= (a.Length() * b.Length());
    return angle = acosf(angle);
}

SVector3 DebugMod::ProjectionOnto(const SVector3& a, const SVector3& b) {
    const SVector3 bn = b / b.Length();
    return bn * SVector3::DotProduct(a, bn);
}

bool DebugMod::AreOnSameSide(const SVector3& p1, const SVector3& p2, const SVector3& a, const SVector3& b) {
    const SVector3 cp1 = SVector3::CrossProduct(b - a, p1 - a);
    const SVector3 cp2 = SVector3::CrossProduct(b - a, p2 - a);

    if (SVector3::DotProduct(cp1, cp2) >= 0) {
        return true;
    }

    return false;
}

SVector3 DebugMod::ComputeTriangleNormal(const SVector3& t1, const SVector3& t2, const SVector3& t3) {
    const SVector3 u = t2 - t1;
    const SVector3 v = t3 - t1;
    const SVector3 normal = SVector3::CrossProduct(u, v);

    return normal;
}

bool DebugMod::IsInTriangle(
    const SVector3& point, const SVector3& triangle1, const SVector3& triangle2, const SVector3& triangle3
) {
    // Test to see if it is within an infinite prism that the triangle outlines.
    const bool within_tri_prisim = AreOnSameSide(point, triangle1, triangle2, triangle3) && AreOnSameSide(
                point, triangle2, triangle1, triangle3
            )
            && AreOnSameSide(point, triangle3, triangle1, triangle2);

    // If it isn't it will never be on the triangle
    if (!within_tri_prisim) {
        return false;
    }

    // Calulate Triangle's Normal
    const SVector3 n = ComputeTriangleNormal(triangle1, triangle2, triangle3);

    // Project the point onto this normal
    const SVector3 proj = ProjectionOnto(point, n);

    // If the distance from the triangle to the point is 0
    //	it lies on the triangle
    if (proj.Length() == 0) {
        return true;
    }

    return false;
}

void DebugMod::VertexTriangluation(const std::vector<SVector3>& vertices, std::vector<unsigned short>& indices) {
    // If there are 2 or less verts,
    // no triangle can be created,
    // so exit
    if (vertices.size() < 3) {
        return;
    }
    // If it is a triangle no need to calculate it
    if (vertices.size() == 3) {
        indices.push_back(0);
        indices.push_back(1);
        indices.push_back(2);
        return;
    }

    // Create a list of vertices
    std::vector<SVector3> tVerts = vertices;

    while (true) {
        // For every vertex
        for (int i = 0; i < int(tVerts.size()); i++) {
            // pPrev = the previous vertex in the list
            SVector3 pPrev;
            if (i == 0) {
                pPrev = tVerts[tVerts.size() - 1];
            }
            else {
                pPrev = tVerts[i - 1];
            }

            // pCur = the current vertex;
            SVector3 pCur = tVerts[i];

            // pNext = the next vertex in the list
            SVector3 pNext;
            if (i == tVerts.size() - 1) {
                pNext = tVerts[0];
            }
            else {
                pNext = tVerts[i + 1];
            }

            // Check to see if there are only 3 verts left
            // if so this is the last triangle
            if (tVerts.size() == 3) {
                // Create a triangle from pCur, pPrev, pNext
                for (int j = 0; j < int(tVerts.size()); j++) {
                    if (vertices[j] == pCur)
                        indices.push_back(j);
                    if (vertices[j] == pPrev)
                        indices.push_back(j);
                    if (vertices[j] == pNext)
                        indices.push_back(j);
                }

                tVerts.clear();
                break;
            }
            if (tVerts.size() == 4) {
                // Create a triangle from pCur, pPrev, pNext
                for (int j = 0; j < int(vertices.size()); j++) {
                    if (vertices[j] == pCur)
                        indices.push_back(j);
                    if (vertices[j] == pPrev)
                        indices.push_back(j);
                    if (vertices[j] == pNext)
                        indices.push_back(j);
                }

                SVector3 tempVec;
                for (int j = 0; j < int(tVerts.size()); j++) {
                    if (tVerts[j] != pCur
                        && tVerts[j] != pPrev
                        && tVerts[j] != pNext) {
                        tempVec = tVerts[j];
                        break;
                    }
                }

                // Create a triangle from pCur, pPrev, pNext
                for (int j = 0; j < int(vertices.size()); j++) {
                    if (vertices[j] == pPrev)
                        indices.push_back(j);
                    if (vertices[j] == pNext)
                        indices.push_back(j);
                    if (vertices[j] == tempVec)
                        indices.push_back(j);
                }

                tVerts.clear();
                break;
            }

            // If Vertex is not an interior vertex
            float angle = AngleBetween(pPrev - pCur, pNext - pCur) * (180 / 3.14159265359);
            if (angle <= 0 && angle >= 180)
                continue;

            // If any vertices are within this triangle
            bool inTri = false;
            for (int j = 0; j < int(vertices.size()); j++) {
                if (IsInTriangle(vertices[j], pPrev, pCur, pNext)
                    && vertices[j] != pPrev
                    && vertices[j] != pCur
                    && vertices[j] != pNext) {
                    inTri = true;
                    break;
                }
            }
            if (inTri)
                continue;

            // Create a triangle from pCur, pPrev, pNext
            for (int j = 0; j < int(vertices.size()); j++) {
                if (vertices[j] == pCur)
                    indices.push_back(j);
                if (vertices[j] == pPrev)
                    indices.push_back(j);
                if (vertices[j] == pNext)
                    indices.push_back(j);
            }

            // Delete pCur from the list
            for (int j = 0; j < int(tVerts.size()); j++) {
                if (tVerts[j] == pCur) {
                    tVerts.erase(tVerts.begin() + j);
                    break;
                }
            }

            // reset i to the start
            // -1 since loop will add 1 to it
            i = -1;
        }

        // if no triangles were created
        if (indices.size() == 0)
            break;

        // if no more vertices
        if (tVerts.size() == 0)
            break;
    }
}

std::string DebugMod::BehaviorToString(ECompiledBehaviorType p_Type) {
    switch (p_Type) {
        case ECompiledBehaviorType::BT_ConditionScope: return "BT_ConditionScope";
        case ECompiledBehaviorType::BT_Random: return "BT_Random";
        case ECompiledBehaviorType::BT_Match: return "BT_Match";
        case ECompiledBehaviorType::BT_Sequence: return "BT_Sequence";
        case ECompiledBehaviorType::BT_Dummy: return "BT_Dummy";
        case ECompiledBehaviorType::BT_Dummy2: return "BT_Dummy2";
        case ECompiledBehaviorType::BT_Error: return "BT_Error";
        case ECompiledBehaviorType::BT_Wait: return "BT_Wait";
        case ECompiledBehaviorType::BT_WaitForStanding: return "BT_WaitForStanding";
        case ECompiledBehaviorType::BT_WaitBasedOnDistanceToTarget: return "BT_WaitBasedOnDistanceToTarget";
        case ECompiledBehaviorType::BT_WaitForItemHandled: return "BT_WaitForItemHandled";
        case ECompiledBehaviorType::BT_AbandonOrder: return "BT_AbandonOrder";
        case ECompiledBehaviorType::BT_CompleteOrder: return "BT_CompleteOrder";
        case ECompiledBehaviorType::BT_PlayAct: return "BT_PlayAct";
        case ECompiledBehaviorType::BT_ConfiguredAct: return "BT_ConfiguredAct";
        case ECompiledBehaviorType::BT_PlayReaction: return "BT_PlayReaction";
        case ECompiledBehaviorType::BT_SimpleReaction: return "BT_SimpleReaction";
        case ECompiledBehaviorType::BT_SituationAct: return "BT_SituationAct";
        case ECompiledBehaviorType::BT_SituationApproach: return "BT_SituationApproach";
        case ECompiledBehaviorType::BT_SituationGetHelp: return "BT_SituationGetHelp";
        case ECompiledBehaviorType::BT_SituationFace: return "BT_SituationFace";
        case ECompiledBehaviorType::BT_SituationConversation: return "BT_SituationConversation";
        case ECompiledBehaviorType::BT_Holster: return "BT_Holster";
        case ECompiledBehaviorType::BT_SpeakWait: return "BT_SpeakWait";
        case ECompiledBehaviorType::BT_SpeakWaitWithFallbackIfAlone: return "BT_SpeakWaitWithFallbackIfAlone";
        case ECompiledBehaviorType::BT_ConfiguredSpeak: return "BT_ConfiguredSpeak";
        case ECompiledBehaviorType::BT_ConditionedConfiguredSpeak: return "BT_ConditionedConfiguredSpeak";
        case ECompiledBehaviorType::BT_ConditionedConfiguredAct: return "BT_ConditionedConfiguredAct";
        case ECompiledBehaviorType::BT_SpeakCustomOrDefaultDistractionAckSoundDef: return
                    "BT_SpeakCustomOrDefaultDistractionAckSoundDef";
        case ECompiledBehaviorType::BT_SpeakCustomOrDefaultDistractionInvestigationSoundDef: return
                    "BT_SpeakCustomOrDefaultDistractionInvestigationSoundDef";
        case ECompiledBehaviorType::BT_SpeakCustomOrDefaultDistractionStndSoundDef: return
                    "BT_SpeakCustomOrDefaultDistractionStndSoundDef";
        case ECompiledBehaviorType::BT_Pickup: return "BT_Pickup";
        case ECompiledBehaviorType::BT_Drop: return "BT_Drop";
        case ECompiledBehaviorType::BT_PlayConversation: return "BT_PlayConversation";
        case ECompiledBehaviorType::BT_PlayAnimation: return "BT_PlayAnimation";
        case ECompiledBehaviorType::BT_MoveToLocation: return "BT_MoveToLocation";
        case ECompiledBehaviorType::BT_MoveToTargetKnownPosition: return "BT_MoveToTargetKnownPosition";
        case ECompiledBehaviorType::BT_MoveToTargetActualPosition: return "BT_MoveToTargetActualPosition";
        case ECompiledBehaviorType::BT_MoveToInteraction: return "BT_MoveToInteraction";
        case ECompiledBehaviorType::BT_MoveToNPC: return "BT_MoveToNPC";
        case ECompiledBehaviorType::BT_FollowTargetKnownPosition: return "BT_FollowTargetKnownPosition";
        case ECompiledBehaviorType::BT_FollowTargetActualPosition: return "BT_FollowTargetActualPosition";
        case ECompiledBehaviorType::BT_PickUpItem: return "BT_PickUpItem";
        case ECompiledBehaviorType::BT_GrabItem: return "BT_GrabItem";
        case ECompiledBehaviorType::BT_PutDownItem: return "BT_PutDownItem";
        case ECompiledBehaviorType::BT_Search: return "BT_Search";
        case ECompiledBehaviorType::BT_LimitedSearch: return "BT_LimitedSearch";
        case ECompiledBehaviorType::BT_MoveTo: return "BT_MoveTo";
        case ECompiledBehaviorType::BT_Reposition: return "BT_Reposition";
        case ECompiledBehaviorType::BT_SituationMoveTo: return "BT_SituationMoveTo";
        case ECompiledBehaviorType::BT_FormationMove: return "BT_FormationMove";
        case ECompiledBehaviorType::BT_SituationJumpTo: return "BT_SituationJumpTo";
        case ECompiledBehaviorType::BT_AmbientWalk: return "BT_AmbientWalk";
        case ECompiledBehaviorType::BT_AmbientStand: return "BT_AmbientStand";
        case ECompiledBehaviorType::BT_CrowdAmbientStand: return "BT_CrowdAmbientStand";
        case ECompiledBehaviorType::BT_AmbientItemUse: return "BT_AmbientItemUse";
        case ECompiledBehaviorType::BT_AmbientLook: return "BT_AmbientLook";
        case ECompiledBehaviorType::BT_Act: return "BT_Act";
        case ECompiledBehaviorType::BT_Patrol: return "BT_Patrol";
        case ECompiledBehaviorType::BT_MoveToPosition: return "BT_MoveToPosition";
        case ECompiledBehaviorType::BT_AlertedStand: return "BT_AlertedStand";
        case ECompiledBehaviorType::BT_AlertedDebug: return "BT_AlertedDebug";
        case ECompiledBehaviorType::BT_AttentionToPerson: return "BT_AttentionToPerson";
        case ECompiledBehaviorType::BT_StunnedByFlashGrenade: return "BT_StunnedByFlashGrenade";
        case ECompiledBehaviorType::BT_CuriousIdle: return "BT_CuriousIdle";
        case ECompiledBehaviorType::BT_InvestigateWeapon: return "BT_InvestigateWeapon";
        case ECompiledBehaviorType::BT_DeliverWeapon: return "BT_DeliverWeapon";
        case ECompiledBehaviorType::BT_RecoverUnconscious: return "BT_RecoverUnconscious";
        case ECompiledBehaviorType::BT_GetOutfit: return "BT_GetOutfit";
        case ECompiledBehaviorType::BT_RadioCall: return "BT_RadioCall";
        case ECompiledBehaviorType::BT_EscortOut: return "BT_EscortOut";
        case ECompiledBehaviorType::BT_StashItem: return "BT_StashItem";
        case ECompiledBehaviorType::BT_CautiousSearchPosition: return "BT_CautiousSearchPosition";
        case ECompiledBehaviorType::BT_LockdownWarning: return "BT_LockdownWarning";
        case ECompiledBehaviorType::BT_WakeUpUnconscious: return "BT_WakeUpUnconscious";
        case ECompiledBehaviorType::BT_DeadBodyInvestigate: return "BT_DeadBodyInvestigate";
        case ECompiledBehaviorType::BT_GuardDeadBody: return "BT_GuardDeadBody";
        case ECompiledBehaviorType::BT_DragDeadBody: return "BT_DragDeadBody";
        case ECompiledBehaviorType::BT_CuriousBystander: return "BT_CuriousBystander";
        case ECompiledBehaviorType::BT_DeadBodyBystander: return "BT_DeadBodyBystander";
        case ECompiledBehaviorType::BT_StandOffArrest: return "BT_StandOffArrest";
        case ECompiledBehaviorType::BT_StandOffReposition: return "BT_StandOffReposition";
        case ECompiledBehaviorType::BT_StandAndAim: return "BT_StandAndAim";
        case ECompiledBehaviorType::BT_CloseCombat: return "BT_CloseCombat";
        case ECompiledBehaviorType::BT_MoveToCloseCombat: return "BT_MoveToCloseCombat";
        case ECompiledBehaviorType::BT_MoveAwayFromCloseCombat: return "BT_MoveAwayFromCloseCombat";
        case ECompiledBehaviorType::BT_CoverFightSeasonTwo: return "BT_CoverFightSeasonTwo";
        case ECompiledBehaviorType::BT_ShootFromPosition: return "BT_ShootFromPosition";
        case ECompiledBehaviorType::BT_StandAndShoot: return "BT_StandAndShoot";
        case ECompiledBehaviorType::BT_CheckLastPosition: return "BT_CheckLastPosition";
        case ECompiledBehaviorType::BT_ProtoSearchIdle: return "BT_ProtoSearchIdle";
        case ECompiledBehaviorType::BT_ProtoApproachSearchArea: return "BT_ProtoApproachSearchArea";
        case ECompiledBehaviorType::BT_ProtoSearchPosition: return "BT_ProtoSearchPosition";
        case ECompiledBehaviorType::BT_ShootTarget: return "BT_ShootTarget";
        case ECompiledBehaviorType::BT_TriggerAlarm: return "BT_TriggerAlarm";
        case ECompiledBehaviorType::BT_MoveInCover: return "BT_MoveInCover";
        case ECompiledBehaviorType::BT_MoveToCover: return "BT_MoveToCover";
        case ECompiledBehaviorType::BT_HomeAttackOrigin: return "BT_HomeAttackOrigin";
        case ECompiledBehaviorType::BT_Shoot: return "BT_Shoot";
        case ECompiledBehaviorType::BT_Aim: return "BT_Aim";
        case ECompiledBehaviorType::BT_MoveToRandomNeighbourNode: return "BT_MoveToRandomNeighbourNode";
        case ECompiledBehaviorType::BT_MoveToRandomNeighbourNodeAiming: return "BT_MoveToRandomNeighbourNodeAiming";
        case ECompiledBehaviorType::BT_MoveToAndPlayCombatPositionAct: return "BT_MoveToAndPlayCombatPositionAct";
        case ECompiledBehaviorType::BT_MoveToAimingAndPlayCombatPositionAct: return
                    "BT_MoveToAimingAndPlayCombatPositionAct";
        case ECompiledBehaviorType::BT_PlayJumpyReaction: return "BT_PlayJumpyReaction";
        case ECompiledBehaviorType::BT_JumpyInvestigation: return "BT_JumpyInvestigation";
        case ECompiledBehaviorType::BT_AgitatedPatrol: return "BT_AgitatedPatrol";
        case ECompiledBehaviorType::BT_AgitatedGuard: return "BT_AgitatedGuard";
        case ECompiledBehaviorType::BT_HeroEscort: return "BT_HeroEscort";
        case ECompiledBehaviorType::BT_Escort: return "BT_Escort";
        case ECompiledBehaviorType::BT_ControlledFormationMove: return "BT_ControlledFormationMove";
        case ECompiledBehaviorType::BT_EscortSearch: return "BT_EscortSearch";
        case ECompiledBehaviorType::BT_LeadEscort: return "BT_LeadEscort";
        case ECompiledBehaviorType::BT_LeadEscort2: return "BT_LeadEscort2";
        case ECompiledBehaviorType::BT_AimReaction: return "BT_AimReaction";
        case ECompiledBehaviorType::BT_FollowHitman: return "BT_FollowHitman";
        case ECompiledBehaviorType::BT_RideTheLightning: return "BT_RideTheLightning";
        case ECompiledBehaviorType::BT_Scared: return "BT_Scared";
        case ECompiledBehaviorType::BT_Flee: return "BT_Flee";
        case ECompiledBehaviorType::BT_AgitatedBystander: return "BT_AgitatedBystander";
        case ECompiledBehaviorType::BT_SentryFrisk: return "BT_SentryFrisk";
        case ECompiledBehaviorType::BT_SentryIdle: return "BT_SentryIdle";
        case ECompiledBehaviorType::BT_SentryWarning: return "BT_SentryWarning";
        case ECompiledBehaviorType::BT_SentryCheckItem: return "BT_SentryCheckItem";
        case ECompiledBehaviorType::BT_VIPScared: return "BT_VIPScared";
        case ECompiledBehaviorType::BT_VIPSafeRoomTrespasser: return "BT_VIPSafeRoomTrespasser";
        case ECompiledBehaviorType::BT_DefendVIP: return "BT_DefendVIP";
        case ECompiledBehaviorType::BT_CautiousVIP: return "BT_CautiousVIP";
        case ECompiledBehaviorType::BT_CautiousGuardVIP: return "BT_CautiousGuardVIP";
        case ECompiledBehaviorType::BT_InfectedConfused: return "BT_InfectedConfused";
        case ECompiledBehaviorType::BT_EnterInfected: return "BT_EnterInfected";
        case ECompiledBehaviorType::BT_CureInfected: return "BT_CureInfected";
        case ECompiledBehaviorType::BT_SickActInfected: return "BT_SickActInfected";
        case ECompiledBehaviorType::BT_Smart: return "BT_Smart";
        case ECompiledBehaviorType::BT_Controlled: return "BT_Controlled";
        case ECompiledBehaviorType::BT_SpeakTest: return "BT_SpeakTest";
        case ECompiledBehaviorType::BT_Conversation: return "BT_Conversation";
        case ECompiledBehaviorType::BT_RunToHelp: return "BT_RunToHelp";
        case ECompiledBehaviorType::BT_WaitForDialog: return "BT_WaitForDialog";
        case ECompiledBehaviorType::BT_WaitForConfiguredAct: return "BT_WaitForConfiguredAct";
        case ECompiledBehaviorType::BT_TestFlashbangGrenadeThrow: return "BT_TestFlashbangGrenadeThrow";
        case ECompiledBehaviorType::BT_BEHAVIORS_END: return "BT_BEHAVIORS_END";
        case ECompiledBehaviorType::BT_RenewEvent: return "BT_RenewEvent";
        case ECompiledBehaviorType::BT_ExpireEvent: return "BT_ExpireEvent";
        case ECompiledBehaviorType::BT_ExpireEvents: return "BT_ExpireEvents";
        case ECompiledBehaviorType::BT_SetEventHandled: return "BT_SetEventHandled";
        case ECompiledBehaviorType::BT_RenewSharedEvent: return "BT_RenewSharedEvent";
        case ECompiledBehaviorType::BT_ExpireSharedEvent: return "BT_ExpireSharedEvent";
        case ECompiledBehaviorType::BT_ExpireAllEvents: return "BT_ExpireAllEvents";
        case ECompiledBehaviorType::BT_CreateOrJoinSituation: return "BT_CreateOrJoinSituation";
        case ECompiledBehaviorType::BT_JoinSituation: return "BT_JoinSituation";
        case ECompiledBehaviorType::BT_ForceActorToJoinSituation: return "BT_ForceActorToJoinSituation";
        case ECompiledBehaviorType::BT_JoinSituationWithActor: return "BT_JoinSituationWithActor";
        case ECompiledBehaviorType::BT_LeaveSituation: return "BT_LeaveSituation";
        case ECompiledBehaviorType::BT_Escalate: return "BT_Escalate";
        case ECompiledBehaviorType::BT_GotoPhase: return "BT_GotoPhase";
        case ECompiledBehaviorType::BT_RenewGoal: return "BT_RenewGoal";
        case ECompiledBehaviorType::BT_ExpireGoal: return "BT_ExpireGoal";
        case ECompiledBehaviorType::BT_RenewGoalOf: return "BT_RenewGoalOf";
        case ECompiledBehaviorType::BT_ExpireGoalOf: return "BT_ExpireGoalOf";
        case ECompiledBehaviorType::BT_SetTension: return "BT_SetTension";
        case ECompiledBehaviorType::BT_TriggerSpotted: return "BT_TriggerSpotted";
        case ECompiledBehaviorType::BT_CopyKnownLocation: return "BT_CopyKnownLocation";
        case ECompiledBehaviorType::BT_UpdateKnownLocation: return "BT_UpdateKnownLocation";
        case ECompiledBehaviorType::BT_TransferKnownObjectPositions: return "BT_TransferKnownObjectPositions";
        case ECompiledBehaviorType::BT_WitnessAttack: return "BT_WitnessAttack";
        case ECompiledBehaviorType::BT_Speak: return "BT_Speak";
        case ECompiledBehaviorType::BT_StartDynamicEnforcer: return "BT_StartDynamicEnforcer";
        case ECompiledBehaviorType::BT_StopDynamicEnforcer: return "BT_StopDynamicEnforcer";
        case ECompiledBehaviorType::BT_StartRangeBasedDynamicEnforcer: return "BT_StartRangeBasedDynamicEnforcer";
        case ECompiledBehaviorType::BT_StopRangeBasedDynamicEnforcerForLocation: return
                    "BT_StopRangeBasedDynamicEnforcerForLocation";
        case ECompiledBehaviorType::BT_StopRangeBasedDynamicEnforcer: return "BT_StopRangeBasedDynamicEnforcer";
        case ECompiledBehaviorType::BT_SetDistracted: return "BT_SetDistracted";
        case ECompiledBehaviorType::BT_IgnoreAllDistractionsExceptTheNewest: return
                    "BT_IgnoreAllDistractionsExceptTheNewest";
        case ECompiledBehaviorType::BT_IgnoreDistractions: return "BT_IgnoreDistractions";
        case ECompiledBehaviorType::BT_PerceptibleEntityNotifyWillReact: return "BT_PerceptibleEntityNotifyWillReact";
        case ECompiledBehaviorType::BT_PerceptibleEntityNotifyReacted: return "BT_PerceptibleEntityNotifyReacted";
        case ECompiledBehaviorType::BT_PerceptibleEntityNotifyInvestigating: return
                    "BT_PerceptibleEntityNotifyInvestigating";
        case ECompiledBehaviorType::BT_PerceptibleEntityNotifyInvestigated: return
                    "BT_PerceptibleEntityNotifyInvestigated";
        case ECompiledBehaviorType::BT_PerceptibleEntityNotifyTerminate: return "BT_PerceptibleEntityNotifyTerminate";
        case ECompiledBehaviorType::BT_LeaveDistractionAssistantRole: return "BT_LeaveDistractionAssistantRole";
        case ECompiledBehaviorType::BT_LeaveDistractionAssitingGuardRole: return "BT_LeaveDistractionAssitingGuardRole";
        case ECompiledBehaviorType::BT_RequestSuitcaseAssistanceOverRadio: return
                    "BT_RequestSuitcaseAssistanceOverRadio";
        case ECompiledBehaviorType::BT_RequestSuitcaseAssistanceFaceToFace: return
                    "BT_RequestSuitcaseAssistanceFaceToFace";
        case ECompiledBehaviorType::BT_ExpireArrestReasons: return "BT_ExpireArrestReasons";
        case ECompiledBehaviorType::BT_SetDialogSwitch_NPCID: return "BT_SetDialogSwitch_NPCID";
        case ECompiledBehaviorType::BT_InfectedAssignToFollowPlayer: return "BT_InfectedAssignToFollowPlayer";
        case ECompiledBehaviorType::BT_InfectedRemoveFromFollowPlayer: return "BT_InfectedRemoveFromFollowPlayer";
        case ECompiledBehaviorType::BT_Log: return "BT_Log";
        case ECompiledBehaviorType::BT_COMMANDS_END: return "BT_COMMANDS_END";
        case ECompiledBehaviorType::BT_Invalid: return "BT_Invalid";
        default: return "<unknown>";
    }
}

DEFINE_PLUGIN_DETOUR(DebugMod, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData&) {
    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(DebugMod, void, OnClearScene, ZEntitySceneContext* th, bool forReload) {
    m_RenderActorBoxes = false;
    m_RenderActorNames = false;
    m_RenderActorRepoIds = false;
    m_RenderActorBehaviors = false;

    m_DrawReasoningGrid = false;
    m_ShowVisibility = false;
    m_ShowLayers = false;
    m_ShowIndices = false;
    m_Lines.clear();
    m_Triangles.clear();

    m_DrawNavMesh = false;
    m_DrawObstacles = false;
    m_NavMesh = {};
    m_NavpData.clear();
    m_Vertices.clear();
    m_Indices.clear();
    m_NavMeshLines.clear();
    m_NavMeshConnectivityLines.clear();
    m_ObstaclesToEntityIDs.clear();

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(
    DebugMod, void, ZPFObstacleEntity_UpdateObstacle, ZPFObstacleEntity* th, uint32 nObstacleBlockageFlags,
    bool bEnabled, bool forceUpdate
) {
    p_Hook->CallOriginal(th, nObstacleBlockageFlags, bEnabled, forceUpdate);

    m_ObstaclesToEntityIDs[th->m_obstacle.m_internal.GetTarget()] = th->GetType()->m_nEntityId;

    return HookResult<void>(HookAction::Return());
}

DEFINE_ZHM_PLUGIN(DebugMod);
