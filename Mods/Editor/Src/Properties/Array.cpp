#pragma once

#include "Editor.h"

#include <ResourceLib_HM3.h>

bool Editor::ArrayProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data,
    const std::string& s_PropertyName, const STypeID* p_TypeID
) {
    bool s_IsChanged = false;
    const IArrayType* s_ArrayType = static_cast<IArrayType*>(p_TypeID->typeInfo());
    size_t s_ArraySize = s_ArrayType->m_pArrayFunctions->size(p_Data);

    ImGui::PushFont(SDK()->GetImGuiBoldFont());
    ImGui::AlignTextToFramePadding();

    const bool s_IsTreeNodeOpen = ImGui::TreeNodeEx(
        fmt::format("{} ({} {})##{}",
            s_PropertyName,
            s_ArraySize,
            s_ArraySize == 1 ? "element" : "elements",
            p_Id
        ).c_str()
    );

    ImGui::PopFont();

    if (s_IsTreeNodeOpen) {
        s_IsChanged = DrawArrayElements(
            p_Id,
            p_Entity,
            p_Property,
            p_Data,
            s_ArrayType
        );

        ImGui::TreePop();
    }

    return s_IsChanged;
}

bool Editor::DrawArrayElements(
    const std::string& p_Id,
    ZEntityRef p_Entity,
    ZEntityProperty* p_Property,
    void* p_Data,
    const IArrayType* p_ArrayType
) {
    bool s_IsAnyElementChanged = false;
    const STypeID* s_ElementTypeID = p_ArrayType->m_pArrayElementType;
    const std::string s_ElementTypeName = s_ElementTypeID->typeInfo()->m_pTypeName;

    void* s_Iterator = p_ArrayType->m_pArrayFunctions->begin(p_Data);
    void* s_End = p_ArrayType->m_pArrayFunctions->end(p_Data);

    int s_Index = 0;

    for (; s_Iterator != s_End; s_Iterator = p_ArrayType->m_pArrayFunctions->next(p_Data, s_Iterator), ++s_Index) {
        ImGui::PushID(static_cast<int>(s_Index));

        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("[%zu]", s_Index);

        ImGui::SameLine();

        const std::string s_ElementId = std::format("{}[{}]", p_Id, s_Index);

        s_IsAnyElementChanged |= DrawEntityPropertyValue(
            s_ElementId,
            "",
            s_ElementTypeName,
            s_ElementTypeID,
            p_Entity,
            p_Property,
            s_Iterator
        );

        ImGui::PopID();
    }

    return s_IsAnyElementChanged;
}