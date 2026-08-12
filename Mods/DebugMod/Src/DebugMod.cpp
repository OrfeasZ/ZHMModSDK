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
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZColor.h>
#include <Glacier/ZGrid.h>
#include <Glacier/ZBehavior.h>

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
	if (ImGui::Button(ICON_MD_BUILD " DEBUG")) {
		m_DebugMenuActive = !m_DebugMenuActive;
	}

	if (ImGui::Button(ICON_MD_PLACE " POSITIONS")) {
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
	const auto s_Showing = ImGui::Begin("Debug", &m_DebugMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing) {
		if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Render health###PlayerHealth", &m_RenderPlayerHealth);
			ImGui::Checkbox("Render outfit hit points###PlayerOutfitHitPoints", &m_RenderPlayerOutfitHitPoints);
		}

		if (ImGui::CollapsingHeader("Actors", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Render position boxes", &m_RenderActorBoxes);
			ImGui::Checkbox("Render names", &m_RenderActorNames);
			ImGui::Checkbox("Render repository IDs", &m_RenderActorRepoIds);
			ImGui::Checkbox("Render behaviors", &m_RenderActorBehaviors);
			ImGui::Checkbox("Render health###ActorHealth", &m_RenderActorHealth);
			ImGui::Checkbox("Render outfit hit points###ActorOutfitHitPoints", &m_RenderActorOutfitHitPoints);
		}

		if (ImGui::CollapsingHeader("Reasoning grid", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Checkbox("Draw reasoning grid", &m_DrawReasoningGrid)) {
				if (m_Triangles.size() == 0) {
					GenerateReasoningGridVertices();
				}
			}

			ImGui::Checkbox("Show visibility", &m_ShowVisibility);
			ImGui::Checkbox("Show layers", &m_ShowLayers);
			ImGui::Checkbox("Show indices", &m_ShowIndices);
		}

		if (ImGui::CollapsingHeader("Guide path finder", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Checkbox("Draw nav mesh", &m_DrawNavMesh)) {
				if (m_Areas.size() == 0) {
					BuildNavMeshRenderData();
				}
			}

			ImGui::Checkbox("Draw planner areas", &m_DrawPlannerAreas);
			ImGui::Checkbox("Draw planner areas solid", &m_DrawPlannerAreasSolid);
			ImGui::Checkbox("Colorize area usage flags", &m_ColorizeAreaUsageFlags);
			ImGui::Checkbox("Draw obstacles", &m_DrawObstacles);
			ImGui::Checkbox("Draw planner connectivity", &m_DrawDrawPlannerConnectivity);
			ImGui::Checkbox("Draw area penalty multipliers", &m_DrawAreaPenaltyMults);
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

	if (m_RenderPlayerHealth || m_RenderPlayerOutfitHitPoints) {
		const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

		if (!s_CurrentCamera) {
			return;
		}

		auto s_LocalHitman = SDK()->GetLocalPlayer();

		if (s_LocalHitman) {
			auto s_CameraTransform = s_CurrentCamera->GetObjectToWorldMatrix();

			auto* s_SpatialEntity = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();
			auto s_PlayerTransform = s_SpatialEntity->GetObjectToWorldMatrix();

			float4 s_Min, s_Max;

			s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

			const float4 s_Center = (s_Min + s_Max) * 0.5f;
			const float4 s_Extents = (s_Max - s_Min) * 0.5f;

			float4 s_LocalPosition = s_Center + float4(0.f, 0.f, s_Extents.z, 0.f);
			float4 s_WorldPosition = s_PlayerTransform * s_LocalPosition;

			s_WorldPosition.z -= 0.5f;
			s_CameraTransform.Trans = s_WorldPosition;

			std::string s_Text;

			if (m_RenderPlayerHealth) {
				const auto s_HM5Health = s_LocalHitman.m_pInterfaceRef->m_pHealth;
				const auto s_Health = (s_HM5Health->m_fHitPoints / s_HM5Health->m_fMaxHitPoints) * 100.f;
				char s_Buffer[64];

				snprintf(s_Buffer, sizeof(s_Buffer), "Health: %.0f%%", s_Health);

				s_Text += s_Buffer;
			}

			if (m_RenderPlayerOutfitHitPoints) {
				if (s_Text.length() > 0) {
					s_Text += "\n\n";
				}

				char s_Buffer[64];

				snprintf(
					s_Buffer,
					sizeof(s_Buffer),
					"Hit points: %.0f",
					s_LocalHitman.m_pInterfaceRef->m_rOutfitKit.m_pInterfaceRef->m_fHitPoints
				);

				s_Text += s_Buffer;
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

	if (m_RenderActorBoxes ||
		m_RenderActorNames ||
		m_RenderActorRepoIds ||
		m_RenderActorBehaviors ||
		m_RenderActorHealth ||
		m_RenderActorOutfitHitPoints) {
		for (size_t i = 0; i < *Globals::NextActorId; ++i) {
			auto* s_Actor = Globals::ActorManager->m_activatedActors[i].m_pInterfaceRef;

			ZEntityRef s_Ref;
			s_Actor->GetID(s_Ref);

			auto* s_SpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();
			auto s_ActorTransform = s_SpatialEntity->GetObjectToWorldMatrix();

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

				auto s_CameraTransform = s_CurrentCamera->GetObjectToWorldMatrix();

				const float4 s_Center = (s_Min + s_Max) * 0.5f;
				const float4 s_Extents = (s_Max - s_Min) * 0.5f;

				float4 s_LocalPosition = s_Center + float4(0.f, 0.f, s_Extents.z, 0.f);
				float4 s_WorldPosition = s_ActorTransform * s_LocalPosition;

				s_WorldPosition.z -= 0.5f;
				s_CameraTransform.Trans = s_WorldPosition;

				std::string s_Text;

				if (m_RenderActorNames) {
					s_Text += s_Actor->GetActorName().c_str();
				}

				if (m_RenderActorRepoIds) {
					auto* s_RepoEntity = s_Ref.QueryInterface<ZRepositoryItemEntity>();

					if (s_Text.length() > 0) {
						s_Text += "\n\n";
					}

					s_Text += s_RepoEntity->m_sId.ToString().c_str();
				}

				if (m_RenderActorBehaviors) {
					const SBehaviorBase* s_BehaviorBase = Globals::BehaviorService->m_aBehaviorStates[i].
						m_pCurrentBehavior;

					if (s_BehaviorBase) {
						if (s_Text.length() > 0) {
							s_Text += "\n\n";
						}

						s_Text += CompiledBehaviorTypeToString(s_BehaviorBase->eBehaviorType);
					}
				}

				if (m_RenderActorHealth) {
					if (s_Text.length() > 0) {
						s_Text += "\n\n";
					}

					char s_Buffer[64];

					snprintf(s_Buffer, sizeof(s_Buffer), "Health: %.0f%%", s_Actor->m_fCurrentHitPoints);

					s_Text += s_Buffer;
				}

				if (m_RenderActorOutfitHitPoints) {
					if (s_Text.length() > 0) {
						s_Text += "\n\n";
					}

					char s_Buffer[64];

					snprintf(
						s_Buffer,
						sizeof(s_Buffer),
						"Hit points: %.0f",
						s_Actor->m_rOutfit.m_pInterfaceRef->m_fHitPoints
					);

					s_Text += s_Buffer;
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

		p_Renderer->SetDistanceCullingEnabled(true);

		SMatrix s_WorldMatrix = s_CurrentCamera->GetObjectToWorldMatrix();
		const size_t s_WaypointCount = s_ReasoningGrid->m_WaypointList.size();

		static const SVector4 s_Color = SVector4(0.f, 0.f, 0.f, 1.f);
		static const float s_Scale = 0.2f;

		for (size_t i = 0; i < s_WaypointCount; ++i) {
			float4 s_WorldPosition = s_ReasoningGrid->m_WaypointList[i].vPos;

			s_WorldPosition.z += 0.5f;
			s_WorldMatrix.Trans = s_WorldPosition;

			const std::string s_Text = std::to_string(i);

			p_Renderer->DrawText3D(s_Text.c_str(), s_WorldMatrix, s_Color, s_Scale);
		}

		p_Renderer->SetDistanceCullingEnabled(false);
	}
}

void DebugMod::DrawNavMesh(IRenderer* p_Renderer) {
	static const SVector4 s_GreenTriangleColor = SVector4(0.19608f, 0.80392f, 0.19608f, 0.49804f);
	static const SVector4 s_YellowTriangleColor = SVector4(1.f, 1.f, 0.f, 0.49804f);

	if (m_DrawPlannerAreasSolid) {
		for (size_t i = 0; i < m_Areas.size(); ++i) {
			if (m_ColorizeAreaUsageFlags && m_Areas[i]->m_area->m_usageFlags ==
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

		SMatrix s_WorldMatrix = s_CurrentCamera->GetObjectToWorldMatrix();

		static const SVector4 s_Color = SVector4(1.f, 1.f, 1.f, 1.f);
		static const float s_Scale = 0.2f;

		for (size_t i = 0; i < m_Areas.size(); ++i) {
			SVector3 s_WorldPosition = m_Areas[i]->m_area->m_pos;

			const DirectX::XMVECTOR s_WorldPosition2 = DirectX::XMVectorSet(
				s_WorldPosition.x, s_WorldPosition.y, s_WorldPosition.z, 1.0f
			);

			s_WorldPosition.z += 2.f;
			s_WorldMatrix.Trans = float4(s_WorldPosition.x, s_WorldPosition.y, s_WorldPosition.z, 1.0f);

			std::string s_Text;

			if (!m_Areas[i]->m_area->m_flags.IsImpassable() || m_Areas[i]->m_area->m_flags.
				ApplyObCostWhenFlagsDontMatch()) {
				const uint32_t obCostMult = m_Areas[i]->m_area->m_flags.GetObCostMult();
				const uint32_t staticCostMult = m_Areas[i]->m_area->m_flags.GetStaticCostMult();
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

	SMatrix s_WorldMatrix = s_CurrentCamera->GetObjectToWorldMatrix();

	static const SVector4 s_Color = SVector4(1.f, 1.f, 1.f, 1.f);
	static const float s_Scale = 0.1f;

	for (size_t i = 0; i < s_ObstacleManagerDeprecated->m_obstacles.size(); ++i) {
		ZPFObstacleManagerDeprecated::ZPFObstacleInternalDep* s_PFObstacleInternalDep = (
			ZPFObstacleManagerDeprecated::ZPFObstacleInternalDep*)(s_ObstacleManagerDeprecated->m_obstacles[i].
				m_internal.GetTarget());
		const SMatrix s_Transform = s_ObstacleManagerDeprecated->m_obstacles[i].GetTransform();
		const float4 s_HalfSize = s_ObstacleManagerDeprecated->m_obstacles[i].GetHalfSize();
		float4 s_TopCenter = s_Transform.Trans + s_Transform.ZAxis * (s_HalfSize.z + 0.5f);
		s_TopCenter.z += 2.0f;

		s_WorldMatrix.Trans = s_TopCenter;

		std::string s_Text;

		if (const auto it = m_ObstacleToEntityID.find(s_PFObstacleInternalDep); it != m_ObstacleToEntityID.end()) {
			s_Text += fmt::format("Entity ID: {:016X}\n", it->second);
		}

		s_Text += fmt::format(
			"Obstacle flags: 0x{:04X}\nPenalty: {}",
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

void DebugMod::BuildNavMeshRenderData() {
	static const SVector4 s_LineColor = SVector4(0.f, 1.f, 0.f, 1.f);
	static const SVector4 s_AdjacentLineColor = SVector4(1.f, 1.f, 1.f, 1.f);

	const uintptr_t s_NavpData = reinterpret_cast<uintptr_t>(Globals::Pathfinder->m_aLoadedNavMeshes[0]
		.m_pNavpowerResource);
	const uint32_t s_NavpDataSize = Globals::Pathfinder->m_aLoadedNavMeshes[0].m_nNavpowerResourceSize;

	m_NavpData.resize(s_NavpDataSize);

	std::memcpy(m_NavpData.data(), reinterpret_cast<void*>(s_NavpData), s_NavpDataSize);

	m_NavMesh.read(reinterpret_cast<uintptr_t>(m_NavpData.data()), s_NavpDataSize);

	for (auto& section : m_NavMesh.m_aSections) {
		for (auto& graph : section.m_aNavGraphs) {
			for (auto& area : graph.m_areas) {
				m_Areas.push_back(&area);
			}
		}
	}

	m_Vertices.resize(m_Areas.size());
	m_Indices.resize(m_Areas.size());
	m_NavMeshLines.reserve(m_Areas.size() * 3);
	m_NavMeshConnectivityLines.reserve(m_Areas.size() * 3);

	std::map<NavPower::Binary::Area*, uint32_t> s_AreaPointerToIndexMap = GetAreaPointerToIndexMap();

	for (size_t i = 0; i < m_Areas.size(); ++i) {
		const size_t s_VertexCount = m_Areas[i]->m_edges.size();

		m_Vertices[i].reserve(s_VertexCount);

		const SVector3 s_Centroid = m_Areas[i]->CalculateCentroid();

		for (size_t j = 0; j < s_VertexCount; ++j) {
			m_Vertices[i].push_back(m_Areas[i]->m_edges[j]->m_pos);

			const size_t s_NextIndex = (j + 1) % s_VertexCount;
			Line& s_Line = m_NavMeshLines.emplace_back();

			s_Line.start = m_Areas[i]->m_edges[j]->m_pos;
			s_Line.startColor = s_LineColor;

			s_Line.end = m_Areas[i]->m_edges[s_NextIndex]->m_pos;
			s_Line.endColor = s_LineColor;

			NavPower::Binary::Area* s_AdjArea = m_Areas[i]->m_edges[j]->m_pAdjArea;

			if (s_AdjArea) {
				const uint32_t s_AdjacentAreaIndex = s_AreaPointerToIndexMap[s_AdjArea];
				NavPower::Area& s_AdjacentArea = *m_Areas[s_AdjacentAreaIndex - 1];
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

std::map<NavPower::Binary::Area*, uint32_t> DebugMod::GetAreaPointerToIndexMap() {
	std::map<NavPower::Binary::Area*, uint32_t> s_AreaPointerToIndexMap;
	uint32_t s_AreaIndex = 1;

	for (NavPower::Area* area : m_Areas) {
		s_AreaPointerToIndexMap.emplace(area->m_area, s_AreaIndex);

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

const char* DebugMod::CompiledBehaviorTypeToString(ECompiledBehaviorType p_Type) {
	switch (p_Type) {
		case ECompiledBehaviorType::BT_ConditionScope: return "Condition scope";
		case ECompiledBehaviorType::BT_Random: return "Random";
		case ECompiledBehaviorType::BT_Match: return "Match";
		case ECompiledBehaviorType::BT_Sequence: return "Sequence";
		case ECompiledBehaviorType::BT_Dummy: return "Dummy";
		case ECompiledBehaviorType::BT_Dummy2: return "Dummy2";
		case ECompiledBehaviorType::BT_Error: return "Error";
		case ECompiledBehaviorType::BT_Wait: return "Wait";
		case ECompiledBehaviorType::BT_WaitForStanding: return "Wait for standing";
		case ECompiledBehaviorType::BT_WaitBasedOnDistanceToTarget: return "Wait based on distance to target";
		case ECompiledBehaviorType::BT_WaitForItemHandled: return "Wait for item handled";
		case ECompiledBehaviorType::BT_AbandonOrder: return "Abandon order";
		case ECompiledBehaviorType::BT_CompleteOrder: return "Complete order";
		case ECompiledBehaviorType::BT_PlayAct: return "Play act";
		case ECompiledBehaviorType::BT_ConfiguredAct: return "Configured act";
		case ECompiledBehaviorType::BT_PlayReaction: return "Play reaction";
		case ECompiledBehaviorType::BT_SimpleReaction: return "Simple reaction";
		case ECompiledBehaviorType::BT_SituationAct: return "Situation act";
		case ECompiledBehaviorType::BT_SituationApproach: return "Situation approach";
		case ECompiledBehaviorType::BT_SituationGetHelp: return "Situation get help";
		case ECompiledBehaviorType::BT_SituationFace: return "Situation face";
		case ECompiledBehaviorType::BT_SituationConversation: return "Situation conversation";
		case ECompiledBehaviorType::BT_Holster: return "Holster";
		case ECompiledBehaviorType::BT_SpeakWait: return "Speak wait";
		case ECompiledBehaviorType::BT_SpeakWaitWithFallbackIfAlone: return "Speak wait with fallback if alone";
		case ECompiledBehaviorType::BT_ConfiguredSpeak: return "Configured speak";
		case ECompiledBehaviorType::BT_ConditionedConfiguredSpeak: return "Conditioned configured speak";
		case ECompiledBehaviorType::BT_ConditionedConfiguredAct: return "Conditioned configured act";
		case ECompiledBehaviorType::BT_SpeakCustomOrDefaultDistractionAckSoundDef: return
			"Speak custom or default distraction acknowledgment sound definition";
		case ECompiledBehaviorType::BT_SpeakCustomOrDefaultDistractionInvestigationSoundDef: return
			"Speak custom or default distraction investigation sound definition";
		case ECompiledBehaviorType::BT_SpeakCustomOrDefaultDistractionStndSoundDef: return
			"Speak custom or default distraction standdown sound definition";
		case ECompiledBehaviorType::BT_Pickup: return "Pickup";
		case ECompiledBehaviorType::BT_Drop: return "Drop";
		case ECompiledBehaviorType::BT_PlayConversation: return "Play conversation";
		case ECompiledBehaviorType::BT_PlayAnimation: return "Play animation";
		case ECompiledBehaviorType::BT_MoveToLocation: return "Move to location";
		case ECompiledBehaviorType::BT_MoveToTargetKnownPosition: return "Move to target known position";
		case ECompiledBehaviorType::BT_MoveToTargetActualPosition: return "Move to target actual position";
		case ECompiledBehaviorType::BT_MoveToInteraction: return "Move to interaction";
		case ECompiledBehaviorType::BT_MoveToNPC: return "Move to NPC";
		case ECompiledBehaviorType::BT_FollowTargetKnownPosition: return "Follow target known position";
		case ECompiledBehaviorType::BT_FollowTargetActualPosition: return "Follow target actual position";
		case ECompiledBehaviorType::BT_PickUpItem: return "Pick ip item";
		case ECompiledBehaviorType::BT_GrabItem: return "Grab item";
		case ECompiledBehaviorType::BT_PutDownItem: return "Put down item";
		case ECompiledBehaviorType::BT_Search: return "Search";
		case ECompiledBehaviorType::BT_LimitedSearch: return "Limited search";
		case ECompiledBehaviorType::BT_MoveTo: return "Move to";
		case ECompiledBehaviorType::BT_Reposition: return "Reposition";
		case ECompiledBehaviorType::BT_SituationMoveTo: return "Situation move to";
		case ECompiledBehaviorType::BT_FormationMove: return "Formation move";
		case ECompiledBehaviorType::BT_SituationJumpTo: return "Situation jump to";
		case ECompiledBehaviorType::BT_AmbientWalk: return "Ambient walk";
		case ECompiledBehaviorType::BT_AmbientStand: return "Ambient stand";
		case ECompiledBehaviorType::BT_CrowdAmbientStand: return "Crowd ambient stand";
		case ECompiledBehaviorType::BT_AmbientItemUse: return "Ambient item use";
		case ECompiledBehaviorType::BT_AmbientLook: return "Ambient look";
		case ECompiledBehaviorType::BT_Act: return "Act";
		case ECompiledBehaviorType::BT_Patrol: return "Patrol";
		case ECompiledBehaviorType::BT_MoveToPosition: return "Move to position";
		case ECompiledBehaviorType::BT_AlertedStand: return "Alerted stand";
		case ECompiledBehaviorType::BT_AlertedDebug: return "Alerted debug";
		case ECompiledBehaviorType::BT_AttentionToPerson: return "Attention to person";
		case ECompiledBehaviorType::BT_StunnedByFlashGrenade: return "Stunned by flash grenade";
		case ECompiledBehaviorType::BT_CuriousIdle: return "Curious idle";
		case ECompiledBehaviorType::BT_InvestigateWeapon: return "Investigate weapon";
		case ECompiledBehaviorType::BT_DeliverWeapon: return "Deliver weapon";
		case ECompiledBehaviorType::BT_RecoverUnconscious: return "Recover unconscious";
		case ECompiledBehaviorType::BT_GetOutfit: return "Get outfit";
		case ECompiledBehaviorType::BT_RadioCall: return "Radio call";
		case ECompiledBehaviorType::BT_EscortOut: return "Escort out";
		case ECompiledBehaviorType::BT_StashItem: return "Stash item";
		case ECompiledBehaviorType::BT_CautiousSearchPosition: return "Cautious search position";
		case ECompiledBehaviorType::BT_LockdownWarning: return "Lockdown warning";
		case ECompiledBehaviorType::BT_WakeUpUnconscious: return "Wake up unconscious";
		case ECompiledBehaviorType::BT_DeadBodyInvestigate: return "Dead body investigate";
		case ECompiledBehaviorType::BT_GuardDeadBody: return "Guard dead body";
		case ECompiledBehaviorType::BT_DragDeadBody: return "Drag dead body";
		case ECompiledBehaviorType::BT_CuriousBystander: return "Curious bystander";
		case ECompiledBehaviorType::BT_DeadBodyBystander: return "Dead body bystander";
		case ECompiledBehaviorType::BT_StandOffArrest: return "Stand off arrest";
		case ECompiledBehaviorType::BT_StandOffReposition: return "Stand off reposition";
		case ECompiledBehaviorType::BT_StandAndAim: return "Stand and aim";
		case ECompiledBehaviorType::BT_CloseCombat: return "Close combat";
		case ECompiledBehaviorType::BT_MoveToCloseCombat: return "Move to close combat";
		case ECompiledBehaviorType::BT_MoveAwayFromCloseCombat: return "Move away from close combat";
		case ECompiledBehaviorType::BT_CoverFightSeasonTwo: return "Cover fight season two";
		case ECompiledBehaviorType::BT_ShootFromPosition: return "Shoot from position";
		case ECompiledBehaviorType::BT_StandAndShoot: return "Stand and shoot";
		case ECompiledBehaviorType::BT_CheckLastPosition: return "Check last position";
		case ECompiledBehaviorType::BT_ProtoSearchIdle: return "Proto search idle";
		case ECompiledBehaviorType::BT_ProtoApproachSearchArea: return "Proto approach search area";
		case ECompiledBehaviorType::BT_ProtoSearchPosition: return "Proto search position";
		case ECompiledBehaviorType::BT_ShootTarget: return "Shoot target";
		case ECompiledBehaviorType::BT_TriggerAlarm: return "Trigger alarm";
		case ECompiledBehaviorType::BT_MoveInCover: return "Move in cover";
		case ECompiledBehaviorType::BT_MoveToCover: return "Move to cover";
		case ECompiledBehaviorType::BT_HomeAttackOrigin: return "Home attack origin";
		case ECompiledBehaviorType::BT_Shoot: return "Shoot";
		case ECompiledBehaviorType::BT_Aim: return "Aim";
		case ECompiledBehaviorType::BT_MoveToRandomNeighbourNode: return "Move to random neighbour node";
		case ECompiledBehaviorType::BT_MoveToRandomNeighbourNodeAiming: return "Move to random neighbour node aiming";
		case ECompiledBehaviorType::BT_MoveToAndPlayCombatPositionAct: return "Move to and play combat position act";
		case ECompiledBehaviorType::BT_MoveToAimingAndPlayCombatPositionAct: return
			"Move to aiming and play combat position act";
		case ECompiledBehaviorType::BT_PlayJumpyReaction: return "Play jumpy reaction";
		case ECompiledBehaviorType::BT_JumpyInvestigation: return "Jumpy investigation";
		case ECompiledBehaviorType::BT_AgitatedPatrol: return "Agitated patrol";
		case ECompiledBehaviorType::BT_AgitatedGuard: return "Agitated guard";
		case ECompiledBehaviorType::BT_HeroEscort: return "Hero escort";
		case ECompiledBehaviorType::BT_Escort: return "Escort";
		case ECompiledBehaviorType::BT_ControlledFormationMove: return "Controlled formation move";
		case ECompiledBehaviorType::BT_EscortSearch: return "Escort search";
		case ECompiledBehaviorType::BT_LeadEscort: return "Lead escort";
		case ECompiledBehaviorType::BT_LeadEscort2: return "Lead escort 2";
		case ECompiledBehaviorType::BT_AimReaction: return "Aim reaction";
		case ECompiledBehaviorType::BT_FollowHitman: return "Follow hitman";
		case ECompiledBehaviorType::BT_RideTheLightning: return "Ride the lightning";
		case ECompiledBehaviorType::BT_Scared: return "Scared";
		case ECompiledBehaviorType::BT_Flee: return "Flee";
		case ECompiledBehaviorType::BT_AgitatedBystander: return "Agitated bystander";
		case ECompiledBehaviorType::BT_SentryFrisk: return "Sentry frisk";
		case ECompiledBehaviorType::BT_SentryIdle: return "Sentry idle";
		case ECompiledBehaviorType::BT_SentryWarning: return "Sentry warning";
		case ECompiledBehaviorType::BT_SentryCheckItem: return "Sentry check item";
		case ECompiledBehaviorType::BT_VIPScared: return "VIP scared";
		case ECompiledBehaviorType::BT_VIPSafeRoomTrespasser: return "VIP safe room trespasser";
		case ECompiledBehaviorType::BT_DefendVIP: return "Defend VIP";
		case ECompiledBehaviorType::BT_CautiousVIP: return "Cautious VIP";
		case ECompiledBehaviorType::BT_CautiousGuardVIP: return "Cautious guard VIP";
		case ECompiledBehaviorType::BT_InfectedConfused: return "Infected confused";
		case ECompiledBehaviorType::BT_EnterInfected: return "Enter infected";
		case ECompiledBehaviorType::BT_CureInfected: return "Cure infected";
		case ECompiledBehaviorType::BT_SickActInfected: return "Sick act infected";
		case ECompiledBehaviorType::BT_Smart: return "Smart";
		case ECompiledBehaviorType::BT_Controlled: return "Controlled";
		case ECompiledBehaviorType::BT_SpeakTest: return "Speak test";
		case ECompiledBehaviorType::BT_Conversation: return "Conversation";
		case ECompiledBehaviorType::BT_RunToHelp: return "Run to help";
		case ECompiledBehaviorType::BT_WaitForDialog: return "Wait for dialog";
		case ECompiledBehaviorType::BT_WaitForConfiguredAct: return "Wait for configured act";
		case ECompiledBehaviorType::BT_TestFlashbangGrenadeThrow: return "Test flashbang grenade throw";
		case ECompiledBehaviorType::BT_BEHAVIORS_END: return "Behaviors end";
		case ECompiledBehaviorType::BT_RenewEvent: return "Renew event";
		case ECompiledBehaviorType::BT_ExpireEvent: return "Expire event";
		case ECompiledBehaviorType::BT_ExpireEvents: return "Expire events";
		case ECompiledBehaviorType::BT_SetEventHandled: return "Set event handled";
		case ECompiledBehaviorType::BT_RenewSharedEvent: return "Renew shared event";
		case ECompiledBehaviorType::BT_ExpireSharedEvent: return "Expire shared event";
		case ECompiledBehaviorType::BT_ExpireAllEvents: return "Expire all events";
		case ECompiledBehaviorType::BT_CreateOrJoinSituation: return "Create or join situation";
		case ECompiledBehaviorType::BT_JoinSituation: return "Join situation";
		case ECompiledBehaviorType::BT_ForceActorToJoinSituation: return "Force actor to join situation";
		case ECompiledBehaviorType::BT_JoinSituationWithActor: return "Join situation with actor";
		case ECompiledBehaviorType::BT_LeaveSituation: return "Leave situation";
		case ECompiledBehaviorType::BT_Escalate: return "Escalate";
		case ECompiledBehaviorType::BT_GotoPhase: return "Goto phase";
		case ECompiledBehaviorType::BT_RenewGoal: return "Renew goal";
		case ECompiledBehaviorType::BT_ExpireGoal: return "Expire goal";
		case ECompiledBehaviorType::BT_RenewGoalOf: return "Renew goal of";
		case ECompiledBehaviorType::BT_ExpireGoalOf: return "Expire goal of";
		case ECompiledBehaviorType::BT_SetTension: return "Set tension";
		case ECompiledBehaviorType::BT_TriggerSpotted: return "Trigger spotted";
		case ECompiledBehaviorType::BT_CopyKnownLocation: return "Copy known location";
		case ECompiledBehaviorType::BT_UpdateKnownLocation: return "Update known location";
		case ECompiledBehaviorType::BT_TransferKnownObjectPositions: return "Transfer known object positions";
		case ECompiledBehaviorType::BT_WitnessAttack: return "Witness attack";
		case ECompiledBehaviorType::BT_Speak: return "Speak";
		case ECompiledBehaviorType::BT_StartDynamicEnforcer: return "Start dynamic enforcer";
		case ECompiledBehaviorType::BT_StopDynamicEnforcer: return "Stop dynamic enforcer";
		case ECompiledBehaviorType::BT_StartRangeBasedDynamicEnforcer: return "Start range-based dynamic enforcer";
		case ECompiledBehaviorType::BT_StopRangeBasedDynamicEnforcerForLocation: return
			"Stop range-based dynamic rnforcer for location";
		case ECompiledBehaviorType::BT_StopRangeBasedDynamicEnforcer: return "Stop range-based dynamic enforcer";
		case ECompiledBehaviorType::BT_SetDistracted: return "Set distracted";
		case ECompiledBehaviorType::BT_IgnoreAllDistractionsExceptTheNewest: return
			"Ignore all distractions except the newest";
		case ECompiledBehaviorType::BT_IgnoreDistractions: return "Ignore distractions";
		case ECompiledBehaviorType::BT_PerceptibleEntityNotifyWillReact: return "Perceptible entity notify will react";
		case ECompiledBehaviorType::BT_PerceptibleEntityNotifyReacted: return "Perceptible entity notify reacted";
		case ECompiledBehaviorType::BT_PerceptibleEntityNotifyInvestigating: return
			"Perceptible entity notify investigating";
		case ECompiledBehaviorType::BT_PerceptibleEntityNotifyInvestigated: return
			"Perceptible entity notify investigated";
		case ECompiledBehaviorType::BT_PerceptibleEntityNotifyTerminate: return "Perceptible entity notify terminate";
		case ECompiledBehaviorType::BT_LeaveDistractionAssistantRole: return "Leave distraction assistant role";
		case ECompiledBehaviorType::BT_LeaveDistractionAssitingGuardRole: return "Leave distraction assiting guard role";
		case ECompiledBehaviorType::BT_RequestSuitcaseAssistanceOverRadio: return
			"Request suitcase assistance over radio";
		case ECompiledBehaviorType::BT_RequestSuitcaseAssistanceFaceToFace: return
			"Request suitcase assistance face to face";
		case ECompiledBehaviorType::BT_ExpireArrestReasons: return "Expire arrest reasons";
		case ECompiledBehaviorType::BT_SetDialogSwitch_NPCID: return "Set dialog switch NPC ID";
		case ECompiledBehaviorType::BT_InfectedAssignToFollowPlayer: return "Infected assign to follow player";
		case ECompiledBehaviorType::BT_InfectedRemoveFromFollowPlayer: return "Infected remove from follow player";
		case ECompiledBehaviorType::BT_Log: return "Log";
		case ECompiledBehaviorType::BT_COMMANDS_END: return "Commands end";
		case ECompiledBehaviorType::BT_Invalid: return "Invalid";
		default: return "<Unknown>";
	}
}

DEFINE_PLUGIN_DETOUR(DebugMod, bool, OnLoadScene, ZEntitySceneContext* th, SSceneInitParameters&) {
	return HookResult<bool>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(DebugMod, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene) {
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
	m_Areas.clear();
	m_Vertices.clear();
	m_Indices.clear();
	m_NavMeshLines.clear();
	m_NavMeshConnectivityLines.clear();
	m_ObstacleToEntityID.clear();

	return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(
	DebugMod, void, ZPFObstacleEntity_UpdateObstacle, ZPFObstacleEntity* th, uint32 nObstacleBlockageFlags,
	bool bEnabled, bool forceUpdate
) {
	p_Hook->CallOriginal(th, nObstacleBlockageFlags, bEnabled, forceUpdate);

	m_ObstacleToEntityID[th->m_obstacle.m_internal.GetTarget()] = th->GetType()->m_nEntityID;

	return HookResult<void>(HookAction::Return());
}

DEFINE_ZHM_PLUGIN(DebugMod);
