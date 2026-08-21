#include "Glacier/ZCameraEntity.h"

#include "Rendering/ViewFrustum.h"
#include "Functions.h"

#undef min
#undef max

void ViewFrustum::UpdateClipPlanes(const SMatrix& p_View, const SMatrix& p_Projection) {
    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

    if (!s_CurrentCamera) {
        return;
    }

    if (m_IsDistanceCullingEnabled && m_MaxDrawDistance > 0.f) {
        const float s_FovYDeg = s_CurrentCamera->GetFovYDeg();
        const float s_AspectWByH = s_CurrentCamera->GetAspectWByH();
        const float s_NearZ = s_CurrentCamera->GetNearZ();

        if (s_FovYDeg != m_FovYDeg ||
            s_AspectWByH != m_AspectWByH ||
            s_NearZ != m_NearZ ||
            m_FarZ != m_MaxDrawDistance) {
            m_FovYDeg = s_FovYDeg;
            m_AspectWByH = s_AspectWByH;
            m_NearZ = s_NearZ;
            m_FarZ = m_MaxDrawDistance;

            m_ClipPlaneProjectionMatrix = MatrixPerspectiveFovRH(m_FovYDeg, m_AspectWByH, m_NearZ, m_MaxDrawDistance);
        }
    }
    else {
        m_FarZ = -1.f;
        m_ClipPlaneProjectionMatrix = p_Projection;
    }

    const DirectX::FXMMATRIX s_View = *reinterpret_cast<DirectX::FXMMATRIX*>(&p_View);
    const DirectX::FXMMATRIX s_Projection = *reinterpret_cast<DirectX::FXMMATRIX*>(&m_ClipPlaneProjectionMatrix);

    MatrixCreateClipPlanesNormalized(m_Planes, s_View * s_Projection);
}

SMatrix ViewFrustum::MatrixPerspectiveFovRH(
    const float p_FovYDeg, const float p_AspectWByH, const float p_NearZ, const float p_FarZ
) {
    const float s_FovYRad = DirectX::XMConvertToRadians(p_FovYDeg);
    const float s_Height = std::tan(s_FovYRad * 0.5f) * (p_NearZ * 2.f);
    const float s_Width = s_Height * p_AspectWByH;

    return MatrixPerspectiveRH(s_Width, s_Height, p_NearZ, p_FarZ);
}

SMatrix ViewFrustum::MatrixPerspectiveRH(
    const float p_Width, const float p_Height, const float p_NearZ, const float p_FarZ
) {
    return SMatrix(
        { (p_NearZ * 2.f) / p_Width, 0.f, 0.f, 0.f },
        { 0.f, (p_NearZ * 2.f) / p_Height, 0.f, 0.f },
        { 0.f, 0.f, -p_FarZ / (p_FarZ - p_NearZ), -1.f },
        { 0.f, 0.f, -(p_FarZ * p_NearZ) / (p_FarZ - p_NearZ), 0.f }
    );
}

void ViewFrustum::MatrixCreateClipPlanes(float4* p_Planes, const SMatrix& p_ViewProjection) {
    const SMatrix s_ViewProjection = p_ViewProjection.Transposed();

    static const float4 s_PlaneFront = { 0.f, 0.f, -1.f, 0.f };
    static const float4 s_PlaneBack = { 0.f, 0.f, 1.f, -1.f };
    static const float4 s_PlaneLeft = { -1.f, 0.f, 0.f, -1.f };
    static const float4 s_PlaneRight = { 1.f, 0.f, 0.f, -1.f };
    static const float4 s_PlaneBottom = { 0.f, -1.f, 0.f, -1.f };
    static const float4 s_PlaneTop = { 0.f, 1.f, 0.f, -1.f };

    p_Planes[0] = s_ViewProjection.WVectorTransformH(s_PlaneFront);
    p_Planes[1] = s_ViewProjection.WVectorTransformH(s_PlaneBack);
    p_Planes[2] = s_ViewProjection.WVectorTransformH(s_PlaneLeft);
    p_Planes[3] = s_ViewProjection.WVectorTransformH(s_PlaneRight);
    p_Planes[4] = s_ViewProjection.WVectorTransformH(s_PlaneBottom);
    p_Planes[5] = s_ViewProjection.WVectorTransformH(s_PlaneTop);
}

void ViewFrustum::MatrixCreateClipPlanesNormalized(float4* p_Planes, const SMatrix& p_ViewProjection) {
    MatrixCreateClipPlanes(p_Planes, p_ViewProjection);

    for (int i = 0; i < 6; ++i) {
        p_Planes[i].Normalize();
    }
}

bool ViewFrustum::ContainsPoint(const SVector3& p_Point) const {
    return CheckPointInsidePlanes(p_Point) != ContainmentType::FullyOutside;
}

bool ViewFrustum::ContainsAABB(const AABB& p_AABB) const {
    return CheckAABBInsidePlanes(p_AABB) != ContainmentType::FullyOutside;
}

