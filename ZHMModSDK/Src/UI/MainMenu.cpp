#include "MainMenu.h"

#include <imgui.h>

#include "IconsMaterialDesign.h"
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
    ImGui::Text("~ or ^ (under ESC) to toggle this menu");

    if (ImGui::Button(ICON_MD_TOKEN " MODS"))
    {
        ModSDK::GetInstance()->GetUIModSelector()->Show();
    }

    ModSDK::GetInstance()->OnDrawMenu();

    ImGui::EndMainMenuBar();
}
