#pragma once

#include "ZString.h"
#include "Hash.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

class ZRuntimeResourceID {
public:
    constexpr ZRuntimeResourceID() :
        m_IDHigh(-1),
        m_IDLow(-1) {}

    constexpr ZRuntimeResourceID(uint64_t p_ID) :
        m_IDHigh(p_ID >> 32),
        m_IDLow(p_ID & 0xFFFFFFFF) {}

    constexpr ZRuntimeResourceID(uint32_t p_IDHigh, uint32_t p_IDLow) :
        m_IDHigh(p_IDHigh),
        m_IDLow(p_IDLow) {}

    /**
     * Create a runtime resource ID from a string.
     *
     * The string can be in two formats:
     * 1. Hash: 16 hexadecimal characters (eg. "00B09C0C1A885059")
     * 2. IOI Path: (eg. "[modules:/zspatialentity.class].pc_entityblueprint")
     *
     * In the first case, the string is parsed as a hexadecimal number and
     * used as-is. In the second case, the string is MD5 hashed and truncated.
     */
    static ZRuntimeResourceID FromString(const std::string& p_String) {
        // If hash starts with 00, is exactly 16 chars long, and is hex, then decode it directly to a uint64.
        if (p_String.starts_with("00") && p_String.size() == 16 && p_String.find_first_not_of("0123456789abcdefABCDEF")
            == std::string::npos) {
            return {std::stoull(p_String, nullptr, 16)};
        }

        // Otherwise hash it.
        const auto s_Hash = Hash::MD5(std::string_view(p_String));

        const uint32_t s_IDHigh = ((s_Hash.A >> 24) & 0x000000FF)
                | ((s_Hash.A >> 8) & 0x0000FF00)
                | ((s_Hash.A << 8) & 0x00FF0000);

        const uint32_t s_IDLow = ((s_Hash.B >> 24) & 0x000000FF)
                | ((s_Hash.B >> 8) & 0x0000FF00)
                | ((s_Hash.B << 8) & 0x00FF0000)
                | ((s_Hash.B << 24) & 0xFF000000);

        return {s_IDHigh, s_IDLow};
    }

    bool operator==(const ZRuntimeResourceID& rhs) const {
        return GetID() == rhs.GetID();
    }

    unsigned long long GetID() const {
        return (static_cast<unsigned long long>(m_IDHigh) << 32) | m_IDLow;
    }

    unsigned int GetHashCode() const {
        return m_IDHigh ^ m_IDLow;
    }

public:
    union {
        struct {
            uint32_t m_IDHigh;
            uint32_t m_IDLow;
        };
    };
};


template <>
struct std::hash<ZRuntimeResourceID> {
    size_t operator()(const ZRuntimeResourceID& p_Value) const noexcept {
        return p_Value.GetID();
    }
};

inline std::ostream& operator<<(std::ostream& p_Stream, const ZRuntimeResourceID& p_Value) {
    return p_Stream << fmt::format("RuntimeResId<{:08X}{:08X}>", p_Value.m_IDHigh, p_Value.m_IDLow);
}

template <>
struct fmt::formatter<ZRuntimeResourceID> {
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
        return ctx.begin();
    }

    auto format(const ZRuntimeResourceID& r, format_context& ctx) const -> format_context::iterator {
        return fmt::format_to(ctx.out(), "RuntimeResId<{:08X}{:08X}>", r.m_IDHigh, r.m_IDLow);
    }
};


class ZResourceID {
public:
    ZString m_uri;
};

inline std::ostream& operator<<(std::ostream& p_Stream, const ZResourceID& p_Value) {
    return p_Stream << fmt::format("ResId<{}>", p_Value.m_uri);
}

template <>
struct fmt::formatter<ZResourceID> {
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
        return ctx.begin();
    }

    auto format(const ZResourceID& r, format_context& ctx) const -> format_context::iterator {
        return fmt::format_to(ctx.out(), "ResId<{}>", r.m_uri);
    }
};