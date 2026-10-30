#pragma once

#include "ZObjectPool.h"

template <typename T>
class TObjectPool {
public:
    size_t IndexOf(T* p_Object) {
        return (reinterpret_cast<uintptr_t>(p_Object) - reinterpret_cast<uintptr_t>(m_pStart)) / sizeof(T);
    }

    bool Contains(const T* p_Object) const {
        return p_Object >= m_pStart && p_Object < m_pEnd;
    }

    ZObjectPool m_Pool;
    T* m_pStart;
    T* m_pEnd;
};