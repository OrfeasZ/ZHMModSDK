#pragma once

#include "ZPrimitives.h"
#include <Windows.h>

class ZApplicationEngineWin32
{
public:
	virtual ~ZApplicationEngineWin32() = default;

public:
	PAD(0x330);
	HWND m_Hwnd;
};
