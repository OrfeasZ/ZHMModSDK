#pragma once

#include <algorithm>
#include <vector>

#include <IRenderer.h>
#include <Glacier/ZMath.h>

namespace DebugLines {
    constexpr float c_DrawHeightOffset = 0.15f;
    constexpr float c_EndMarkerHeight = 1.f;
    constexpr float c_DashLength = 0.4f;

    inline SVector3 RaiseAboveNavMesh(const SVector3& p_Point) {
        return SVector3(p_Point.x, p_Point.y, p_Point.z + c_DrawHeightOffset);
    }

    inline SVector4 WithAlpha(const SVector4& p_Color, float p_Alpha) {
        return SVector4(p_Color.x, p_Color.y, p_Color.z, p_Alpha);
    }

    inline void DrawLine(IRenderer* p_Renderer, const SVector3& p_From, const SVector3& p_To, const SVector4& p_Color) {
        p_Renderer->DrawLine3D(RaiseAboveNavMesh(p_From), RaiseAboveNavMesh(p_To), p_Color, p_Color);
    }

    inline void DrawPolyline(IRenderer* p_Renderer, const std::vector<SVector3>& p_Points, const SVector4& p_Color) {
        for (size_t i = 1; i < p_Points.size(); ++i) {
            DrawLine(p_Renderer, p_Points[i - 1], p_Points[i], p_Color);
        }
    }

    inline void DrawDashedLine(
        IRenderer* p_Renderer, const SVector3& p_From, const SVector3& p_To, const SVector4& p_Color
    ) {
        const SVector3 s_Delta = p_To - p_From;
        const int s_DashCount = (std::max)(1, static_cast<int>((p_To - p_From).Length() / c_DashLength));

        for (int i = 0; i < s_DashCount; i += 2) {
            const float s_Start = static_cast<float>(i) / s_DashCount;
            const float s_End = static_cast<float>(i + 1) / s_DashCount;

            DrawLine(
                p_Renderer,
                SVector3(p_From.x + s_Delta.x * s_Start, p_From.y + s_Delta.y * s_Start, p_From.z + s_Delta.z * s_Start),
                SVector3(p_From.x + s_Delta.x * s_End, p_From.y + s_Delta.y * s_End, p_From.z + s_Delta.z * s_End),
                p_Color
            );
        }
    }

    inline void DrawDashedPolyline(
        IRenderer* p_Renderer, const std::vector<SVector3>& p_Points, const SVector4& p_Color
    ) {
        for (size_t i = 1; i < p_Points.size(); ++i) {
            DrawDashedLine(p_Renderer, p_Points[i - 1], p_Points[i], p_Color);
        }
    }

    inline void DrawEndMarker(IRenderer* p_Renderer, const SVector3& p_Point, const SVector4& p_Color) {
        p_Renderer->DrawLine3D(
            p_Point, SVector3(p_Point.x, p_Point.y, p_Point.z + c_EndMarkerHeight), p_Color, p_Color
        );
    }
}
