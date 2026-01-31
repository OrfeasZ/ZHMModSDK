#pragma once

#include "TArray.h"
#include "ZPrimitives.h"

class ZBitArray {
public:
    bool Get(unsigned int nBitIndex) const {
        const unsigned int byteIndex = nBitIndex >> 3;
        const unsigned int bitOffset = nBitIndex & 7;

        return (1 << bitOffset) & m_aBytes[byteIndex];
    }

    TArray<uint8> m_aBytes;
    uint32 m_nSize;
};