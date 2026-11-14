#pragma once

#include "IComponentInterface.h"

class IScaleformGFxManager {
public:
    virtual ~IScaleformGFxManager() = 0;
    virtual void IScaleformGFxManager_Unk1() = 0;
    virtual void IScaleformGFxManager_Unk2() = 0;
    virtual bool GetIsMenuMode() const = 0;
    virtual bool GetIsMenuModeUnsafe() const = 0;
};

class ZScaleformGFxManager : public IComponentInterface, public IScaleformGFxManager {};