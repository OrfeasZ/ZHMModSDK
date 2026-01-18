#include "Editor.h"
#include "imgui.h"

bool Editor::SMatrix43Property(
    const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data
) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<SMatrix43*>(p_Data);

    if (m_UseQneTransforms) {
        auto s_QneTransform = MatrixToQneTransform(*s_Value);

        if (ImGuiCopyWidget(("##QNE JSON_" + p_Id).c_str())) {
            CopyToClipboard(
                fmt::format(
                    "{{\"rotation\":{{\"x\":{},\"y\":{},\"z\":{}}},\"position\":{{\"x\":{},\"y\":{},\"z\":{}}},"
                    "\"scale\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
                    s_QneTransform.Rotation.x, s_QneTransform.Rotation.y, s_QneTransform.Rotation.z,
                    s_QneTransform.Position.x, s_QneTransform.Position.y, s_QneTransform.Position.z,
                    s_QneTransform.Scale.x, s_QneTransform.Scale.y, s_QneTransform.Scale.z
                )
            );
        }

        if (ImGui::DragFloat3((p_Id + "p").c_str(), &s_QneTransform.Position.x, 0.1f)) {
            const auto s_Matrix = QneTransformToMatrix(s_QneTransform);
            *s_Value = s_Matrix.ToMatrix43();

            s_IsChanged = true;
        }

        if (ImGui::DragFloat3((p_Id + "r").c_str(), &s_QneTransform.Rotation.x, 0.1f)) {
            const auto s_Matrix = QneTransformToMatrix(s_QneTransform);
            *s_Value = s_Matrix.ToMatrix43();

            s_IsChanged = true;
        }

        // We can't update scale like this. Use the gizmo instead.
        /*if (ImGui::DragFloat3((p_Id + "s").c_str(), &s_QneTransform.Scale.x, 0.1f)) {
            const auto s_Matrix = QneTransformToMatrix(s_QneTransform);
            OnSetPropertyValue(p_Entity, p_Property->m_nPropertyId, ZVariant(s_Matrix.ToMatrix43()), std::nullopt);
        }*/
    }
    else {
        if (ImGuiCopyWidget(("##RT JSON_" + p_Id).c_str())) {
            CopyToClipboard(
                fmt::format(
                    "{{"
                    "\"XAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},"
                    "\"YAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},"
                    "\"ZAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},"
                    "\"Trans\":{{\"x\":{},\"y\":{},\"z\":{}}}"
                    "}}",
                    s_Value->XAxis.x,
                    s_Value->XAxis.y,
                    s_Value->XAxis.z,
                    s_Value->YAxis.x,
                    s_Value->YAxis.y,
                    s_Value->YAxis.z,
                    s_Value->ZAxis.x,
                    s_Value->ZAxis.y,
                    s_Value->ZAxis.z,
                    s_Value->Trans.x,
                    s_Value->Trans.y,
                    s_Value->Trans.z
                )
            );
        }

        if (ImGui::DragFloat3((p_Id + "x").c_str(), &s_Value->XAxis.x, 0.1f)) {
            s_IsChanged = true;
        }

        if (ImGui::DragFloat3((p_Id + "y").c_str(), &s_Value->YAxis.x, 0.1f)) {
            s_IsChanged = true;
        }

        if (ImGui::DragFloat3((p_Id + "z").c_str(), &s_Value->ZAxis.x, 0.1f)) {
            s_IsChanged = true;
        }

        if (ImGui::DragFloat3((p_Id + "t").c_str(), &s_Value->Trans.x, 0.1f)) {
            s_IsChanged = true;
        }
    }

    return s_IsChanged;
}
