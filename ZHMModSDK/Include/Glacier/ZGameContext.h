#pragma once

#include "Reflection.h"

class ZSceneData;

class IGameContext
{
public:
    virtual void SetPendingTransition(const ZSceneData& data) = 0;
};

class ZGameContext :
    public IComponentInterface,
    public IGameContext
{
};
