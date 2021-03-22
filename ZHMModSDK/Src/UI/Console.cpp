#include "Console.h"

#include <imgui.h>

using namespace UI;

std::vector<Console::LogLine>* Console::m_LogLines = nullptr;

void Console::Draw()
{
	bool s_Open = true;
	ImGui::Begin("Console", &s_Open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	auto& s_ImGuiIO = ImGui::GetIO();
	ImGui::SetWindowSize(ImVec2(s_ImGuiIO.DisplaySize.x - 60, 200), ImGuiCond_Always);
	ImGui::SetWindowPos(ImVec2(30, 30), ImGuiCond_Always);

	// Render the list of log lines.
	ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

	for (auto& s_LogLine : *m_LogLines)
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

	ImGui::EndChild();

	// Render the text input.
	ImGui::Separator();

	char s_Command[2048];
	memset(s_Command, 0, sizeof(s_Command));
	ImGui::InputText("ConsoleInput", s_Command, IM_ARRAYSIZE(s_Command), ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::SetItemDefaultFocus();

	ImGui::End();
}

void Console::AddLogLine(spdlog::level::level_enum p_Level, const ZString& p_Text)
{
	if (m_LogLines == nullptr)
		m_LogLines = new std::vector<LogLine>();
	
	m_LogLines->push_back(LogLine { p_Level, ZString::CopyFrom(p_Text) });
}
