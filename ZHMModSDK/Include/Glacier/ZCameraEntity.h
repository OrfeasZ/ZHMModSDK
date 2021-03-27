#pragma once

#include "ZEntity.h"

class ZSpatialEntity :
	public ZEntityImpl
{
public:
	PAD(0x88);
};

class ZBoundedEntity :
	public ZSpatialEntity
{
public:
	PAD(0x18);
};

class ZRenderableEntity :
	public ZBoundedEntity
{
public:
	PAD(0x18);
};

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
	virtual void ICameraEntity_unk05() = 0;
	virtual void ICameraEntity_unk06() = 0;
	virtual void ICameraEntity_unk07() = 0;
	virtual void ICameraEntity_unk08() = 0;
	virtual void ICameraEntity_unk09() = 0;
	virtual void ICameraEntity_unk10() = 0;
	virtual void ICameraEntity_unk11() = 0;
	virtual void ICameraEntity_unk12() = 0;
	virtual void ICameraEntity_unk13() = 0;
	virtual void ICameraEntity_unk14() = 0;
	virtual void ICameraEntity_unk15() = 0;
	virtual void ICameraEntity_unk16() = 0;
	virtual void ICameraEntity_unk17() = 0;
	virtual void ICameraEntity_unk18() = 0;
	virtual SMatrix44* GetProjectionMatrix() = 0;
	virtual SMatrix44* GetFPSProjectionMatrix() = 0;
	virtual float4* Project(float4* result, const float4& pos);
	virtual float4* Unproject(float4* result, const float4& pos);
};

class ZCameraEntity :
	public ZRenderableEntity,
	public ICameraEntity // at +0xD0
{
	
};
