#include "MainMenu.h"

#include <imgui.h>

#include "ModSDK.h"
#include "ModSelector.h"

using namespace UI;

void MainMenu::Draw(bool p_HasFocus)
{
	if (!p_HasFocus)
		return;
	
	ImGui::BeginMainMenuBar();

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	ImGui::Text("ZHM MOD SDK");
	ImGui::PopFont();
	ImGui::Text("Press the ~ key (under ESC) to open this menu");

	if (ImGui::Button("SELECT MODS"))
	{
		ModSDK::GetInstance()->GetUIModSelector()->Show();
	}

	ModSDK::GetInstance()->OnDrawMenu();

	ImGui::EndMainMenuBar();
}
