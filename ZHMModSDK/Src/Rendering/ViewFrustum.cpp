#include "Glacier/ZCameraEntity.h"

#include "Rendering/ViewFrustum.h"
#include "Functions.h"

#undef min
#undef max

void ViewFrustum::UpdateClipPlanes(const SMatrix& p_View, const SMatrix& p_Projection)
{
    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

    if (!s_CurrentCamera)
    {
        return;
    }

    if (m_MaxDrawDistance > 0.f)
    {
        const float s_FovYDeg = s_CurrentCamera->GetFovYDeg();
        const float s_AspectWByH = s_CurrentCamera->GetAspectWByH();
        const float s_NearZ = s_CurrentCamera->GetNearZ();

        if (s_FovYDeg != m_FovYDeg ||
            s_AspectWByH != m_AspectWByH ||
            s_NearZ != m_NearZ)
        {
            m_FovYDeg = s_FovYDeg;
            m_AspectWByH = s_AspectWByH;
            m_NearZ = s_NearZ;

            m_ClipPlaneProjectionMatrix = MatrixPerspectiveFovRH(m_FovYDeg, m_AspectWByH, m_NearZ, m_MaxDrawDistance);
        }
    }
    else
    {
        m_ClipPlaneProjectionMatrix = p_Projection;
    }

    const DirectX::FXMMATRIX s_View = *reinterpret_cast<DirectX::FXMMATRIX*>(&p_View);
    const DirectX::FXMMATRIX s_Projection = *reinterpret_cast<DirectX::FXMMATRIX*>(&m_ClipPlaneProjectionMatrix);

    MatrixCreateClipPlanesNormalized(m_Planes, s_View * s_Projection);
}

SMatrix ViewFrustum::MatrixPerspectiveFovRH(const float p_FovYDeg, const float p_AspectWByH, const float p_NearZ, const float p_FarZ) {
    const float s_Height = tanf(p_FovYDeg * 0.5f) * (p_NearZ * 2.f);
    const float s_Width = s_Height * p_AspectWByH;

    return MatrixPerspectiveRH(s_Width, s_Height, p_NearZ, p_FarZ);
}

SMatrix ViewFrustum::MatrixPerspectiveRH(const float p_Width, const float p_Height, const float p_NearZ, const float p_FarZ) {
    return SMatrix(
        { (p_NearZ * 2.f) / p_Width, 0.f, 0.f, 0.f },
        { 0.f, (p_NearZ * 2.f) / p_Height, 0.f, 0.f },
        { 0.f, 0.f, -p_FarZ / (p_FarZ - p_NearZ), -1.f },
        { 0.f, 0.f, -(p_FarZ * p_NearZ) / (p_FarZ - p_NearZ), 0.f }
    );
}

void ViewFrustum::MatrixCreateClipPlanes(float4* p_Planes, const SMatrix& p_ViewProjection)
{
    const SMatrix s_ViewProjection = p_ViewProjection.Transposed();

    static const float4 s_PlaneFront = { 0.f,  0.f, -1.f,  0.f };
    static const float4 s_PlaneBack = { 0.f,  0.f,  1.f, -1.f };
    static const float4 s_PlaneLeft = { -1.f,  0.f,  0.f, -1.f };
    static const float4 s_PlaneRight = { 1.f,  0.f,  0.f, -1.f };
    static const float4 s_PlaneBottom = { 0.f, -1.f,  0.f, -1.f };
    static const float4 s_PlaneTop = { 0.f,  1.f,  0.f, -1.f };

    p_Planes[0] = s_ViewProjection.WVectorTransformH(s_PlaneFront);
    p_Planes[1] = s_ViewProjection.WVectorTransformH(s_PlaneBack);
    p_Planes[2] = s_ViewProjection.WVectorTransformH(s_PlaneLeft);
    p_Planes[3] = s_ViewProjection.WVectorTransformH(s_PlaneRight);
    p_Planes[4] = s_ViewProjection.WVectorTransformH(s_PlaneBottom);
    p_Planes[5] = s_ViewProjection.WVectorTransformH(s_PlaneTop);
}

void ViewFrustum::MatrixCreateClipPlanesNormalized(float4* p_Planes, const SMatrix& p_ViewProjection)
{
    MatrixCreateClipPlanes(p_Planes, p_ViewProjection);

    for (int i = 0; i < 6; ++i)
    {
        p_Planes[i].Normalize();
    }
}

bool ViewFrustum::ContainsPoint(const SVector3& p_Point) const {
    return CheckPointInsidePlanes(p_Point) != ECheckInsideFlag::CHECK_INSIDE_FULLY_OUTSIDE;
}

bool ViewFrustum::ContainsAABB(const AABB& p_AABB) const {
    return CheckAABBInsidePlanes(p_AABB) != ECheckInsideFlag::CHECK_INSIDE_FULLY_OUTSIDE;
}

