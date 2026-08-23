#pragma once

#include <cstdint>

#include "ZVirtualMemory.h"

template <typename T>
class ZInfiniteBuffer {
public:
    void Resize(uint32_t p_NewSize) {
        m_nSize = p_NewSize;

        if (p_NewSize > m_nActualSize) {
            AdjustAllocation();
        }
    }

    void AdjustAllocation() {
        const uint32_t s_PageSize = ZVirtualMemory::PageSize(m_PageSize);
        const uint32_t s_NewActualSize = (m_nSize + s_PageSize - 1) & -s_PageSize;

        if (s_NewActualSize < m_nActualSize) {
            ZVirtualMemory::Release(
                m_pData,
                s_NewActualSize,
                m_nActualSize - s_NewActualSize,
                m_PageSize
            );
        }
        else if (s_NewActualSize > m_nActualSize) {
            ZVirtualMemory::Commit(
                m_pData,
                m_nActualSize,
                s_NewActualSize - m_nActualSize,
                m_PageSize
            );
        }

        m_nActualSize = s_NewActualSize;
    }

    T* m_pData; // 0x0
    uint32_t m_nSize; // 0x8
    uint32_t m_nActualSize; // 0xC
    uint32_t m_nMaxSize; // 0x10
    ZVirtualMemory::EVirtualMemoryPageSize m_PageSize; // 0x14
};