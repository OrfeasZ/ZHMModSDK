#pragma once

#include "Reflection.h"

struct ZGameTime
{
    int64_t m_nTicks;

    double ToSeconds() const
    {
        return static_cast<double>(m_nTicks) / 1024.0 / 1024.0;
    }
};

class ZGameTimeManager :
    public IComponentInterface
{
public:
    int64_t m_unk0x8;
    ZGameTime m_tSystemTime;
    ZGameTime m_tLevelTime;
    ZGameTime m_tLastLevelTime;
    ZGameTime m_tLevelTimeDelta;
    ZGameTime m_tRealTime;
    ZGameTime m_tLastRealTime;
    ZGameTime m_tRealTimeDelta;
    float m_fTimeMultiplier0;
    float m_fTimeMultiplier1;
    bool m_unk0x50;
    PAD(0x1F);
    bool m_bPaused; // 0x70
    uint32_t m_nRenderedFrames;
};
