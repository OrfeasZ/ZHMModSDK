#pragma once

#include "ZPrimitives.h"
#include <string_view>
#include <minmax.h>

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

	ZString(const char* str, uint32_t size) :
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

	inline ~ZString()
	{
		if (IsAllocated())
		{
			free(const_cast<char*>(m_pChars));
		}
	}

	uint32_t size() const
	{
		return m_nLength & 0x3FFFFFFF;
	}

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

	bool StartsWith(const ZString& p_Other) const
	{
		if (size() < p_Other.size())
			return false;

		return strncmp(c_str(), p_Other.c_str(), p_Other.size()) == 0;
	}

	bool IsAllocated() const
	{
		return (m_nLength & 0xC0000000) == 0;
	}

public:
	static ZString CopyFrom(const ZString& p_Other)
	{
		ZString s_String;
		s_String.Allocate(p_Other.c_str(), p_Other.size());
		
		return s_String;
	}

private:
	void Allocate(const char* str, size_t size)
	{
		m_nLength = static_cast<uint32_t>(size);
		m_pChars = reinterpret_cast<char*>(malloc(size));
		memcpy(const_cast<char*>(m_pChars), str, size);
	}

private:
	int32_t m_nLength;
	const char* m_pChars;
};
