#include <Editor.h>

#include "Functions.h"
#include "Glacier/ZSpatialEntity.h"
#include "Glacier/ZPhysics.h"
#include "Glacier/ZCameraEntity.h"
#include "Glacier/ZPathfinder.h"
#include "Glacier/ZScaleform.h"

void Editor::DrawEntityAABB(IRenderer* p_Renderer, float p_Alpha) {
    if (IsSpecialEntityTreeNode(m_SelectedEntity)
    ) {
        return;
    }

    if (const auto s_SelectedEntity = m_SelectedEntity) {
        if (auto* s_SpatialEntity = s_SelectedEntity.QueryInterface<ZSpatialEntity>()) {
            auto s_Transform = s_SpatialEntity->GetObjectToWorldMatrix();

            float4 s_Min, s_Max;

            s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

            p_Renderer->SetFrustumCullingEnabled(false);

            p_Renderer->DrawOBB3D(s_Min, s_Max, s_Transform, SVector4(0.f, 1.f, 1.f, p_Alpha));

            if (m_EntityHighlightMode == EntityHighlightMode::LinesAndTriangles) {
                p_Renderer->DrawBoundingQuads3D(s_Min, s_Max, s_Transform, SVector4(0.f, 1.f, 1.f, 0.1f * p_Alpha));
            }

            p_Renderer->SetFrustumCullingEnabled(true);
        }
    }
}

void Editor::DrawEntityManipulator(bool p_HasFocus) {
    auto s_ImgGuiIO = ImGui::GetIO();

    ImGuizmo::BeginFrame();

    // The raycast waits on the physics world, so it runs without the tree lock.
    if (p_HasFocus) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
            !s_ImgGuiIO.WantCaptureMouse &&
            !m_IsGizmoBlocked &&
            !Globals::ScaleformGFxManager->GetIsMenuModeUnsafe()) {
            if (!m_HoldingMouse) {
                const auto s_MousePos = ImGui::GetMousePos();
                const auto s_ViewportPos = ImGui::GetMainViewport()->Pos;
                OnMouseDown(SVector2(s_MousePos.x - s_ViewportPos.x, s_MousePos.y - s_ViewportPos.y));
            }

            m_HoldingMouse = true;
        }
        else {
            m_HoldingMouse = false;
        }
    }

    std::shared_lock s_TreeLock(m_CachedEntityTreeMutex);

    const bool s_IsGizmoDragging = m_GizmoEdit && !m_GizmoEdit->IsCommitted();

    if (p_HasFocus && !s_ImgGuiIO.WantTextInput) {
        const auto s_GizmoMode = m_GizmoMode;
        const auto s_GizmoSpace = m_GizmoSpace;

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
            OnSelectEntity({}, false, std::nullopt);
        }

        // Only spawned entities can be destroyed.
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && m_SpawnedEntities.contains(m_SelectedEntity)) {
            OnDestroyEntity(m_SelectedEntity, std::nullopt);
        }

        if (s_IsGizmoDragging && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            CommitGizmoEdit(ChangeMode::Revert);
            m_IsGizmoBlocked = true;
        }
        else if (s_IsGizmoDragging && (m_GizmoMode != s_GizmoMode || m_GizmoSpace != s_GizmoSpace)) {
            CommitGizmoEdit(ChangeMode::Commit);
            m_IsGizmoBlocked = true;
        }
    }

    // A cancelled drag stays cancelled until the mouse button is released.
    if (m_IsGizmoBlocked && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        m_IsGizmoBlocked = false;
    }

    ImGuizmo::Enable(p_HasFocus && !m_IsGizmoBlocked);

    if (IsSpecialEntityTreeNode(m_SelectedEntity)) {
        return;
    }

    if (const auto s_SelectedEntity = m_SelectedEntity) {
        if (const auto s_CurrentCamera = Functions::GetCurrentCamera->Call()) {
            if (const auto s_SpatialEntity = s_SelectedEntity.QueryInterface<ZSpatialEntity>()) {
                auto s_ModelMatrix = s_SpatialEntity->GetObjectToWorldMatrix();

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

                const auto* s_Viewport = ImGui::GetMainViewport();
                ImGuizmo::SetRect(s_Viewport->Pos.x, s_Viewport->Pos.y, s_Viewport->Size.x, s_Viewport->Size.y);

                const bool s_IsScale = m_GizmoMode == ImGuizmo::SCALE;

                if (s_IsScale && !s_SelectedEntity.QueryInterface<ZGeomEntity>()) {
                    return;
                }

                // The scale gizmo edits the primitive scale property instead of the transform.
                const ChangeKey s_Key = s_IsScale
                    ? ChangeKey { s_SelectedEntity, Hash::Crc32("m_PrimitiveScale") }
                    : ChangeKey { s_SelectedEntity };

                if (s_IsScale) {
                    ZVariant<SVector3> s_Scale = s_SelectedEntity.GetProperty<SVector3>("m_PrimitiveScale");

                    s_ModelMatrix.ScaleTransform(s_Scale.Get());
                }

                // The update thread moves the entity a frame later, so we keep drawing the edited
                // transform until its commit is applied.
                if (m_GizmoEdit && m_GizmoEdit->Key == s_Key) {
                    s_ModelMatrix = m_GizmoEdit->Transform;
                    m_GizmoEdit->IsDrawn = true;
                }

                const SMatrix s_StartMatrix = s_ModelMatrix;

                float s_TranslateSnapValue[3] = {
                    static_cast<float>(m_SnapValue), static_cast<float>(m_SnapValue),
                    static_cast<float>(m_SnapValue)
                };
                float s_AngleSnapValue = static_cast<float>(m_AngleSnapValue);
                float s_ScaleSnapValue = static_cast<float>(m_ScaleSnapValue);

                const float* s_Snap = nullptr;

                if (m_GizmoMode == ImGuizmo::TRANSLATE && m_UseSnap)
                    s_Snap = s_TranslateSnapValue;
                else if (m_GizmoMode == ImGuizmo::ROTATE && m_UseAngleSnap)
                    s_Snap = &s_AngleSnapValue;
                else if (m_GizmoMode == ImGuizmo::SCALE && m_UseScaleSnap)
                    s_Snap = &s_ScaleSnapValue;

                // A drag sends a preview each frame and commits once on release.
                if (ImGuizmo::Manipulate(
                    &s_ViewMatrix.XAxis.x, &s_ProjectionMatrix.XAxis.x, m_GizmoMode, m_GizmoSpace,
                    &s_ModelMatrix.XAxis.x, NULL, s_Snap
                )) {
                    if (!s_IsGizmoDragging || m_GizmoEdit->Key != s_Key) {
                        m_GizmoEdit = ActiveEdit { .Key = s_Key, .StartTransform = s_StartMatrix, .IsDrawn = true };
                    }

                    m_GizmoEdit->Transform = s_ModelMatrix;

                    if (s_IsScale) {
                        OnSetPropertyValue(
                            s_SelectedEntity, *s_Key.PropertyId, ZObjectRef::From<SVector3>(s_ModelMatrix.GetScale()),
                            std::nullopt, ChangeMode::Preview
                        );
                    }
                    else {
                        OnEntityTransformChange(
                            s_SelectedEntity, s_ModelMatrix, false, std::nullopt, ChangeMode::Preview
                        );
                    }
                }

                if (m_GizmoEdit && !m_GizmoEdit->IsCommitted() && !ImGuizmo::IsUsing()) {
                    CommitGizmoEdit(ChangeMode::Commit);
                }
            }
        }
    }
}

