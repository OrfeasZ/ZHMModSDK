#include "DebugMod.h"

#include <numbers>

#include <Glacier/ZCameraEntity.h>

void DebugMod::DrawPositionBox(bool p_HasFocus)
{
    if (!p_HasFocus || !m_PositionsMenuActive)
    {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("POSITIONS", &m_PositionsMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing)
    {
        SMatrix s_HitmanTrans;
        SMatrix s_CameraTrans;

        // Enable Hitman input.
        TEntityRef<ZHitman5> s_LocalHitman;
        Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

        auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

        const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

        if (s_HitmanSpatial)
            s_HitmanTrans = s_HitmanSpatial->GetWorldMatrix();

        if (s_CurrentCamera)
            s_CameraTrans = s_CurrentCamera->GetWorldMatrix();

        ImGui::TextUnformatted("Hitman Transform:");

        if (ImGui::BeginTable("DebugMod_HitmanPosition", 4))
        {
            for (int i = 0; i < 4; ++i)
            {
                ImGui::TableNextRow();

                for (int j = 0; j < 4; ++j)
                {
                    ImGui::TableSetColumnIndex(j);
                    ImGui::Text("%f", s_HitmanTrans.flt[(i * 4) + j]);
                }
            }

            ImGui::EndTable();
        }

        ImGui::TextUnformatted("Camera Transform:");

        if (ImGui::BeginTable("DebugMod_Camera_Position", 4))
        {
            for (int i = 0; i < 4; ++i)
            {
                ImGui::TableNextRow();

                for (int j = 0; j < 4; ++j)
                {
                    ImGui::TableSetColumnIndex(j);
                    ImGui::Text("%f", s_CameraTrans.flt[(i * 4) + j]);
                }
            }

            ImGui::EndTable();
        }

        if (ImGui::Button("Copy Hitman Transform"))
        {
            CopyToClipboard(fmt::format("{}", s_HitmanTrans));
        }

        ImGui::SameLine();

        if (ImGui::Button("RT JSON##HitmanRT"))
        {
            CopyToClipboard(fmt::format(
                "{{\"XAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"YAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"ZAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"Trans\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
                s_HitmanTrans.XAxis.x, s_HitmanTrans.XAxis.y, s_HitmanTrans.XAxis.z,
                s_HitmanTrans.YAxis.x, s_HitmanTrans.YAxis.y, s_HitmanTrans.YAxis.z,
                s_HitmanTrans.ZAxis.x, s_HitmanTrans.ZAxis.y, s_HitmanTrans.ZAxis.z,
                s_HitmanTrans.Trans.x, s_HitmanTrans.Trans.y, s_HitmanTrans.Trans.z
            ));
        }

        ImGui::SameLine();

        if (ImGui::Button("QN JSON##HitmanQN"))
        {
            // This is adapted from QN: https://github.com/atampy25/quickentity-rs/blob/b94dbab32c91ccd0e1612a2e5dda8fb83eb2a8e9/src/lib.rs#L243
            constexpr double c_RAD2DEG = 180.0 / std::numbers::pi;

            double s_RotationX = abs(s_HitmanTrans.XAxis.z) < 0.9999999f
                ? atan2f(-s_HitmanTrans.YAxis.z, s_HitmanTrans.ZAxis.z) * c_RAD2DEG
                : atan2f(s_HitmanTrans.ZAxis.y, s_HitmanTrans.YAxis.y) * c_RAD2DEG;

            double s_RotationY = asinf(min(max(-1.f, s_HitmanTrans.XAxis.z), 1.f)) * c_RAD2DEG;

            double s_RotationZ = abs(s_HitmanTrans.XAxis.z) < 0.9999999f
                ? atan2f(-s_HitmanTrans.XAxis.y, s_HitmanTrans.XAxis.x) * c_RAD2DEG
                : 0.f;

            CopyToClipboard(fmt::format(
                "{{\"rotation\":{{\"x\":{},\"y\":{},\"z\":{}}},\"position\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
                s_RotationX, s_RotationY, s_RotationZ,
                s_HitmanTrans.Trans.x, s_HitmanTrans.Trans.y, s_HitmanTrans.Trans.z
            ));
        }

        if (ImGui::Button("Copy Camera Transform"))
        {
            CopyToClipboard(fmt::format("{}", s_CameraTrans));
        }

        ImGui::SameLine();

        if (ImGui::Button("RT JSON##CameraRT"))
        {
            CopyToClipboard(fmt::format(
                "{{\"XAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"YAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"ZAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"Trans\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
                s_CameraTrans.XAxis.x, s_CameraTrans.XAxis.y, s_CameraTrans.XAxis.z,
                s_CameraTrans.YAxis.x, s_CameraTrans.YAxis.y, s_CameraTrans.YAxis.z,
                s_CameraTrans.ZAxis.x, s_CameraTrans.ZAxis.y, s_CameraTrans.ZAxis.z,
                s_CameraTrans.Trans.x, s_CameraTrans.Trans.y, s_CameraTrans.Trans.z
            ));
        }

        ImGui::SameLine();

        if (ImGui::Button("QN JSON##CameraQN"))
        {
            // This is adapted from QN: https://github.com/atampy25/quickentity-rs/blob/b94dbab32c91ccd0e1612a2e5dda8fb83eb2a8e9/src/lib.rs#L243
            constexpr double c_RAD2DEG = 180.0 / std::numbers::pi;

            double s_RotationX = abs(s_CameraTrans.XAxis.z) < 0.9999999f
                ? atan2f(-s_CameraTrans.YAxis.z, s_CameraTrans.ZAxis.z) * c_RAD2DEG
                : atan2f(s_CameraTrans.ZAxis.y, s_CameraTrans.YAxis.y) * c_RAD2DEG;

            double s_RotationY = asinf(min(max(-1.f, s_CameraTrans.XAxis.z), 1.f)) * c_RAD2DEG;

            double s_RotationZ = abs(s_CameraTrans.XAxis.z) < 0.9999999f
                ? atan2f(-s_CameraTrans.XAxis.y, s_CameraTrans.XAxis.x) * c_RAD2DEG
                : 0.f;

            CopyToClipboard(fmt::format(
                "{{\"rotation\":{{\"x\":{},\"y\":{},\"z\":{}}},\"position\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
                s_RotationX, s_RotationY, s_RotationZ,
                s_CameraTrans.Trans.x, s_CameraTrans.Trans.y, s_CameraTrans.Trans.z
            ));
        }

        ImGui::Checkbox("Use Snap", &m_UseSnap);
        ImGui::SameLine();
        ImGui::InputFloat3("", m_SnapValue);
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}
