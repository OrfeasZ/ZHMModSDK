#pragma once

#include "Editor.h"

#include <ResourceLib_HM3.h>

void Editor::TArrayProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data,
    const std::string& s_PropertyName, const STypeID* p_TypeID
) {
    auto* s_Array = reinterpret_cast<TArray<uint8_t>*>(p_Data);

    if (!s_Array) {
        ImGui::TextDisabled("null");

        return;
    }

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
        const STypeID* s_ElementTypeID = s_ArrayType->m_pArrayElementType;
        const std::string s_ElementTypeName = s_ElementTypeID->typeInfo()->m_pTypeName;

        void* s_Iterator = s_ArrayType->m_pArrayFunctions->begin(p_Data);
        void* s_End = s_ArrayType->m_pArrayFunctions->end(p_Data);

        int index = 0;
        for (; s_Iterator != s_End; s_Iterator = s_ArrayType->m_pArrayFunctions->next(p_Data, s_Iterator), ++index) {
            ImGui::PushID(static_cast<int>(index));

            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("[%zu]", index);

            ImGui::SameLine();

            const std::string s_ElementId = std::format("{}[{}]", p_Id, index);

            DrawEntityPropertyValue(
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

        ImGui::TreePop();
    }
}
    }

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
        for (size_t i = 0; i < s_ArraySize; ++i) {
            uint8_t* s_ElementPtr = s_BasePtr + i * s_ElementSize;

            ImGui::PushID(static_cast<int>(i));

            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("[%zu]", i);

            ImGui::SameLine();

            const std::string s_ElementId = std::format("{}[{}]", p_Id, i);

            DrawEntityPropertyValue(
                s_ElementId,
                "",
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
