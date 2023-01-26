#pragma once

#include "ZString.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

class ZRuntimeResourceID
{
public:
	constexpr ZRuntimeResourceID() :
		m_IDHigh(-1),
		m_IDLow(-1)
	{

	}

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

	bool operator==(const ZRuntimeResourceID& rhs) const
	{
		return GetID() == rhs.GetID();
	}

	unsigned long long GetID() const
	{
		return (static_cast<unsigned long long>(m_IDHigh) << 32) | m_IDLow;
	}

	unsigned int GetHashCode() const
	{
		return m_IDHigh ^ m_IDLow;
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

template <>
struct fmt::formatter<ZRuntimeResourceID>
{
	constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator
	{
		return ctx.begin();
	}

	auto format(const ZRuntimeResourceID& r, format_context& ctx) const -> format_context::iterator
	{
		return fmt::format_to(ctx.out(), "RuntimeResId<{:08X}{:08X}>", r.m_IDHigh, r.m_IDLow);
	}
};


class ZResourceID
{
public:
	ZString m_uri;
};

inline std::ostream& operator<<(std::ostream& p_Stream, const ZResourceID& p_Value)
{
	return p_Stream << fmt::format("ResId<{}>", p_Value.m_uri);
}

template <>
struct fmt::formatter<ZResourceID>
{
	constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator
	{
		return ctx.begin();
	}

	auto format(const ZResourceID& r, format_context& ctx) const -> format_context::iterator
	{
		return fmt::format_to(ctx.out(), "ResId<{}>", r.m_uri);
	}
};