void Editor::RefreshPathfinderObstacle(ZEntityRef p_Entity, bool p_Force) {
    auto* s_Obstacle = p_Entity.QueryInterface<ZPFObstacleEntity>();

    if (!s_Obstacle) {
        return;
    }

    uint32 s_Flags = s_Obstacle->m_nObstacleBlockageFlags;

    if (const auto* s_ActorFlagObstacle = p_Entity.QueryInterface<ZActorFlagPFObstacleEntity>()) {
        s_Flags = s_ActorFlagObstacle->GetEffectiveFlags();
    }

    Hooks::ZPFObstacleEntity_UpdateObstacle->Call(s_Obstacle, s_Flags, s_Obstacle->m_bEnabled, p_Force);
}

// Static physics objects and pathfinder obstacles don't follow their entities when they
// move, so we move them ourselves.
void Editor::RefreshStaticPhysicsAndObstacles(ZEntityRef p_Root) {
    ForEachEntityInTree(p_Root, [&](ZEntityRef p_Entity) {
        const auto s_PhysicsAspect = p_Entity.QueryInterface<ZStaticPhysicsAspect>();

        if (s_PhysicsAspect && s_PhysicsAspect->m_pPhysicsObject) {
            if (const auto s_SpatialEntity = p_Entity.QueryInterface<ZSpatialEntity>()) {
                s_PhysicsAspect->m_pPhysicsObject->SetTransform(s_SpatialEntity->GetObjectToWorldMatrix());
            }
        }

        RefreshPathfinderObstacle(p_Entity, false);
    });
}
