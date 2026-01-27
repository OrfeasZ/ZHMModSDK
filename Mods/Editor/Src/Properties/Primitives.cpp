#include "Editor.h"
#include "imgui.h"

bool Editor::StringProperty(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto* s_RealData = static_cast<ZString*>(p_Data);

    static char s_StringBuffer[65536] = {};
    const auto s_StringSize = min(s_RealData->size(), sizeof(s_StringBuffer) - 1);

    memcpy(s_StringBuffer, s_RealData->c_str(), s_StringSize);
    s_StringBuffer[s_StringSize] = '\0';

    if (ImGui::InputText(p_Id.c_str(), s_StringBuffer, sizeof(s_StringBuffer))) {
        *s_RealData = ZString(s_StringBuffer);
        s_IsChanged = true;
    }

    return s_IsChanged;
}

bool Editor::BoolProperty(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<bool*>(p_Data);

    if (ImGui::Checkbox(p_Id.c_str(), s_Value)) {
        s_IsChanged = true;
    }

    return s_IsChanged;
}

bool Editor::Uint8Property(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<uint8*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragScalar(p_Id.c_str(), ImGuiDataType_U8, s_Value)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("Uint8_" + p_Id).c_str())) {
        CopyToClipboard(std::to_string(*s_Value));
    }

    return s_IsChanged;
}

bool Editor::Uint16Property(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<uint16*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragScalar(p_Id.c_str(), ImGuiDataType_U16, s_Value)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("Uint16_" + p_Id).c_str())) {
        CopyToClipboard(std::to_string(*s_Value));
    }

    return s_IsChanged;
}

bool Editor::Uint32Property(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<uint32*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragScalar(p_Id.c_str(), ImGuiDataType_U32, s_Value)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("Uint32_" + p_Id).c_str())) {
        CopyToClipboard(std::to_string(*s_Value));
    }

    return s_IsChanged;
}

bool Editor::Uint64Property(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<uint64*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragScalar(p_Id.c_str(), ImGuiDataType_U64, s_Value)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("Uint64_" + p_Id).c_str())) {
        CopyToClipboard(std::to_string(*s_Value));
    }

    return s_IsChanged;
}

bool Editor::Int8Property(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<int8*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragScalar(p_Id.c_str(), ImGuiDataType_S8, s_Value)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("Int8_" + p_Id).c_str())) {
        CopyToClipboard(std::to_string(*s_Value));
    }

    return s_IsChanged;
}

bool Editor::Int16Property(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<int16*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragScalar(p_Id.c_str(), ImGuiDataType_S16, s_Value)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("Int16_" + p_Id).c_str())) {
        CopyToClipboard(std::to_string(*s_Value));
    }

    return s_IsChanged;
}

bool Editor::Int32Property(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<int32*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragScalar(p_Id.c_str(), ImGuiDataType_S32, s_Value)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("Int32_" + p_Id).c_str())) {
        CopyToClipboard(std::to_string(*s_Value));
    }

    return s_IsChanged;
}

bool Editor::Int64Property(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<int64*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragScalar(p_Id.c_str(), ImGuiDataType_S64, s_Value)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("Int64_" + p_Id).c_str())) {
        CopyToClipboard(std::to_string(*s_Value));
    }

    return s_IsChanged;
}

bool Editor::Float32Property(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<float32*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragScalar(p_Id.c_str(), ImGuiDataType_Float, s_Value, 0.1f)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("Float32_" + p_Id).c_str())) {
        CopyToClipboard(std::to_string(*s_Value));
    }

    return s_IsChanged;
}

bool Editor::Float64Property(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<float64*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragScalar(p_Id.c_str(), ImGuiDataType_Double, s_Value, 0.1f)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("Float64_" + p_Id).c_str())) {
        CopyToClipboard(std::to_string(*s_Value));
    }

    return s_IsChanged;
}
