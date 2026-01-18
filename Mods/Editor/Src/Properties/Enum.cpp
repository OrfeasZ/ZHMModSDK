#include "Editor.h"
#include "imgui.h"

#include <Glacier/IEnumType.h>

bool Editor::EnumProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    const auto s_PropertyInfo = p_Property->m_pType->getPropertyInfo();
    auto s_Type = reinterpret_cast<IEnumType*>(s_PropertyInfo->m_pType->typeInfo());
    int32_t s_Value = 0;

    switch (s_Type->m_nTypeSize) {
        case 1:
            s_Value = *static_cast<int8_t*>(p_Data);
            break;
        case 2:
            s_Value = *static_cast<int16_t*>(p_Data);
            break;
        case 4:
            s_Value = *static_cast<int32_t*>(p_Data);
            break;
    }

    std::string s_CurrentValue;

    for (auto& s_EnumValue : s_Type->m_entries) {
        if (s_EnumValue.m_nValue == s_Value) {
            s_CurrentValue = s_EnumValue.m_pName;
            break;
        }
    }

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::BeginCombo(p_Id.c_str(), s_CurrentValue.c_str())) {
        for (auto& s_EnumValue : s_Type->m_entries) {
            if (ImGui::Selectable(s_EnumValue.m_pName, s_EnumValue.m_nValue == s_Value)) {
                switch (s_Type->m_nTypeSize) {
                    case 1:
                        *static_cast<int8_t*>(p_Data) = static_cast<int8_t>(s_EnumValue.m_nValue);
                        break;
                    case 2:
                        *static_cast<int16_t*>(p_Data) = static_cast<int16_t>(s_EnumValue.m_nValue);
                        break;
                    case 4:
                        *static_cast<int32_t*>(p_Data) = static_cast<int32_t>(s_EnumValue.m_nValue);
                        break;
                    default:
                        break;
                }

                s_Value = s_EnumValue.m_nValue;

                s_IsChanged = true;
            }
        }

        ImGui::EndCombo();
    }

    if (ImGuiCopyWidget(("Enum_" + p_Id).c_str())) {
        CopyToClipboard(s_CurrentValue);
    }

    return s_IsChanged;
}
