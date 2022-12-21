#include "DebugMod.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZScene.h>
#include <Glacier/ZActor.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/EntityFactory.h>
#include <Glacier/ZCollision.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/ZHttp.h>
#include <Glacier/ZPhysics.h>
#include <Glacier/ZSetpieceEntity.h>
#include <Glacier/ZContentKitManager.h>
#include <Glacier/ZAction.h>
#include <Glacier/ZItem.h>
#include <Glacier/ZInventory.h>
#include <ZBinaryReader.h>
#include <Crypto.h>

#include <Functions.h>
#include <Globals.h>

#include <ImGuizmo.h>
#include <lz4.h>

#include <winhttp.h>
#include <numbers>
#include <filesystem>
#include <imgui_internal.h>

DebugMod::~DebugMod()
{
	const ZMemberDelegate<DebugMod, void(const SGameUpdateEvent&)> s_Delegate(this, &DebugMod::OnFrameUpdate);
	Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void DebugMod::Init()
{
	Hooks::ZHttpResultDynamicObject_OnBufferReady->AddDetour(this, &DebugMod::ZHttpBufferReady);
	Hooks::Http_WinHttpCallback->AddDetour(this, &DebugMod::WinHttpCallback);
	Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &DebugMod::OnClearScene);
}

void DebugMod::OnEngineInitialized()
{
	const ZMemberDelegate<DebugMod, void(const SGameUpdateEvent&)> s_Delegate(this, &DebugMod::OnFrameUpdate);
	Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void DebugMod::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
	/*if (Functions::ZInputAction_Analog->Call(&m_DeleteAction, -1) > 0.f)
	{
		if (const auto s_CurrentCamera = Functions::GetCurrentCamera->Call())
		{
			const auto s_CameraTrans = s_CurrentCamera->GetWorldMatrix();
			const auto s_From = s_CameraTrans.Trans;
			const auto s_To = s_From - (s_CameraTrans.ZAxis * 200.f);
			DoRaycast(s_From, s_To);
		}
	}*/
}

void DebugMod::OnDrawMenu()
{
	if (ImGui::Button("DEBUG MENU"))
	{
		m_DebugMenuActive = !m_DebugMenuActive;
	}

	if (ImGui::Button("POSITIONS MENU"))
	{
		m_PositionsMenuActive = !m_PositionsMenuActive;
	}

	if (ImGui::Button("ENTITY MENU"))
	{
		m_EntityMenuActive = !m_EntityMenuActive;
	}

	if (ImGui::Button("PLAYER MENU"))
	{
		m_PlayerMenuActive = !m_PlayerMenuActive;
	}

	if (ImGui::Button("ITEMS MENU"))
	{
		m_ItemsMenuActive = !m_ItemsMenuActive;
	}

	if (ImGui::Button("ASSETS MENU"))
	{
		m_AssetsMenuActive = !m_AssetsMenuActive;
	}

	if (ImGui::Button("NPCs MENU"))
	{
		m_NPCsMenuActive = !m_NPCsMenuActive;
	}

	if (ImGui::Button("SPAWN CANON"))
	{
		auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

		if (!s_Scene)
		{
			Logger::Debug("Scene not loaded.");
		}
		else
		{
			//const auto s_ID = ResId<"[assembly:/_pro/environment/templates/props/containers/military_containers_a.template?/military_box_metal_e_00.entitytemplate].pc_entitytype">;
			//const auto s_ID = ResId<"[assembly:/deeznuts.entitytemplate].pc_entitytype">;
			//const auto s_ID = ResId<"[assembly:/templates/gameplay/ai2/actors.template?/npcactor.entitytemplate].pc_entitytype">;
			//const auto s_ID = ResId<"[assembly:/_pro/characters/templates/hero/agent47/agent47.template?/agent47_default.entitytemplate].pc_entitytype">;
			const auto s_ID = ResId<"[assembly:/_pro/design/setpieces/unique/setpiece_italy_unique.template?/setpiece_italy_unique_cannon_fortress_a.entitytemplate].pc_entitytype">;

			Logger::Debug("Getting resource wew: {}", s_ID);

			TResourcePtr<ZTemplateEntityFactory> s_Resource;
			Globals::ResourceManager->GetResourcePtr(s_Resource, s_ID, 0);

			Logger::Debug("Resource: {} {}", s_Resource.m_nResourceIndex, fmt::ptr(s_Resource.GetResource()));

			/*const auto s_BrickID = ResId<"[assembly:/_pro/scenes/missions/paris/_scene_lumumba.brick].pc_entitytype">;
			Logger::Debug("Getting brick resource wew: {}", s_BrickID);

			TResourcePtr<ZTemplateEntityFactory> s_BrickResource;
			Globals::ResourceManager->GetResourcePtr(s_BrickResource, s_BrickID, 0);

			Logger::Debug("Brick resource: {} {}", s_BrickResource.m_nResourceIndex, fmt::ptr(s_BrickResource.GetResource()));*/

			if (!s_Resource)
			{
				Logger::Debug("Resource is not loaded.");
			}
			else
			{
				// Spawn some shit now.
				ZEntityRef s_NewEntity;
				Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_NewEntity, "", s_Resource, s_Scene.m_ref, nullptr, -1);

				if (!s_NewEntity)
				{
					Logger::Debug("Failed to spawn entity.");
				}
				else
				{
					s_NewEntity.SetProperty("m_eRoomBehaviour", ZSpatialEntity::ERoomBehaviour::ROOM_DYNAMIC);

					m_EntityMutex.lock();
					m_SelectedEntity = s_NewEntity;
					m_EntityMutex.unlock();

					/*
					TEntityRef<ZHitman5> s_LocalHitman;
					Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);
					auto s_Actor = s_NewEntity.QueryInterface<ZActor>();

					Logger::Debug("Spawned entity {}!", fmt::ptr(s_Actor));

					// Set outfit and other properties.
					s_Actor->m_sActorName = "Ur Mum";
					s_Actor->m_nOutfitVariation = 0;
					s_Actor->m_bStartEnabled = true;
					s_Actor->m_OutfitRepositoryID = ZRepositoryID("8f928c7a-99dd-43db-a027-1310cf49f3ad");
					s_Actor->m_eRequiredVoiceVariation = EActorVoiceVariation::eAVV_Undefined;

					auto s_ActorSpatial = s_NewEntity.QueryInterface<ZSpatialEntity>();
					const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
					
					s_ActorSpatial->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());

					s_Actor->Activate(0);#1#

					/*auto s_NewHitman = s_NewEntity.QueryInterface<ZHitman5>();

					Logger::Debug("Spawned entity {}!", fmt::ptr(s_NewHitman));

					// Set outfit and other properties.
					s_NewHitman->m_InitialOutfitId = "bb1115ba-d250-4f8f-b486-f5aba8499ebb";
					s_NewHitman->m_CharacterId = "21174318-919a-4683-bd3b-09068f7b6fac";
					s_NewHitman->m_bIsInvincible = false;

					auto s_ActorSpatial = s_NewEntity.QueryInterface<ZSpatialEntity>();
					const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
					
					//s_ActorSpatial->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());

					s_NewHitman->Activate(0);#1#

					auto s_NewSpatialEntity = s_NewEntity.QueryInterface<ZSpatialEntity>();

					Logger::Debug("Spawned entity {}!", fmt::ptr(s_NewSpatialEntity));

					const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

					s_NewSpatialEntity->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());

					m_EntityMutex.lock();

					m_SelectedEntity = s_NewEntity;

					m_EntityMutex.unlock();*/
				}
			}
		}
	}
}

void DebugMod::OnDrawUI(bool p_HasFocus)
{
	ImGuizmo::BeginFrame();

	DrawOptions(p_HasFocus);
	DrawPositionBox(p_HasFocus);
	DrawEntityBox(p_HasFocus);
	DrawPlayerBox(p_HasFocus);
	DrawItemsBox(p_HasFocus);
	DrawAssetsBox(p_HasFocus);
	DrawNPCsBox(p_HasFocus);

	auto s_ImgGuiIO = ImGui::GetIO();

	if (p_HasFocus)
	{
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !s_ImgGuiIO.WantCaptureMouse)
		{
			const auto s_MousePos = ImGui::GetMousePos();
			
			OnMouseDown(SVector2(s_MousePos.x, s_MousePos.y), !m_HoldingMouse);

			m_HoldingMouse = true;
		}
		else
		{
			/*// If we stopped clicking, update collisions.
			if (m_HoldingMouse && *Globals::CollisionManager)
			{
				m_EntityMutex.lock_shared();

				if (m_SelectedEntity)
				{
					if (const auto s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>())
					{
						if (const auto s_PhysicsAspect = m_SelectedEntity.QueryInterface<ZStaticPhysicsAspect>())
						{
							Logger::Debug("Found physics aspect. Updating its transform.");
							s_PhysicsAspect->m_pPhysicsObject->SetTransform(s_SpatialEntity->GetWorldMatrix());
						}
					}
				}

				m_EntityMutex.unlock_shared();
			}*/

			m_HoldingMouse = false;
		}

		if (ImGui::IsKeyPressed(s_ImgGuiIO.KeyMap[ImGuiKey_Tab]))
		{
			if (m_GizmoMode == ImGuizmo::TRANSLATE)
				m_GizmoMode = ImGuizmo::ROTATE;
			else if (m_GizmoMode == ImGuizmo::ROTATE)
				m_GizmoMode = ImGuizmo::SCALE;
			else if (m_GizmoMode == ImGuizmo::SCALE)
				m_GizmoMode = ImGuizmo::TRANSLATE;
		}

		if (ImGui::IsKeyPressed(s_ImgGuiIO.KeyMap[ImGuiKey_Space]))
		{
			m_GizmoSpace = m_GizmoSpace == ImGuizmo::WORLD ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
		}
	}

	ImGuizmo::Enable(p_HasFocus);

	m_EntityMutex.lock_shared();

	if (m_SelectedEntity)
	{
		if (const auto s_CurrentCamera = Functions::GetCurrentCamera->Call())
		{
			if (const auto s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>())
			{
				auto s_ModelMatrix = s_SpatialEntity->GetWorldMatrix();
				auto s_ViewMatrix = s_CurrentCamera->GetViewMatrix();
				const SMatrix s_ProjectionMatrix = *s_CurrentCamera->GetProjectionMatrix();

				ImGuizmo::SetRect(0, 0, s_ImgGuiIO.DisplaySize.x, s_ImgGuiIO.DisplaySize.y);

				if (ImGuizmo::Manipulate(&s_ViewMatrix.XAxis.x, &s_ProjectionMatrix.XAxis.x, m_GizmoMode, m_GizmoSpace, &s_ModelMatrix.XAxis.x, NULL, m_useSnap ? &m_SnapValue[0] : NULL))
				{
					s_SpatialEntity->SetWorldMatrix(s_ModelMatrix);

					if (const auto s_PhysicsAspect = m_SelectedEntity.QueryInterface<ZStaticPhysicsAspect>())
						s_PhysicsAspect->m_pPhysicsObject->SetTransform(s_SpatialEntity->GetWorldMatrix());
				}
			}
		}
	}

	m_EntityMutex.unlock_shared();
}

