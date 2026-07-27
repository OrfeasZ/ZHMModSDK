#pragma once

#include "ZPrimitives.h"

class ZSparseBitArray {
public:
    class Iterator {
    public:
        Iterator(const ZSparseBitArray* p_Mask)
            : m_BitMask(p_Mask) {
            m_nIndex = p_Mask->m_nSize > 0 ? p_Mask->NextBit(-1) : p_Mask->m_nSize;
        }

        Iterator(const ZSparseBitArray* p_Mask, int32 p_Index)
            : m_BitMask(p_Mask) {
            m_nIndex = p_Index < static_cast<int32>(p_Mask->m_nSize) ? p_Mask->NextBit(p_Index) : p_Mask->m_nSize;
        }

        bool operator!=(const Iterator& p_Other) const {
            return m_nIndex != p_Other.m_nIndex;
        }

        int operator*() const {
            return m_nIndex;
        }

        Iterator& operator++() {
            if (m_nIndex != m_BitMask->m_nSize) {
                m_nIndex = m_BitMask->NextBitFast(m_nIndex);
            }

            return *this;
        }

        const ZSparseBitArray* m_BitMask;
        int32 m_nIndex;
    };

    Iterator begin() const {
        return Iterator(this, m_nSize > 0 ? NextBit(-1) : m_nSize);
    }

    Iterator end() const {
        return Iterator(this, m_nSize);
    }

    int32 SkipStencil(int32 p_Index) const {
        const int32 BITS_PER_BLOCK = 64;
        const int32 STENCIL_GRANULARITY = 128;

        // 1. Calculate which 64-bit block and which bit inside that block we are targeting
        int32 s_StencilBitIndex = p_Index / STENCIL_GRANULARITY;
        int32 s_BlockIdx = s_StencilBitIndex / BITS_PER_BLOCK;
        int32 s_BitOffset = s_StencilBitIndex % BITS_PER_BLOCK;

        int32 s_StencilBlockCount = (m_nStencilSize / BITS_PER_BLOCK) + 1;

        // Boundary check
        if (s_BlockIdx >= s_StencilBlockCount) {
            return m_nSize;
        }

        uint64 s_CurrentBlock = m_pStencil[s_BlockIdx];

        // 2. Clear bits lower than our current bitOffset to find the "next" available bit
        uint64 s_Mask = ~((1ULL << s_BitOffset) - 1);
        uint64 s_RemainingBits = s_CurrentBlock & s_Mask;

        // 3. Search for the next set bit
        unsigned long s_FoundBitPos = 0;

        // Check current block first
        if (s_RemainingBits != 0) {
            _BitScanForward64(&s_FoundBitPos, s_RemainingBits);
        }
        else {
            // 4. If current block is empty, scan subsequent blocks
            int32 s_NextBlockIdx = s_BlockIdx + 1;

            while (s_NextBlockIdx < s_StencilBlockCount && m_pStencil[s_NextBlockIdx] == 0) {
                s_NextBlockIdx++;
            }

            if (s_NextBlockIdx >= s_StencilBlockCount) {
                return m_nSize; // Reached the end
            }

            s_BlockIdx = s_NextBlockIdx;

            _BitScanForward64(&s_FoundBitPos, m_pStencil[s_BlockIdx]);
        }

        // 5. Calculate the final index
        // The result is ((BlockIndex * 64) + BitPosition) * 128
        return static_cast<int32>(
            (static_cast<int64>(s_BlockIdx) * BITS_PER_BLOCK + s_FoundBitPos) * STENCIL_GRANULARITY
        );
    }

    /**
     * Standard NextBit: Uses SkipStencil immediately to find the next
     * potentially occupied region before starting the search.
     */
    int32 NextBit(int32 p_Index) const {
        // Start by leaping over empty regions using the stencil
        int32 s_CurrentIndex = SkipStencil(p_Index + 1);

        for (int32 i = s_CurrentIndex; ; i = (i + 64) & ~63) {
            // Every 128-bit boundary, check if we can skip ahead
            if ((i % 128) == 0) {
                i = SkipStencil(i);
            }

            if (i == m_nSize) {
                return i;
            }

            // Check the current 64-bit word in the mask
            uint64 s_Word = m_pMask[i / 64] >> (i % 64);
            unsigned long s_BitPos;

            if (_BitScanForward64(&s_BitPos, s_Word)) {
                return i + static_cast<int32>(s_BitPos);
            }
        }
    }

    /**
     * NextBitFast: Begins searching from the very next bit without an
     * initial stencil skip. Ideal for dense bit arrays.
     */
    int32 NextBitFast(int32 p_Index) const {
        // Start scanning from the next bit position immediately
        for (int32 i = p_Index + 1; ; i = (i + 64) & ~63) {
            // Still check stencil at 128-bit boundaries to maintain skip logic
            if ((i % 128) == 0) {
                i = SkipStencil(i);
            }

            if (i == m_nSize) {
                return i;
            }

            uint64_t s_Word = m_pMask[i / 64] >> (i % 64);
            unsigned long s_BitPos;

            if (_BitScanForward64(&s_BitPos, s_Word)) {
                return i + static_cast<int32>(s_BitPos);
            }
        }
    }

    uint64* m_pStencil;
    uint64* m_pMask;
    uint32 m_nSize;
    uint32 m_nStencilSize;
    uint32 m_nAllocatedSize;
};