bool ViewFrustum::ContainsOBB(const SMatrix& p_Transform, const float4& p_Center, const float4& p_HalfSize) const {
    return CheckOBBInsidePlanes(p_Transform, p_Center, p_HalfSize) != ContainmentType::FullyOutside;
}

bool ViewFrustum::ContainsSphere(const SVector3& p_Center, const float p_Radius) const {
    return CheckSphereInsidePlanes(p_Center, p_Radius) != ContainmentType::FullyOutside;
}

void ViewFrustum::SetDistanceCullingEnabled(const bool p_Enabled) {
    m_IsDistanceCullingEnabled = p_Enabled;
}

bool ViewFrustum::IsDistanceCullingEnabled() const {
    return m_IsDistanceCullingEnabled;
}

void ViewFrustum::SetMaxDrawDistance(const float p_MaxDrawDistance) {
    m_MaxDrawDistance = p_MaxDrawDistance;
}

float ViewFrustum::GetMaxDrawDistance() const {
    return m_MaxDrawDistance;
}

ViewFrustum::ContainmentType ViewFrustum::CheckPointInsidePlanes(const SVector3& p_Point) const {
    constexpr float s_Epsilon = 1.f / 4096.f;

    for (const auto& s_Plane : m_Planes) {
        const SVector3 s_Normal = s_Plane;

        const float s_Distance =
            s_Normal.x * p_Point.x +
            s_Normal.y * p_Point.y +
            s_Normal.z * p_Point.z +
            s_Plane.w;

        if (s_Distance > s_Epsilon) {
            return ContainmentType::FullyOutside;
        }
    }

    return ContainmentType::FullyInside;
}

ViewFrustum::ContainmentType ViewFrustum::CheckAABBInsidePlanes(const AABB& p_AABB) const {
    const SVector3 s_Center = (p_AABB.min + p_AABB.max) * 0.5f;
    const SVector3 s_HalfSize = (p_AABB.max - p_AABB.min) * 0.5f;

    bool s_IsPartiallyInside = false;

    constexpr float s_Epsilon = 1.f / 4096.f;

    for (const auto& s_Plane : m_Planes) {
        const SVector3 s_Normal = s_Plane;

        const float s_Distance =
            s_Normal.x * s_Center.x +
            s_Normal.y * s_Center.y +
            s_Normal.z * s_Center.z +
            s_Plane.w;

        const float s_Radius =
            std::fabs(s_Normal.x) * s_HalfSize.x +
            std::fabs(s_Normal.y) * s_HalfSize.y +
            std::fabs(s_Normal.z) * s_HalfSize.z;

        if (s_Distance - s_Radius > s_Epsilon) {
            return ContainmentType::FullyOutside;
        }

        if (s_Distance + s_Radius > 0.f) {
            s_IsPartiallyInside = true;
        }
    }

    return s_IsPartiallyInside ? ContainmentType::PartiallyInside : ContainmentType::FullyInside;
}

ViewFrustum::ContainmentType ViewFrustum::CheckOBBInsidePlanes(
    const SMatrix& p_Transform,
    const float4& p_Center,
    const float4& p_HalfSize
) const {
    const SVector3 s_AxisX = p_Transform.XAxis;
    const SVector3 s_AxisY = p_Transform.YAxis;
    const SVector3 s_AxisZ = p_Transform.ZAxis;

    bool s_IsPartiallyInside = false;

    constexpr float s_Epsilon = 1.f / 4096.f;

    for (const auto& s_Plane : m_Planes) {
        const SVector3 s_Normal = s_Plane;

        const float s_Distance =
            s_Normal.x * p_Center.x +
            s_Normal.y * p_Center.y +
            s_Normal.z * p_Center.z +
            s_Plane.w;

        const float s_Radius =
            std::fabs(s_Normal * s_AxisX) * p_HalfSize.x +
            std::fabs(s_Normal * s_AxisY) * p_HalfSize.y +
            std::fabs(s_Normal * s_AxisZ) * p_HalfSize.z;

        if (s_Distance - s_Radius > s_Epsilon) {
            return ContainmentType::FullyOutside;
        }

        if (s_Distance + s_Radius > 0.f) {
            s_IsPartiallyInside = true;
        }
    }

    return s_IsPartiallyInside ? ContainmentType::PartiallyInside : ContainmentType::FullyInside;
}

ViewFrustum::ContainmentType ViewFrustum::CheckSphereInsidePlanes(const SVector3& p_Center, const float p_Radius) const {
    bool s_IsPartiallyInside = false;

    for (const auto& s_Plane : m_Planes) {
        const SVector3 s_Normal = s_Plane;

        const float s_Distance =
            s_Normal.x * p_Center.x +
            s_Normal.y * p_Center.y +
            s_Normal.z * p_Center.z +
            s_Plane.w;

        constexpr float s_Epsilon = 1.f / 4096.f;

        if (s_Distance - p_Radius > s_Epsilon) {
            return ContainmentType::FullyOutside;
        }

        if (s_Distance + p_Radius > 0.f) {
            s_IsPartiallyInside = true;
        }
    }

    return s_IsPartiallyInside ? ContainmentType::PartiallyInside : ContainmentType::FullyInside;
}