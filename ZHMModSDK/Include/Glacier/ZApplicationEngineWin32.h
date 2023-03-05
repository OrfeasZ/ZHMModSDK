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
    PAD(0x09); // 0x158
    bool m_bSceneLoaded; // 0x161
    PAD(0x1D6); // 0x162
    HWND m_Hwnd; // 0x338
};

static_assert(offsetof(ZApplicationEngineWin32, m_pEngineAppCommon) == 0xB8);
static_assert(offsetof(ZApplicationEngineWin32, m_bSceneLoaded) == 0x161);
static_assert(offsetof(ZApplicationEngineWin32, m_Hwnd) == 0x338);
