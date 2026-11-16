#include "Widgets.h"

#include <algorithm>
#include <format>

#include "IPluginInterface.h"

static constexpr ImVec4 c_LinkColor = ImVec4(0.2f, 0.6f, 1.f, 1.f);

static constexpr float c_TagFontSize = 10.f;
static constexpr float c_GhostIconFontSize = 15.f;

static constexpr float c_EditorScale = 12.f / 14.f;

static float g_UserScale = 1.f;

void SetEditorScale(const float p_Scale) {
    g_UserScale = p_Scale;
}

float ScaledSize(const float p_CanvasPixels) {
    return p_CanvasPixels * c_EditorScale * g_UserScale * ImGui::GetIO().DisplaySize.y / 900.f;
}

float FontSize(const float p_CanvasPixels) {
    return p_CanvasPixels * 2.f * c_EditorScale * g_UserScale;
}

void PushFrameHeight(const float p_CanvasPixels) {
    ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, (ScaledSize(p_CanvasPixels) - ImGui::GetFontSize()) * 0.5f);
}

void PushFrameHeight(const float p_CanvasPixels, const float p_PaddingX) {
    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(ScaledSize(p_PaddingX), (ScaledSize(p_CanvasPixels) - ImGui::GetFontSize()) * 0.5f)
    );
}

void PushSelectedButtonStyle() {
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    ImGui::PushFont(SDK()->GetImGuiBoldFont(), 0.f);
}

void PopSelectedButtonStyle() {
    ImGui::PopFont();
    ImGui::PopStyleColor();
}

bool GhostButton(const char* p_Icon, const std::string& p_Id, const char* p_Tooltip, const bool p_IsOn) {
    const float s_Size = ScaledSize(20.f);

    const bool s_IsClicked = ImGui::InvisibleButton(p_Id.c_str(), ImVec2(s_Size, s_Size));
    const bool s_IsHovered = ImGui::IsItemHovered();
    const bool s_IsActive = ImGui::IsItemActive();

    const ImVec2 s_Min = ImGui::GetItemRectMin();
    const ImVec2 s_Max = ImGui::GetItemRectMax();
    ImDrawList* s_DrawList = ImGui::GetWindowDrawList();

    if (p_IsOn || s_IsActive) {
        s_DrawList->AddRectFilled(s_Min, s_Max, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
    }
    else if (s_IsHovered) {
        s_DrawList->AddRectFilled(s_Min, s_Max, ImGui::GetColorU32(ImGuiCol_Button));
    }

    const ImU32 s_IconColor = ImGui::GetColorU32(
        p_IsOn || s_IsHovered ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : c_MutedTextColor
    );

    ImGui::PushFont(nullptr, FontSize(c_GhostIconFontSize));

    DrawCenteredText(s_DrawList, s_Min, s_Max, s_IconColor, p_Icon);
    ImGui::PopFont();

    if (s_IsHovered && p_Tooltip) {
        ImGui::SetTooltip("%s", p_Tooltip);
    }

    return s_IsClicked;
}

bool IconButton(const char* p_Icon, const char* p_Tooltip, const bool p_IsOn) {
    const float s_Size = ScaledSize(20.f);

    if (p_IsOn) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
    ImGui::PushFont(nullptr, FontSize(c_GhostIconFontSize));

    const bool s_IsClicked = ImGui::Button(p_Icon, ImVec2(s_Size, s_Size));

    ImGui::PopFont();
    ImGui::PopStyleVar();

    if (p_IsOn) {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", p_Tooltip);
    }

    return s_IsClicked;
}

bool BeginPanelWindow(const char* p_Name, const ImGuiWindowFlags p_Flags) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, ImGui::GetStyle().WindowPadding.y));
    const bool s_IsOpen = ImGui::Begin(p_Name, nullptr, p_Flags);
    ImGui::PopStyleVar();

    return s_IsOpen;
}

void BeginPanelPadding(const char* p_Id) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaledSize(6.f), 0.f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));

    ImGui::BeginChild(
        p_Id,
        ImVec2(0.f, 0.f),
        ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding,
        ImGuiWindowFlags_NoScrollbar
    );

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void EndPanelPadding() {
    ImGui::EndChild();
}

