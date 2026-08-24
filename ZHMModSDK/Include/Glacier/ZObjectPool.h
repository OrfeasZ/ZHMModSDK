#pragma once

#include "ZPrimitives.h"
#include "ZMutex.h"
#include "ZInfiniteBuffer.h"

class ZObjectPool {
public:
    void* Alloc() {
        while (true) {
            int64_t s_FreeListStart = _InterlockedExchangeAdd64(&m_nFreeListStart, 0);

            if (static_cast<uint32_t>(s_FreeListStart) == UINT32_MAX) {
                m_Mutex.Lock();

                if (_InterlockedExchangeAdd64(&m_nFreeListStart, 0) == s_FreeListStart) {
                    const uint32_t s_OldSize = m_nSize;
                    const uint32_t s_NewSize = s_OldSize + m_nGrowCount * m_nObjectSize;

                    if (s_NewSize > m_Buffer.m_nMaxSize) {
                        std::exit(-1);
                    }

                    m_Buffer.Resize(s_NewSize);

                    const uint32_t s_FirstIndex = s_OldSize >> 2;
                    const uint32_t s_LastIndex =
                        s_FirstIndex + m_nObjectDelta * (m_nGrowCount - 1);

                    for (
                        uint32_t s_Index = s_FirstIndex;
                        s_Index < s_LastIndex;
                        s_Index += m_nObjectDelta
                    ) {
                        m_pData[s_Index] = s_Index + m_nObjectDelta;
                    }

                    m_nSize = s_NewSize;

                    while (true) {
                        const int64_t s_OldFreeListStart =
                            _InterlockedExchangeAdd64(&m_nFreeListStart, 0);

                        m_pData[s_LastIndex] =
                            static_cast<uint32_t>(s_OldFreeListStart);

                        const int64_t s_NewFreeListStart =
                            static_cast<int64_t>(
                                (static_cast<uint64_t>(s_OldFreeListStart) +
                                    0x100000000ULL) &
                                0xFFFFFFFF00000000ULL
                            ) |
                            s_FirstIndex;

                        if (_InterlockedCompareExchange64(
                            &m_nFreeListStart,
                            s_NewFreeListStart,
                            s_OldFreeListStart
                        ) == s_OldFreeListStart) {
                            break;
                        }
                    }
                }

                m_Mutex.Unlock();
                continue;
            }

            const uint32_t s_Index = static_cast<uint32_t>(s_FreeListStart);
            const uint32_t s_NextIndex = m_pData[s_Index];

            const int64_t s_NewFreeListStart =
                static_cast<int64_t>(
                    (static_cast<uint64_t>(s_FreeListStart) +
                        0x100000000ULL) &
                    0xFFFFFFFF00000000ULL
                ) |
                s_NextIndex;

            if (_InterlockedCompareExchange64(
                &m_nFreeListStart,
                s_NewFreeListStart,
                s_FreeListStart
            ) == s_FreeListStart) {
                return &m_pData[s_Index];
            }
        }
    }

    void Free(void* p_Object) {
        const uint32_t s_Index = static_cast<uint32_t>(
            (reinterpret_cast<uintptr_t>(p_Object) -
                reinterpret_cast<uintptr_t>(m_pData)) >> 2
        );

        while (true) {
            const int64_t s_FreeListStart = _InterlockedExchangeAdd64(&m_nFreeListStart, 0);

            *static_cast<uint32_t*>(p_Object) = static_cast<uint32_t>(s_FreeListStart);

            const int64_t s_NewFreeListStart =
                static_cast<int64_t>(
                    (static_cast<uint64_t>(s_FreeListStart) + 0x100000000ULL) &
                    0xFFFFFFFF00000000ULL
                ) |
                s_Index;

            if (_InterlockedCompareExchange64(
                &m_nFreeListStart,
                s_NewFreeListStart,
                s_FreeListStart
            ) == s_FreeListStart) {
                return;
            }
        }
    }

    bool Contains(const void* p_Object) const {
        return m_pData &&
            reinterpret_cast<uintptr_t>(p_Object) -
            reinterpret_cast<uintptr_t>(m_Buffer.m_pData) <
            m_Buffer.m_nActualSize;
    }

    volatile int64_t m_nFreeListStart;
    uint32_t* m_pData;
    uint32_t m_nObjectSize;
    uint32_t m_nMaxObjectCount;
    uint32_t m_nGrowCount;
    uint32_t m_nObjectDelta;
    uint32_t m_nSize;
    ZInfiniteBuffer<void> m_Buffer;
    ZMutex m_Mutex;
};

static_assert(sizeof(ZObjectPool) == 112);
static_assert(offsetof(ZObjectPool, m_pData) == 8);
static_assert(offsetof(ZObjectPool, m_Buffer.m_nActualSize) == 52);