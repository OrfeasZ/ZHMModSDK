#include <Editor.h>

#include "Functions.h"
#include "Glacier/ZSpatialEntity.h"
#include "Glacier/ZPhysics.h"
#include "Glacier/ZCameraEntity.h"

void Editor::DrawEntityAABB(IRenderer* p_Renderer) {
    if (const auto s_SelectedEntity = m_SelectedEntity) {
        if (auto* s_SpatialEntity = s_SelectedEntity.QueryInterface<ZSpatialEntity>()) {
            auto s_Transform = s_SpatialEntity->GetWorldMatrix();

            float4 s_Min, s_Max;

            s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

            if (p_Renderer->GetCurrentPrimitiveType() == PrimitiveType::Line) {
                p_Renderer->DrawOBB3D(
                    SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform,
                    SVector4(0.f, 1.f, 1.f, 1.f)
                );
            }
            else if (p_Renderer->GetCurrentPrimitiveType() == PrimitiveType::Triangle &&
                m_EntityHighlightMode == EntityHighlightMode::LinesAndTriangles) {
                p_Renderer->DrawBoundingQuads(
                    SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform,
                    SVector4(0.f, 1.f, 1.f, 0.1f)
                );
            }
        }
    }
}

void Editor::DrawEntityManipulator(bool p_HasFocus) {
    auto s_ImgGuiIO = ImGui::GetIO();

    ImGuizmo::BeginFrame();

    if (p_HasFocus) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !s_ImgGuiIO.WantCaptureMouse) {
            const auto s_MousePos = ImGui::GetMousePos();
            OnMouseDown(SVector2(s_MousePos.x, s_MousePos.y), !m_HoldingMouse);
            m_HoldingMouse = true;
        }
        else {
            m_HoldingMouse = false;
        }

        if (!s_ImgGuiIO.WantTextInput) {
            if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
                if (m_GizmoMode == ImGuizmo::TRANSLATE)
                    m_GizmoMode = ImGuizmo::ROTATE;
                else if (m_GizmoMode == ImGuizmo::ROTATE)
                    m_GizmoMode = ImGuizmo::SCALE;
                else if (m_GizmoMode == ImGuizmo::SCALE)
                    m_GizmoMode = ImGuizmo::TRANSLATE;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
                m_GizmoSpace = m_GizmoSpace == ImGuizmo::WORLD ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
                OnSelectEntity({}, std::nullopt);
            }
        }
    }

    ImGuizmo::Enable(p_HasFocus);

    if (const auto s_SelectedEntity = m_SelectedEntity) {
        if (const auto s_CurrentCamera = Functions::GetCurrentCamera->Call()) {
            if (const auto s_SpatialEntity = s_SelectedEntity.QueryInterface<ZSpatialEntity>()) {
                auto s_ModelMatrix = s_SpatialEntity->GetWorldMatrix();

                const auto s_CameraRight = Globals::RenderManager->m_pDevice->m_Constants.cameraRight;
                const auto s_CameraUp = Globals::RenderManager->m_pDevice->m_Constants.cameraUp;
                const auto s_CameraFwd = Globals::RenderManager->m_pDevice->m_Constants.cameraFwd;
                const auto s_CameraPos = Globals::RenderManager->m_pDevice->m_Constants.cameraPos;

                const auto s_ViewMatrix = SMatrix {
                    {s_CameraRight.x, s_CameraRight.y, s_CameraRight.z, 0.f},
                    {s_CameraUp.x, s_CameraUp.y, s_CameraUp.z, 0.f},
                    {-s_CameraFwd.x, -s_CameraFwd.y, -s_CameraFwd.z, 0.f},
                    {s_CameraPos.x, s_CameraPos.y, s_CameraPos.z, 1.f}
                }.Inverse();

                const auto s_ProjectionMatrix = *reinterpret_cast<SMatrix*>(&Globals::RenderManager->m_pDevice->
                    m_Constants.cameraViewToClip);

                ImGuizmo::SetRect(0, 0, s_ImgGuiIO.DisplaySize.x, s_ImgGuiIO.DisplaySize.y);

                if (m_GizmoMode == ImGuizmo::SCALE) {
                    const auto s_GeomEntity = s_SelectedEntity.QueryInterface<ZGeomEntity>();

                    if (s_GeomEntity) {
                        ZVariant<SVector3> s_Scale = m_SelectedEntity.GetProperty<SVector3>("m_PrimitiveScale");

                        s_ModelMatrix.ScaleTransform(s_Scale.Get());

                        float s_ScaleSnapValue = static_cast<float>(m_ScaleSnapValue);
                        if (ImGuizmo::Manipulate(
                            &s_ViewMatrix.XAxis.x, &s_ProjectionMatrix.XAxis.x, m_GizmoMode, m_GizmoSpace,
                            &s_ModelMatrix.XAxis.x, NULL, m_UseScaleSnap ? &s_ScaleSnapValue : NULL
                        )) {
                            m_SelectedEntity.SetProperty<SVector3>("m_PrimitiveScale", s_ModelMatrix.GetScale());

                            const bool s_bRemovePhysics = m_SelectedEntity.GetProperty<bool>("m_bRemovePhysics").Get();

                            if (!s_bRemovePhysics) {
                                m_SelectedEntity.SetProperty<bool>("m_bRemovePhysics", true);
                                m_SelectedEntity.SetProperty<bool>("m_bRemovePhysics", false);
                            }
                        }
                    }
                }
                else if (m_GizmoMode == ImGuizmo::TRANSLATE) {
                    float m_CombinedSnapValue[3] = {
                        static_cast<float>(m_SnapValue), static_cast<float>(m_SnapValue),
                        static_cast<float>(m_SnapValue)
                    };

                    if (ImGuizmo::Manipulate(
                        &s_ViewMatrix.XAxis.x, &s_ProjectionMatrix.XAxis.x, m_GizmoMode, m_GizmoSpace,
                        &s_ModelMatrix.XAxis.x, NULL, m_UseSnap ? m_CombinedSnapValue : NULL
                    )) {
                        OnEntityTransformChange(s_SelectedEntity, s_ModelMatrix, false, std::nullopt);
                    }
                }
                else if (m_GizmoMode == ImGuizmo::ROTATE) {
                    float s_AngleSnapValue = static_cast<float>(m_AngleSnapValue);
                    if (ImGuizmo::Manipulate(
                        &s_ViewMatrix.XAxis.x, &s_ProjectionMatrix.XAxis.x, m_GizmoMode, m_GizmoSpace,
                        &s_ModelMatrix.XAxis.x, NULL, m_UseAngleSnap ? &s_AngleSnapValue : NULL
                    )) {
                        OnEntityTransformChange(s_SelectedEntity, s_ModelMatrix, false, std::nullopt);
                    }
                }
            }
        }
    }
}

