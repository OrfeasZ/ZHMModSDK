#pragma once

#include "ZEntity.h"
#include "ZSpatialEntity.h"
#include "ZGeomEntity.h"

class IRenderDestinationSource :
	public IComponentInterface
{
public:
	~IRenderDestinationSource() = default;
};

class ICameraEntity :
	public IRenderDestinationSource
{
public:
	virtual void ICameraEntity_unk5() = 0;
	virtual void ICameraEntity_unk6() = 0;
	virtual SViewport* GetViewport() = 0;
	virtual void ICameraEntity_unk8() = 0;
	virtual void ICameraEntity_unk9() = 0;
	virtual void ICameraEntity_unk10() = 0;
	virtual float GetNearZ() = 0;
	virtual float GetFarZ() = 0;
	virtual void ICameraEntity_unk13() = 0;
	virtual void ICameraEntity_unk14() = 0;
	virtual void ICameraEntity_unk15() = 0;
	virtual void ICameraEntity_unk16() = 0;
	virtual void ICameraEntity_unk17() = 0;
	virtual void ICameraEntity_unk18() = 0;
	virtual SMatrix44* GetProjectionMatrix() = 0;
	virtual SMatrix44* GetFPSProjectionMatrix() = 0;
	virtual float4* Project(float4* result, const float4* pos);
	virtual float4* Unproject(float4* result, const float4* pos);
};

class ZCameraEntity :
	public ZRenderableEntity,
	public ICameraEntity // at +0xD0
{
public:
	SMatrix GetViewMatrix()
	{
		return GetWorldMatrix().Inverse();
	}
};

static_assert(sizeof(ZRenderableEntity) == 0xD0);
