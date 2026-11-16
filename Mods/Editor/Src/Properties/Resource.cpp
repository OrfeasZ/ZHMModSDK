#include "Editor.h"
#include "imgui.h"

#include <Glacier/ZResource.h>

void Editor::DrawResourceIdLink(const ZRuntimeResourceID& p_ResourceId) const {
    const std::string s_ResourceIdString = fmt::format("{:08X}{:08X}", p_ResourceId.m_IDHigh, p_ResourceId.m_IDLow);

    if (DrawLinkText(s_ResourceIdString)) {
        CopyToClipboard(s_ResourceIdString);
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Click to copy");
    }
}

void Editor::ResourcePtrProperty(void* p_Data) {
    auto* s_ResourcePtr = static_cast<ZResourcePtr*>(p_Data);

    DrawReferenceWell(ICON_MD_DESCRIPTION, ImGui::CalcItemWidth(), [&] {
        if (!s_ResourcePtr || s_ResourcePtr->m_nResourceIndex.val < 0) {
            ImGui::TextColored(c_MutedTextColor, "None");
            return;
        }

        DrawResourceIdLink(s_ResourcePtr->GetResourceInfo().rid);
    });
}

void Editor::ZRuntimeResourceIDProperty(void* p_Data) {
    const auto* s_RuntimeResourceID = static_cast<ZRuntimeResourceID*>(p_Data);

    ImGui::AlignTextToFramePadding();

    if (*s_RuntimeResourceID == -1) {
        ImGui::TextColored(c_MutedTextColor, "None");
        return;
    }

    DrawResourceIdLink(*s_RuntimeResourceID);
}
