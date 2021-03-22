#include "Console.h"

#include <imgui.h>

using namespace UI;

void Console::Draw()
{
    bool s_Open = true;
    ImGui::Begin("Console", &s_Open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    auto& s_ImGuiIO = ImGui::GetIO();
    ImGui::SetWindowSize(ImVec2(s_ImGuiIO.DisplaySize.x - 60, 200), ImGuiCond_Always);
    ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Always);

	// Render the list of log lines.
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

    for (int i = 0; i < 100; ++i)
    {
        ImGui::Text("Console log %d", i);
    }

    ImGui::EndChild();

	// Render the text input.
    ImGui::Separator();

    char s_Command[2048];
    memset(s_Command, 0, sizeof(s_Command));
    ImGui::InputText("ConsoleInput", s_Command, IM_ARRAYSIZE(s_Command), ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::SetItemDefaultFocus();
	
    ImGui::End();
}
