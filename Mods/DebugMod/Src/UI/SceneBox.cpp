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
		static size_t selected = 0;
		ZEntitySceneContext* entitySceneContext = Globals::Hitman5Module->m_pEntitySceneContext;
		std::string entityTemplate = runtimeResourceIDsToResourceIDs[entitySceneContext->m_SceneConfig.m_ridSceneFactory.GetID()];
		std::string entityBlueprint = std::format("{}.pc_entityblueprint", entityTemplate.substr(0, entityTemplate.find_last_of(".")));

		ImGui::Text("Scene name: %s", entitySceneContext->m_sceneData.m_sceneName.c_str());
		ImGui::Text("Type: %s", entitySceneContext->m_sceneData.m_type.c_str());
		ImGui::Text("Code Name Hint: %s", entitySceneContext->m_sceneData.m_codeNameHint.c_str());
		ImGui::Text("Entity Template: %s", entityTemplate);
		ImGui::Text("Entity Blueprint: %s", entityBlueprint);

		ImGui::BeginChild("left pane", ImVec2(300, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

		for (int i = 0; i < entitySceneContext->m_aLoadedBricks.size(); ++i)
		{
			ZRuntimeResourceID runtimeResourceID = entitySceneContext->m_aLoadedBricks[i].runtimeResourceID;
			std::string resourceID = runtimeResourceIDsToResourceIDs[runtimeResourceID.GetID()];

			if (ImGui::Selectable(resourceID.c_str(), selected == i))
			{
				selected = i;
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
