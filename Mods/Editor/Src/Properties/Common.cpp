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

    constexpr auto s_TextColor = ImVec4(1.f, 1.f, 1.f, 0.5f);
    ImGui::TextColored(s_TextColor, "(Unsupported)", s_TypeName.c_str());
}

void Editor::ZEntityRefProperty(
    const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data
) {
    if (auto s_EntityRef = reinterpret_cast<ZEntityRef*>(p_Data)) {
        EntityRefProperty(*s_EntityRef);
    }
    else {
        EntityRefProperty(ZEntityRef{});
    }
}

void Editor::TEntityRefProperty(
    const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data
) {
    if (auto s_EntityRef = reinterpret_cast<TEntityRef<void*>*>(p_Data)) {
        EntityRefProperty(s_EntityRef->m_ref);
    }
    else {
        EntityRefProperty(ZEntityRef{});
    }
}

void Editor::EntityRefProperty(ZEntityRef p_Entity)
{
    if (!p_Entity) {
        constexpr ImVec4 s_TextColor = ImVec4(1.f, 1.f, 1.f, 0.5f);

        ImGui::TextColored(s_TextColor, "(%s)", "null");

        return;
    }

    ImVec4 s_LinkColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, s_LinkColor);
    ImGui::Text("%s", "link");
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered()) {
        ImVec2 s_Min = ImGui::GetItemRectMin();
        ImVec2 s_Max = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(s_Min.x, s_Max.y),
            ImVec2(s_Max.x, s_Max.y),

            ImGui::GetColorU32(s_LinkColor)
        );
    }

    if (ImGui::IsItemClicked()) {
        OnSelectEntity(p_Entity, true, std::nullopt);
    }
}

void Editor::ZRepositoryIDProperty(
    const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data
) {
    if (auto RepositoryId = reinterpret_cast<ZRepositoryID*>(p_Data)) {
        const auto& s_RepositoryId = RepositoryId->ToString();

        ImGui::Text("%s", s_RepositoryId.c_str());

        if (ImGuiCopyWidget(("RepositoryId_" + p_Id).c_str())) {
            CopyToClipboard(s_RepositoryId.c_str());
        }
    }
    else {
        constexpr auto textColor = ImVec4(1.f, 1.f, 1.f, 0.5f);
        ImGui::TextColored(textColor, "(%s)", "null");
    }
}
