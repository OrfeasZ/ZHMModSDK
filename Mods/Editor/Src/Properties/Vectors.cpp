#include "Editor.h"
#include "imgui.h"

void Editor::SVector2Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
    auto s_Value = *static_cast<SVector2*>(p_Data);
    if (ImGui::DragFloat2(p_Id.c_str(), &s_Value.x, 0.1f)) {
        OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
    }
}

void Editor::SVector3Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
    auto s_Value = *static_cast<SVector3*>(p_Data);
    if (ImGui::DragFloat3(p_Id.c_str(), &s_Value.x, 0.1f)) {
        OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
    }
}

void Editor::SVector4Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
    auto s_Value = *static_cast<SVector4*>(p_Data);
    if (ImGui::DragFloat4(p_Id.c_str(), &s_Value.x, 0.1f)) {
        OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
    }
}
