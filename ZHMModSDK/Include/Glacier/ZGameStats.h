#pragma once

#include "TMap.h"
#include "ZString.h"
#include "ZObject.h"

class ZGameStats :
    public TMap<ZString, ZObjectRef>
{
public:
    virtual ~ZGameStats() = 0;
    virtual void unk01() = 0;
};
