#pragma once

#include <cstdint>

template <typename T>
struct SSharedValueContainer {
    uint32_t m_nRefCount;
    T m_value;
};

template <typename T>
class TSharedValue {
public:
    SSharedValueContainer<T>* m_pValue;
};