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
	virtual void SetWorldMatrix(const SMatrix& matrix) = 0;
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
	SMatrix GetWorldMatrix()
	{
		// I have no idea what any of this does.
		// TODO: Read through it and make it more readable.

		// This is probably something like "is this transform dirty and needs to be updated?".
		if ((m_nUnknownFlags & 0x80000) != 0)
			Functions::ZSpatialEntity_UnknownTransformUpdate->Call(this);

		auto v4 = m_mTransform.XAxis.m;
		auto v6 = m_mTransform.YAxis.m;
		auto v7 = m_mTransform.ZAxis.m;

		auto xmmword_141994D10 = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
		auto xmmword_1417B3C00 = _mm_set_ps(1, 0, 0, 0);

		auto v8 = _mm_and_ps(xmmword_141994D10, v4);
		auto v9 = _mm_shuffle_ps(v4, v6, 79);

		SMatrix s_Result;
		s_Result.XAxis.m = v8;
		s_Result.YAxis.m = _mm_and_ps(_mm_shuffle_ps(v9, v9, 56), xmmword_141994D10);
		s_Result.ZAxis.m = _mm_and_ps(_mm_shuffle_ps(v6, v7, 78), xmmword_141994D10);
		s_Result.Trans.m = _mm_add_ps(_mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(v7), 4)), xmmword_1417B3C00);

		return s_Result;
	}

public:
	PAD(0x08);
	SMatrix m_mTransform; // 0x20
	PAD(0x0C); // 0x60
	uint32_t m_nUnknownFlags; // 0x6C
	PAD(0x30); // 0x70
};

static_assert(offsetof(ZSpatialEntity, m_mTransform) == 0x20);
static_assert(offsetof(ZSpatialEntity, m_nUnknownFlags) == 0x6C);

class ZBoundedEntity :
	public ZSpatialEntity
{
public:
	PAD(0x18);
};
