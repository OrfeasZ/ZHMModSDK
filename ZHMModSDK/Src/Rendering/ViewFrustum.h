#pragma once

#include <DirectXMath.h>

#include <Glacier/ZMath.h>

#include "Common.h"
#include "IRenderer.h"

class ZHMSDK_API ViewFrustum {
public:
    void UpdateClipPlanes(const SMatrix& p_View, const SMatrix& p_Projection);
    bool ContainsPoint(const SVector3& p_Point) const;
    bool ContainsAABB(const AABB& p_AABB) const;
    bool ContainsOBB(const SMatrix& p_Transform, const float4& p_Center, const float4& p_HalfSize) const;
    bool ContainsSphere(const SVector3& p_Center, const float p_Radius) const;

    void SetDistanceCullingEnabled(const bool p_Enabled);
    bool IsDistanceCullingEnabled() const;

    void SetMaxDrawDistance(float p_MaxDrawDistance);
    float GetMaxDrawDistance() const;

private:
    enum class ContainmentType {
        FullyOutside,
        PartiallyInside,
        FullyInside,
        Unknown
    };

    SMatrix MatrixPerspectiveFovRH(
        float p_FovYDeg, float p_AspectWByH, float p_NearZ, float p_FarZ
    );
    SMatrix MatrixPerspectiveRH(float p_Width, float p_Height, float p_NearZ, float p_FarZ);
    void MatrixCreateClipPlanes(float4* p_Planes, const SMatrix& p_ViewProjection);
    void MatrixCreateClipPlanesNormalized(float4* p_Planes, const SMatrix& p_ViewProjection);

    ContainmentType CheckPointInsidePlanes(const SVector3& p_Point) const;
    ContainmentType CheckAABBInsidePlanes(const AABB& p_AABB) const;
    ContainmentType CheckOBBInsidePlanes(
        const SMatrix& p_Transform,
        const float4& p_Center,
        const float4& p_HalfSize
    ) const;
    ContainmentType CheckSphereInsidePlanes(const SVector3& p_Center, float p_Radius) const;

    bool m_IsDistanceCullingEnabled = false;
    float m_MaxDrawDistance = 50.f;
    SMatrix m_ClipPlaneProjectionMatrix;
    float4 m_Planes[6];
    float m_FovYDeg = 0.f;
    float m_AspectWByH = 0.f;
    float m_NearZ = 0.f;
    float m_FarZ = -1.f;
};