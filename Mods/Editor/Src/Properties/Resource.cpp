#include "Editor.h"
#include "imgui.h"

#include <Glacier/ZResource.h>

namespace {
    std::string TruncatePathFromStart(std::string_view p_Path, float p_AvailableWidth) {
        if (ImGui::CalcTextSize(p_Path.data(), p_Path.data() + p_Path.size()).x <= p_AvailableWidth) {
            return std::string(p_Path);
        }

        constexpr std::string_view s_Ellipsis = "...";
        const float s_EllipsisWidth = ImGui::CalcTextSize("...").x;

        size_t s_Start = 0;

        while (s_Start < p_Path.size()) {
            const std::string_view s_Remaining = p_Path.substr(s_Start);

            if (ImGui::CalcTextSize(s_Remaining.data(), s_Remaining.data() + s_Remaining.size()).x +
                s_EllipsisWidth <= p_AvailableWidth) {
                break;
            }

            ++s_Start;
        }

        return std::string(s_Ellipsis) + std::string(p_Path.substr(s_Start));
    }
}

void Editor::DrawResourceIdLink(const ZRuntimeResourceID& p_ResourceId) const {
    const std::string s_RuntimeResourceID = fmt::format(
        "{:08X}{:08X}",
        p_ResourceId.m_IDHigh,
        p_ResourceId.m_IDLow
    );

    ZString s_ResourceID;
    const bool s_HasResourceID = SDK()->GetResourceID(p_ResourceId, s_ResourceID);

    const std::string s_DisplayText = s_HasResourceID ? s_ResourceID.c_str() : s_RuntimeResourceID;

    const float s_CopyButtonWidth = ImGui::GetFrameHeight();
    const float s_AvailableWidth = std::max(
        0.f,
        ImGui::GetContentRegionAvail().x -
        s_CopyButtonWidth -
        ImGui::GetStyle().ItemSpacing.x
    );

    const std::string s_VisibleText = TruncatePathFromStart(s_DisplayText, s_AvailableWidth);

    if (DrawLinkText(s_VisibleText)) {
        CopyToClipboard(s_DisplayText);
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", s_HasResourceID ? s_ResourceID.c_str() : "Resource ID not found");
    }

    if (ImGui::BeginPopupContextItem("ResourceIDContextMenu")) {
        if (ImGui::MenuItem("Copy resource ID", nullptr, false, s_HasResourceID)) {
            CopyToClipboard(s_ResourceID.c_str());
        }

        if (ImGui::MenuItem("Copy runtime resource ID")) {
            CopyToClipboard(s_RuntimeResourceID);
        }

        ImGui::EndPopup();
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