void DrawPanelHeader(const char* p_Label, const char* p_Aside, const float p_Width) {
    ImDrawList* s_DrawList = ImGui::GetWindowDrawList();
    const float s_Height = ScaledSize(22.f);
    const float s_Padding = ScaledSize(6.f);
    const ImVec2 s_Min = ImGui::GetCursorScreenPos();
    const float s_TextY = s_Min.y + (s_Height - ImGui::GetFontSize()) * 0.5f;

    s_DrawList->AddRectFilled(s_Min, ImVec2(s_Min.x + p_Width, s_Min.y + s_Height), ImGui::GetColorU32(c_SectionHeaderColor));

    ImGui::PushFont(SDK()->GetImGuiBoldFont(), 0.f);
    s_DrawList->AddText(ImVec2(s_Min.x + s_Padding, s_TextY), ImGui::GetColorU32(ImGuiCol_Text), p_Label);
    ImGui::PopFont();

    if (p_Aside) {
        s_DrawList->AddText(
            ImVec2(s_Min.x + p_Width - s_Padding - ImGui::CalcTextSize(p_Aside).x, s_TextY),
            ImGui::GetColorU32(c_MutedTextColor),
            p_Aside
        );
    }

    ImGui::Dummy(ImVec2(p_Width, s_Height));
}

void DrawCenteredText(
    ImDrawList* p_DrawList, const ImVec2& p_Min, const ImVec2& p_Max, const ImU32 p_Color, const char* p_Text
) {
    const ImVec2 s_TextSize = ImGui::CalcTextSize(p_Text);

    p_DrawList->AddText(
        ImVec2(
            p_Min.x + (p_Max.x - p_Min.x - s_TextSize.x) * 0.5f,
            p_Min.y + (p_Max.y - p_Min.y - s_TextSize.y) * 0.5f
        ),
        p_Color,
        p_Text
    );
}

void DrawToolbarSeparator(const float p_Height) {
    const ImVec2 s_Pos = ImGui::GetCursorScreenPos();
    const float s_Height = ScaledSize(p_Height);
    const float s_Y = s_Pos.y + (ImGui::GetFrameHeight() - s_Height) * 0.5f;

    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(s_Pos.x, s_Y),
        ImVec2(s_Pos.x, s_Y + s_Height),
        ImGui::GetColorU32(c_ToolbarSeparatorColor),
        ScaledSize(1.f)
    );

    ImGui::Dummy(ImVec2(ScaledSize(1.f), ImGui::GetFrameHeight()));
}

float CalcValueTagWidth(const char* p_Text) {
    ImGui::PushFont(SDK()->GetImGuiBoldFont(), FontSize(c_TagFontSize));
    const float s_TextWidth = ImGui::CalcTextSize(p_Text).x;
    ImGui::PopFont();

    return std::max(ScaledSize(14.f), s_TextWidth + ScaledSize(6.f));
}

float DrawValueTag(const char* p_Text, const ImU32 p_Color) {
    const float s_Height = ImGui::GetFrameHeight();

    ImGui::PushFont(SDK()->GetImGuiBoldFont(), FontSize(c_TagFontSize));

    const ImVec2 s_Size(CalcValueTagWidth(p_Text), s_Height);
    const ImVec2 s_Min = ImGui::GetCursorScreenPos();
    const ImVec2 s_Max(s_Min.x + s_Size.x, s_Min.y + s_Size.y);

    ImDrawList* s_DrawList = ImGui::GetWindowDrawList();

    s_DrawList->AddRectFilled(s_Min, s_Max, ImGui::GetColorU32(ImGui::ColorConvertU32ToFloat4(p_Color)));
    DrawCenteredText(s_DrawList, s_Min, s_Max, ImGui::GetColorU32(ImGuiCol_Text), p_Text);

    ImGui::PopFont();

    ImGui::Dummy(s_Size);
    ImGui::SameLine(0.f, 0.f);

    return s_Size.x;
}

void DrawEditingOutline() {
    if (!ImGui::IsItemActive()) {
        return;
    }

    const float s_Thickness = ScaledSize(2.f);
    const float s_Inset = s_Thickness * 0.5f;
    const ImVec2 s_Min = ImGui::GetItemRectMin();
    const ImVec2 s_Max = ImGui::GetItemRectMax();

    ImGui::GetWindowDrawList()->AddRect(
        ImVec2(s_Min.x + s_Inset, s_Min.y + s_Inset),
        ImVec2(s_Max.x - s_Inset, s_Max.y - s_Inset),
        ImGui::GetColorU32(c_AccentColor),
        0.f,
        0,
        s_Thickness
    );
}

