#include "Editor.h"
#include "imgui.h"

bool Editor::SVector2Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<SVector2*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragFloat2(p_Id.c_str(), &s_Value->x, 0.1f)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("SVector2_" + p_Id).c_str())) {
        CopyToClipboard(
            fmt::format(
                "{{\"x\":{},\"y\":{}}}",
                s_Value->x,
                s_Value->y
            )
        );
    }

    return s_IsChanged;
}

bool Editor::SVector3Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<SVector3*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragFloat3(p_Id.c_str(), &s_Value->x, 0.1f)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("SVector3_" + p_Id).c_str())) {
        CopyToClipboard(
            fmt::format(
                "{{\"x\":{},\"y\":{},\"z\":{}}}",
                s_Value->x,
                s_Value->y,
                s_Value->z
            )
        );
    }

    return s_IsChanged;
}

bool Editor::SVector4Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<SVector4*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragFloat4(p_Id.c_str(), &s_Value->x, 0.1f)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("SVector3_" + p_Id).c_str())) {
        CopyToClipboard(
            fmt::format(
                "{{\"x\":{},\"y\":{},\"z\":{},\"w\":{}}}",
                s_Value->x,
                s_Value->y,
                s_Value->z,
                s_Value->w
            )
        );
    }

    return s_IsChanged;
}
