#pragma once

#include <Windows.h>
#include <cstdint>

class ZMutex {
public:
    void Lock() {
        EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(this));
    }

    void Unlock() {
        LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(this));
    }

    uint64_t m_impl[5];
    uint32_t m_nUniqueID;
};