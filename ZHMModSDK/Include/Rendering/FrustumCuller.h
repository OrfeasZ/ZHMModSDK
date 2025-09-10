#pragma once

#include <DirectXMath.h>

#include "Common.h"

class ZHMSDK_API FrustumCuller {
public:
    void Extract(const DirectX::XMMATRIX& viewProj);
    bool ContainsPoint(const DirectX::XMVECTOR& point) const;

private:
    DirectX::XMVECTOR m_planes[6];
};
