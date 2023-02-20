#include <Editor.h>

#include "IconsMaterialDesign.h"

void Editor::DrawPinTracer()
{
    auto s_ImgGuiIO = ImGui::GetIO();

    ImGui::SetNextWindowPos({ s_ImgGuiIO.DisplaySize.x - 400, 710 });
    ImGui::SetNextWindowSize({ 400, s_ImgGuiIO.DisplaySize.y - 710 });
    ImGui::Begin(ICON_MD_PUSH_PIN " Pin Tracer", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

    auto s_Now = std::chrono::system_clock::now();

    ImGui::Text(ICON_MD_LOGIN " Input Pins");

    ImGui::BeginChild("input pins", { 360, 80 }, true);

    for (auto it = m_FiredInputPins.begin(); it != m_FiredInputPins.end();)
    {
        std::chrono::duration<double> s_ElapsedSeconds = s_Now - it->second.m_FireTime;

        if (s_ElapsedSeconds.count() > 1.f)
        {
            it = m_FiredInputPins.erase(it);
            continue;
        }

        ImVec4 s_Color { 0.f, 1.f, 0.f, 1.f }; // Green

        if (s_ElapsedSeconds.count() > 0.2f)
            s_Color = { 1.f, 1.f, 0.f, 1.f }; // Yellow

        if (s_ElapsedSeconds.count() > 0.4f)
            s_Color = { 1.f, 0.6f, 0.f, 1.f }; // Orange

        if (s_ElapsedSeconds.count() > 0.6f)
            s_Color = { 1.f, 0.6f, 0.f, 1.f }; // Red

        if (s_ElapsedSeconds.count() > 0.8f)
            s_Color = { 0.3f, 0.f, 0.f, 1.f }; // Dark Red

        if (s_ElapsedSeconds.count() > 0.9f)
            s_Color = { 0.1f, 0.f, 0.f, 1.f }; // Very Dark Red

        ImGui::TextColored(s_Color, fmt::format("{:x}", it->first).c_str());

        const float s_LastStartX = ImGui::GetItemRectMin().x;
        const float s_LastEndX = ImGui::GetItemRectMax().x;
        const float s_LastWidth = s_LastEndX - s_LastStartX;

        const float s_NextX = s_LastEndX + ImGui::GetStyle().ItemSpacing.x + s_LastWidth;

        if (s_NextX < ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x)
            ImGui::SameLine();

        ++it;
    }

    ImGui::EndChild();

    ImGui::Text(ICON_MD_LOGOUT " Output Pins");

    ImGui::BeginChild("output pins", { 360.f, 0.f }, true);

    for (auto it = m_FiredOutputPins.begin(); it != m_FiredOutputPins.end();)
    {
        std::chrono::duration<double> s_ElapsedSeconds = s_Now - it->second.m_FireTime;

        if (s_ElapsedSeconds.count() > 1.f)
        {
            it = m_FiredOutputPins.erase(it);
            continue;
        }

        ImVec4 s_Color { 0.f, 1.f, 0.f, 1.f }; // Green
        ImVec4 s_TargetColor = { 1.f, 1.f, 0.f, 1.f }; // Yellow
        float t = s_ElapsedSeconds.count() / 0.1f;

        if (s_ElapsedSeconds.count() >= 0.1f)
        {
            s_Color = s_TargetColor;
            s_TargetColor = { 1.f, 0.6f, 0.f, 1.f }; // Orange
            t = (s_ElapsedSeconds.count() - 0.1f) / 0.2f;
        }

        if (s_ElapsedSeconds.count() >= 0.3f)
        {
            s_Color = s_TargetColor;
            s_TargetColor = { 1.f, 0.6f, 0.f, 1.f }; // Red
            t = (s_ElapsedSeconds.count() - 0.3f) / 0.2f;
        }

        if (s_ElapsedSeconds.count() >= 0.5f)
        {
            s_Color = s_TargetColor;
            s_TargetColor = { 0.3f, 0.f, 0.f, 1.f }; // Dark Red
            t = (s_ElapsedSeconds.count() - 0.5f) / 0.2f;
        }

        if (s_ElapsedSeconds.count() >= 0.7f)
        {
            s_Color = s_TargetColor;
            s_TargetColor = { 0.1f, 0.f, 0.f, 1.f }; // Very Dark Red
            t = (s_ElapsedSeconds.count() - 0.7f) / 0.2f;
        }

        if (s_ElapsedSeconds.count() >= 0.9f)
        {
            s_Color = s_TargetColor;
            s_TargetColor = { 0.f, 0.f, 0.f, 0.f }; // Transparent
            t = (s_ElapsedSeconds.count() - 0.9f) / 0.1f;
        }

        ImVec4 s_LerpedColor = {
            s_Color.x + (s_TargetColor.x - s_Color.x) * t,
            s_Color.y + (s_TargetColor.y - s_Color.y) * t,
            s_Color.z + (s_TargetColor.z - s_Color.z) * t,
            s_Color.w + (s_TargetColor.w - s_Color.w) * t,
        };

        ZString s_PinName;

        if (SDK()->GetPinName(it->first, s_PinName))
            ImGui::TextColored(s_LerpedColor, s_PinName.c_str());
        else
            ImGui::TextColored(s_LerpedColor, fmt::format("{:x}", it->first).c_str());

        const float s_LastStartX = ImGui::GetItemRectMin().x;
        const float s_LastEndX = ImGui::GetItemRectMax().x;
        const float s_LastWidth = s_LastEndX - s_LastStartX;

        const float s_NextX = s_LastEndX + ImGui::GetStyle().ItemSpacing.x + s_LastWidth;

        if (s_NextX < ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x)
            ImGui::SameLine();

        ++it;
    }

    ImGui::EndChild();

    ImGui::End();
}
