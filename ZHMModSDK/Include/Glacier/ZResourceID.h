#pragma once

#include "ZString.h"

class ZRuntimeResourceID
{
public:
	constexpr ZRuntimeResourceID(uint64_t p_ID) :
		m_IDHigh(p_ID >> 32),
		m_IDLow(p_ID & 0xFFFFFFFF)
	{
	}

	constexpr ZRuntimeResourceID(uint32_t p_IDHigh, uint32_t p_IDLow) :
		m_IDHigh(p_IDHigh),
		m_IDLow(p_IDLow)
	{
	}

public:
	union
	{
		struct
		{
			uint32_t m_IDHigh;
			uint32_t m_IDLow;
		};
	};
};

class ZResourceID
{
public:
	ZString m_uri;
};
