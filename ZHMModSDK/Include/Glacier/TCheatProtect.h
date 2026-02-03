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
        const uint32_t s_Size = m_sScrambledString.size();

        if (s_Size == 0) {
            return ZString();
        }

        const uint32_t s_SizeWithFlags = m_sScrambledString.sizeWithFlags();

        std::vector<uint8_t> s_Buffer(s_Size);

        memcpy(s_Buffer.data(), m_sScrambledString.c_str(), s_Size);

        for (uint32_t i = 0; i < s_Size - 1; ++i) {
            s_Buffer[i] ^= static_cast<uint8_t>(s_Buffer[i + 1] + s_SizeWithFlags);
        }

        s_Buffer[s_Size - 1] ^= static_cast<uint8_t>(s_SizeWithFlags);

        return ZString::AllocateFromCStr(
            reinterpret_cast<const char*>(s_Buffer.data()), s_Size
        );
    }

    void SetValue(const ZString& s_Value) {
        const uint32_t s_Size = m_sScrambledString.size();

        if (s_Size == 0) {
            m_sScrambledString = ZString();
            return;
        }

        const uint32_t s_SizeWithFlags = m_sScrambledString.sizeWithFlags();

        std::vector<uint8_t> s_Buffer(s_Size);

        memcpy(s_Buffer.data(), s_Value.c_str(), s_Size);

        s_Buffer[s_Size - 1] ^= static_cast<uint8_t>(s_SizeWithFlags);

        for (int32_t i = static_cast<int32_t>(s_Size) - 2; i >= 0; --i) {
            s_Buffer[i] ^= static_cast<uint8_t>(s_Buffer[i + 1] + s_SizeWithFlags);
        }

        m_sScrambledString = ZString::AllocateFromCStr(
            reinterpret_cast<const char*>(s_Buffer.data()), s_Size
        );
    }

    ZString m_sScrambledString;
};