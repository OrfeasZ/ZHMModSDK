#include "Editor.h"
#include "imgui.h"

bool Editor::SVector2Property(const std::string& p_Id, void* p_Data) {
    return DragFloatComponents(p_Id, &static_cast<SVector2*>(p_Data)->x, 2, c_VectorTags, 0.1f);
}

bool Editor::SVector3Property(const std::string& p_Id, void* p_Data) {
    return DragFloatComponents(p_Id, &static_cast<SVector3*>(p_Data)->x, 3, c_VectorTags, 0.1f);
}

bool Editor::SVector4Property(const std::string& p_Id, void* p_Data) {
    return DragFloatComponents(p_Id, &static_cast<SVector4*>(p_Data)->x, 4, c_VectorTags, 0.1f);
}
