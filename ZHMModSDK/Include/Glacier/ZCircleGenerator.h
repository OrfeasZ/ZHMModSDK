#pragma once

#include <numbers>

#include "ZMath.h"

class ZCircleGenerator {
public:
    void Init(
        const float4& p_Center,
        const float4& p_Size,
        const float4& p_Normal,
        const float4& p_StartDir,
        float p_Angle,
        float fStepsToACircle
    ) {
        m_vCenter = p_Center;

        m_vXAxis = p_StartDir * p_Size.x;

        const float4 s_Cross = float4::CrossProduct(p_Normal, p_StartDir);
        m_vYAxis = s_Cross * p_Size.y;

        m_v1 = m_vCenter + m_vXAxis;
        m_v2 = m_v1;

        m_vX.m4 = float4(1.f);
        m_vY.m4 = float4(0.f);

        m_nStep = 0;
        const float s_NumStepsFloat = std::abs(p_Angle * fStepsToACircle / (2.0f * std::numbers::pi_v<float>));
        const int32_t s_NumSteps = static_cast<int32_t>(s_NumStepsFloat);
        m_nNumSteps = s_NumSteps > 2 ? s_NumSteps : 3;

        float s_DeltaTheta = p_Angle / static_cast<float>(m_nNumSteps);
        float s_CosT = std::cos(s_DeltaTheta);
        float s_SinT = std::sin(s_DeltaTheta);

        m_vRow0 = float4(s_CosT, -s_SinT, 0.f, 0.f);
        m_vRow1 = float4(s_SinT, s_CosT, 0.f, 0.f);
    }

    bool MoveNext() {
        if (m_nStep == m_nNumSteps) {
            return false;
        }

        m_v1 = m_v2;
        m_nStep++;

        float s_X = m_vX.m4.x;
        float s_Y = m_vY.m4.x;

        float s_NewX = s_X * m_vRow0.x + s_Y * m_vRow0.y;
        float s_NewY = s_X * m_vRow1.x + s_Y * m_vRow1.y;

        m_vX.m4 = float4(s_NewX);
        m_vY.m4 = float4(s_NewY);

        m_v2 = m_vCenter + (m_vXAxis * s_NewX) + (m_vYAxis * s_NewY);

        return true;
    }

    float4 m_v1;
    float4 m_v2;

    float4 m_vCenter;

    uint32_t m_nStep;
    uint32_t m_nNumSteps;

    float4 m_vRow0;
    float4 m_vRow1;

    float1 m_vX;
    float1 m_vY;

    float4 m_vXAxis;
    float4 m_vYAxis;
};