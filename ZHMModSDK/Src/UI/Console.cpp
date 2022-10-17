#include "Console.h"

#include <imgui.h>

#include "IModSDK.h"

using namespace UI;

Console::Console()
{
	InitializeSRWLock(&m_Lock);
}

void Console::Draw(bool p_HasFocus)
{
	if (!p_HasFocus)
		return;

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_Showing = ImGui::Begin("CONSOLE", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	ImGui::SetWindowCollapsed(true, ImGuiCond_Once);

	const auto& s_ImGuiIO = ImGui::GetIO();
	ImGui::SetWindowSize(ImVec2(s_ImGuiIO.DisplaySize.x - 60, 400), ImGuiCond_Always);
	ImGui::SetWindowPos(ImVec2(30, 80 * (s_ImGuiIO.DisplaySize.y / 2048.f)), ImGuiCond_Always);

	if (s_Showing)
	{
		// Render the list of log lines.
		const float s_FooterHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -s_FooterHeight), false);

		AcquireSRWLockShared(&m_Lock);
		
		ImGui::PushTextWrapPos();

		for (auto& s_LogLine : m_LogLines)
		{
			ImVec4 s_Color;
			bool s_Colored = false;

			if (s_LogLine.Level == spdlog::level::trace)
			{
				s_Color = ImVec4(168.f / 255.f, 61.f / 255.f, 255.f / 255.f, 1.f);
				s_Colored = true;
			}
			else if (s_LogLine.Level == spdlog::level::debug)
			{
				s_Color = ImVec4(61.f / 255.f, 129.f / 255.f, 255.f / 255.f, 1.f);
				s_Colored = true;
			}
			else if (s_LogLine.Level == spdlog::level::warn)
			{
				s_Color = ImVec4(255.f / 255.f, 168.f / 255.f, 61.f / 255.f, 1.f);
				s_Colored = true;
			}
			else if (s_LogLine.Level == spdlog::level::err || s_LogLine.Level == spdlog::level::critical)
			{
				s_Color = ImVec4(255.f / 255.f, 69.f / 255.f, 69.f / 255.f, 1.f);
				s_Colored = true;
			}

			if (s_Colored)
				ImGui::PushStyleColor(ImGuiCol_Text, s_Color);

			ImGui::TextUnformatted(s_LogLine.Text.c_str(), s_LogLine.Text.c_str() + s_LogLine.Text.size());

			if (s_Colored)
				ImGui::PopStyleColor();
		}

		ImGui::PopTextWrapPos();

		// Auto scroll to bottom.
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0);

		ReleaseSRWLockShared(&m_Lock);

		ImGui::EndChild();

		// Render the text input.
		ImGui::Separator();

		char s_Command[2048] = {};
		ImGui::InputText("##consoleCommand", s_Command, IM_ARRAYSIZE(s_Command), ImGuiInputTextFlags_EnterReturnsTrue);

		ImGui::SetItemDefaultFocus();
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}

void Console::AddLogLine(spdlog::level::level_enum p_Level, const ZString& p_Text)
{
	AcquireSRWLockExclusive(&m_Lock);
	
	m_LogLines.push_back(LogLine { p_Level, std::string(p_Text.c_str(), p_Text.size()) });
	
	ReleaseSRWLockExclusive(&m_Lock);
}
