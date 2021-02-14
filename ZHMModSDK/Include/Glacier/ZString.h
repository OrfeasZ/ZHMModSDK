#pragma once

#include "ZPrimitives.h"
#include <string_view>
#include <intrin.h>

class ZString
{
public:
	ZString() :
		m_nLength(0x80000000),
		m_pChars(const_cast<char*>(""))
	{
	}

	ZString(std::string_view str) :
		m_pChars(const_cast<char*>(str.data()))
	{
		m_nLength = static_cast<uint32_t>(str.size()) | 0x80000000;
	}

	ZString(const char* str) :
		m_pChars(const_cast<char*>(str))
	{
		m_nLength = static_cast<uint32_t>(std::strlen(str)) | 0x80000000;
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

private:
	int32_t m_nLength;
	char* m_pChars;
};
