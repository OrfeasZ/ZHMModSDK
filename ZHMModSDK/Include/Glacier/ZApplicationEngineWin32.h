#pragma once

#include "ZPrimitives.h"
#include <Windows.h>

#include "ZEngineAppCommon.h"

class ZApplicationEngineWin32
{
public:
    virtual ~ZApplicationEngineWin32() = default;

public:
    PAD(0xB0);
    ZEngineAppCommon m_pEngineAppCommon; // 0xB8
    PAD(0x1E0); // 0x158
    HWND m_Hwnd; // 0x338
};
