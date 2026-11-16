#pragma once

#include <functional>
#include <string>

#include "imgui.h"

inline constexpr ImVec4 c_MutedTextColor = ImVec4(1.f, 1.f, 1.f, 0.5f);
inline constexpr ImVec4 c_SecondaryTextColor = ImVec4(0.6f, 0.6f, 0.6f, 1.f);
inline constexpr ImVec4 c_HighlightTextColor = ImVec4(0.996f, 0.941f, 0.541f, 1.f);
inline constexpr ImVec4 c_ErrorTextColor = ImVec4(1.f, 0.27f, 0.27f, 1.f);
inline constexpr ImVec4 c_AccentColor = ImVec4(0.26f, 0.59f, 0.98f, 1.f);
inline constexpr ImVec4 c_AlternateRowColor = ImVec4(1.f, 1.f, 1.f, 0.059f);
inline constexpr ImVec4 c_ListRowHoverColor = ImVec4(1.f, 1.f, 1.f, 0.13f);
inline constexpr ImVec4 c_SectionHeaderColor = ImVec4(0.188f, 0.188f, 0.2f, 1.f);
inline constexpr ImVec4 c_SectionHeaderHoverColor = ImVec4(0.227f, 0.227f, 0.243f, 1.f);
inline constexpr ImVec4 c_SectionBorderColor = ImVec4(0.31f, 0.31f, 0.349f, 1.f);
inline constexpr ImVec4 c_RowBorderColor = ImVec4(0.231f, 0.231f, 0.251f, 1.f);
inline constexpr ImVec4 c_ToolbarSeparatorColor = ImVec4(0.431f, 0.431f, 0.502f, 0.502f);

inline constexpr ImU32 c_ValueTagColor = IM_COL32(0x4f, 0x4f, 0x59, 0xff);

inline constexpr const char* c_VectorTags[] = { "X", "Y", "Z", "W" };

inline constexpr ImU32 c_AxisTagColors[] = {
    IM_COL32(0xb3, 0x26, 0x1e, 0xff),
    IM_COL32(0x2e, 0x7d, 0x32, 0xff),
    IM_COL32(0x1f, 0x5f, 0xbf, 0xff),
};

float ScaledSize(float p_CanvasPixels);

float FontSize(float p_CanvasPixels);

void PushFrameHeight(float p_CanvasPixels);

void PushFrameHeight(float p_CanvasPixels, float p_PaddingX);

void PushSelectedButtonStyle();
void PopSelectedButtonStyle();

bool GhostButton(const char* p_Icon, const std::string& p_Id, const char* p_Tooltip, bool p_IsOn = false);
bool IconButton(const char* p_Icon, const char* p_Tooltip, bool p_IsOn = false);

void DrawToolbarSeparator(float p_Height = 14.f);
void DrawPanelHeader(const char* p_Label, const char* p_Aside, float p_Width);

bool DrawPropertySectionHeader(const char* p_Label, const std::string& p_Aside, bool p_DefaultOpen = true);
void DrawCenteredText(ImDrawList* p_DrawList, const ImVec2& p_Min, const ImVec2& p_Max, ImU32 p_Color, const char* p_Text);

// Panel windows have no horizontal padding. Use BeginPanelPadding for other content.
bool BeginPanelWindow(const char* p_Name, ImGuiWindowFlags p_Flags);
void BeginPanelPadding(const char* p_Id);
void EndPanelPadding();

float CalcValueTagWidth(const char* p_Text);
float DrawValueTag(const char* p_Text, ImU32 p_Color);
void DrawEditingOutline();
bool DrawLinkText(const std::string& p_Text);
void DrawReferenceWell(const char* p_Icon, float p_Width, const std::function<void()>& p_DrawContent);

bool TaggedDragScalar(
    const std::string& p_Id,
    const char* p_Tag,
    ImU32 p_TagColor,
    ImGuiDataType p_DataType,
    void* p_Data,
    float p_Width,
    float p_Speed = 1.f,
    const char* p_Format = nullptr,
    ImGuiSliderFlags p_Flags = 0
);
bool DragFloatComponents(const std::string& p_Id, float* p_Values, int p_Count, const char* const* p_Tags, float p_Speed);
