#include "Editor.h"
#include "imgui.h"

void Editor::StringProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto* s_RealData = static_cast<ZString*>(p_Data);

	static char s_StringBuffer[65536] = {};
	memcpy(s_StringBuffer, s_RealData->c_str(), min(s_RealData->size(), sizeof(s_StringBuffer) - 1));
	s_StringBuffer[min(s_RealData->size(), sizeof(s_StringBuffer) - 1) + 1] = '\0';

	if (ImGui::InputText(p_Id.c_str(), s_StringBuffer, sizeof(s_StringBuffer))) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(ZString(s_StringBuffer)), std::nullopt);
	}
}

void Editor::BoolProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto s_Value = *static_cast<bool*>(p_Data);
	if (ImGui::Checkbox(p_Id.c_str(), &s_Value)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}

void Editor::Uint8Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto s_Value = *static_cast<uint8*>(p_Data);
	if (ImGui::InputScalar(p_Id.c_str(), ImGuiDataType_U8, &s_Value)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}

void Editor::Uint16Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto s_Value = *static_cast<uint16*>(p_Data);
	if (ImGui::InputScalar(p_Id.c_str(), ImGuiDataType_U16, &s_Value)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}

void Editor::Uint32Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto s_Value = *static_cast<uint32*>(p_Data);
	if (ImGui::InputScalar(p_Id.c_str(), ImGuiDataType_U32, &s_Value)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}

void Editor::Uint64Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto s_Value = *static_cast<uint64*>(p_Data);
	if (ImGui::InputScalar(p_Id.c_str(), ImGuiDataType_U64, &s_Value)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}

void Editor::Int8Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto s_Value = *static_cast<int8*>(p_Data);
	if (ImGui::InputScalar(p_Id.c_str(), ImGuiDataType_S8, &s_Value)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}

void Editor::Int16Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto s_Value = *static_cast<int16*>(p_Data);
	if (ImGui::InputScalar(p_Id.c_str(), ImGuiDataType_S16, &s_Value)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}

void Editor::Int32Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto s_Value = *static_cast<int32*>(p_Data);
	if (ImGui::InputScalar(p_Id.c_str(), ImGuiDataType_S32, &s_Value)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}

void Editor::Int64Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto s_Value = *static_cast<int64*>(p_Data);
	if (ImGui::InputScalar(p_Id.c_str(), ImGuiDataType_S64, &s_Value)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}

void Editor::Float32Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto s_Value = *static_cast<float32*>(p_Data);
	if (ImGui::InputScalar(p_Id.c_str(), ImGuiDataType_Float, &s_Value)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}

void Editor::Float64Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	auto s_Value = *static_cast<float64*>(p_Data);
	if (ImGui::InputScalar(p_Id.c_str(), ImGuiDataType_Double, &s_Value)) {
		OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
	}
}
