#include "Editor.h"
#include "imgui.h"

void Editor::SMatrix43Property(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
	ImGui::NewLine();

	auto s_Value = *static_cast<SMatrix43*>(p_Data);

	if (m_UseQneTransforms) {
		auto s_QneTransform = MatrixToQneTransform(s_Value);

		if (ImGui::DragFloat3((p_Id + "p").c_str(), &s_QneTransform.Position.x, 0.1f)) {
			const auto s_Matrix = QneTransformToMatrix(s_QneTransform);
			OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Matrix.ToMatrix43()), std::nullopt);
		}

		if (ImGui::DragFloat3((p_Id + "r").c_str(), &s_QneTransform.Rotation.x, 0.1f)) {
			const auto s_Matrix = QneTransformToMatrix(s_QneTransform);
			OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Matrix.ToMatrix43()), std::nullopt);
		}

		// We can't update scale like this. Use the gizmo instead.
		/*if (ImGui::DragFloat3((p_Id + "s").c_str(), &s_QneTransform.Scale.x, 0.1f)) {
			const auto s_Matrix = QneTransformToMatrix(s_QneTransform);
			OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Matrix.ToMatrix43()), std::nullopt);
		}*/
	} else {
		if (ImGui::DragFloat3((p_Id + "x").c_str(), &s_Value.XAxis.x, 0.1f)) {
			OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
		}

		if (ImGui::DragFloat3((p_Id + "y").c_str(), &s_Value.YAxis.x, 0.1f)) {
			OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
		}

		if (ImGui::DragFloat3((p_Id + "z").c_str(), &s_Value.ZAxis.x, 0.1f)) {
			OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
		}

		if (ImGui::DragFloat3((p_Id + "t").c_str(), &s_Value.Trans.x, 0.1f)) {
			OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Value), std::nullopt);
		}
	}
}