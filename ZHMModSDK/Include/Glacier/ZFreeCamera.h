#pragma once

#include "ZCameraEntity.h"
#include "ZEntity.h"

class IFreeCameraControl
{
public:
	virtual void IFreeCameraControl_unk00() = 0;
	virtual void SetActive(bool active) = 0;
	virtual void IFreeCameraControl_unk02() = 0;
	virtual void SetCameraEntity(const TEntityRef<ZCameraEntity>& entity) = 0;
	virtual void IFreeCameraControl_unk04() = 0;
};

class ZFreeCameraControlEntity :
	public ZEntityImpl,
	public IFreeCameraControl
{	
};