void Editor::OnEntityTransformChange(
    ZEntityRef p_Entity, SMatrix p_Transform, bool p_Relative, std::optional<std::string> p_ClientId
) {
    if (auto* s_SpatialEntity = p_Entity.QueryInterface<ZSpatialEntity>()) {
        if (!p_Relative) {
            s_SpatialEntity->SetWorldMatrix(p_Transform);
        }
        else {
            SMatrix s_ParentTrans;

            // Get parent entity transform.
            if (s_SpatialEntity->m_eidParent.m_pInterfaceRef) {
                s_ParentTrans = s_SpatialEntity->m_eidParent.m_pInterfaceRef->GetWorldMatrix();
            }
            else if (p_Entity.GetLogicalParent() && p_Entity.GetLogicalParent().QueryInterface<ZSpatialEntity>()) {
                s_ParentTrans = p_Entity.GetLogicalParent().QueryInterface<ZSpatialEntity>()->GetWorldMatrix();
            }
            else if (p_Entity.GetOwningEntity() && p_Entity.GetOwningEntity().QueryInterface<ZSpatialEntity>()) {
                s_ParentTrans = p_Entity.GetOwningEntity().QueryInterface<ZSpatialEntity>()->GetWorldMatrix();
            }

            // Calculate world transform based on the provided relative transform.
            auto s_WorldTrans = p_Transform * s_ParentTrans;
            s_WorldTrans.Trans = s_ParentTrans.Trans + p_Transform.Trans;
            s_WorldTrans.Trans.w = 1.f;

            s_SpatialEntity->SetWorldMatrix(s_WorldTrans);
        }

        if (const auto s_PhysicsAspect = p_Entity.QueryInterface<ZStaticPhysicsAspect>()) {
            if (s_PhysicsAspect->m_pPhysicsObject) {
                s_PhysicsAspect->m_pPhysicsObject->SetTransform(s_SpatialEntity->GetWorldMatrix());
            }
        }

        m_Server.OnEntityTransformChanged(p_Entity, std::move(p_ClientId));
    }
}
