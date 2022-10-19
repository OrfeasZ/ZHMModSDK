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
#include <Functions.h>
#include <Globals.h>

#include <numbers>

DebugMod::~DebugMod()
{
	const ZMemberDelegate<DebugMod, void(const SGameUpdateEvent&)> s_Delegate(this, &DebugMod::OnFrameUpdate);
	Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
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

void DebugMod::MoveObject()
{
	if (!m_SelectedEntity)
	{
		Logger::Warn("No entity selected.");
		return;
	}

	auto s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>();

	if (!s_SpatialEntity)
	{
		Logger::Error("Not a spatial entity.");
		return;
	}

	auto s_EntityWorldMatrix = s_SpatialEntity->GetWorldMatrix();

	const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

	if (!s_CurrentCamera)
	{
		Logger::Error("No camera spawned.");
		return;
	}

	auto s_Transform = s_CurrentCamera->GetWorldMatrix();
	s_EntityWorldMatrix.Trans = s_Transform.Trans - (s_Transform.ZAxis * float4::Distance(m_From, m_Hit));

	s_SpatialEntity->SetWorldMatrix(s_EntityWorldMatrix);
}

void DebugMod::OnDrawMenu()
{
	if (ImGui::Button("DEBUG MENU"))
	{
		m_MenuActive = !m_MenuActive;
	}

	/*if (ImGui::Button("BADABING BADABOOM"))
	{
		auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

		if (!s_Scene)
		{
			Logger::Debug("Scene not loaded.");
		}
		else
		{
			//const auto s_ID = ResId<"[assembly:/_pro/environment/templates/props/containers/military_containers_a.template?/military_box_metal_e_00.entitytemplate].pc_entitytype">;
			const auto s_ID = ResId<"[assembly:/deeznuts.entitytemplate].pc_entitytype">;
			//const auto s_ID = ResId<"[assembly:/templates/gameplay/ai2/actors.template?/npcactor.entitytemplate].pc_entitytype">;
			//const auto s_ID = ResId<"[assembly:/_pro/characters/templates/hero/agent47/agent47.template?/agent47_default.entitytemplate].pc_entitytype">;

			Logger::Debug("Getting resource wew: {}", s_ID);

			TResourcePtr<ZTemplateEntityFactory> s_Resource;
			Globals::ResourceManager->GetResourcePtr(s_Resource, s_ID, 0);

			Logger::Debug("Resource: {} {}", s_Resource.m_nResourceIndex, fmt::ptr(s_Resource.GetResource()));

			const auto s_BrickID = ResId<"[assembly:/_pro/scenes/missions/paris/_scene_lumumba.brick].pc_entitytype">;
			Logger::Debug("Getting brick resource wew: {}", s_BrickID);

			TResourcePtr<ZTemplateEntityFactory> s_BrickResource;
			Globals::ResourceManager->GetResourcePtr(s_BrickResource, s_BrickID, 0);

			Logger::Debug("Brick resource: {} {}", s_BrickResource.m_nResourceIndex, fmt::ptr(s_BrickResource.GetResource()));

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
					TEntityRef<ZHitman5> s_LocalHitman;
					Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

					/*auto s_Actor = s_NewEntity.QueryInterface<ZActor>();

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

					m_EntityMutex.unlock();
				}
			}
		}
	}*/
}

void DebugMod::DoRaycast(float4 p_From, float4 p_To)
{
	if (!*Globals::CollisionManager)
	{
		Logger::Error("Collision manager not found.");
		return;
	}

	m_From = p_From;
	m_To = p_To;

	ZRayQueryInput s_RayInput {
		.m_vFrom = m_From,
		.m_vTo = m_To,
	};

	ZRayQueryOutput s_RayOutput {};

	Logger::Debug("RayCasting from {} to {}.", m_From, m_To);

	if (!(*Globals::CollisionManager)->RayCastClosestHit(s_RayInput, &s_RayOutput))
	{
		Logger::Error("Raycast failed.");
		return;
	}

	Logger::Debug("Raycast result: {} {}", fmt::ptr(&s_RayOutput), s_RayOutput.m_vPosition);

	m_Hit = s_RayOutput.m_vPosition;
	m_Normal = s_RayOutput.m_vNormal;

	m_EntityMutex.lock();
	
	if (s_RayOutput.m_BlockingEntity)
	{
		const auto& s_Interfaces = *(*s_RayOutput.m_BlockingEntity.m_pEntity)->m_pInterfaces;
		Logger::Trace("Hit entity of type '{}' with id '{:x}'.", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName, (*s_RayOutput.m_BlockingEntity.m_pEntity)->m_nEntityId);
	}

	m_SelectedEntity = s_RayOutput.m_BlockingEntity;

	m_EntityMutex.unlock();
}


void DebugMod::OnDrawUI(bool p_HasFocus)
{
	DrawOptions(p_HasFocus);
	DrawPositionBox(p_HasFocus);
	DrawEntityBox(p_HasFocus);

	if (p_HasFocus)
	{
		auto s_ImgGuiIO = ImGui::GetIO();

		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !s_ImgGuiIO.WantCaptureMouse)
		{
			const auto s_MousePos = ImGui::GetMousePos();
			
			SVector3 s_World;
			SVector3 s_Direction;
			SDK()->ScreenToWorld(SVector2(s_MousePos.x, s_MousePos.y), s_World, s_Direction);

			float4 s_DirectionVec(s_Direction.x, s_Direction.y, s_Direction.z, 1.f);
			
			float4 s_From = float4(s_World.x, s_World.y, s_World.z, 1.f);
			float4 s_To = s_From + (s_DirectionVec * 200.f);

			DoRaycast(s_From, s_To);
		}
	}
}

