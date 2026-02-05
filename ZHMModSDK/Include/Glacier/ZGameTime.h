#pragma once

#include "Reflection.h"

class ZGameTime {
public:
    ZGameTime() = default;

    explicit ZGameTime(int64_t p_Ticks) {
        m_nTicks = p_Ticks;
    }

    explicit ZGameTime(float p_Seconds) {
        m_nTicks = static_cast<int64_t>(p_Seconds * (1 << FractionBits));
    }

    explicit ZGameTime(double p_Seconds) {
        m_nTicks = static_cast<int64_t>(p_Seconds * (1 << FractionBits));
    }

    explicit operator int64_t() const {
        return m_nTicks;
    }

    explicit operator float() const {
        return m_nTicks * TicksToSeconds;
    }

    ZGameTime operator+(const ZGameTime& p_Other) const {
        return ZGameTime(m_nTicks + p_Other.m_nTicks);
    }

    ZGameTime& operator+=(const ZGameTime& p_Other) {
        m_nTicks += p_Other.m_nTicks;

        return *this;
    }

    ZGameTime operator-(const ZGameTime& p_Other) const {
        return ZGameTime(m_nTicks - p_Other.m_nTicks);
    }

    ZGameTime& operator-=(const ZGameTime& p_Other) {
        m_nTicks -= p_Other.m_nTicks;

        return *this;
    }

    ZGameTime operator*(const ZGameTime& p_Other) const {
        return ZGameTime((m_nTicks * p_Other.m_nTicks) >> FractionBits);
    }

    ZGameTime& operator*=(const ZGameTime& p_Other) {
        m_nTicks = (m_nTicks * p_Other.m_nTicks) >> FractionBits;

        return *this;
    }

    ZGameTime operator/(const ZGameTime& p_Other) const {
        return ZGameTime((m_nTicks << FractionBits) / p_Other.m_nTicks);
    }

    ZGameTime& operator/=(const ZGameTime& p_Other) {
        m_nTicks = (m_nTicks << FractionBits) / p_Other.m_nTicks;

        return *this;
    }

    bool operator<(const ZGameTime& rhs) const {
        return m_nTicks < rhs.m_nTicks;
    }

    static ZGameTime FromSeconds(float p_Seconds) {
        return ZGameTime(static_cast<int64_t>(p_Seconds * (1 << FractionBits)));
    }

    static ZGameTime FromSeconds(double p_Seconds) {
        return ZGameTime(static_cast<int64_t>(p_Seconds * (1 << FractionBits)));
    }

    double ToSeconds() const {
        return static_cast<double>(m_nTicks) / (1 << FractionBits);
    }

    int64_t m_nTicks;

    static constexpr int FractionBits = 20;
    static constexpr float TicksToSeconds = 1.0f / (1 << FractionBits);
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