#include "Editor.h"
#include "imgui.h"

#include <Glacier/ZResource.h>

void Editor::ResourcePtrProperty(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    auto* s_ResourcePtr = static_cast<ZResourcePtr*>(p_Data);
    std::string s_RuntimeResourceID = "null";

    if (s_ResourcePtr && s_ResourcePtr->m_nResourceIndex.val >= 0) {
        const auto& rid = s_ResourcePtr->GetResourceInfo().rid;
        s_RuntimeResourceID = fmt::format("{:08X}{:08X}", rid.m_IDHigh, rid.m_IDLow);

        ImGui::TextUnformatted(s_RuntimeResourceID.c_str());
    }
    else {
        constexpr ImVec4 s_TextColor = ImVec4(1.f, 1.f, 1.f, 0.5f);

        ImGui::TextColored(s_TextColor, "(%s)", "null");
    }

    if (ImGuiCopyWidget(("ResourcePtr_" + p_Id).c_str())) {
        CopyToClipboard(s_RuntimeResourceID.c_str());
    }
}

void Editor::ZRuntimeResourceIDProperty(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data) {
    auto* s_RuntimeResourceID = static_cast<ZRuntimeResourceID*>(p_Data);
    std::string s_RuntimeResourceIDString = "null";

    if (*s_RuntimeResourceID != -1) {
        s_RuntimeResourceIDString = fmt::format("{:08X}{:08X}", s_RuntimeResourceID->m_IDHigh, s_RuntimeResourceID->m_IDLow);

        ImGui::TextUnformatted(s_RuntimeResourceIDString.c_str());
    }
    else {
        constexpr ImVec4 s_TextColor = ImVec4(1.f, 1.f, 1.f, 0.5f);

        ImGui::TextColored(s_TextColor, "(%s)", "null");
    }

    if (ImGuiCopyWidget(("ResourcePtr_" + p_Id).c_str())) {
        CopyToClipboard(s_RuntimeResourceIDString.c_str());
    }
}