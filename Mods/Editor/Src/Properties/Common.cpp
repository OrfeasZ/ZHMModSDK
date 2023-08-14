#include "Editor.h"

void Editor::OnSetPropertyValue(ZEntityRef p_Entity, uint32_t p_PropertyId, const ZObjectRef& p_Value, std::optional<std::string> p_ClientId) {
	Hooks::SetPropertyValue->Call(p_Entity, p_PropertyId, p_Value, true);
	m_Server.OnEntityPropertySet(p_Entity, p_PropertyId, std::move(p_ClientId));
}

void Editor::OnSignalEntityPin(ZEntityRef p_Entity, uint32_t p_PinId, bool p_Output) {
	if (p_Output) {
		p_Entity.SignalOutputPin(p_PinId);
	} else {
		p_Entity.SignalInputPin(p_PinId);
	}
}

void Editor::OnSignalEntityPin(ZEntityRef p_Entity, const std::string& p_Pin, bool p_Output) {
	OnSignalEntityPin(p_Entity, Hash::Crc32(p_Pin.c_str(), p_Pin.size()), p_Output);
}

void Editor::UnsupportedProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	const auto s_PropertyInfo = p_Property->m_pType->getPropertyInfo();
	const std::string s_TypeName = s_PropertyInfo->m_pType->typeInfo()->m_pTypeName;
	ImGui::Text(" %s (Unsupported)", s_TypeName.c_str());
}