#pragma once

#include "ZPrimitives.h"

template <typename T>
class TCheatProtect;

template <typename T>
class TCheatProtect {
public:
    struct SStorage {
        union {
            T m_tValue;
            uint8 m_aValueMemory[sizeof(T)];
        };

        union {
            uint32* m_piChecksum;
            uint8 m_aChecksumMemory[sizeof(uint32)];
        };
    };

    SStorage m_Storage;
};

template <>
class TCheatProtect<ZString> {
public:
    ZString m_sScrambledString;
};