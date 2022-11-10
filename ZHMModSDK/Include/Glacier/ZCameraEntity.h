#pragma once

#include "ZEntity.h"
#include "ZSpatialEntity.h"

class ZRenderableEntity :
	public ZBoundedEntity
{
public:
	PAD(0x08);
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
	virtual SViewport* GetViewport() = 0;
	virtual void ICameraEntity_unk08() = 0;
	virtual void ICameraEntity_unk09() = 0;
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
		auto v22 = GetWorldMatrix();

		// I have no idea what any of this does.
		// TODO: Read through it and make it more readable.

		auto v23 = v22.mat[3].m;
		auto v24 = v22.mat[1].m;
		auto v25 = _mm_unpackhi_ps(v24, v23);
		auto v26 = _mm_unpacklo_ps(v24, v23);
		auto v27 = _mm_unpacklo_ps(_mm_unpackhi_ps(v22.mat[0].m, v22.mat[2].m), v25);
		auto v28 = _mm_unpacklo_ps(v22.mat[0].m, v22.mat[2].m);
		auto v29 = _mm_unpackhi_ps(v28, v26);
		auto v30 = _mm_unpacklo_ps(v28, v26);

		auto xmmword_141994D10 = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));

		SMatrix s_Result;
		s_Result.mat[0].m = _mm_and_ps(v30, xmmword_141994D10);
		s_Result.mat[1].m = _mm_and_ps(v29, xmmword_141994D10);

		float4 v31;

		v31.m = _mm_sub_ps(
			_mm_sub_ps(
			_mm_sub_ps(_mm_setzero_ps(), _mm_mul_ps(_mm_shuffle_ps(v23, v23, 0), v30)),
			_mm_mul_ps(_mm_shuffle_ps(v23, v23, 85), v29)),
			_mm_mul_ps(_mm_shuffle_ps(v23, v23, 170), v27));
		v31.w = 1.0f;

		s_Result.mat[3] = v31;
		s_Result.mat[2].m = _mm_and_ps(v27, xmmword_141994D10);

		return s_Result;
	}
};
