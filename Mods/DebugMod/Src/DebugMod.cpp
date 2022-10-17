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

#include <Functions.h>
#include <Globals.h>

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
	if (Functions::ZInputAction_Digital->Call(&m_RaycastAction, -1))
	{
		DoRaycast();
	}

	if (Functions::ZInputAction_Analog->Call(&m_DeleteAction, -1) > 0.0)
	{
		m_EntityMutex.lock_shared();

		MoveObject();
		
		m_EntityMutex.unlock_shared();
	}
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

	if (ImGui::Button("BADABING BADABOOM"))
	{
		auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

		if (!s_Scene)
		{
			Logger::Debug("Scene not loaded.");
		}
		else
		{
			//const auto s_ID = ResId<"[assembly:/_pro/environment/templates/props/lamps/lamps_outdoor_paris_a.template?/facility_vertical_stand_floodlight_00.entitytemplate].pc_entitytype">;
			const auto s_ID = ResId<"[assembly:/templates/gameplay/ai2/actors.template?/npcactor.entitytemplate].pc_entitytype">;

			TResourcePtr<ZTemplateEntityFactory> s_Resource;
			Globals::ResourceManager->GetResourcePtr(s_Resource, s_ID, 0);

			Logger::Debug("Resource: {} {}", s_Resource.m_nResourceIndex, fmt::ptr(s_Resource.GetResource()));

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
					auto s_Actor = s_NewEntity.QueryInterface<ZActor>();

					Logger::Debug("Spawned entity {}!", fmt::ptr(s_Actor));

					TEntityRef<ZHitman5> s_LocalHitman;
					Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);
					
					// Set outfit and other properties.
					s_Actor->m_sActorName = "Ur Mum";
					s_Actor->m_nOutfitVariation = 1;
					s_Actor->m_bStartEnabled = true;
					s_Actor->m_OutfitRepositoryID = ZRepositoryID("E222CC14-8D48-42DE-9AF6-1B745DBB3614");
					s_Actor->m_eRequiredVoiceVariation = EActorVoiceVariation::eAVV_MECH03;

					auto s_ActorSpatial = s_NewEntity.QueryInterface<ZSpatialEntity>();
					const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
					
					s_ActorSpatial->m_mTransform = s_HitmanSpatial->m_mTransform;
					s_ActorSpatial->m_mTransform.Trans.x += 2.f;

					s_Actor->Activate(0);
				}
			}
		}
	}
}

void DebugMod::DoRaycast()
{
	if (!*Globals::CollisionManager)
	{
		Logger::Error("Collision manager not found.");
		return;
	}

	const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

	if (!s_CurrentCamera)
	{
		Logger::Error("No camera spawned.");
		return;
	}

	auto s_Transform = s_CurrentCamera->GetWorldMatrix();

	m_From = s_Transform.Trans;
	m_To = m_From - (s_Transform.ZAxis * 200.f);

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

	if (p_HasFocus)
	{
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			auto s_MousePos = ImGui::GetMousePos();
		}
	}
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

	SVector2 s_StartPos;
	if (p_Renderer->WorldToScreen(SVector3(m_From.x, m_From.y, m_From.z), s_StartPos))
		p_Renderer->DrawText2D("0", s_StartPos, SVector4(1, 1, 1, 1), 0, 0.5f);

	SVector2 s_EndPos;
	if (p_Renderer->WorldToScreen(SVector3(m_To.x, m_To.y, m_To.z), s_EndPos))
		p_Renderer->DrawText2D("1", s_EndPos, SVector4(1, 1, 1, 1), 0, 0.5f);

	SVector2 s_HitPos;
	if (p_Renderer->WorldToScreen(SVector3(m_Hit.x, m_Hit.y, m_Hit.z), s_HitPos))
		p_Renderer->DrawText2D("H", s_HitPos, SVector4(1, 1, 1, 1), 0, 0.5f);

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
		SVector3(m_Hit.x - 0.05f, m_Hit.y - 0.05f, m_Hit.z - 0.05f),
		SVector3(m_Hit.x + 0.05f, m_Hit.y + 0.05f, m_Hit.z + 0.05f),
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
	);
}

DECLARE_ZHM_PLUGIN(DebugMod);
