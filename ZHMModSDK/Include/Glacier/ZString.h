#pragma once

#include <string_view>
#include <minmax.h>
#include <ostream>

#include "Common.h"

#include <spdlog/fmt/ostr.h>

#include "Hash.h"

class ZString {
public:
    class ZImpl {
    public:
        uint32_t m_nAllocatedSize;
        volatile long m_nRefcount;
        ZImpl* m_pNext;
        char m_pDataStart[0]; // String data starts at the end of this struct.
    };

    ZString() :
        m_nLength(0x80000000),
        m_pChars(const_cast<char*>("")) {}

    ZString(std::string_view str) :
        m_pChars(str.data()) {
        m_nLength = static_cast<uint32_t>(str.size()) | 0x80000000;
    }

    ZString(const std::string& str) : ZString() {
        Allocate(str.c_str(), str.size());
    }

    template <size_t N>
    ZString(const char (&str)[N]) : ZString() {
        Allocate(str, N - 1);
    }

    ZString(const ZString& p_Other) {
        if (p_Other.IsAllocated()) {
            // TODO: Increase ref count instead.
            Allocate(p_Other.c_str(), p_Other.size());
        }
        else {
            m_nLength = p_Other.m_nLength;
            m_pChars = p_Other.m_pChars;
        }
    }

    ZString(ZString&& p_Other) noexcept {
        m_nLength = p_Other.m_nLength;
        m_pChars = p_Other.m_pChars;

        p_Other.m_nLength = 0x80000000;
        p_Other.m_pChars = const_cast<char*>("");
    }

    ZHMSDK_API ~ZString();

    static ZString AllocateFromCStr(const char* p_Str, uint32_t p_Size) {
        ZString s_String;
        s_String.Allocate(p_Str, p_Size);

        return s_String;
    }

    static ZString AllocateFromCStr(const char* p_Str) {
        return AllocateFromCStr(p_Str, strlen(p_Str));
    }

    ZString& operator=(const ZString& p_Other) {
        if (this == &p_Other)
            return *this;

        if (p_Other.IsAllocated()) {
            // TODO: Increase ref count instead.
            Allocate(p_Other.c_str(), p_Other.size());
        }
        else {
            m_nLength = p_Other.m_nLength;
            m_pChars = p_Other.m_pChars;
        }

        return *this;
    }

    ZString& operator=(ZString&& p_Other) noexcept {
        if (this == &p_Other)
            return *this;

        m_nLength = p_Other.m_nLength;
        m_pChars = p_Other.m_pChars;

        p_Other.m_nLength = 0x80000000;
        p_Other.m_pChars = const_cast<char*>("");

        return *this;
    }

    [[nodiscard]]
    uint32_t size() const {
        return m_nLength & 0x3FFFFFFF;
    }

    [[nodiscard]]
    const char* c_str() const {
        return m_pChars;
    }

    bool operator<(const ZString& p_Other) const {
        return strncmp(c_str(), p_Other.c_str(), min(size(), p_Other.size())) >> 31;
    }

    bool operator==(const ZString& p_Other) const {
        if (size() != p_Other.size())
            return false;

        return strncmp(c_str(), p_Other.c_str(), size()) == 0;
    }

    [[nodiscard]]
    bool StartsWith(const ZString& p_Other) const {
        if (size() < p_Other.size())
            return false;

        return strncmp(c_str(), p_Other.c_str(), p_Other.size()) == 0;
    }

    [[nodiscard]]
    ZString Replace(const ZString& p_Search, const ZString& p_Replace) {
        const char* s_Pos = strstr(c_str(), p_Search.c_str());

        if (s_Pos == nullptr)
            return *this;

        std::string s_Prefix(c_str(), s_Pos - c_str());
        std::string s_Suffix(s_Pos + p_Search.size(), size() - (s_Pos - c_str() + p_Search.size()));
        std::string s_Result = s_Prefix + p_Replace.c_str() + s_Suffix;
        return s_Result;
    }

    [[nodiscard]]
    bool IsAllocated() const {
        return (m_nLength & 0xC0000000) == 0;
    }

    [[nodiscard]]
    std::string_view ToStringView() const {
        return std::string_view(c_str(), size());
    }

    operator std::string_view() const {
        return ToStringView();
    }

    uint32_t GetHashCode() const {
        return Hash::Fnv1a(c_str(), size());
    }

public:
    static ZString CopyFrom(const ZString& p_Other) {
        ZString s_String;
        s_String.Allocate(p_Other.c_str(), p_Other.size());

        return s_String;
    }

private:
    ZHMSDK_API void Allocate(const char* str, uint32_t size);

private:
    uint32_t m_nLength;
    const char* m_pChars;

    friend class ModSDK;
};

template <>
struct fmt::formatter<ZString> {
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
        return ctx.begin();
    }

    auto format(const ZString& r, format_context& ctx) const -> format_context::iterator {
        return fmt::format_to(ctx.out(), "{}", r.ToStringView());
    }
};

inline std::ostream& operator<<(std::ostream& p_Stream, const ZString& p_String) {
    return p_Stream.write(p_String.c_str(), p_String.size());
}

inline ZString operator""_zs(const char* p_String, size_t p_Size) {
    return {std::string_view(p_String, p_Size)};
}