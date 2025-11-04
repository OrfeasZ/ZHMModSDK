#pragma once

#include "Editor.h"

#include <ResourceLib_HM3.h>

void Editor::TArrayProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data,
    const std::string& s_PropertyName, const std::string& s_TypeName, const STypeID* p_TypeID
) {
    auto* s_Array = reinterpret_cast<TArray<uint8_t>*>(p_Data);

    if (!s_Array) {
        ImGui::TextDisabled("null");

        return;
    }

    const IArrayType* s_ArrayType = static_cast<IArrayType*>(p_TypeID->typeInfo());
    const STypeID* s_ElementTypeID = s_ArrayType->m_pArrayElementType;
    const std::string s_ElementTypeName = s_ElementTypeID->typeInfo()->m_pTypeName;
    const size_t s_ElementSize = s_ElementTypeID->typeInfo()->m_nTypeSize;
    const size_t s_ElementAlign = s_ElementTypeID->typeInfo()->m_nTypeAlignment;

    size_t s_ArraySize;

    if (s_Array->fitsInline() && s_Array->hasInlineFlag()) {
        s_ArraySize = s_Array->m_nInlineCount;
    }
    else {
        s_ArraySize = (reinterpret_cast<uintptr_t>(s_Array->m_pEnd) -
            reinterpret_cast<uintptr_t>(s_Array->m_pBegin)) / s_ElementSize;
    }

    uint8_t* s_BasePtr;

    if (s_Array->fitsInline() && s_Array->hasInlineFlag()) {
        s_BasePtr = reinterpret_cast<uint8_t*>(&s_Array->m_pBegin);
    }
    else {
        s_BasePtr = reinterpret_cast<uint8_t*>(s_Array->m_pBegin);
    }

    ImGui::PushFont(SDK()->GetImGuiBoldFont());
    ImGui::AlignTextToFramePadding();

    const bool s_IsTreeNodeOpen = ImGui::TreeNodeEx(
        fmt::format("{} ({} elements)##{}", s_PropertyName, s_ArraySize, p_Id).c_str()
    );

    ImGui::PopFont();

    if (s_IsTreeNodeOpen) {
        for (size_t i = 0; i < s_ArraySize; ++i) {
            uint8_t* s_ElementPtr = s_BasePtr + i * s_ElementSize;

            ImGui::PushID(static_cast<int>(i));

            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("[%zu]", i);

            ImGui::SameLine();

            const std::string s_ElementId = std::format("{}[{}]", p_Id, i);

            DrawEntityPropertyValue(
                s_ElementId, "",
                s_ElementTypeName,
                s_ElementTypeID,
                p_Entity,
                p_Property,
                s_ElementPtr
            );

            ImGui::PopID();
        }

        ImGui::TreePop();
    }
}
