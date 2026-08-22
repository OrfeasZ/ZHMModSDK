#pragma once

#include <cstdint>

template <typename T>
class ZInfiniteBuffer {
public:
    T* m_pData;
    uint32_t m_nSize;
    uint32_t m_nActualSize;
    uint32_t m_nMaxSize;
};