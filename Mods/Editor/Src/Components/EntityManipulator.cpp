#include <Editor.h>

#include "Functions.h"
#include "Glacier/ZSpatialEntity.h"
#include "Glacier/ZPhysics.h"
#include "Glacier/ZCameraEntity.h"

void Editor::DrawEntityAABB(IRenderer* p_Renderer)
{
    if (m_SelectedEntity)
    {
        if (auto* s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>())
        {
            SMatrix s_Transform;
            Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

            float4 s_Min, s_Max;

            s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

            p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(0.f, 0.f, 1.f, 1.f));
        }
    }
}

void Editor::DrawEntityManipulator(bool p_HasFocus)
{
    auto s_ImgGuiIO = ImGui::GetIO();

    ImGuizmo::BeginFrame();

    if (p_HasFocus)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !s_ImgGuiIO.WantCaptureMouse)
        {
            const auto s_MousePos = ImGui::GetMousePos();
            OnMouseDown(SVector2(s_MousePos.x, s_MousePos.y), !m_HoldingMouse);
            m_HoldingMouse = true;
        }
        else
        {
            m_HoldingMouse = false;
        }

        if (ImGui::IsKeyPressed(s_ImgGuiIO.KeyMap[ImGuiKey_Tab]))
        {
            if (m_GizmoMode == ImGuizmo::TRANSLATE)
                m_GizmoMode = ImGuizmo::ROTATE;
            else if (m_GizmoMode == ImGuizmo::ROTATE)
                m_GizmoMode = ImGuizmo::SCALE;
            else if (m_GizmoMode == ImGuizmo::SCALE)
                m_GizmoMode = ImGuizmo::TRANSLATE;
        }

        if (ImGui::IsKeyPressed(s_ImgGuiIO.KeyMap[ImGuiKey_Space]))
        {
            m_GizmoSpace = m_GizmoSpace == ImGuizmo::WORLD ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
        }

        if (ImGui::IsKeyPressed(s_ImgGuiIO.KeyMap[ImGuiKey_Backspace]))
        {
            m_SelectedEntity = {};
        }
    }

    ImGuizmo::Enable(p_HasFocus);

    if (m_SelectedEntity)
    {
        if (const auto s_CurrentCamera = Functions::GetCurrentCamera->Call())
        {
            if (const auto s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>())
            {
                auto s_ModelMatrix = s_SpatialEntity->GetWorldMatrix();
                auto s_ViewMatrix = s_CurrentCamera->GetViewMatrix();
                const SMatrix s_ProjectionMatrix = *s_CurrentCamera->GetProjectionMatrix();

                ImGuizmo::SetRect(0, 0, s_ImgGuiIO.DisplaySize.x, s_ImgGuiIO.DisplaySize.y);

                if (ImGuizmo::Manipulate(&s_ViewMatrix.XAxis.x, &s_ProjectionMatrix.XAxis.x, m_GizmoMode, m_GizmoSpace, &s_ModelMatrix.XAxis.x, NULL, m_UseSnap ? &m_SnapValue[0] : NULL))
                {
                    s_SpatialEntity->SetWorldMatrix(s_ModelMatrix);

                    if (const auto s_PhysicsAspect = m_SelectedEntity.QueryInterface<ZStaticPhysicsAspect>())
                        s_PhysicsAspect->m_pPhysicsObject->SetTransform(s_SpatialEntity->GetWorldMatrix());
                }
            }
        }
    }
}