bool DrawLinkText(const std::string& p_Text) {
    ImGui::PushStyleColor(ImGuiCol_Text, c_LinkColor);
    ImGui::TextUnformatted(p_Text.c_str());
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered()) {
        const ImVec2 s_Min = ImGui::GetItemRectMin();
        const ImVec2 s_Max = ImGui::GetItemRectMax();

        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(s_Min.x, s_Max.y),
            ImVec2(s_Max.x, s_Max.y),
            ImGui::GetColorU32(c_LinkColor)
        );

        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    return ImGui::IsItemClicked();
}

void DrawReferenceWell(const char* p_Icon, const float p_Width, const std::function<void()>& p_DrawContent) {
    const ImVec2 s_Min = ImGui::GetCursorScreenPos();
    const ImVec2 s_Size(std::max(1.f, p_Width), ImGui::GetFrameHeight());
    const ImVec2 s_Max(s_Min.x + s_Size.x, s_Min.y + s_Size.y);

    ImGui::GetWindowDrawList()->AddRectFilled(s_Min, s_Max, ImGui::GetColorU32(ImGuiCol_FrameBg));
    ImGui::PushClipRect(s_Min, s_Max, true);

    ImGui::SetCursorScreenPos(ImVec2(s_Min.x + ScaledSize(3.f), s_Min.y));
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(c_MutedTextColor, "%s", p_Icon);
    ImGui::SameLine(0.f, ScaledSize(4.f));

    p_DrawContent();

    ImGui::PopClipRect();

    ImGui::SetCursorScreenPos(s_Min);
    ImGui::Dummy(s_Size);
}

bool TaggedDragScalar(
    const std::string& p_Id,
    const char* p_Tag,
    const ImU32 p_TagColor,
    const ImGuiDataType p_DataType,
    void* p_Data,
    const float p_Width,
    const float p_Speed,
    const char* p_Format,
    const ImGuiSliderFlags p_Flags
) {
    const float s_TagWidth = DrawValueTag(p_Tag, p_TagColor);

    ImGui::SetNextItemWidth(std::max(1.f, p_Width - s_TagWidth));

    const bool s_IsChanged = ImGui::DragScalar(
        p_Id.c_str(), p_DataType, p_Data, p_Speed, nullptr, nullptr, p_Format, p_Flags
    );

    DrawEditingOutline();

    return s_IsChanged;
}

bool DragFloatComponents(
    const std::string& p_Id, float* p_Values, const int p_Count, const char* const* p_Tags, const float p_Speed
) {
    const float s_Spacing = ScaledSize(3.f);
    const float s_FieldWidth = (ImGui::CalcItemWidth() - s_Spacing * (p_Count - 1)) / p_Count;

    bool s_IsChanged = false;

    for (int i = 0; i < p_Count; ++i) {
        if (i > 0) {
            ImGui::SameLine(0.f, s_Spacing);
        }

        const ImU32 s_TagColor = i < static_cast<int>(std::size(c_AxisTagColors)) ? c_AxisTagColors[i] : c_ValueTagColor;

        s_IsChanged |= TaggedDragScalar(
            std::format("{}_{}", p_Id, i),
            p_Tags[i],
            s_TagColor,
            ImGuiDataType_Float,
            &p_Values[i],
            s_FieldWidth,
            p_Speed
        );
    }

    return s_IsChanged;
}

bool DrawPropertySectionHeader(const char* p_Label, const std::string& p_Aside, const bool p_DefaultOpen) {
    ImGui::PushStyleColor(ImGuiCol_Header, c_SectionHeaderColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, c_SectionHeaderHoverColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, c_SectionHeaderHoverColor);
    PushFrameHeight(24.f, 6.f);
    ImGui::PushFont(SDK()->GetImGuiBoldFont(), 0.f);

    ImGui::SetNextItemAllowOverlap();

    const bool s_IsOpen = ImGui::CollapsingHeader(p_Label, p_DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);

    ImGui::PopFont();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    const ImVec2 s_Min = ImGui::GetItemRectMin();
    const ImVec2 s_Max = ImGui::GetItemRectMax();
    ImDrawList* s_DrawList = ImGui::GetWindowDrawList();

    s_DrawList->AddLine(s_Min, ImVec2(s_Max.x, s_Min.y), ImGui::GetColorU32(c_SectionBorderColor), ScaledSize(1.f));

    const ImVec2 s_AsideSize = ImGui::CalcTextSize(p_Aside.c_str());

    s_DrawList->AddText(
        ImVec2(s_Max.x - ScaledSize(6.f) - s_AsideSize.x, s_Min.y + (s_Max.y - s_Min.y - s_AsideSize.y) * 0.5f),
        ImGui::GetColorU32(c_MutedTextColor),
        p_Aside.c_str()
    );

    return s_IsOpen;
}
