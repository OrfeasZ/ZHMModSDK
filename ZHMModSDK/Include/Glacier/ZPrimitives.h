#pragma once

#include <cstdint>
#include <cstdio>

#include "ZString.h"

typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;
typedef float float32;
typedef double float64;

class ZGuid
{
public:
	enum class GuidFormat
	{
		Dashes,
		NoDashes,
		Brackets,
		Parentheses,
	};

	ZGuid(const ZString& p_Data, GuidFormat p_Format = GuidFormat::Dashes)
	{
		FromString(p_Data, p_Format);
	}

	ZGuid(const ZGuid& p_Other)
	{
		m_nHigh = p_Other.m_nHigh;
		m_nLow = p_Other.m_nLow;
	}

	void operator=(const ZString& p_Data)
	{
		FromString(p_Data, GuidFormat::Dashes);
	}

	void operator=(const char* p_Data)
	{
		FromString(p_Data, GuidFormat::Dashes);
	}

	bool operator<(const ZGuid& rhs) const
	{
		if (data1 != rhs.data1)
		{
			return data1 < rhs.data1;
		}

		if (data2 != rhs.data2)
		{
			return data2 < rhs.data2;
		}

		if (data3 != rhs.data3)
		{
			return data3 < rhs.data3;
		}

		for (unsigned int i = 0; i < sizeof(data4); ++i)
		{
			if (data4[i] != rhs.data4[i])
			{
				return data4[i] < rhs.data4[i];
			}
		}

		return false;
	}

	bool operator==(const ZGuid& rhs) const
	{
		if (data1 != rhs.data1)
		{
			return false;
		}

		if (data2 != rhs.data2)
		{
			return false;
		}

		if (data3 != rhs.data3)
		{
			return false;
		}

		for (unsigned int i = 0; i < sizeof(data4); ++i)
		{
			if (data4[i] != rhs.data4[i])
			{
				return false;
			}
		}

		return true;
	}

	void FromString(const ZString& p_Data, GuidFormat p_Format = GuidFormat::Dashes)
	{
#pragma warning(disable:4477)
		if (p_Format == GuidFormat::Dashes)
		{
			sscanf_s(p_Data.c_str(), "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
				&data1, &data2, &data3,
				&data4[0], &data4[1], &data4[2],
				&data4[3], &data4[4], &data4[5],
				&data4[6], &data4[7]);
		}
		else if (p_Format == GuidFormat::NoDashes)
		{
			sscanf_s(p_Data.c_str(), "%08lX%04hX%04hX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
				&data1, &data2, &data3,
				&data4[0], &data4[1], &data4[2],
				&data4[3], &data4[4], &data4[5],
				&data4[6], &data4[7]);
		}
		else if (p_Format == GuidFormat::Brackets)
		{
			sscanf_s(p_Data.c_str(), "{%08lX%04hX%04hX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
				&data1, &data2, &data3,
				&data4[0], &data4[1], &data4[2],
				&data4[3], &data4[4], &data4[5],
				&data4[6], &data4[7]);
		}
		else if (p_Format == GuidFormat::Parentheses)
		{
			sscanf_s(p_Data.c_str(), "(%08lX%04hX%04hX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX)",
				&data1, &data2, &data3,
				&data4[0], &data4[1], &data4[2],
				&data4[3], &data4[4], &data4[5],
				&data4[6], &data4[7]);
		}
#pragma warning(default:4477)
	}

	ZString ToString(GuidFormat p_Format = GuidFormat::Dashes) const
	{
		char s_GUID[128];
		memset(s_GUID, 0, sizeof(s_GUID));

		int s_Ret = -1;

		if (p_Format == GuidFormat::Dashes)
		{
			s_Ret = sprintf_s(s_GUID, sizeof(s_GUID),
				"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
				data1, data2, data3,
				data4[0], data4[1], data4[2], data4[3],
				data4[4], data4[5], data4[6], data4[7]);
		}
		else if (p_Format == GuidFormat::NoDashes)
		{
			s_Ret = sprintf_s(s_GUID, sizeof(s_GUID),
				"%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
				data1, data2, data3,
				data4[0], data4[1], data4[2], data4[3],
				data4[4], data4[5], data4[6], data4[7]);
		}
		else if (p_Format == GuidFormat::Brackets)
		{
			s_Ret = sprintf_s(s_GUID, sizeof(s_GUID),
				"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
				data1, data2, data3,
				data4[0], data4[1], data4[2], data4[3],
				data4[4], data4[5], data4[6], data4[7]);
		}
		else if (p_Format == GuidFormat::Parentheses)
		{
			s_Ret = sprintf_s(s_GUID, sizeof(s_GUID),
				"(%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X)",
				data1, data2, data3,
				data4[0], data4[1], data4[2], data4[3],
				data4[4], data4[5], data4[6], data4[7]);
		}

		if (s_Ret == -1)
			return "";

		return ZString::CopyFrom(s_GUID);
	}

	unsigned int GetHashCode() const
	{
		return this->data1 ^ (this->data3 | (this->data2 << 16)) ^ (this->data4[7] | (this->data4[2] << 24));
	}

public:
	union
	{
		struct
		{

			uint32_t data1;
			uint16_t data2;
			uint16_t data3;
			uint8_t data4[8];
		};

		struct
		{
			uint64_t m_nHigh;
			uint64_t m_nLow;
		};
	};
};

class ZRepositoryID :
	public ZGuid
{
public:
	ZRepositoryID(const ZString& p_Data, GuidFormat p_Format = GuidFormat::Dashes) : ZGuid(p_Data, p_Format) {}
	ZRepositoryID(const ZRepositoryID& p_Other) : ZGuid(p_Other) {}

	void operator=(const ZString& p_Data)
	{
		FromString(p_Data, GuidFormat::Dashes);
	}

	void operator=(const char* p_Data)
	{
		FromString(p_Data, GuidFormat::Dashes);
	}
};

class STokenID
{
public:
	uint32_t m_nValue;
	bool m_bValid;
};
