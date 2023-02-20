#pragma once

#include <string_view>
#include <minmax.h>
#include <ostream>

#include "Common.h"

class ZString
{
public:
    ZString() :
        m_nLength(0x80000000),
        m_pChars(const_cast<char*>(""))
    {
    }

    ZString(std::string_view str) :
        m_pChars(str.data())
    {
        m_nLength = static_cast<uint32_t>(str.size()) | 0x80000000;
    }

    ZString(const char* str) :
        m_pChars(str)
    {
        m_nLength = static_cast<uint32_t>(std::strlen(str)) | 0x80000000;
    }

    ZString(const char* str, size_t size) :
        m_pChars(str)
    {
        m_nLength = static_cast<uint32_t>(size) | 0x80000000;
    }

    ZString(const ZString& p_Other)
    {
        if (p_Other.IsAllocated())
        {
            Allocate(p_Other.c_str(), p_Other.size());
        }
        else
        {
            m_nLength = p_Other.m_nLength;
            m_pChars = p_Other.m_pChars;
        }
    }

    ZHMSDK_API ~ZString();

    [[nodiscard]]
    uint32_t size() const
    {
        return m_nLength & 0x3FFFFFFF;
    }

    [[nodiscard]]
    const char* c_str() const
    {
        return m_pChars;
    }

    bool operator<(const ZString& p_Other) const
    {
        return strncmp(c_str(), p_Other.c_str(), min(size(), p_Other.size())) >> 31;
    }

    bool operator==(const ZString& p_Other) const
    {
        if (size() != p_Other.size())
            return false;

        return strncmp(c_str(), p_Other.c_str(), size()) == 0;
    }

    [[nodiscard]]
    bool StartsWith(const ZString& p_Other) const
    {
        if (size() < p_Other.size())
            return false;

        return strncmp(c_str(), p_Other.c_str(), p_Other.size()) == 0;
    }

    [[nodiscard]]
    bool IsAllocated() const
    {
        return (m_nLength & 0xC0000000) == 0;
    }

    [[nodiscard]]
    std::string_view ToStringView() const
    {
        return std::string_view(c_str(), size());
    }

    operator std::string_view() const
    {
        return ToStringView();
    }

public:
    static ZString CopyFrom(const ZString& p_Other)
    {
        ZString s_String;
        s_String.Allocate(p_Other.c_str(), p_Other.size());

        return s_String;
    }

private:
    ZHMSDK_API void Allocate(const char* str, size_t size);

private:
    int32_t m_nLength;
    const char* m_pChars;
};

inline std::ostream& operator<<(std::ostream& p_Stream, const ZString& p_String)
{
    return p_Stream.write(p_String.c_str(), p_String.size());
}
