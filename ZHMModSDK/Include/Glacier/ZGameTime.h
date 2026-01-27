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
    int64 m_nTicksPerSecond; // 0x8
    int64 m_nLastTimeTicks; // 0x10
    ZGameTime m_tGameTime; // 0x18
    ZGameTime m_tGameTimePrev; // 0x20
    ZGameTime m_tGameTimeDelta; // 0x28
    ZGameTime m_tRealTime; // 0x30
    ZGameTime m_tRealTimePrev; // 0x38
    ZGameTime m_tRealTimeDelta; // 0x40
    float m_fGameTimeMultiplier; // 0x48
    float m_fDebugTimeMultiplier; // 0x4C
    PAD(0x8); // 0x50
    ZGameTime m_FrameWait; // 0x58
    ZGameTime m_FrameStep; // 0x60
    ZGameTime m_FrameRemain; // 0x68
    bool m_bPaused; // 0x70
    uint32 m_nFrameCount; // 0x74
};