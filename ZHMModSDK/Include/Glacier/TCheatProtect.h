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
    ZString GetValue() {
        const uint32_t s_Length = m_sScrambledString.size();

        std::vector<uint8_t> s_Buffer(s_Length);

        memcpy(s_Buffer.data(), m_sScrambledString.c_str(), s_Length);

        for (uint32_t i = 0; i < s_Length - 1; ++i) {
            s_Buffer[i] ^= static_cast<uint8_t>(s_Buffer[i + 1] + m_sScrambledString.m_nLength);
        }

        s_Buffer[s_Length - 1] ^= static_cast<uint8_t>(m_sScrambledString.m_nLength);

        return ZString::AllocateFromCStr(
            reinterpret_cast<const char*>(s_Buffer.data()), s_Length
        );
    }

    void SetValue(const ZString& s_Value) {
        const uint32_t s_Length = m_sScrambledString.size();

        std::vector<uint8_t> s_Buffer(s_Length);

        memcpy(s_Buffer.data(), s_Value.c_str(), s_Length);

        s_Buffer[s_Length - 1] ^= static_cast<uint8_t>(s_Value.m_nLength);

        for (uint32_t i = s_Length - 2; i >= 0; --i) {
            s_Buffer[i] ^= static_cast<uint8_t>(s_Buffer[i + 1] + s_Value.m_nLength);
        }

        m_sScrambledString = ZString::AllocateFromCStr(
            reinterpret_cast<const char*>(s_Buffer.data()), s_Length
        );
    }

    ZString m_sScrambledString;
};