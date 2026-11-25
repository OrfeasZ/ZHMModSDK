#include "MainMenu.h"

#include <imgui.h>

#include "IconsMaterialDesign.h"
#include "imgui_internal.h"
#include "ModSDK.h"
#include "ModSelector.h"
#include "Hotkeys.h"

using namespace UI;

void MainMenu::Draw(bool p_HasFocus) {
    if (!p_HasFocus)
        return;

    const auto* s_Viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowSize(ImVec2(s_Viewport->Size.x, 0.f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(s_Viewport->Pos, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 0));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.14f, 0.14f, 0.14f, 1.00f));

    ImGui::Begin(
        "#MainMenu", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar
    );

    ImGui::GetCurrentWindow()->DC.LayoutType = ImGuiLayoutType_Horizontal;

    ImGui::AlignTextToFramePadding();
    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    ImGui::Text("ZHM MOD SDK");
    ImGui::PopFont();

    ImGui::AlignTextToFramePadding();
    ImGui::Text("~ or ^ (under ESC) to toggle this menu");

    if (ImGui::Button(ICON_MD_TOKEN " MODS")) {
        ModSDK::GetInstance()->GetUIModSelector()->Show();
    }

    if (ImGui::Button(ICON_MD_KEYBOARD " SHORTCUTS")) {
        ModSDK::GetInstance()->GetHotkeys()->Show();
    }

    ModSDK::GetInstance()->OnDrawMenu();

    ImGui::End();

    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor();
}