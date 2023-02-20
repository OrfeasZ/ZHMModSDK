#pragma once

#include "ZGameTime.h"

class SGameUpdateEvent
{
public:
    ZGameTime m_GameTimeDelta;
    ZGameTime m_GameTimeCurrent;
    ZGameTime m_GameTimePrevious;
    ZGameTime m_RealTimeDelta;
    ZGameTime m_RealTimeCurrent;
    ZGameTime m_RealTimePrevious;
    float m_fGameTimeMultiplier;
};
