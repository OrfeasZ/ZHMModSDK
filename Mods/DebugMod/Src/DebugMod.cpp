#include "DebugMod.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZScene.h>
#include <Glacier/ZActor.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/EntityFactory.h>

#include <Functions.h>
#include <Globals.h>

#include "Glacier/ZModule.h"

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

					m_EntityMutex.lock();

					m_EntitiesToTrack.push_back(s_NewEntity);

					m_EntityMutex.unlock();
				}
			}
		}
	}

	if (ImGui::Button("BLOOP"))
	{
		m_EntityMutex.lock();

		m_EntitiesToTrack.clear();

		m_EntityMutex.unlock();
	}*/
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

	for (auto& s_Entity : m_EntitiesToTrack)
	{
		auto* s_SpatialEntity = s_Entity.QueryInterface<ZSpatialEntity>();

		SMatrix s_Transform;
		Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

		float4 s_Min, s_Max;

		s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

		p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(0.f, 0.f, 1.f, 1.f));
	}
	
	m_EntityMutex.unlock_shared();
}

DECLARE_ZHM_PLUGIN(DebugMod);
