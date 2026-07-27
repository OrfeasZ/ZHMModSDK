#pragma once

#include "ZPrimitives.h"
#include "Hash.h"

template <typename T>
class TCheatProtect;

template <typename T>
class TCheatProtect {
    union SChecksum {
        uint32_t m_nChecksum;
        uint8_t m_ChecksumKey;
    };

    struct SStorage {
        union {
            T m_tValue;
            uint8_t m_ValueKey;
        };

        SChecksum* m_piChecksum;
    };

public:
    TCheatProtect() = delete;
    TCheatProtect(const TCheatProtect& p_Other) = delete;
    TCheatProtect(TCheatProtect&& p_Other) = delete;

    TCheatProtect& operator=(const TCheatProtect& p_Other) = delete;
    TCheatProtect& operator=(TCheatProtect&& p_Other) = delete;

    operator T() const {
        return Get();
    }

    TCheatProtect& operator=(T p_Value) {
        Set(p_Value);
        return *this;
    }

    [[nodiscard]] T Get() const {
        uint8_t s_Decrypted[sizeof(TCheatProtect)];
        memcpy(s_Decrypted, this, sizeof(TCheatProtect));
        const auto s_DecryptedStorage = reinterpret_cast<SStorage*>(s_Decrypted);

        const auto s_Checksum = GetChecksum();

        // Unscramble the value and its salt.
        for (size_t i = 0; i < offsetof(SStorage, m_piChecksum); ++i) {
            if (i < offsetof(SStorage, m_piChecksum) - 1) {
                s_Decrypted[i] ^= s_Checksum->m_ChecksumKey + s_Decrypted[i + 1];
            }
            else {
                s_Decrypted[i] ^= s_Checksum->m_ChecksumKey;
            }
        }

        #if _DEBUG
        const auto s_ExpectedChecksum = s_Checksum->m_nChecksum;
        const auto s_ActualChecksum = Hash::Fnv1a(
            reinterpret_cast<const char*>(s_Decrypted), offsetof(SStorage, m_piChecksum)
        );

        assert(s_ExpectedChecksum == s_ActualChecksum);
        #endif

        return s_DecryptedStorage->m_tValue;
    }

    void Set(T p_Value) {
        SStorage s_Storage {
            .m_tValue = p_Value
        };

        // Set the last byte before the checksum to 0, since it's
        // XORd with itself during decryption. This allows us to
        // calculate a checksum and then use its first byte as the
        // encryption key.
        reinterpret_cast<uint8_t*>(&s_Storage)[offsetof(SStorage, m_piChecksum) - 1] = 0;

        // Calculate the checksum.
        const auto s_Checksum = Hash::Fnv1a(
            reinterpret_cast<const char*>(&s_Storage), offsetof(SStorage, m_piChecksum)
        );

        // Rewrite the checksum value.
        const auto s_ChecksumPtr = GetChecksum();
        s_ChecksumPtr->m_nChecksum = s_Checksum;

        // Now write the pointer to the storage and scramble everything.
        s_Storage.m_piChecksum = s_ChecksumPtr;

        // Write the encryption key.
        reinterpret_cast<uint8_t*>(&s_Storage)[offsetof(SStorage, m_piChecksum) - 1] = s_ChecksumPtr->m_ChecksumKey;

        // Scramble the value and its accompanying data.
        for (int i = offsetof(SStorage, m_piChecksum) - 2; i >= 0; --i) {
            reinterpret_cast<uint8_t*>(&s_Storage)[i] ^= s_ChecksumPtr->m_ChecksumKey +
                    reinterpret_cast<uint8_t*>(&s_Storage)[i + 1];
        }

        // And then scramble the pointer.
        for (int i = sizeof(TCheatProtect) - 1; i >= offsetof(SStorage, m_piChecksum); --i) {
            if (i < sizeof(TCheatProtect) - 1) {
                reinterpret_cast<uint8_t*>(&s_Storage)[i] ^= s_Storage.m_ValueKey +
                        reinterpret_cast<uint8_t*>(&s_Storage)[i + 1];
            }
            else {
                reinterpret_cast<uint8_t*>(&s_Storage)[i] ^= s_Storage.m_ValueKey;
            }
        }

        m_Storage = s_Storage;

        #if _DEBUG
        // Finally, verify that the resulting value is encrypted correctly.
        assert(Get() == p_Value);
        #endif
    }

private:
    SChecksum* GetChecksum() const {
        uint8_t s_Decrypted[sizeof(TCheatProtect)];
        memcpy(s_Decrypted, this, sizeof(TCheatProtect));
        const auto s_DecryptedStorage = reinterpret_cast<SStorage*>(s_Decrypted);

        // Unscramble the checksum pointer.
        for (size_t i = offsetof(SStorage, m_piChecksum); i < sizeof(TCheatProtect); ++i) {
            if (i < sizeof(TCheatProtect) - 1) {
                s_Decrypted[i] ^= m_Storage.m_ValueKey + s_Decrypted[i + 1];
            }
            else {
                s_Decrypted[i] ^= m_Storage.m_ValueKey;
            }
        }

        return s_DecryptedStorage->m_piChecksum;
    }

    SStorage m_Storage;

    static_assert(
        offsetof(SStorage, m_piChecksum) != sizeof(T),
        "TCheatProtect doesn't currently support types larger than 7 bytes."
    );
};

template <>
class TCheatProtect<ZString> {
public:
    operator ZString() const {
        return Get();
    }

    TCheatProtect& operator=(const ZString& p_Value) {
        Set(p_Value);
        return *this;
    }

    [[nodiscard]] ZString Get() const {
        const uint32_t s_Size = m_sScrambledString.size();

        if (s_Size == 0) {
            return {};
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

    void Set(const ZString& s_Value) {
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