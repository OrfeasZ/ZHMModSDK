#pragma once

#include "ZPrimitives.h"

class ZResourcePtr
{
public:
	uint32_t m_nResourceIndex;
	PAD(0x04);
};

template<typename T>
class TResourcePtr : public ZResourcePtr
{
};
