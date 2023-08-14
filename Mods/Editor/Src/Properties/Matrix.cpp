#include "Editor.h"
#include "imgui.h"

void Editor::SMatrix43Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	ImGui::NewLine();

	auto s_Value = *static_cast<SMatrix43*>(p_Data);

	if (ImGui::InputFloat3((p_Id + "x").c_str(), &s_Value.XAxis.x)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}

	if (ImGui::InputFloat3((p_Id + "y").c_str(), &s_Value.YAxis.x)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}

	if (ImGui::InputFloat3((p_Id + "z").c_str(), &s_Value.ZAxis.x)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}

	if (ImGui::InputFloat3((p_Id + "t").c_str(), &s_Value.Trans.x)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}