void DebugMod::DrawOptions(bool p_HasFocus)
{
	if (!p_HasFocus || !m_MenuActive)
		return;

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	auto s_Showing = ImGui::Begin("DEBUG MENU", &m_MenuActive);
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
	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("POSITIONS");
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
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}


void DebugMod::DrawEntityBox(bool p_HasFocus)
{
	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("SELECTED ENTITY");
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
			}
		}

		m_EntityMutex.unlock_shared();
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
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
		p_Renderer->DrawText2D("0", s_StartPos, SVector4(1, 1, 1, 1), 0, 0.5f);*/

	SVector2 s_EndPos;
	if (p_Renderer->WorldToScreen(SVector3(m_To.x, m_To.y, m_To.z), s_EndPos))
		p_Renderer->DrawText2D("o", s_EndPos, SVector4(1, 1, 1, 1), 0, 0.25f);

	SVector2 s_HitPos;
	if (p_Renderer->WorldToScreen(SVector3(m_Hit.x, m_Hit.y, m_Hit.z), s_HitPos))
		p_Renderer->DrawText2D("x", s_HitPos, SVector4(1, 1, 1, 1), 0, 0.25f);

	/*p_Renderer->DrawBox3D(
		SVector3(m_From.x - 0.05f, m_From.y - 0.05f, m_From.z - 0.05f),
		SVector3(m_From.x + 0.05f, m_From.y + 0.05f, m_From.z + 0.05f),
		SVector4(0.f, 0.f, 1.f, 1.0f)
	);*/

	/*p_Renderer->DrawBox3D(
		SVector3(m_To.x - 0.05f, m_To.y - 0.05f, m_To.z - 0.05f),
		SVector3(m_To.x + 0.05f, m_To.y + 0.05f, m_To.z + 0.05f),
		SVector4(0.f, 1.f, 0.f, 1.0f)
	);*/

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

	/*p_Renderer->DrawLine3D(
		SVector3(m_Hit.x + (m_Normal.x * 0.15f), m_From.y + (m_Normal.y * 0.15f), m_From.z + (m_Normal.z * 0.15f)),
		SVector3(m_Hit.x, m_Hit.y, m_Hit.z),
		SVector4(0.63f, 0.13f, 0.94f, 1.f),
		SVector4(0.63f, 0.13f, 0.94f, 1.f)
	);*/
}

DECLARE_ZHM_PLUGIN(DebugMod);
