#pragma once

#include "ZPrimitives.h"

class ZLevelSavableHandler;

class ZLevelManager
{
public:
    virtual ~ZLevelManager() {}

public:
    PAD(0x160);
    ZLevelSavableHandler* m_pSavableHandler;
};
