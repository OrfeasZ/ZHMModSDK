#include "Editor.h"
#include "imgui.h"

#include <Glacier/IEnumType.h>

void Editor::EnumProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
    auto s_Value = *static_cast<int32*>(p_Data);
    const auto s_PropertyInfo = p_Property->m_pType->getPropertyInfo();
    auto s_Type = reinterpret_cast<IEnumType*>(s_PropertyInfo->m_pType->typeInfo());

    std::string s_CurrentValue;

    for (auto& s_EnumValue : s_Type->m_entries) {
        if (s_EnumValue.m_nValue == s_Value) {
            s_CurrentValue = s_EnumValue.m_pName;
            break;
        }
    }

    if (ImGui::BeginCombo(p_Id.c_str(), s_CurrentValue.c_str())) {
        for (auto& s_EnumValue : s_Type->m_entries) {
            if (ImGui::Selectable(s_EnumValue.m_pName, s_EnumValue.m_nValue == s_Value)) {
                auto s_EnumValueObj = ZObjectRef::From<int32>(s_EnumValue.m_nValue);
                s_EnumValueObj.UNSAFE_SetType(s_PropertyInfo->m_pType);

                OnSetPropertyValue(
                    p_Entity,
                    p_Property->m_nPropertyId,
                    s_EnumValueObj,
                    std::nullopt
                );
            }
        }

        ImGui::EndCombo();
    }
}
