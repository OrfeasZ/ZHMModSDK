#include "Editor.h"
#include "imgui.h"

bool Editor::SMatrix43Property(const std::string& p_Id, void* p_Data) {
    bool s_IsChanged = false;
    auto* s_Value = static_cast<SMatrix43*>(p_Data);

    if (m_UseQneTransforms) {
        auto s_QneTransform = MatrixToQneTransform(*s_Value);

        bool s_IsQneChanged = DrawVectorChildRow(p_Id + "p", "Position", nullptr, &s_QneTransform.Position.x);
        s_IsQneChanged |= DrawVectorChildRow(p_Id + "r", "Rotation", "\xC2\xB0", &s_QneTransform.Rotation.x);

        // We can't update scale like this. Use the gizmo instead.
        if (s_IsQneChanged) {
            *s_Value = QneTransformToMatrix(s_QneTransform).ToMatrix43();
            s_IsChanged = true;
        }
    }
    else {
        s_IsChanged |= DrawVectorChildRow(p_Id + "x", "X axis", nullptr, &s_Value->XAxis.x);
        s_IsChanged |= DrawVectorChildRow(p_Id + "y", "Y axis", nullptr, &s_Value->YAxis.x);
        s_IsChanged |= DrawVectorChildRow(p_Id + "z", "Z axis", nullptr, &s_Value->ZAxis.x);
        s_IsChanged |= DrawVectorChildRow(p_Id + "t", "Translation", nullptr, &s_Value->Trans.x);
    }

    return s_IsChanged;
}
