#pragma once

#include "ZPrimitives.h"
#include <string_view>
#include <intrin.h>

class ZString
{
public:
	inline ZString() :
		m_nLength(0x80000000),
		m_pChars(const_cast<char*>(""))
	{
		_bittestandset(reinterpret_cast<long*>(&m_nLength), 30);
	}

	inline ZString(std::string_view str) :
		m_nLength(static_cast<uint32_t>(str.size())),
		m_pChars(const_cast<char*>(str.data()))
	{
		_bittestandset(reinterpret_cast<long*>(&m_nLength), 30);
	}

	inline ZString(const char* str) :
		m_nLength(static_cast<uint32_t>(std::strlen(str))),
		m_pChars(const_cast<char*>(str))
	{
		_bittestandset(reinterpret_cast<long*>(&m_nLength), 30);
	}

	inline uint32_t size() const
	{
		return m_nLength;
	}

	inline const char* c_str() const
	{
		return m_pChars;
	}

	inline bool operator<(const ZString& other) const
	{
		return strcmp(m_pChars, other.m_pChars) >> 31;
	}

private:
	int32_t m_nLength;
	char* m_pChars;
};
