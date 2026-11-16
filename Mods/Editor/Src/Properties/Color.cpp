#include "Editor.h"
#include <Glacier/SColorRGB.h>
#include <Glacier/SColorRGBA.h>

static constexpr const char* c_ColorTags[] = { "R", "G", "B", "A" };

bool Editor::DrawColorSwatch(const std::string& p_Id, float* p_Color, const bool p_HasAlpha) {
    bool s_IsChanged = false;
    const float s_Size = ScaledSize(20.f);
    const ImVec4 s_Color(p_Color[0], p_Color[1], p_Color[2], p_HasAlpha ? p_Color[3] : 1.f);

    if (ImGui::ColorButton(
        (p_Id + "_Swatch").c_str(),
        s_Color,
        p_HasAlpha ? ImGuiColorEditFlags_AlphaPreviewHalf : ImGuiColorEditFlags_NoAlpha,
        ImVec2(s_Size, s_Size)
    )) {
        ImGui::OpenPopup((p_Id + "_Picker").c_str());
    }

    if (ImGui::BeginPopup((p_Id + "_Picker").c_str())) {
        const ImGuiColorEditFlags s_Flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR;

        s_IsChanged = p_HasAlpha
            ? ImGui::ColorPicker4((p_Id + "_Picker4").c_str(), p_Color, s_Flags | ImGuiColorEditFlags_AlphaBar)
            : ImGui::ColorPicker3((p_Id + "_Picker3").c_str(), p_Color, s_Flags);

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    return s_IsChanged;
}

bool Editor::ColorComponentsProperty(const std::string& p_Id, float* p_Color, const bool p_HasAlpha) {
    const float s_StartX = ImGui::GetCursorPosX();
    const float s_Width = ImGui::CalcItemWidth();

    bool s_IsChanged = DrawColorSwatch(p_Id, p_Color, p_HasAlpha);

    ImGui::SetNextItemWidth(s_Width - (ImGui::GetCursorPosX() - s_StartX));
    s_IsChanged |= DragFloatComponents(p_Id, p_Color, p_HasAlpha ? 4 : 3, c_ColorTags, 0.01f);

    return s_IsChanged;
}

bool Editor::SColorRGBProperty(const std::string& p_Id, void* p_Data) {
    return ColorComponentsProperty(p_Id, &static_cast<SColorRGB*>(p_Data)->r, false);
}

bool Editor::SColorRGBAProperty(const std::string& p_Id, void* p_Data) {
    return ColorComponentsProperty(p_Id, &static_cast<SColorRGBA*>(p_Data)->r, true);
}
