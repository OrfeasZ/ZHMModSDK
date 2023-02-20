#pragma once

#include "ZCameraEntity.h"
#include "ZEntity.h"

class IFreeCameraControl
{
public:
    virtual void IFreeCameraControl_unk0() = 0;
    virtual void SetActive(bool active) = 0;
    virtual void IFreeCameraControl_unk2() = 0;
    virtual void SetCameraEntity(const TEntityRef<ZCameraEntity>& entity) = 0;
    virtual void IFreeCameraControl_unk4() = 0;
};

class ZFreeCameraControlEntity :
    public ZEntityImpl,
    public IFreeCameraControl
{
public:
    PAD(0x11); // 0x20
    bool m_bFreezeCamera; // 0x31
};
