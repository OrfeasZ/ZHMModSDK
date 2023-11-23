#include "Editor.h"
#include <Glacier/SColorRGB.h>
#include <Glacier/SColorRGBA.h>

void Editor::SColorRGBProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data)
{
	auto s_Value = *static_cast<SColorRGB*>(p_Data);

	if (ImGui::ColorEdit3(p_Id.c_str(), &s_Value.r))
	{
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}

void Editor::SColorRGBAProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data)
{
	auto s_Value = *static_cast<SColorRGBA*>(p_Data);

	if (ImGui::ColorEdit4(p_Id.c_str(), &s_Value.r))
	{
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}
