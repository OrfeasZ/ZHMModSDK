#include "Editor.h"
#include "imgui.h"
#include <algorithm>
#include <cstddef>

bool Editor::StringProperty(const std::string& p_Id, void* p_Data) {
    bool s_IsChanged = false;
    auto* s_RealData = static_cast<ZString*>(p_Data);

    static char s_StringBuffer[65536] = {};
    const auto s_StringSize = std::min<std::size_t>(s_RealData->size(), sizeof(s_StringBuffer) - 1);

    memcpy(s_StringBuffer, s_RealData->c_str(), s_StringSize);
    s_StringBuffer[s_StringSize] = '\0';

    if (ImGui::InputText(p_Id.c_str(), s_StringBuffer, sizeof(s_StringBuffer))) {
        *s_RealData = ZString(s_StringBuffer);
        s_IsChanged = true;
    }

    DrawEditingOutline();

    return s_IsChanged;
}

bool Editor::BoolProperty(const std::string& p_Id, void* p_Data) {
    auto* s_Value = static_cast<bool*>(p_Data);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ScaledSize(2.f));
    PushFrameHeight(16.f);

    const std::string s_Label = (*s_Value ? "True###" : "False###") + p_Id;
    const bool s_IsChanged = ImGui::Checkbox(s_Label.c_str(), s_Value);

    ImGui::PopStyleVar();

    return s_IsChanged;
}

const Editor::ScalarPropertyType* Editor::FindScalarPropertyType(const std::string_view p_TypeName) {
    static constexpr ScalarPropertyType c_ScalarPropertyTypes[] = {
        { "uint8", "u8", ImGuiDataType_U8, 1.f },
        { "int8", "i8", ImGuiDataType_S8, 1.f },
        { "uint16", "u16", ImGuiDataType_U16, 1.f },
        { "int16", "i16", ImGuiDataType_S16, 1.f },
        { "uint32", "u32", ImGuiDataType_U32, 1.f },
        { "int32", "i32", ImGuiDataType_S32, 1.f },
        { "uint64", "u64", ImGuiDataType_U64, 1.f },
        { "int64", "i64", ImGuiDataType_S64, 1.f },
        { "float32", "f32", ImGuiDataType_Float, 0.1f },
        { "float64", "f64", ImGuiDataType_Double, 0.1f },
    };

    const auto s_It = std::ranges::find(c_ScalarPropertyTypes, p_TypeName, &ScalarPropertyType::m_TypeName);

    return s_It != std::end(c_ScalarPropertyTypes) ? s_It : nullptr;
}
