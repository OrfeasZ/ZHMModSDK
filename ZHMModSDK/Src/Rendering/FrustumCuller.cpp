#include "Rendering/FrustumCuller.h"

void FrustumCuller::Extract(const DirectX::XMMATRIX& viewProj) {
    DirectX::XMMATRIX m = DirectX::XMMatrixTranspose(viewProj);

    // Leva
    m_planes[0] = DirectX::XMVectorAdd(m.r[3], m.r[0]);
    // Desna
    m_planes[1] = DirectX::XMVectorSubtract(m.r[3], m.r[0]);
    // Dole
    m_planes[2] = DirectX::XMVectorAdd(m.r[3], m.r[1]);
    // Gore
    m_planes[3] = DirectX::XMVectorSubtract(m.r[3], m.r[1]);
    // Blizu
    m_planes[4] = DirectX::XMVectorAdd(m.r[3], m.r[2]);
    // Daleko
    m_planes[5] = DirectX::XMVectorSubtract(m.r[3], m.r[2]);

    // Normalizuj ravni
    for (int i = 0; i < 6; i++) {
        m_planes[i] = DirectX::XMPlaneNormalize(m_planes[i]);
    }
}

bool FrustumCuller::ContainsPoint(const DirectX::XMVECTOR& point) const {
    for (int i = 0; i < 6; i++) {
        if (DirectX::XMVectorGetX(DirectX::XMPlaneDotCoord(m_planes[i], point)) < 0.f) {
            return false;
        }
    }

    return true;
}
