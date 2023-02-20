#pragma once

#include "ZPrimitives.h"

class ZEntitySceneContext;

class ZEntityManager
{
public:
    virtual ~ZEntityManager() {}

public:
    ZEntitySceneContext* m_pContext;
};
