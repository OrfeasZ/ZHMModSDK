#pragma once

#include "ZString.h"

class ZRuntimeResourceID
{
public:
	union
	{
		uint64_t m_ID;

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
