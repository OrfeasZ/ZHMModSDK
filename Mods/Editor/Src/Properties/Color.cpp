#include "Editor.h"
#include <Glacier/SColorRGB.h>
#include <Glacier/SColorRGBA.h>

bool Editor::SColorRGBProperty(
    const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data
) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<SColorRGB*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::ColorEdit3(p_Id.c_str(), &s_Value->r)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("SColorRGB_" + p_Id).c_str())) {
        const uint32_t s_Color = s_Value->GetAsUInt32();

        const uint8_t r = s_Color & 0xFF;
        const uint8_t g = (s_Color >> 8) & 0xFF;
        const uint8_t b = (s_Color >> 16) & 0xFF;

        CopyToClipboard(
            fmt::format("#{0:02X}{1:02X}{2:02X}", r, g, b)
        );
    }

    return s_IsChanged;
}

bool Editor::SColorRGBAProperty(
    const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data
) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<SColorRGBA*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::ColorEdit4(p_Id.c_str(), &s_Value->r)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("SColorRGBA_" + p_Id).c_str())) {
        const uint32_t s_Color = s_Value->GetAsUInt32();

        const uint8_t r = s_Color & 0xFF;
        const uint8_t g = (s_Color >> 8) & 0xFF;
        const uint8_t b = (s_Color >> 16) & 0xFF;
        const uint8_t a = (s_Color >> 24) & 0xFF;

        CopyToClipboard(
            fmt::format("#{0:02X}{1:02X}{2:02X}{3:02X}", r, g, b, a)
        );
    }

    return s_IsChanged;
}
