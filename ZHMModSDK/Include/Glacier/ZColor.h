#pragma once

#include "ZMath.h"

class ZColor {
public:
    static SVector4 UnpackUnsigned(const uint32 p_PackedVector4) {
        const uint32 s_Alpha = (p_PackedVector4 >> 24) & 0xFF;
        const uint32 s_Blue = (p_PackedVector4 >> 16) & 0xFF;
        const uint32 s_Green = (p_PackedVector4 >> 8) & 0xFF;
        const uint32 s_Red = p_PackedVector4 & 0xFF;

        return SVector4(
            static_cast<float>(s_Red) / 255.f,
            static_cast<float>(s_Green) / 255.f,
            static_cast<float>(s_Blue) / 255.f,
            static_cast<float>(s_Alpha) / 255.f
        );
    }
};