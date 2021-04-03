#include "DebugMod.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZScene.h>
#include <Glacier/ZActor.h>


#include "Functions.h"
#include "Globals.h"

void DebugMod::OnDrawMenu()
{
	if (ImGui::Button("DEBUG MENU"))
	{
		m_MenuActive = !m_MenuActive;
	}
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
		ImGui::Checkbox("Render NPC position lines", &m_RenderNpcLines);
		ImGui::Checkbox("Render NPC names", &m_RenderNpcNames);
		ImGui::Checkbox("Render NPC repository IDs", &m_RenderNpcRepoIds);
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}

void DebugMod::OnDraw3D(IRenderer* p_Renderer)
{
	if (m_RenderNpcLines || m_RenderNpcNames || m_RenderNpcRepoIds)
	{
		for (int i = 0; i < *Globals::NextActorId; ++i)
		{
			auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

			ZEntityRef s_Ref;
			s_Actor->GetID(&s_Ref);

			auto* s_SpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

			SMatrix s_Transform;
			Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

			if (m_RenderNpcLines)
			{
				p_Renderer->DrawLine3D(
					SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z),
					SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2),
					SVector4(1.f, 1.f, 1.f, 1.f),
					SVector4(1.f, 0.f, 0.f, 1.f)
				);
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
}

DECLARE_ZHM_PLUGIN(DebugMod);