void DebugMod::OnMouseDown(SVector2 p_Pos, bool p_FirstClick)
{
	SVector3 s_World;
	SVector3 s_Direction;
	SDK()->ScreenToWorld(p_Pos, s_World, s_Direction);

	float4 s_DirectionVec(s_Direction.x, s_Direction.y, s_Direction.z, 1.f);

	float4 s_From = float4(s_World.x, s_World.y, s_World.z, 1.f);
	float4 s_To = s_From + (s_DirectionVec * 200.f);

	if (!*Globals::CollisionManager)
	{
		Logger::Error("Collision manager not found.");
		return;
	}

	ZRayQueryInput s_RayInput {
		.m_vFrom = s_From,
		.m_vTo = s_To,
	};

	ZRayQueryOutput s_RayOutput {};

	Logger::Debug("RayCasting from {} to {}.", s_From, s_To);

	if (!(*Globals::CollisionManager)->RayCastClosestHit(s_RayInput, &s_RayOutput))
	{
		Logger::Error("Raycast failed.");
		return;
	}

	Logger::Debug("Raycast result: {} {}", fmt::ptr(&s_RayOutput), s_RayOutput.m_vPosition);

	m_From = s_From;
	m_To = s_To;
	m_Hit = s_RayOutput.m_vPosition;
	m_Normal = s_RayOutput.m_vNormal;

	if (p_FirstClick)
	{
		m_EntityMutex.lock();

		if (s_RayOutput.m_BlockingEntity)
		{
			const auto& s_Interfaces = *(*s_RayOutput.m_BlockingEntity.m_pEntity)->m_pInterfaces;
			Logger::Trace("Hit entity of type '{}' with id '{:x}'.", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName, (*s_RayOutput.m_BlockingEntity.m_pEntity)->m_nEntityId);
		}

		m_SelectedEntity = s_RayOutput.m_BlockingEntity;

		m_EntityMutex.unlock();
	}
	else
	{
		/*m_EntityMutex.lock_shared();

		if (m_SelectedEntity)
		{
			if (const auto s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>())
			{
				auto s_EntityWorldMatrix = s_SpatialEntity->GetWorldMatrix();				
				s_EntityWorldMatrix.Trans = m_Hit;
				s_SpatialEntity->SetWorldMatrix(s_EntityWorldMatrix);
			}
		}

		m_EntityMutex.unlock_shared();*/
	}
}

