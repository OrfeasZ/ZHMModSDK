#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

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

#ifndef CONCAT_IMPL
#define CONCAT_IMPL(x, y) x##y
#endif

#ifndef MACRO_CONCAT
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#endif

#ifndef PAD
#define PAD(SIZE) unsigned char MACRO_CONCAT(_pad, __COUNTER__)[SIZE];
#endif

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

	ZGuid(const std::string& p_Data, GuidFormat p_Format = GuidFormat::Dashes)
	{
		FromString(p_Data, p_Format);
	}

	ZGuid(const ZGuid& p_Other)
	{
		m_nHigh = p_Other.m_nHigh;
		m_nLow = p_Other.m_nLow;
	}

	void operator=(const std::string& p_Data)
	{
		FromString(p_Data, GuidFormat::Dashes);
	}

	void operator=(const char* p_Data)
	{
		FromString(p_Data, GuidFormat::Dashes);
	}

	void FromString(const std::string& p_Data, GuidFormat p_Format = GuidFormat::Dashes)
	{
#pragma warning(disable:4477)
		if (p_Format == GuidFormat::Dashes)
		{
			sscanf_s(p_Data.c_str(),
				"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
				&data1, &data2, &data3,
				&data4[0], &data4[1], &data4[2], &data4[3],
				&data4[4], &data4[5], &data4[6], &data4[7]);
		}
		else if (p_Format == GuidFormat::NoDashes)
		{
			sscanf_s(p_Data.c_str(),
				"%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
				&data1, &data2, &data3,
				&data4[0], &data4[1], &data4[2], &data4[3],
				&data4[4], &data4[5], &data4[6], &data4[7]);
		}
		else if (p_Format == GuidFormat::Brackets)
		{
			sscanf_s(p_Data.c_str(),
				"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
				&data1, &data2, &data3,
				&data4[0], &data4[1], &data4[2], &data4[3],
				&data4[4], &data4[5], &data4[6], &data4[7]);
		}
		else if (p_Format == GuidFormat::Parentheses)
		{
			sscanf_s(p_Data.c_str(),
				"(%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X)",
				&data1, &data2, &data3,
				&data4[0], &data4[1], &data4[2], &data4[3],
				&data4[4], &data4[5], &data4[6], &data4[7]);
		}
#pragma warning(default:4477)
	}

	std::string ToString(GuidFormat p_Format = GuidFormat::Dashes) const
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

		return s_GUID;
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
	ZRepositoryID(const std::string& p_Data, GuidFormat p_Format = GuidFormat::Dashes) : ZGuid(p_Data, p_Format) {}
	ZRepositoryID(const ZRepositoryID& p_Other) : ZGuid(p_Other) {}

	void operator=(const std::string& p_Data)
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
