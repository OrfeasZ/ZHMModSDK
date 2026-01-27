#pragma once

#include <cstdint>
#include <algorithm>

struct SColorRGBA {
    uint32_t GetAsUInt32() const {
        const uint32_t r = std::clamp(static_cast<int>(this->r * 256.0f), 0, 255);
        const uint32_t g = std::clamp(static_cast<int>(this->g * 256.0f), 0, 255);
        const uint32_t b = std::clamp(static_cast<int>(this->b * 256.0f), 0, 255);
        const uint32_t a = std::clamp(static_cast<int>(this->a * 256.0f), 0, 255);

        return (a << 24) | (b << 16) | (g << 8) | r; // 0xAABBGGRR
    }

    float r;
    float g;
    float b;
    float a;
};