void DebugMod::DrawOptions(bool p_HasFocus)
{
	if (!p_HasFocus || !m_DebugMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	auto s_Showing = ImGui::Begin("DEBUG MENU", &m_DebugMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing)
	{
		ImGui::Checkbox("Render NPC position boxes", &m_RenderNpcBoxes);
		ImGui::Checkbox("Render NPC names", &m_RenderNpcNames);
		ImGui::Checkbox("Render NPC repository IDs", &m_RenderNpcRepoIds);
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}

void DebugMod::DrawPositionBox(bool p_HasFocus)
{
	if (!p_HasFocus || !m_PositionsMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("POSITIONS", &m_PositionsMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing)
	{
		SMatrix s_HitmanTrans;
		SMatrix s_CameraTrans;

		// Enable Hitman input.
		TEntityRef<ZHitman5> s_LocalHitman;
		Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

		auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

		const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

		if (s_HitmanSpatial)
			s_HitmanTrans = s_HitmanSpatial->GetWorldMatrix();

		if (s_CurrentCamera)
			s_CameraTrans = s_CurrentCamera->GetWorldMatrix();

		ImGui::TextUnformatted("Hitman Transform:");

		if (ImGui::BeginTable("DebugMod_HitmanPosition", 4))
		{
			for (int i = 0; i < 4; ++i)
			{
				ImGui::TableNextRow();

				for (int j = 0; j < 4; ++j)
				{
					ImGui::TableSetColumnIndex(j);
					ImGui::Text("%f", s_HitmanTrans.flt[(i * 4) + j]);
				}
			}

			ImGui::EndTable();
		}

		ImGui::TextUnformatted("Camera Transform:");

		if (ImGui::BeginTable("DebugMod_Camera_Position", 4))
		{
			for (int i = 0; i < 4; ++i)
			{
				ImGui::TableNextRow();

				for (int j = 0; j < 4; ++j)
				{
					ImGui::TableSetColumnIndex(j);
					ImGui::Text("%f", s_CameraTrans.flt[(i * 4) + j]);
				}
			}

			ImGui::EndTable();
		}

		if (ImGui::Button("Copy Hitman Transform"))
		{
			CopyToClipboard(fmt::format("{}", s_HitmanTrans));
		}

		ImGui::SameLine();

		if (ImGui::Button("RT JSON##HitmanRT"))
		{
			CopyToClipboard(fmt::format(
				"{{\"XAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"YAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"ZAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"Trans\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
				s_HitmanTrans.XAxis.x, s_HitmanTrans.XAxis.y, s_HitmanTrans.XAxis.z,
				s_HitmanTrans.YAxis.x, s_HitmanTrans.YAxis.y, s_HitmanTrans.YAxis.z,
				s_HitmanTrans.ZAxis.x, s_HitmanTrans.ZAxis.y, s_HitmanTrans.ZAxis.z,
				s_HitmanTrans.Trans.x, s_HitmanTrans.Trans.y, s_HitmanTrans.Trans.z
			));
		}

		ImGui::SameLine();

		if (ImGui::Button("QN JSON##HitmanQN"))
		{
			// This is adapted from QN: https://github.com/atampy25/quickentity-rs/blob/b94dbab32c91ccd0e1612a2e5dda8fb83eb2a8e9/src/lib.rs#L243
			constexpr double c_RAD2DEG = 180.0 / std::numbers::pi;

			double s_RotationX = abs(s_HitmanTrans.XAxis.z) < 0.9999999f
				? atan2f(-s_HitmanTrans.YAxis.z, s_HitmanTrans.ZAxis.z) * c_RAD2DEG
				: atan2f(s_HitmanTrans.ZAxis.y, s_HitmanTrans.YAxis.y) * c_RAD2DEG;

			double s_RotationY = asinf(min(max(-1.f, s_HitmanTrans.XAxis.z), 1.f)) * c_RAD2DEG;

			double s_RotationZ = abs(s_HitmanTrans.XAxis.z) < 0.9999999f
				? atan2f(-s_HitmanTrans.XAxis.y, s_HitmanTrans.XAxis.x) * c_RAD2DEG
				: 0.f;

			CopyToClipboard(fmt::format(
				"{{\"rotation\":{{\"x\":{},\"y\":{},\"z\":{}}},\"position\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
				s_RotationX, s_RotationY, s_RotationZ,
				s_HitmanTrans.Trans.x, s_HitmanTrans.Trans.y, s_HitmanTrans.Trans.z
			));
		}

		if (ImGui::Button("Copy Camera Transform"))
		{
			CopyToClipboard(fmt::format("{}", s_CameraTrans));
		}

		ImGui::SameLine();

		if (ImGui::Button("RT JSON##CameraRT"))
		{
			CopyToClipboard(fmt::format(
				"{{\"XAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"YAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"ZAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"Trans\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
				s_CameraTrans.XAxis.x, s_CameraTrans.XAxis.y, s_CameraTrans.XAxis.z,
				s_CameraTrans.YAxis.x, s_CameraTrans.YAxis.y, s_CameraTrans.YAxis.z,
				s_CameraTrans.ZAxis.x, s_CameraTrans.ZAxis.y, s_CameraTrans.ZAxis.z,
				s_CameraTrans.Trans.x, s_CameraTrans.Trans.y, s_CameraTrans.Trans.z
			));
		}

		ImGui::SameLine();

		if (ImGui::Button("QN JSON##CameraQN"))
		{
			// This is adapted from QN: https://github.com/atampy25/quickentity-rs/blob/b94dbab32c91ccd0e1612a2e5dda8fb83eb2a8e9/src/lib.rs#L243
			constexpr double c_RAD2DEG = 180.0 / std::numbers::pi;

			double s_RotationX = abs(s_CameraTrans.XAxis.z) < 0.9999999f
				? atan2f(-s_CameraTrans.YAxis.z, s_CameraTrans.ZAxis.z) * c_RAD2DEG
				: atan2f(s_CameraTrans.ZAxis.y, s_CameraTrans.YAxis.y) * c_RAD2DEG;

			double s_RotationY = asinf(min(max(-1.f, s_CameraTrans.XAxis.z), 1.f)) * c_RAD2DEG;

			double s_RotationZ = abs(s_CameraTrans.XAxis.z) < 0.9999999f
				? atan2f(-s_CameraTrans.XAxis.y, s_CameraTrans.XAxis.x) * c_RAD2DEG
				: 0.f;

			CopyToClipboard(fmt::format(
				"{{\"rotation\":{{\"x\":{},\"y\":{},\"z\":{}}},\"position\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
				s_RotationX, s_RotationY, s_RotationZ,
				s_CameraTrans.Trans.x, s_CameraTrans.Trans.y, s_CameraTrans.Trans.z
			));
		}
		
		ImGui::Checkbox("Use Snap", &m_useSnap);
		ImGui::SameLine();
		ImGui::InputFloat3("", m_SnapValue);
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}

void DebugMod::DrawEntityBox(bool p_HasFocus)
{
	if (!p_HasFocus || !m_EntityMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("SELECTED ENTITY", &m_EntityMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing)
	{
		m_EntityMutex.lock_shared();

		if (!m_SelectedEntity)
		{
			ImGui::Text("No entity selected.");
		}
		else
		{
			const auto& s_Interfaces = *(*m_SelectedEntity.m_pEntity)->m_pInterfaces;

			ImGui::TextUnformatted(fmt::format("Entity Type: {}", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName).c_str());
			ImGui::TextUnformatted(fmt::format("Entity ID: {:016x}", (*m_SelectedEntity.m_pEntity)->m_nEntityId).c_str());

			std::string s_InterfacesStr;

			for (const auto& s_Interface : s_Interfaces)
			{
				if (s_Interface.m_pTypeId->typeInfo() == nullptr)
					continue;

				if (!s_InterfacesStr.empty())
					s_InterfacesStr += ", ";

				s_InterfacesStr += s_Interface.m_pTypeId->typeInfo()->m_pTypeName;
			}

			ImGui::TextUnformatted(fmt::format("Entity Interfaces: {}", s_InterfacesStr).c_str());

			std::string s_Properties01;

			for (const auto& s_Property : *(*m_SelectedEntity.m_pEntity)->m_pProperties01)
			{
				if (!s_Property.m_pType->getPropertyInfo()->m_pName)
					continue;

				if (!s_Properties01.empty())
					s_Properties01 += ", ";

				s_Properties01 += s_Property.m_pType->getPropertyInfo()->m_pName;
			}

			ImGui::TextUnformatted(fmt::format("Entity Properties1: {}", s_Properties01).c_str());

			std::string s_Properties02;

			for (const auto& s_Property : *(*m_SelectedEntity.m_pEntity)->m_pProperties02)
			{
				if (!s_Property.m_pType->getPropertyInfo()->m_pName)
					continue;

				if (!s_Properties02.empty())
					s_Properties02 += ", ";

				s_Properties02 += s_Property.m_pType->getPropertyInfo()->m_pName;
			}

			ImGui::TextUnformatted(fmt::format("Entity Properties2: {}", s_Properties02).c_str());
		

			if (const auto s_Spatial = m_SelectedEntity.QueryInterface<ZSpatialEntity>())
			{
				const auto s_Trans = s_Spatial->GetWorldMatrix();

				ImGui::TextUnformatted("Entity Transform:");

				if (ImGui::BeginTable("DebugMod_HitmanPosition", 4))
				{
					for (int i = 0; i < 4; ++i)
					{
						ImGui::TableNextRow();

						for (int j = 0; j < 4; ++j)
						{
							ImGui::TableSetColumnIndex(j);
							ImGui::Text("%f", s_Trans.flt[(i * 4) + j]);
						}
					}

					ImGui::EndTable();
				}

				if (ImGui::Button("Copy Transform"))
				{
					CopyToClipboard(fmt::format("{}", s_Trans));
				}

				ImGui::SameLine();

				if (ImGui::Button("RT JSON##EntRT"))
				{
					CopyToClipboard(fmt::format(
						"{{\"XAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"YAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"ZAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"Trans\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
						s_Trans.XAxis.x, s_Trans.XAxis.y, s_Trans.XAxis.z,
						s_Trans.YAxis.x, s_Trans.YAxis.y, s_Trans.YAxis.z,
						s_Trans.ZAxis.x, s_Trans.ZAxis.y, s_Trans.ZAxis.z,
						s_Trans.Trans.x, s_Trans.Trans.y, s_Trans.Trans.z
					));
				}

				ImGui::SameLine();

				if (ImGui::Button("QN JSON##EntQN"))
				{
					// This is adapted from QN: https://github.com/atampy25/quickentity-rs/blob/b94dbab32c91ccd0e1612a2e5dda8fb83eb2a8e9/src/lib.rs#L243
					constexpr double c_RAD2DEG = 180.0 / std::numbers::pi;

					double s_RotationX = abs(s_Trans.XAxis.z) < 0.9999999f
						? atan2f(-s_Trans.YAxis.z, s_Trans.ZAxis.z) * c_RAD2DEG
						: atan2f(s_Trans.ZAxis.y, s_Trans.YAxis.y) * c_RAD2DEG;

					double s_RotationY = asinf(min(max(-1.f, s_Trans.XAxis.z), 1.f)) * c_RAD2DEG;

					double s_RotationZ = abs(s_Trans.XAxis.z) < 0.9999999f
						? atan2f(-s_Trans.XAxis.y, s_Trans.XAxis.x) * c_RAD2DEG
						: 0.f;

					CopyToClipboard(fmt::format(
						"{{\"rotation\":{{\"x\":{},\"y\":{},\"z\":{}}},\"position\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
						s_RotationX, s_RotationY, s_RotationZ,
						s_Trans.Trans.x, s_Trans.Trans.y, s_Trans.Trans.z
					));
				}

				if (ImGui::Button("Copy ID"))
				{
					CopyToClipboard(fmt::format("{:016x}", (*m_SelectedEntity.m_pEntity)->m_nEntityId));
				}

				if (ImGui::Button("Move to Hitman"))
				{
					TEntityRef<ZHitman5> s_LocalHitman;
					Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

					auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

					s_Spatial->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());

					if (const auto s_PhysicsAspect = m_SelectedEntity.QueryInterface<ZStaticPhysicsAspect>())
						s_PhysicsAspect->m_pPhysicsObject->SetTransform(s_Spatial->GetWorldMatrix());
				}
			}
		}

		m_EntityMutex.unlock_shared();
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}

void DebugMod::DrawPlayerBox(bool p_HasFocus)
{
	if (!p_HasFocus || !m_PlayerMenuActive)
	{
		return;
	}

	ZContentKitManager* contentKitManager = Globals::ContentKitManager;
	TEntityRef<ZHitman5> s_LocalHitman;

	Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("PLAYER", &m_PlayerMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing)
	{
		if (s_LocalHitman.m_pInterfaceRef)
		{
			static bool isInvincible = s_LocalHitman.m_ref.GetProperty<bool>("m_bIsInvincible").Get();

			if (ImGui::Checkbox("Is Invincible", &isInvincible))
			{
				s_LocalHitman.m_ref.SetProperty("m_bIsInvincible", isInvincible);
			}
		}

		static char outfitName[256] { "" };

		ImGui::Text("Outfit");
		ImGui::SameLine();

		const bool isInputTextEnterPressed = ImGui::InputText("##OutfitName", outfitName, sizeof(outfitName), ImGuiInputTextFlags_EnterReturnsTrue);
		const bool isInputTextActive = ImGui::IsItemActive();
		const bool isInputTextActivated = ImGui::IsItemActivated();

		if (isInputTextActivated)
		{
			ImGui::OpenPopup("##popup");
		}

		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
		ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

		static TEntityRef<ZGlobalOutfitKit>* globalOutfitKit = nullptr;
		static char currentCharacterSetIndex[3] { "0" };
		static const char* currentcharSetCharacterType = "HeroA";
		static const char* currentcharSetCharacterType2 = "HeroA";
		static char currentOutfitVariationIndex[3] { "0" };

		if (ImGui::BeginPopup("##popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
		{
			for (auto it = contentKitManager->m_repositoryGlobalOutfitKits.begin(); it != contentKitManager->m_repositoryGlobalOutfitKits.end(); ++it)
			{
				TEntityRef<ZGlobalOutfitKit>* globalOutfitKit2 = &it->second;
				const char* outfitName2 = globalOutfitKit2->m_pInterfaceRef->m_sCommonName.c_str();

				if (!strstr(outfitName2, outfitName))
				{
					continue;
				}

				if (ImGui::Selectable(outfitName2))
				{
					ImGui::ClearActiveID();
					strcpy_s(outfitName, outfitName2);

					EquipOutfit(it->second, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), s_LocalHitman.m_pInterfaceRef);

					globalOutfitKit = globalOutfitKit2;
				}
			}

			if (isInputTextEnterPressed || (!isInputTextActive && !ImGui::IsWindowFocused()))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::Text("Character Set Index");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharacterSetIndex", currentCharacterSetIndex))
		{
			if (globalOutfitKit)
			{
				for (size_t i = 0; i < globalOutfitKit->m_pInterfaceRef->m_aCharSets.size(); ++i)
				{
					std::string characterSetIndex = std::to_string(i);
					bool isSelected = currentCharacterSetIndex == characterSetIndex.c_str();

					if (ImGui::Selectable(characterSetIndex.c_str(), isSelected))
					{
						strcpy_s(currentCharacterSetIndex, characterSetIndex.c_str());

						if (globalOutfitKit)
						{
							EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), s_LocalHitman.m_pInterfaceRef);
						}
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("CharSet Character Type");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharSetCharacterType", currentcharSetCharacterType))
		{
			if (globalOutfitKit)
			{
				for (size_t i = 0; i < 3; ++i)
				{
					bool isSelected = currentcharSetCharacterType == charSetCharacterTypes[i];

					if (ImGui::Selectable(charSetCharacterTypes[i], isSelected))
					{
						currentcharSetCharacterType = charSetCharacterTypes[i];

						if (globalOutfitKit)
						{
							EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), s_LocalHitman.m_pInterfaceRef);
						}
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("Outfit Variation");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##OutfitVariation", currentOutfitVariationIndex))
		{
			if (globalOutfitKit)
			{
				unsigned int currentCharacterSetIndex2 = std::stoi(currentCharacterSetIndex);
				size_t variationCount = globalOutfitKit->m_pInterfaceRef->m_aCharSets[currentCharacterSetIndex2].m_pInterfaceRef->m_aCharacters[0].m_pInterfaceRef->m_aVariations.size();

				for (size_t i = 0; i < variationCount; ++i)
				{
					std::string outfitVariationIndex = std::to_string(i);
					bool isSelected = currentOutfitVariationIndex == outfitVariationIndex.c_str();

					if (ImGui::Selectable(outfitVariationIndex.c_str(), isSelected))
					{
						strcpy_s(currentOutfitVariationIndex, outfitVariationIndex.c_str());

						if (globalOutfitKit)
						{
							EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), s_LocalHitman.m_pInterfaceRef);
						}
					}
				}
			}

			ImGui::EndCombo();
		}

		static bool weaponsAllowed = false;
		static bool authorityFigure = false;

		if (globalOutfitKit)
		{
			ImGui::Checkbox("Weapons Allowed", &globalOutfitKit->m_pInterfaceRef->m_bWeaponsAllowed);
			ImGui::Checkbox("Authority Figure", &globalOutfitKit->m_pInterfaceRef->m_bAuthorityFigure);
		}

		ImGui::Separator();

		static char npcName[256] { "" };

		ImGui::Text("NPC Name");
		ImGui::SameLine();

		ImGui::InputText("##NPCName", npcName, sizeof(npcName));
		ImGui::SameLine();

		if (ImGui::Button("Get NPC Outfit"))
		{
			ZActor* actor = Globals::ActorManager->GetActorByName(npcName);

			if (actor)
			{
				EquipOutfit(actor->m_rOutfit, actor->m_nOutfitCharset, currentcharSetCharacterType2, actor->m_nOutfitVariation, s_LocalHitman.m_pInterfaceRef);
			}
		}

		if (ImGui::Button("Get Nearest NPC's Outfit"))
		{
			ZSpatialEntity* hitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

			for (int i = 0; i < *Globals::NextActorId; ++i)
			{
				ZActor* actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
				ZEntityRef s_Ref;

				actor->GetID(&s_Ref);

				ZSpatialEntity* actorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

				SVector3 temp = actorSpatialEntity->m_mTransform.Trans - hitmanSpatialEntity->m_mTransform.Trans;
				float distance = sqrt(temp.x * temp.x + temp.y * temp.y + temp.z * temp.z);

				if (distance <= 3.0f)
				{
					EquipOutfit(actor->m_rOutfit, actor->m_nOutfitCharset, currentcharSetCharacterType2, actor->m_nOutfitVariation, s_LocalHitman.m_pInterfaceRef);

					break;
				}
			}
		}

		ImGui::Text("CharSet Character Type");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharSetCharacterType2", currentcharSetCharacterType2))
		{
			if (globalOutfitKit)
			{
				for (size_t i = 0; i < 3; ++i)
				{
					bool isSelected = currentcharSetCharacterType2 == charSetCharacterTypes[i];

					if (ImGui::Selectable(charSetCharacterTypes[i], isSelected))
					{
						currentcharSetCharacterType2 = charSetCharacterTypes[i];
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Separator();

		if (ImGui::Button("Teleport All Items To Player"))
		{
			ZSpatialEntity* s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
			ZHM5ActionManager* hm5ActionManager = Globals::HM5ActionManager;

			for (unsigned int i = 0; i < hm5ActionManager->m_Actions.size(); ++i)
			{
				ZHM5Action* action = hm5ActionManager->m_Actions[i];

				if (action->m_eActionType == EActionType::AT_PICKUP)
				{
					ZHM5Item* item = action->m_Object.QueryInterface<ZHM5Item>();

					item->m_rGeomentity.m_pInterfaceRef->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());
				}
			}
		}

		if (ImGui::Button("Teleport All NPCs To Player"))
		{
			TEntityRef<ZHitman5> localHitman;

			Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &localHitman);

			ZSpatialEntity* hitmanSpatialEntity = localHitman.m_ref.QueryInterface<ZSpatialEntity>();

			for (int i = 0; i < *Globals::NextActorId; ++i)
			{
				ZActor* actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
				ZEntityRef s_Ref;

				actor->GetID(&s_Ref);

				ZSpatialEntity* actorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

				actorSpatialEntity->SetWorldMatrix(hitmanSpatialEntity->GetWorldMatrix());
			}
		}
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}

void DebugMod::DrawItemsBox(bool p_HasFocus)
{
	if (!p_HasFocus || !m_ItemsMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("ITEMS", &m_ItemsMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing && p_HasFocus)
	{
		THashMap<ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>* repositoryData = nullptr;

		if (repositoryResource.m_nResourceIndex == -1)
		{
			const auto s_ID = ResId<"[assembly:/repository/pro.repo].pc_repo">;

			Globals::ResourceManager->GetResourcePtr(repositoryResource, s_ID, 0);
		}

		if (repositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID)
		{
			repositoryData = static_cast<THashMap<ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(repositoryResource.GetResourceData());
		}
		else
		{
			ImGui::PopFont();
			ImGui::End();
			ImGui::PopFont();

			return;
		}

		ZContentKitManager* contentKitManager = Globals::ContentKitManager;
		TEntityRef<ZHitman5> s_LocalHitman;

		Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

		ZSpatialEntity* s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
		ZHM5ActionManager* hm5ActionManager = Globals::HM5ActionManager;
		std::vector<ZHM5Action*> actions;

		if (hm5ActionManager->m_Actions.size() == 0)
		{
			ImGui::PopFont();
			ImGui::End();
			ImGui::PopFont();

			return;
		}

		for (unsigned int i = 0; i < hm5ActionManager->m_Actions.size(); ++i)
		{
			ZHM5Action* action = hm5ActionManager->m_Actions[i];

			if (action && action->m_eActionType == EActionType::AT_PICKUP)
			{
				actions.push_back(action);
			}
		}

		static size_t selected = 0;
		size_t count = actions.size();

		ImGui::BeginChild("left pane", ImVec2(300, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

		for (size_t i = 0; i < count; i++)
		{
			ZHM5Action* action = actions[i];
			ZHM5Item* item = action->m_Object.QueryInterface<ZHM5Item>();
			std::string title = std::format("{} {}", item->m_pItemConfigDescriptor->m_sTitle.c_str(), i + 1);

			if (ImGui::Selectable(title.c_str(), selected == i))
			{
				selected = i;
				textureResourceData.clear();
			}
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

		ZHM5Action* action = actions[selected];
		ZHM5Item* item = action->m_Object.QueryInterface<ZHM5Item>();
		ZDynamicObject* dynamicObject = &repositoryData->find(item->m_pItemConfigDescriptor->m_RepositoryId)->second;
		TArray<SDynamicObjectKeyValuePair>* entries = dynamicObject->As<TArray<SDynamicObjectKeyValuePair>>();
		std::string image;

		for (size_t i = 0; i < entries->size(); ++i)
		{
			std::string key = entries->operator[](i).sKey.c_str();

			if (key == "Image")
			{
				image = ConvertDynamicObjectValueTString(&entries->operator[](i).value);

				break;
			}
		}

		if (textureResourceData.size() == 0)
		{
			unsigned long long ddsTextureHash = GetDDSTextureHash(image);

			LoadResource(ddsTextureHash, textureResourceData);

			SDK()->LoadTextureFromMemory(textureResourceData, &textureSrvGPUHandle, width, height);
		}

		ImGui::Image(reinterpret_cast<ImTextureID>(textureSrvGPUHandle.ptr), ImVec2(static_cast<float>(width / 2), static_cast<float>(height / 2)));

		for (unsigned int i = 0; i < entries->size(); ++i)
		{
			std::string key = std::format("{}:", entries->operator[](i).sKey.c_str());
			IType* type = entries->operator[](i).value.m_pTypeID->typeInfo();

			if (strcmp(type->m_pTypeName, "TArray<ZDynamicObject>") == 0)
			{
				key += " [";

				ImGui::Text(key.c_str());

				TArray<ZDynamicObject>* array = entries->operator[](i).value.As<TArray<ZDynamicObject>>();

				for (unsigned int j = 0; j < array->size(); ++j)
				{
					std::string value = ConvertDynamicObjectValueTString(&array->operator[](j));

					if (!value.empty())
					{
						ImGui::Text(std::format("\t{}", value).c_str());
					}
				}

				ImGui::Text("]");
			}
			else
			{
				ImGui::Text(key.c_str());

				std::string value = ConvertDynamicObjectValueTString(&entries->operator[](i).value);

				ImGui::SameLine();
				ImGui::Text(value.c_str());
			}
		}

		if (ImGui::Button("Teleport Item To Player"))
		{
			ZSpatialEntity* s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
			//ZGeomEntity* geomEntity = s_LocalHitman.m_ref.QueryInterface<ZGeomEntity>();

			//ZEntityRef entityRef;

			//geomEntity->GetID(&entityRef);

			//item->m_pGeomEntity.m_ref = entityRef;
			//item->m_pGeomEntity.m_pInterfaceRef = geomEntity;
			item->m_rGeomentity.m_pInterfaceRef->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());
		}

		ImGui::EndChild();
		ImGui::EndGroup();
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}

void DebugMod::DrawAssetsBox(bool p_HasFocus)
{
	if (!p_HasFocus || !m_AssetsMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("ASSETS", &m_AssetsMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing && p_HasFocus)
	{
		if (repositoryProps.size() == 0)
		{
			LoadRepositoryProps();
		}

		ZContentKitManager* contentKitManager = Globals::ContentKitManager;
		static char propTitle[36] { "" };
		static char propAssemblyPath[512] { "" };
		static char numberOfPropsToSpawn[5] { "1" };
		static char numberOfPropsToSpawn2[5] { "1" };
		static char numberOfPropsToSpawn3[5] { "1" };
		static int button = 1;
		static char npcName[100] {};

		ImGui::Text("Repository Props");
		ImGui::Text("");
		ImGui::Text("Prop Title");
		ImGui::SameLine();

		const bool isInputTextEnterPressed = ImGui::InputText("##PropRepositoryID", propTitle, sizeof(propTitle), ImGuiInputTextFlags_EnterReturnsTrue);
		const bool isInputTextActive = ImGui::IsItemActive();
		const bool isInputTextActivated = ImGui::IsItemActivated();

		if (isInputTextActivated)
		{
			ImGui::OpenPopup("##popup");
		}

		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
		ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

		if (ImGui::BeginPopup("##popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
		{
			for (auto it = repositoryProps.begin(); it != repositoryProps.end(); ++it)
			{
				const char* propTitle2 = it->first.c_str();

				if (!strstr(propTitle2, propTitle))
				{
					continue;
				}

				if (ImGui::Selectable(propTitle2))
				{
					ImGui::ClearActiveID();
					strcpy_s(propTitle, propTitle2);

					int numberOfPropsToSpawn2 = std::atoi(numberOfPropsToSpawn);

					for (int i = 0; i < numberOfPropsToSpawn2; ++i)
					{
						SpawnRepositoryProp(it->second, button == 1);
					}
				}
			}

			if (isInputTextEnterPressed || (!isInputTextActive && !ImGui::IsWindowFocused()))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (ImGui::RadioButton("Add To World", button == 1))
		{
			button = 1;
		}

		ImGui::SameLine();

		if (ImGui::RadioButton("Add To Inventory", button == 2))
		{
			button = 2;
		}

		ImGui::Text("Number Of Props To Spawn");
		ImGui::SameLine();

		ImGui::InputText("##NumberOfPropsToSpawn", numberOfPropsToSpawn, sizeof(numberOfPropsToSpawn));

		ImGui::Separator();
		ImGui::Text("Non Repository Props");
		ImGui::Text("");
		ImGui::Text("Prop Assembly Path");
		ImGui::SameLine();

		ImGui::InputText("##Prop Assembly Path", propAssemblyPath, sizeof(propAssemblyPath));
		ImGui::SameLine();

		if (ImGui::Button("Spawn Prop"))
		{
			int numberOfPropsToSpawn3 = std::atoi(numberOfPropsToSpawn2);

			for (int i = 0; i < numberOfPropsToSpawn3; ++i)
			{
				SpawnNonRepositoryProp(propAssemblyPath);
			}
		}

		ImGui::Text("Number Of Props To Spawn");
		ImGui::SameLine();

		ImGui::InputText("##NumberOfPropsToSpawn2", numberOfPropsToSpawn2, sizeof(numberOfPropsToSpawn2));
		ImGui::Separator();

		ImGui::Text("NPCs");
		ImGui::Text("");
		ImGui::Text("NPC Name");
		ImGui::SameLine();

		ImGui::InputText("##NPCName", npcName, sizeof(npcName));

		static char outfitName[256] { "" };

		ImGui::Text("Outfit");
		ImGui::SameLine();

		const bool isInputTextEnterPressed2 = ImGui::InputText("##OutfitName", outfitName, sizeof(outfitName), ImGuiInputTextFlags_EnterReturnsTrue);
		const bool isInputTextActive2 = ImGui::IsItemActive();
		const bool isInputTextActivated2 = ImGui::IsItemActivated();

		if (isInputTextActivated2)
		{
			ImGui::OpenPopup("##popup2");
		}

		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
		ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

		static ZRepositoryID repositoryID = ZRepositoryID("");
		static TEntityRef<ZGlobalOutfitKit>* globalOutfitKit = nullptr;
		static char currentCharacterSetIndex[3] { "0" };
		static const char* currentcharSetCharacterType = "HeroA";
		static char currentOutfitVariationIndex[3] { "0" };

		if (ImGui::BeginPopup("##popup2", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
		{
			for (auto it = contentKitManager->m_repositoryGlobalOutfitKits.begin(); it != contentKitManager->m_repositoryGlobalOutfitKits.end(); ++it)
			{
				TEntityRef<ZGlobalOutfitKit>* globalOutfitKit2 = &it->second;
				const char* outfitName2 = globalOutfitKit2->m_pInterfaceRef->m_sCommonName.c_str();

				if (!strstr(outfitName2, outfitName))
				{
					continue;
				}

				if (ImGui::Selectable(outfitName2))
				{
					ImGui::ClearActiveID();
					strcpy_s(outfitName, outfitName2);

					repositoryID = it->first;
					globalOutfitKit = globalOutfitKit2;
				}
			}

			if (isInputTextEnterPressed2 || (!isInputTextActive2 && !ImGui::IsWindowFocused()))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::Text("Character Set Index");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharacterSetIndex", currentCharacterSetIndex))
		{
			if (globalOutfitKit)
			{
				for (size_t i = 0; i < globalOutfitKit->m_pInterfaceRef->m_aCharSets.size(); ++i)
				{
					std::string characterSetIndex = std::to_string(i);
					bool isSelected = currentCharacterSetIndex == characterSetIndex.c_str();

					if (ImGui::Selectable(characterSetIndex.c_str(), isSelected))
					{
						strcpy_s(currentCharacterSetIndex, characterSetIndex.c_str());
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("CharSet Character Type");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharSetCharacterType", currentcharSetCharacterType))
		{
			if (globalOutfitKit)
			{
				for (size_t i = 0; i < 3; ++i)
				{
					bool isSelected = currentcharSetCharacterType == charSetCharacterTypes[i];

					if (ImGui::Selectable(charSetCharacterTypes[i], isSelected))
					{
						currentcharSetCharacterType = charSetCharacterTypes[i];
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("Outfit Variation");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##OutfitVariation", currentOutfitVariationIndex))
		{
			if (globalOutfitKit)
			{
				unsigned int currentCharacterSetIndex2 = std::stoi(currentCharacterSetIndex);
				size_t variationCount = globalOutfitKit->m_pInterfaceRef->m_aCharSets[currentCharacterSetIndex2].m_pInterfaceRef->m_aCharacters[0].m_pInterfaceRef->m_aVariations.size();

				for (size_t i = 0; i < variationCount; ++i)
				{
					std::string outfitVariationIndex = std::to_string(i);
					bool isSelected = currentOutfitVariationIndex == outfitVariationIndex.c_str();

					if (ImGui::Selectable(outfitVariationIndex.c_str(), isSelected))
					{
						strcpy_s(currentOutfitVariationIndex, outfitVariationIndex.c_str());
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("Number Of Props To Spawn");
		ImGui::SameLine();

		ImGui::InputText("##NumberOfPropsToSpawn3", numberOfPropsToSpawn3, sizeof(numberOfPropsToSpawn3));

		if (ImGui::Button("Spawn NPC"))
		{
			int numberOfPropsToSpawn4 = std::atoi(numberOfPropsToSpawn3);

			for (int i = 0; i < numberOfPropsToSpawn4; ++i)
			{
				SpawnNPC(npcName, repositoryID, globalOutfitKit, currentCharacterSetIndex, currentcharSetCharacterType, currentOutfitVariationIndex);
			}
		}
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}

void DebugMod::DrawNPCsBox(bool p_HasFocus)
{
	if (!p_HasFocus || !m_NPCsMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("NPCs", &m_NPCsMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing && p_HasFocus)
	{
		ZContentKitManager* contentKitManager = Globals::ContentKitManager;
		static size_t selected = 0;

		ImGui::BeginChild("left pane", ImVec2(300, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

		static char npcName[256] { "" };

		ImGui::Text("NPC Name");
		ImGui::SameLine();

		ImGui::InputText("##NPCName", npcName, sizeof(npcName));

		for (int i = 0; i < *Globals::NextActorId; ++i)
		{
			ZActor* actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
			std::string npcName2 = actor->m_sActorName.c_str();

			if (!strstr(npcName2.c_str(), npcName))
			{
				continue;
			}

			if (ImGui::Selectable(npcName2.c_str(), selected == i))
			{
				selected = i;
			}
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

		ZActor* actor = Globals::ActorManager->m_aActiveActors[selected].m_pInterfaceRef;
		static char outfitName[256] { "" };

		ImGui::Text("Outfit");
		ImGui::SameLine();

		const bool isInputTextEnterPressed = ImGui::InputText("##OutfitName", outfitName, sizeof(outfitName), ImGuiInputTextFlags_EnterReturnsTrue);
		const bool isInputTextActive = ImGui::IsItemActive();
		const bool isInputTextActivated = ImGui::IsItemActivated();

		if (isInputTextActivated)
		{
			ImGui::OpenPopup("##popup");
		}

		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
		ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

		static TEntityRef<ZGlobalOutfitKit>* globalOutfitKit = nullptr;
		static char currentCharacterSetIndex[3] { "0" };
		static const char* currentcharSetCharacterType = "Actor";
		static const char* currentcharSetCharacterType2 = "Actor";
		static char currentOutfitVariationIndex[3] { "0" };

		if (ImGui::BeginPopup("##popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
		{
			for (auto it = contentKitManager->m_repositoryGlobalOutfitKits.begin(); it != contentKitManager->m_repositoryGlobalOutfitKits.end(); ++it)
			{
				TEntityRef<ZGlobalOutfitKit>* globalOutfitKit2 = &it->second;
				const char* outfitName2 = globalOutfitKit2->m_pInterfaceRef->m_sCommonName.c_str();

				if (!strstr(outfitName2, outfitName))
				{
					continue;
				}

				if (ImGui::Selectable(outfitName2))
				{
					ImGui::ClearActiveID();
					strcpy_s(outfitName, outfitName2);

					EquipOutfit(it->second, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), actor);

					globalOutfitKit = globalOutfitKit2;
				}
			}

			if (isInputTextEnterPressed || (!isInputTextActive && !ImGui::IsWindowFocused()))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::Text("Character Set Index");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharacterSetIndex", currentCharacterSetIndex))
		{
			if (globalOutfitKit)
			{
				for (size_t i = 0; i < globalOutfitKit->m_pInterfaceRef->m_aCharSets.size(); ++i)
				{
					std::string characterSetIndex = std::to_string(i);
					bool isSelected = currentCharacterSetIndex == characterSetIndex.c_str();

					if (ImGui::Selectable(characterSetIndex.c_str(), isSelected))
					{
						strcpy_s(currentCharacterSetIndex, characterSetIndex.c_str());

						if (globalOutfitKit)
						{
							EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), actor);
						}
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("CharSet Character Type");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharSetCharacterType", currentcharSetCharacterType))
		{
			if (globalOutfitKit)
			{
				for (size_t i = 0; i < 3; ++i)
				{
					bool isSelected = currentcharSetCharacterType == charSetCharacterTypes[i];

					if (ImGui::Selectable(charSetCharacterTypes[i], isSelected))
					{
						currentcharSetCharacterType = charSetCharacterTypes[i];

						if (globalOutfitKit)
						{
							EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), actor);
						}
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("Outfit Variation");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##OutfitVariation", currentOutfitVariationIndex))
		{
			if (globalOutfitKit)
			{
				unsigned int currentCharacterSetIndex2 = std::stoi(currentCharacterSetIndex);
				size_t variationCount = globalOutfitKit->m_pInterfaceRef->m_aCharSets[currentCharacterSetIndex2].m_pInterfaceRef->m_aCharacters[0].m_pInterfaceRef->m_aVariations.size();

				for (size_t i = 0; i < variationCount; ++i)
				{
					std::string outfitVariationIndex = std::to_string(i);
					bool isSelected = currentOutfitVariationIndex == outfitVariationIndex.c_str();

					if (ImGui::Selectable(outfitVariationIndex.c_str(), isSelected))
					{
						strcpy_s(currentOutfitVariationIndex, outfitVariationIndex.c_str());

						if (globalOutfitKit)
						{
							EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), actor);
						}
					}
				}
			}

			ImGui::EndCombo();
		}

		static bool weaponsAllowed = false;
		static bool authorityFigure = false;

		if (globalOutfitKit)
		{
			ImGui::Checkbox("Weapons Allowed", &globalOutfitKit->m_pInterfaceRef->m_bWeaponsAllowed);
			ImGui::Checkbox("Authority Figure", &globalOutfitKit->m_pInterfaceRef->m_bAuthorityFigure);
		}

		ImGui::Separator();

		static char npcName2[256] { "" };

		ImGui::Text("NPC Name");
		ImGui::SameLine();

		ImGui::InputText("##NPCName", npcName2, sizeof(npcName2));
		ImGui::SameLine();

		if (ImGui::Button("Get NPC Outfit"))
		{
			ZActor* actor2 = Globals::ActorManager->GetActorByName(npcName2);

			if (actor2)
			{
				EquipOutfit(actor2->m_rOutfit, actor2->m_nOutfitCharset, currentcharSetCharacterType2, actor2->m_nOutfitVariation, actor);
			}
		}

		if (ImGui::Button("Get Nearest NPC's Outfit"))
		{
			ZEntityRef s_Ref;

			actor->GetID(&s_Ref);

			ZSpatialEntity* actorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

			for (int i = 0; i < *Globals::NextActorId; ++i)
			{
				ZActor* actor2 = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
				ZEntityRef s_Ref;

				actor2->GetID(&s_Ref);

				ZSpatialEntity* actorSpatialEntity2 = s_Ref.QueryInterface<ZSpatialEntity>();

				SVector3 temp = actorSpatialEntity->m_mTransform.Trans - actorSpatialEntity2->m_mTransform.Trans;
				float distance = sqrt(temp.x * temp.x + temp.y * temp.y + temp.z * temp.z);

				if (distance <= 3.0f)
				{
					EquipOutfit(actor2->m_rOutfit, actor2->m_nOutfitCharset, currentcharSetCharacterType2, actor2->m_nOutfitVariation, actor);

					break;
				}
			}
		}

		ImGui::Text("CharSet Character Type");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##CharSetCharacterType", currentcharSetCharacterType2))
		{
			if (globalOutfitKit)
			{
				for (size_t i = 0; i < 3; ++i)
				{
					bool isSelected = currentcharSetCharacterType2 == charSetCharacterTypes[i];

					if (ImGui::Selectable(charSetCharacterTypes[i], isSelected))
					{
						currentcharSetCharacterType2 = charSetCharacterTypes[i];
					}
				}
			}

			ImGui::EndCombo();
		}

		if (ImGui::Button("Teleport NPC To Player"))
		{
			TEntityRef<ZHitman5> localHitman;

			Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &localHitman);

			ZEntityRef s_Ref;

			actor->GetID(&s_Ref);

			ZSpatialEntity* hitmanSpatialEntity = localHitman.m_ref.QueryInterface<ZSpatialEntity>();
			ZSpatialEntity* actorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

			actorSpatialEntity->SetWorldMatrix(hitmanSpatialEntity->GetWorldMatrix());
		}

		ImGui::EndChild();
		ImGui::EndGroup();
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}

void DebugMod::EquipOutfit(TEntityRef<ZGlobalOutfitKit>& globalOutfitKit, unsigned int currentCharacterSetIndex, const char* currentcharSetCharacterType, unsigned int currentOutfitVariationIndex, ZHitman5* localHitman)
{
	std::vector<ZRuntimeResourceID> heroOutfitVariations;

	if (strcmp(currentcharSetCharacterType, "HeroA") != 0)
	{
		ZOutfitVariationCollection* outfitVariationCollection = globalOutfitKit.m_pInterfaceRef->m_aCharSets[currentCharacterSetIndex].m_pInterfaceRef;
		TEntityRef<ZCharsetCharacterType>* charsetCharacterType = nullptr;
		TEntityRef<ZCharsetCharacterType>* charsetCharacterType2 = &outfitVariationCollection->m_aCharacters[2];

		if (strcmp(currentcharSetCharacterType, "Actor") == 0)
		{
			charsetCharacterType = &outfitVariationCollection->m_aCharacters[0];
		}
		else if (strcmp(currentcharSetCharacterType, "Nude") == 0)
		{
			charsetCharacterType = &outfitVariationCollection->m_aCharacters[1];
		}

		for (size_t i = 0; i < charsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
		{
			heroOutfitVariations.push_back(charsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit);
		}

		if (charsetCharacterType)
		{
			for (size_t i = 0; i < charsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
			{
				charsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = charsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit;
			}
		}
	}

	Functions::ZHitman5_SetOutfit->Call(localHitman, globalOutfitKit, currentCharacterSetIndex, currentOutfitVariationIndex, false, false);

	if (strcmp(currentcharSetCharacterType, "HeroA") != 0)
	{
		ZOutfitVariationCollection* outfitVariationCollection = globalOutfitKit.m_pInterfaceRef->m_aCharSets[currentCharacterSetIndex].m_pInterfaceRef;
		TEntityRef<ZCharsetCharacterType>* charsetCharacterType = &outfitVariationCollection->m_aCharacters[2];

		for (size_t i = 0; i < heroOutfitVariations.size(); ++i)
		{
			charsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = heroOutfitVariations[i];
		}
	}
}

void DebugMod::EquipOutfit(TEntityRef<ZGlobalOutfitKit>& globalOutfitKit, unsigned int currentCharacterSetIndex, const char* currentcharSetCharacterType, unsigned int currentOutfitVariationIndex, ZActor* actor)
{
	std::vector<ZRuntimeResourceID> actorOutfitVariations;

	if (strcmp(currentcharSetCharacterType, "Actor") != 0)
	{
		ZOutfitVariationCollection* outfitVariationCollection = globalOutfitKit.m_pInterfaceRef->m_aCharSets[currentCharacterSetIndex].m_pInterfaceRef;

		TEntityRef<ZCharsetCharacterType>* charsetCharacterType = nullptr;
		TEntityRef<ZCharsetCharacterType>* charsetCharacterType2 = &outfitVariationCollection->m_aCharacters[0];

		if (strcmp(currentcharSetCharacterType, "Nude") == 0)
		{
			charsetCharacterType = &outfitVariationCollection->m_aCharacters[1];
		}
		else if (strcmp(currentcharSetCharacterType, "HeroA") == 0)
		{
			charsetCharacterType = &outfitVariationCollection->m_aCharacters[2];
		}

		for (size_t i = 0; i < charsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
		{
			actorOutfitVariations.push_back(charsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit);
		}

		if (charsetCharacterType)
		{
			for (size_t i = 0; i < charsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
			{
				charsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = charsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit;
			}
		}
	}

	Functions::ZActor_SetOutfit->Call(actor, globalOutfitKit, currentCharacterSetIndex, currentOutfitVariationIndex, false);

	if (strcmp(currentcharSetCharacterType, "Actor") != 0)
	{
		ZOutfitVariationCollection* outfitVariationCollection = globalOutfitKit.m_pInterfaceRef->m_aCharSets[currentCharacterSetIndex].m_pInterfaceRef;
		TEntityRef<ZCharsetCharacterType>* charsetCharacterType = &outfitVariationCollection->m_aCharacters[0];

		for (size_t i = 0; i < actorOutfitVariations.size(); ++i)
		{
			charsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = actorOutfitVariations[i];
		}
	}
}

void DebugMod::SpawnRepositoryProp(const ZRepositoryID& repositoryID, bool addToWorld)
{
	TEntityRef<ZHitman5> localHitman;

	Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &localHitman);

	if (addToWorld)
	{
		auto scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

		if (!scene)
		{
			Logger::Debug("Scene not loaded.");
		}
		else
		{
			const auto s_ID = ResId<"[modules:/zitemspawner.class].pc_entitytype">;
			const auto s_ID2 = ResId<"[modules:/zitemrepositorykeyentity.class].pc_entitytype">;

			TResourcePtr<ZTemplateEntityFactory> s_Resource, s_Resource2;

			Globals::ResourceManager->GetResourcePtr(s_Resource, s_ID, 0);
			Globals::ResourceManager->GetResourcePtr(s_Resource2, s_ID2, 0);

			Logger::Debug("Resource: {} {}", s_Resource.m_nResourceIndex, fmt::ptr(s_Resource.GetResource()));

			if (!s_Resource)
			{
				Logger::Debug("Resource is not loaded.");
			}
			else
			{
				ZEntityRef newEntity, newEntity2;

				Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, newEntity, "", s_Resource, scene.m_ref, nullptr, -1);
				Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, newEntity2, "", s_Resource2, scene.m_ref, nullptr, -1);

				if (!newEntity)
				{
					Logger::Debug("Failed to spawn entity.");
				}

				if (!newEntity2)
				{
					Logger::Debug("Failed to spawn entity2.");
				}

				const auto hitmanSpatialEntity = localHitman.m_ref.QueryInterface<ZSpatialEntity>();
				ZItemSpawner* itemSpawner = newEntity.QueryInterface<ZItemSpawner>();

				itemSpawner->m_ePhysicsMode = ZItemSpawner::EPhysicsMode::EPM_KINEMATIC;
				itemSpawner->m_rMainItemKey.m_ref = newEntity2;
				itemSpawner->m_rMainItemKey.m_pInterfaceRef = newEntity2.QueryInterface<ZItemRepositoryKeyEntity>();
				itemSpawner->m_rMainItemKey.m_pInterfaceRef->m_RepositoryId = repositoryID;
				itemSpawner->m_bUsePlacementAttach = false;
				itemSpawner->SetWorldMatrix(hitmanSpatialEntity->GetWorldMatrix());

				Functions::ZItemSpawner_RequestContentLoad->Call(itemSpawner);
			}
		}
	}
	else
	{
		TArray<TEntityRef<ZCharacterSubcontroller>>* controllers = &localHitman.m_pInterfaceRef->m_pCharacter.m_pInterfaceRef->m_rSubcontrollerContainer.m_pInterfaceRef->m_aReferencedControllers;
		ZCharacterSubcontrollerInventory* inventory = static_cast<ZCharacterSubcontrollerInventory*>(controllers->operator[](6).m_pInterfaceRef);
		TArray<ZRepositoryID> aModifierIds;

		unsigned long long result = Functions::ZCharacterSubcontrollerInventory_AddDynamicItemToInventory->Call(inventory, repositoryID, "", &aModifierIds, 2);
	}
}

void DebugMod::SpawnNonRepositoryProp(const char* propAssemblyPath)
{
	auto scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

	if (!scene)
	{
		Logger::Debug("Scene not loaded.");
	}
	else
	{
		Hash::MD5Hash hash = Hash::MD5(std::string_view(propAssemblyPath, strlen(propAssemblyPath)));

		uint32_t idHigh = ((hash.A >> 24) & 0x000000FF)
			| ((hash.A >> 8) & 0x0000FF00)
			| ((hash.A << 8) & 0x00FF0000);

		uint32_t idLow = ((hash.B >> 24) & 0x000000FF)
			| ((hash.B >> 8) & 0x0000FF00)
			| ((hash.B << 8) & 0x00FF0000)
			| ((hash.B << 24) & 0xFF000000);

		ZRuntimeResourceID runtimeResourceID = ZRuntimeResourceID(idHigh, idLow);
		TResourcePtr<ZTemplateEntityFactory> resource;

		Globals::ResourceManager->GetResourcePtr(resource, runtimeResourceID, 0);

		if (!resource)
		{
			Logger::Debug("Resource is not loaded.");
		}
		else
		{
			ZEntityRef newEntity;

			Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, newEntity, "", resource, scene.m_ref, nullptr, -1);

			if (!newEntity)
			{
				Logger::Debug("Failed to spawn entity.");
			}
			else
			{
				newEntity.SetProperty("m_eRoomBehaviour", ZSpatialEntity::ERoomBehaviour::ROOM_DYNAMIC);
			}

			TEntityRef<ZHitman5> localHitman;

			Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &localHitman);

			const auto hitmanSpatialEntity = localHitman.m_ref.QueryInterface<ZSpatialEntity>();
			const auto propSpatialEntity = newEntity.QueryInterface<ZSpatialEntity>();

			propSpatialEntity->SetWorldMatrix(hitmanSpatialEntity->GetWorldMatrix());

			ZSetpieceEntity* setPieceEntity = newEntity.QueryInterface<ZSetpieceEntity>();

			if (setPieceEntity)
			{
				setPieceEntity->Activate(0);
			}
			else
			{
				newEntity.GetBaseEntity()->Activate(0);
			}
		}
	}
}

void DebugMod::SpawnNPC(const char* npcName, const ZRepositoryID& repositoryID, TEntityRef<ZGlobalOutfitKit>* globalOutfitKit, const char* currentCharacterSetIndex, const char* currentcharSetCharacterType, const char* currentOutfitVariationIndex)
{
	auto scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

	if (!scene)
	{
		Logger::Debug("Scene not loaded.");
	}
	else
	{
		const auto runtimeResourceID = ResId<"[assembly:/templates/gameplay/ai2/actors.template?/npcactor.entitytemplate].pc_entitytype">;
		TResourcePtr<ZTemplateEntityFactory> resource;

		Globals::ResourceManager->GetResourcePtr(resource, runtimeResourceID, 0);

		if (!resource)
		{
			Logger::Debug("Resource is not loaded.");
		}
		else
		{
			ZEntityRef newEntity;

			Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, newEntity, "", resource, scene.m_ref, nullptr, -1);

			if (newEntity)
			{
				TEntityRef<ZHitman5> localHitman;

				Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &localHitman);

				ZActor* actor = newEntity.QueryInterface<ZActor>();

				actor->m_sActorName = npcName;
				actor->m_bStartEnabled = true;
				actor->m_nOutfitCharset = std::stoi(currentCharacterSetIndex);
				actor->m_nOutfitVariation = std::stoi(currentOutfitVariationIndex);
				actor->m_OutfitRepositoryID = repositoryID;
				actor->m_eRequiredVoiceVariation = EActorVoiceVariation::eAVV_Undefined;

				actor->Activate(0);

				ZSpatialEntity* actorSpatialEntity = newEntity.QueryInterface<ZSpatialEntity>();
				ZSpatialEntity* hitmanSpatialEntity = localHitman.m_ref.QueryInterface<ZSpatialEntity>();

				actorSpatialEntity->SetWorldMatrix(hitmanSpatialEntity->GetWorldMatrix());

				if (globalOutfitKit)
				{
					EquipOutfit(*globalOutfitKit, std::stoi(currentCharacterSetIndex), currentcharSetCharacterType, std::stoi(currentOutfitVariationIndex), actor);
				}
			}
		}
	}
}

void DebugMod::LoadRepositoryProps()
{
	THashMap<ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>* repositoryData = nullptr;

	if (repositoryResource.m_nResourceIndex == -1)
	{
		const auto s_ID = ResId<"[assembly:/repository/pro.repo].pc_repo">;

		Globals::ResourceManager->GetResourcePtr(repositoryResource, s_ID, 0);
	}

	if (repositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID)
	{
		repositoryData = static_cast<THashMap<ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(repositoryResource.GetResourceData());

		for (auto it = repositoryData->begin(); it != repositoryData->end(); ++it)
		{
			ZDynamicObject* dynamicObject = &it->second;
			TArray<SDynamicObjectKeyValuePair>* entries = dynamicObject->As<TArray<SDynamicObjectKeyValuePair>>();
			std::string id;

			for (size_t i = 0; i < entries->size(); ++i)
			{
				std::string key = entries->operator[](i).sKey.c_str();

				if (key == "ID_")
				{
					id = ConvertDynamicObjectValueTString(&entries->operator[](i).value);
				}

				if (key == "Title")
				{
					std::string title = ConvertDynamicObjectValueTString(&entries->operator[](i).value);

					repositoryProps.insert(std::make_pair(title, ZRepositoryID(id.c_str())));

					break;
				}
			}
		}
	}
}

std::string DebugMod::ConvertDynamicObjectValueTString(ZDynamicObject* dynamicObject)
{
	std::string result;
	IType* type = dynamicObject->m_pTypeID->typeInfo();

	if (strcmp(type->m_pTypeName, "ZString") == 0)
	{
		ZString* value = dynamicObject->As<ZString>();

		result = value->c_str();
	}
	else if (strcmp(type->m_pTypeName, "bool") == 0)
	{
		bool value = *dynamicObject->As<bool>();

		if (value)
		{
			result = "true";
		}
		else
		{
			result = "false";
		}
	}
	else if (strcmp(type->m_pTypeName, "float64") == 0)
	{
		double value = *dynamicObject->As<double>();

		result = std::to_string(value).c_str();
	}
	else
	{
		Logger::Debug(type->m_pTypeName);
	}

	return result;
}

void DebugMod::LoadResource(unsigned long long hash, std::vector<char>& resourceData)
{
	static std::string rpkgFilePath = GetPatchRPKGFilePath();
	ZBinaryReader binaryReader = ZBinaryReader(rpkgFilePath);

	binaryReader.Seek(0xD);

	unsigned int resourceCount = binaryReader.Read<unsigned int>();

	binaryReader.Seek(0x19);

	unsigned int patchDeletionEntryCount = binaryReader.Read<unsigned int>();

	binaryReader.Seek(0x1D + 8 * patchDeletionEntryCount);

	while (true)
	{
		unsigned long long hash2 = binaryReader.Read<unsigned long long>();

		if (hash2 == hash)
		{
			unsigned long long resourceDataOffset = binaryReader.Read<unsigned long long>();
			unsigned int dataSize = binaryReader.Read<unsigned int>();

			bool isResourceEncrypted = (dataSize & 0x80000000) == 0x80000000;
			bool isResourceCompressed = (dataSize & 0x3FFFFFFF) != 0;

			binaryReader.Seek(resourceDataOffset);

			TResourcePtr<ZTemplateEntityFactory> resource;

			Globals::ResourceManager->GetResourcePtr(resource, ZRuntimeResourceID(hash), 0);

			ZResourceContainer::SResourceInfo resourceInfo = (*Globals::ResourceContainer)->m_resources[resource.m_nResourceIndex];

			if (isResourceEncrypted)
			{
				dataSize &= 0x3FFFFFFF;
			}
			else
			{
				dataSize = resourceInfo.finalDataSize;
			}

			std::vector<char> inputResourceData;

			inputResourceData.reserve(dataSize);
			binaryReader.ReadBytes(inputResourceData.data(), dataSize);

			if (isResourceEncrypted)
			{
				Crypto::XORData(inputResourceData.data(), dataSize);
			}

			std::vector<char> outputResourceData = std::vector<char>(resourceInfo.finalDataSize, 0);

			if (isResourceCompressed)
			{
				LZ4_decompress_safe(inputResourceData.data(), outputResourceData.data(), dataSize, resourceInfo.finalDataSize);

				resourceData = outputResourceData;
			}
			else
			{
				resourceData = inputResourceData;
			}

			break;
		}

		size_t currentPositon = binaryReader.GetPosition();

		binaryReader.Seek(currentPositon + 12);
	}
}

std::string DebugMod::GetPatchRPKGFilePath()
{
	std::string rpkgFilePath;

	for (const auto& entry : std::filesystem::directory_iterator("../Runtime"))
	{
		if (entry.path().string().starts_with("../Runtime\\chunk0"))
		{
			rpkgFilePath = entry.path().string();
		}
		else
		{
			break;
		}
	}

	return rpkgFilePath;
}

unsigned long long DebugMod::GetDDSTextureHash(const std::string image)
{
	static std::unordered_map<std::string, unsigned long long> oresEntries;

	if (oresEntries.empty())
	{
		const auto s_ID = ResId<"[assembly:/_pro/online/default/offlineconfig/config.blobs].pc_blobs">;
		TResourcePtr<ZTemplateEntityFactory> resource;

		Globals::ResourceManager->GetResourcePtr(resource, s_ID, 0);

		ZResourceContainer::SResourceInfo resourceInfo = (*Globals::ResourceContainer)->m_resources[resource.m_nResourceIndex];

		unsigned long long oresHash = resourceInfo.rid.GetID();
		std::vector<char> oresResourceData;

		LoadResource(oresHash, oresResourceData);

		ZBinaryReader binaryReader = ZBinaryReader(&oresResourceData);

		binaryReader.Seek(0x10);
		binaryReader.Seek(binaryReader.Read<unsigned int>() + 0xC);

		unsigned resourceCount = binaryReader.Read<unsigned int>();

		for (unsigned int i = 0; i < resourceCount; ++i)
		{
			unsigned int stringLength = binaryReader.Read<unsigned int>();

			binaryReader.Seek(0x4, ZBinaryReader::ESeekOrigin::current);

			unsigned long long stringOffset = binaryReader.Read<unsigned long long>();
			ZRuntimeResourceID runtimeResourceID = binaryReader.Read<ZRuntimeResourceID>();

			size_t currentPosition = binaryReader.GetPosition();

			binaryReader.Seek(stringOffset + 0x10 - 0x4);

			unsigned int stringLength2 = binaryReader.Read<unsigned int>();
			std::string image2 = binaryReader.ReadString(stringLength2 - 1);

			binaryReader.Seek(currentPosition);

			oresEntries[image2.c_str()] = runtimeResourceID.GetID();
		}
	}

	return oresEntries[image];
}

void DebugMod::CopyToClipboard(const std::string& p_String) const
{
	if (!OpenClipboard(nullptr))
		return;

	EmptyClipboard();

	auto s_GlobalData = GlobalAlloc(GMEM_MOVEABLE, p_String.size() + 1);

	if (!s_GlobalData)
	{
		CloseClipboard();
		return;
	}

	auto s_GlobalDataPtr = GlobalLock(s_GlobalData);

	if (!s_GlobalDataPtr)
	{
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

void DebugMod::OnDraw3D(IRenderer* p_Renderer)
{
	if (m_RenderNpcBoxes || m_RenderNpcNames || m_RenderNpcRepoIds)
	{
		for (int i = 0; i < *Globals::NextActorId; ++i)
		{
			auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

			ZEntityRef s_Ref;
			s_Actor->GetID(&s_Ref);

			auto* s_SpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

			SMatrix s_Transform;
			Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

			if (m_RenderNpcBoxes)
			{
				float4 s_Min, s_Max;

				s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

				p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(1.f, 0.f, 0.f, 1.f));
			}

			if (m_RenderNpcNames)
			{
				SVector2 s_ScreenPos;
				if (p_Renderer->WorldToScreen(SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.05f), s_ScreenPos))
					p_Renderer->DrawText2D(s_Actor->m_sActorName, s_ScreenPos, SVector4(1.f, 0.f, 0.f, 1.f), 0.f, 0.5f);
			}

			if (m_RenderNpcRepoIds)
			{
				auto* s_RepoEntity = s_Ref.QueryInterface<ZRepositoryItemEntity>();
				SVector2 s_ScreenPos;
				bool s_Success;

				if (m_RenderNpcNames)
					s_Success = p_Renderer->WorldToScreen(SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.1f), s_ScreenPos);
				else
					s_Success = p_Renderer->WorldToScreen(SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.05f), s_ScreenPos);

				if (s_Success)
					p_Renderer->DrawText2D(s_RepoEntity->m_sId.ToString(), s_ScreenPos, SVector4(1.f, 0.f, 0.f, 1.f), 0.f, 0.5f);
			}
		}
	}

	m_EntityMutex.lock_shared();

	if (m_SelectedEntity)
	{
		auto* s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>();

		SMatrix s_Transform;
		Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

		float4 s_Min, s_Max;

		s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

		p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(0.f, 0.f, 1.f, 1.f));
	}
	
	m_EntityMutex.unlock_shared();

	/*SVector2 s_StartPos;
	if (p_Renderer->WorldToScreen(SVector3(m_From.x, m_From.y, m_From.z), s_StartPos))
		p_Renderer->DrawText2D("0", s_StartPos, SVector4(1, 1, 1, 1), 0, 0.5f);

	SVector2 s_EndPos;
	if (p_Renderer->WorldToScreen(SVector3(m_To.x, m_To.y, m_To.z), s_EndPos))
		p_Renderer->DrawText2D("o", s_EndPos, SVector4(1, 1, 1, 1), 0, 0.25f);

	SVector2 s_HitPos;
	if (p_Renderer->WorldToScreen(SVector3(m_Hit.x, m_Hit.y, m_Hit.z), s_HitPos))
		p_Renderer->DrawText2D("x", s_HitPos, SVector4(1, 1, 1, 1), 0, 0.25f);

	p_Renderer->DrawBox3D(
		SVector3(m_From.x - 0.05f, m_From.y - 0.05f, m_From.z - 0.05f),
		SVector3(m_From.x + 0.05f, m_From.y + 0.05f, m_From.z + 0.05f),
		SVector4(0.f, 0.f, 1.f, 1.0f)
	);

	p_Renderer->DrawBox3D(
		SVector3(m_To.x - 0.05f, m_To.y - 0.05f, m_To.z - 0.05f),
		SVector3(m_To.x + 0.05f, m_To.y + 0.05f, m_To.z + 0.05f),
		SVector4(0.f, 1.f, 0.f, 1.0f)
	);

	p_Renderer->DrawBox3D(
		SVector3(m_Hit.x - 0.01f, m_Hit.y - 0.01f, m_Hit.z - 0.01f),
		SVector3(m_Hit.x + 0.01f, m_Hit.y + 0.01f, m_Hit.z + 0.01f),
		SVector4(0.f, 1.f, 1.f, 1.0f)
	);

	p_Renderer->DrawLine3D(
		SVector3(m_From.x, m_From.y, m_From.z),
		SVector3(m_To.x, m_To.y, m_To.z),
		SVector4(1.f, 1.f, 1.f, 1.f),
		SVector4(1.f, 1.f, 1.f, 1.f)
	);

	p_Renderer->DrawLine3D(
		SVector3(m_Hit.x + (m_Normal.x * 0.15f), m_From.y + (m_Normal.y * 0.15f), m_From.z + (m_Normal.z * 0.15f)),
		SVector3(m_Hit.x, m_Hit.y, m_Hit.z),
		SVector4(0.63f, 0.13f, 0.94f, 1.f),
		SVector4(0.63f, 0.13f, 0.94f, 1.f)
	);*/
}

DECLARE_PLUGIN_DETOUR(DebugMod, void, ZHttpBufferReady, ZHttpResultDynamicObject* th)
{
	/*Logger::Debug("ZHttp thing wow wow {} {} {} {}", fmt::ptr(th), offsetof(ZHttpResultDynamicObject, m_buffer), fmt::ptr(&th->m_buffer), fmt::ptr(th->m_buffer.data()));

	std::string s_Data(static_cast<char*>(th->m_buffer.data()), th->m_buffer.size());

	Logger::Debug("Some shit: {}", s_Data);

	th->m_buffer = ZBuffer::FromData(s_Data);

	std::string s_Data2(static_cast<char*>(th->m_buffer.data()), th->m_buffer.size());

	Logger::Debug("Explosions shit: {}", s_Data2);*/

	return HookResult<void>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(DebugMod, void, WinHttpCallback, void* dwContext, void* hInternet, void* param_3, int dwInternetStatus, void* param_5, int length_param_6)
{
	/*if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_SENDING_REQUEST)
	{
		WinHttpAddRequestHeaders(hInternet, L"Accept-Encoding: identity", (ULONG)-1L, WINHTTP_ADDREQ_FLAG_REPLACE);
		Logger::Info("header set");
	}*/

	return HookResult<void>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(DebugMod, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear)
{
	m_EntityMutex.lock();
	m_SelectedEntity = ZEntityRef();
	m_EntityMutex.unlock();

	return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(DebugMod);
