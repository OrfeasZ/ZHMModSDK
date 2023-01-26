#include "DebugMod.h"

#include <Glacier/ZModule.h>

void DebugMod::DrawSceneBox(bool p_HasFocus)
{
	if (!p_HasFocus || !m_SceneMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("SCENE", &m_SceneMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_Showing)
	{
		static size_t s_Selected = 0;
		const ZEntitySceneContext* s_EntitySceneContext = Globals::Hitman5Module->m_pEntitySceneContext;
		const std::string s_EntityTemplate = m_RuntimeResourceIDsToResourceIDs[s_EntitySceneContext->m_SceneConfig.m_ridSceneFactory.GetID()];
		const std::string s_EntityBlueprint = std::format("{}.pc_entityblueprint", s_EntityTemplate.substr(0, s_EntityTemplate.find_last_of(".")));

		ImGui::Text("Scene name: %s", s_EntitySceneContext->m_sceneData.m_sceneName.c_str());
		ImGui::Text("Type: %s", s_EntitySceneContext->m_sceneData.m_type.c_str());
		ImGui::Text("Code Name Hint: %s", s_EntitySceneContext->m_sceneData.m_codeNameHint.c_str());
		ImGui::Text("Entity Template: %s", s_EntityTemplate);
		ImGui::Text("Entity Blueprint: %s", s_EntityBlueprint);

		ImGui::BeginChild("left pane", ImVec2(300, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

		for (int i = 0; i < s_EntitySceneContext->m_aLoadedBricks.size(); ++i)
		{
			ZRuntimeResourceID s_RuntimeResourceId = s_EntitySceneContext->m_aLoadedBricks[i].runtimeResourceID;
			std::string s_ResourceId = m_RuntimeResourceIDsToResourceIDs[s_RuntimeResourceId.GetID()];

			if (ImGui::Selectable(s_ResourceId.c_str(), s_Selected == i))
			{
				s_Selected = i;
			}
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

		ImGui::Text("TEXT");

		ImGui::EndChild();
		ImGui::EndGroup();
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}
