#pragma once

#include "ZString.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

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

inline std::ostream& operator<<(std::ostream& p_Stream, const ZRuntimeResourceID& p_Value)
{
	return p_Stream << fmt::format("RuntimeResId<{:08X}{:08X}>", p_Value.m_IDHigh, p_Value.m_IDLow);
}

class ZResourceID
{
public:
	ZString m_uri;
};

inline std::ostream& operator<<(std::ostream& p_Stream, const ZResourceID& p_Value)
{
	return p_Stream << fmt::format("ResId<{}>", p_Value.m_uri);
}
