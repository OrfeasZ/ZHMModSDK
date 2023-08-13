#include "Editor.h"
#include "imgui.h"

#include <Glacier/ZResource.h>

void Editor::ResourceProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto* s_Resource = static_cast<ZResourcePtr*>(p_Data);
	std::string s_ResourceName = "null";

	if (s_Resource && s_Resource->m_nResourceIndex >= 0)
		s_ResourceName = fmt::format("{}", s_Resource->GetResourceInfo().rid);

	ImGui::Text(" %s", s_ResourceName.c_str());
}