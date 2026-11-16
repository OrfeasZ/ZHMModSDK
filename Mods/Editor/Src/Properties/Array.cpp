#pragma once

#include "Editor.h"

#include <ResourceLib_HM3.h>

bool Editor::ArrayProperty(void* p_Data, const STypeID* p_TypeID) {
    const IArrayType* s_ArrayType = reinterpret_cast<IArrayType*>(p_TypeID->GetTypeInfo());
    const bool s_IsFixedSize = s_ArrayType->containerType.type.IsFixedArray();
    const size_t s_ArraySize = s_ArrayType->containerType.pVTab->getSize(p_Data);
    const float s_EndX = ImGui::GetCursorPosX() + ImGui::CalcItemWidth();

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(
        c_MutedTextColor,
        "%zu %s%s",
        s_ArraySize,
        s_ArraySize == 1 ? "element" : "elements",
        s_IsFixedSize ? " \xC2\xB7 fixed size" : ""
    );

    if (s_IsFixedSize) {
        return false;
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(s_EndX - ScaledSize(20.f));

    if (!GhostButton(ICON_MD_ADD, "##AddElement", "Add element")) {
        return false;
    }

    s_ArrayType->setSize(p_Data, static_cast<uint32_t>(s_ArraySize + 1));

    return true;
}

bool Editor::DrawArrayElements(
    const std::string& p_Id,
    ZEntityRef p_Entity,
    SPropertyData* p_Property,
    void* p_Data,
    const IArrayType* p_ArrayType
) {
    bool s_IsAnyElementChanged = false;
    const STypeID* s_ElementTypeID = p_ArrayType->containerType.elementType;
    const bool s_IsFixedSize = p_ArrayType->containerType.type.IsFixedArray();

    std::vector<void*> s_Elements;

    void* s_Iterator = p_ArrayType->containerType.pVTab->begin(p_Data);
    void* s_End = p_ArrayType->containerType.pVTab->end(p_Data);

    for (; s_Iterator != s_End; s_Iterator = p_ArrayType->containerType.pVTab->next(p_Data, s_Iterator)) {
        s_Elements.push_back(s_Iterator);
    }

    std::optional<size_t> s_RemovedIndex;

    for (size_t i = 0; i < s_Elements.size(); ++i) {
        const auto s_Result = DrawPropertyRow(
            std::format("{}[{}]", p_Id, i),
            std::format("[{}]", i),
            s_ElementTypeID,
            p_Entity,
            p_Property,
            s_Elements[i],
            s_IsFixedSize ? EPropertyRowButton::None : EPropertyRowButton::Remove
        );

        s_IsAnyElementChanged |= s_Result.m_Changed;

        if (s_Result.m_RemoveClicked) {
            s_RemovedIndex = i;
        }
    }

    if (s_RemovedIndex) {
        const auto* s_TypeFunctions = s_ElementTypeID->GetTypeInfo()->m_pTypeFunctions;

        for (size_t i = *s_RemovedIndex; i + 1 < s_Elements.size(); ++i) {
            s_TypeFunctions->assign(s_Elements[i], s_Elements[i + 1]);
        }

        p_ArrayType->setSize(p_Data, static_cast<uint32_t>(s_Elements.size() - 1));
        s_IsAnyElementChanged = true;
    }

    return s_IsAnyElementChanged;
}
