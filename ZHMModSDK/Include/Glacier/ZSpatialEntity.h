#pragma once

#include "ZEntity.h"
#include "ZPrimitives.h"
#include "ZMath.h"

class ZSpatialEntity :
	public ZEntityImpl
{
public:
	virtual void ZSpatialEntity_unk08() = 0;
	virtual void ZSpatialEntity_unk09() = 0;
	virtual void ZSpatialEntity_unk10() = 0;
	virtual void ZSpatialEntity_unk11() = 0;
	virtual void ZSpatialEntity_unk12() = 0;
	virtual void ZSpatialEntity_unk13() = 0;
	virtual void ZSpatialEntity_unk14() = 0;
	virtual void ZSpatialEntity_unk15() = 0;
	virtual void ZSpatialEntity_unk16() = 0;
	virtual void ZSpatialEntity_unk17() = 0;
	virtual void ZSpatialEntity_unk18() = 0;
	virtual void ZSpatialEntity_unk19() = 0;
	virtual void ZSpatialEntity_unk20() = 0;
	virtual void ZSpatialEntity_unk21() = 0;
	virtual void ZSpatialEntity_unk22() = 0;
	virtual void ZSpatialEntity_unk23() = 0;
	virtual void ZSpatialEntity_unk24() = 0;
	virtual void ZSpatialEntity_unk25() = 0;
	virtual void ZSpatialEntity_unk26() = 0;
	virtual void ZSpatialEntity_unk27() = 0;
	virtual void ZSpatialEntity_unk28() = 0;
	virtual void ZSpatialEntity_unk29() = 0;
	virtual void ZSpatialEntity_unk30() = 0;
	virtual void ZSpatialEntity_unk31() = 0;
	virtual void CalculateBounds(float4& min, float4& max, uint32_t a3, uint32_t a4) = 0;
	virtual void ZSpatialEntity_unk33() = 0;
	virtual void ZSpatialEntity_unk34() = 0;
	virtual void ZSpatialEntity_unk35() = 0;
	virtual void ZSpatialEntity_unk36() = 0;
	virtual void ZSpatialEntity_unk37() = 0;
	virtual void ZSpatialEntity_unk38() = 0;
	virtual void ZSpatialEntity_unk39() = 0;
	virtual void ZSpatialEntity_unk40() = 0;
	virtual void ZSpatialEntity_unk41() = 0;
	virtual void ZSpatialEntity_unk42() = 0;

public:
	PAD(0x88);
};

class ZBoundedEntity :
	public ZSpatialEntity
{
public:
	PAD(0x18);
};
