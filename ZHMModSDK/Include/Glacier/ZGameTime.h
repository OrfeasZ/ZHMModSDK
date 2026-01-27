#pragma once

#include "Reflection.h"

struct ZGameTime {
    int64_t m_nTicks;

    double ToSeconds() const {
        return static_cast<double>(m_nTicks) / 1024.0 / 1024.0;
    }
};

class ZGameTimeManager : public IComponentInterface {
public:
    int64 m_nTicksPerSecond;
    int64 m_nLastTimeTicks;
    ZGameTime m_tGameTime;
    ZGameTime m_tGameTimePrev;
    ZGameTime m_tGameTimeDelta;
    ZGameTime m_tRealTime;
    ZGameTime m_tRealTimePrev;
    ZGameTime m_tRealTimeDelta;
    float m_fGameTimeMultiplier;
    float m_fDebugTimeMultiplier;
    ZGameTime m_FrameWait;
    ZGameTime m_FrameStep;
    ZGameTime m_FrameRemain;
    bool m_bPaused;
    uint32 m_nFrameCount;
};