bool ViewFrustum::ContainsOBB(const SMatrix& p_Transform, const float4& p_Center, const float4& p_HalfSize) const
{
    return CheckOBBInsidePlanes(p_Transform, p_Center, p_HalfSize) != ECheckInsideFlag::CHECK_INSIDE_FULLY_OUTSIDE;
}

ViewFrustum::ECheckInsideFlag ViewFrustum::CheckPointInsidePlanes(const SVector3& p_Point) const
{
    float positiveExtentSum = 0.f;
    float negativeExtentSum = 0.f;

    for (const auto& plane : m_Planes)
    {
        const SVector3 normal(plane.x, plane.y, plane.z);

        const float distance =
            normal.x * p_Point.x +
            normal.y * p_Point.y +
            normal.z * p_Point.z +
            plane.w;

        positiveExtentSum += std::max(distance, 0.0f);
        negativeExtentSum += std::max(distance, 0.0f);
    }

    if (negativeExtentSum > 0.f) {
        return ECheckInsideFlag::CHECK_INSIDE_FULLY_OUTSIDE;
    }

    constexpr float epsilon = 1.f / 4096.f;

    return positiveExtentSum <= epsilon
        ? ECheckInsideFlag::CHECK_INSIDE_FULLY_INSIDE
        : ECheckInsideFlag::CHECK_INSIDE_PARTIALLY_INSIDE;
}

ViewFrustum::ECheckInsideFlag ViewFrustum::CheckAABBInsidePlanes(const AABB& p_AABB) const
{
    float positiveExtentSum = 0.f;
    float negativeExtentSum = 0.f;

    const SVector3 center = (p_AABB.min + p_AABB.max) * 0.5f;
    const SVector3 halfSize = (p_AABB.max - p_AABB.min) * 0.5f;

    for (const auto& plane : m_Planes)
    {
        const SVector3 normal(plane.x, plane.y, plane.z);

        const float distance =
            normal.x * center.x +
            normal.y * center.y +
            normal.z * center.z +
            plane.w;

        const float radius =
            std::fabs(normal.x) * halfSize.x +
            std::fabs(normal.y) * halfSize.y +
            std::fabs(normal.z) * halfSize.z;

        positiveExtentSum += std::max(distance + radius, 0.0f);
        negativeExtentSum += std::max(distance - radius, 0.0f);
    }

    if (negativeExtentSum > 0.f) {
        return ECheckInsideFlag::CHECK_INSIDE_FULLY_OUTSIDE;
    }

    constexpr float epsilon = 1.f / 4096.f;

    return positiveExtentSum <= epsilon
        ? ECheckInsideFlag::CHECK_INSIDE_FULLY_INSIDE
        : ECheckInsideFlag::CHECK_INSIDE_PARTIALLY_INSIDE;
}

ViewFrustum::ECheckInsideFlag ViewFrustum::CheckOBBInsidePlanes(const SMatrix& p_ObjectInternal, const float4& p_LocalCenter, const float4& p_LocalSize) const
{
    const float4 worldCenter = p_ObjectInternal.WVectorTransform(p_LocalCenter);

    const SVector3 axisX(p_ObjectInternal.XAxis.x, p_ObjectInternal.XAxis.y, p_ObjectInternal.XAxis.z);
    const SVector3 axisY(p_ObjectInternal.YAxis.x, p_ObjectInternal.YAxis.y, p_ObjectInternal.YAxis.z);
    const SVector3 axisZ(p_ObjectInternal.ZAxis.x, p_ObjectInternal.ZAxis.y, p_ObjectInternal.ZAxis.z);

    float positiveExtentSum = 0.f;
    float negativeExtentSum = 0.f;

    for (const auto& plane : m_Planes)
    {
        const SVector3 normal(plane.x, plane.y, plane.z);

        const float distance = normal.x * worldCenter.x +
            normal.y * worldCenter.y +
            normal.z * worldCenter.z +
            plane.w;

        const float radius = std::fabs(normal * axisX) * p_LocalSize.x +
            std::fabs(normal * axisY) * p_LocalSize.y +
            std::fabs(normal * axisZ) * p_LocalSize.z;

        positiveExtentSum += std::max(distance + radius, 0.0f);
        negativeExtentSum += std::max(distance - radius, 0.0f);
    }

    if (negativeExtentSum > 0.f) {
        return ECheckInsideFlag::CHECK_INSIDE_FULLY_OUTSIDE;
    }

    constexpr float epsilon = 1.f / 4096.f;

    return positiveExtentSum <= epsilon
        ? ECheckInsideFlag::CHECK_INSIDE_FULLY_INSIDE
        : ECheckInsideFlag::CHECK_INSIDE_PARTIALLY_INSIDE;
}
