#include "Editor.h"

void Editor::OnSetPropertyValue(
    ZEntityRef p_Entity, uint32_t p_PropertyId, const ZObjectRef& p_Value, std::optional<std::string> p_ClientId
) {
    Hooks::SetPropertyValue->Call(p_Entity, p_PropertyId, p_Value, true);
    m_Server.OnEntityPropertySet(p_Entity, p_PropertyId, std::move(p_ClientId));
}

void Editor::OnSignalEntityPin(ZEntityRef p_Entity, uint32_t p_PinId, bool p_Output) {
    if (p_Output) {
        p_Entity.SignalOutputPin(p_PinId);
    }
    else {
        p_Entity.SignalInputPin(p_PinId);
    }
}

void Editor::OnSignalEntityPin(ZEntityRef p_Entity, const std::string& p_Pin, bool p_Output) {
    OnSignalEntityPin(p_Entity, Hash::Crc32(p_Pin.c_str(), p_Pin.size()), p_Output);
}

void Editor::UnsupportedProperty(
    const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data
) {
    const auto s_PropertyInfo = p_Property->m_pType->getPropertyInfo();
    const std::string s_TypeName = s_PropertyInfo->m_pType->typeInfo()->m_pTypeName;

    constexpr auto textColor = ImVec4(1.f, 1.f, 1.f, 0.5f);
    ImGui::TextColored(textColor, "(Unsupported)", s_TypeName.c_str());
}

void Editor::TEntityRefProperty(
    const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data
) {
    if (auto EntityRef = reinterpret_cast<TEntityRef<void*>*>(p_Data)) {
        ImVec4 linkColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f); // Light blue, like a link

        ImGui::PushStyleColor(ImGuiCol_Text, linkColor);
        ImGui::Text("%s", "link");
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered()) {
            // Underline on hover
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(min.x, max.y),
                ImVec2(max.x, max.y),
                ImGui::GetColorU32(linkColor)
            );
        }

        if (ImGui::IsItemClicked()) {
            OnSelectEntity(EntityRef->m_ref, std::nullopt);
        }
    }
    else {
        constexpr auto textColor = ImVec4(1.f, 1.f, 1.f, 0.5f);
        ImGui::TextColored(textColor, "(%s)", "null");
    }
}

void Editor::ZRepositoryIDProperty(
    const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data
) {
    if (auto RepositoryId = reinterpret_cast<ZRepositoryID*>(p_Data)) {
        ImGui::Text("%s", RepositoryId->ToString().c_str());
    }
    else {
        constexpr auto textColor = ImVec4(1.f, 1.f, 1.f, 0.5f);
        ImGui::TextColored(textColor, "(%s)", "null");
    }
}
