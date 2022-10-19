#pragma once

#include "ZPrimitives.h"
#include <emmintrin.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

struct SViewport
{
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

inline std::ostream& operator<<(std::ostream& p_Stream, const SViewport& p_Value)
{
	return p_Stream << "(" << p_Value.x << ", " << p_Value.y << ", " << p_Value.w << ", " << p_Value.h << ")";
}

class SVector2
{
public:
	SVector2() : x(0.f), y(0.f) {}
	SVector2(float p_X, float p_Y) : x(p_X), y(p_Y) {}
	
public:
	float32 x; // 0x0
	float32 y; // 0x4
};

inline std::ostream& operator<<(std::ostream& p_Stream, const SVector2& p_Value)
{
	return p_Stream << "(" << p_Value.x << ", " << p_Value.y << ")";
}

class SVector3
{
public:
	SVector3() : x(0.f), y(0.f), z(0.f) {}
	SVector3(float p_X, float p_Y, float p_Z) : x(p_X), y(p_Y), z(p_Z) {}
	
public:
	float32 x; // 0x0
	float32 y; // 0x4
	float32 z; // 0x8
};

inline std::ostream& operator<<(std::ostream& p_Stream, const SVector3& p_Value)
{
	return p_Stream << "(" << p_Value.x << ", " << p_Value.y << ", " << p_Value.z << ")";
}

class SVector4
{
public:
	SVector4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
	SVector4(float p_X, float p_Y, float p_Z, float p_W) : x(p_X), y(p_Y), z(p_Z), w(p_W) {}
	
public:
	float32 x; // 0x0
	float32 y; // 0x4
	float32 z; // 0x8
	float32 w; // 0x10
};

inline std::ostream& operator<<(std::ostream& p_Stream, const SVector4& p_Value)
{
	return p_Stream << "(" << p_Value.x << ", " << p_Value.y << ", " << p_Value.z << ", " << p_Value.w << ")";
}

class SMatrix43
{
public:
	SMatrix43() :
		XAxis(1.f, 0.f, 0.f),
		YAxis(0.f, 1.f, 0.f),
		ZAxis(0.f, 0.f, 1.f),
		Trans(0.f, 0.f, 0.f)
	{		
	}
	
public:
	SVector3 XAxis; // 0x0
	SVector3 YAxis; // 0xC
	SVector3 ZAxis; // 0x18
	SVector3 Trans; // 0x24
};

class SSESMatrix43
{
public:
	__m128 data[3];
};

struct alignas(16) float4
{
	float4() : m(_mm_setzero_ps()) {}

	float4(__m128 p_Value) : m(p_Value) {}
	
	float4(float p_X, float p_Y, float p_Z, float p_W) : x(p_X), y(p_Y), z(p_Z), w(p_W) {}

	float4 operator-(const float4& p_Vec) const
	{
		return _mm_sub_ps(m, p_Vec.m);
	}

	float4 operator+(const float4& p_Vec) const
	{
		return _mm_add_ps(m, p_Vec.m);
	}

	float4 operator*(const float4& p_Vec) const
	{
		return _mm_mul_ps(m, p_Vec.m);
	}

	float4 operator/(const float4& p_Vec) const
	{
		return _mm_div_ps(m, p_Vec.m);
	}

	float4 operator*(float p_Value) const
	{
		return _mm_mul_ps(m, _mm_load1_ps(&p_Value));
	}

	static float4 CrossProduct(float4& v1, float4& v2)
	{
		return _mm_sub_ps(
			_mm_mul_ps(_mm_shuffle_ps(v1.m, v1.m, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(v2.m, v2.m, _MM_SHUFFLE(3, 1, 0, 2))),
			_mm_mul_ps(_mm_shuffle_ps(v1.m, v1.m, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(v2.m, v2.m, _MM_SHUFFLE(3, 0, 2, 1)))
		);
	}

	inline static float DotProduct(const float4& v1, const float4& v2)
	{
		return _mm_cvtss_f32(_mm_dp_ps(v1.m, v2.m, 0x71));
	}

	inline static float4 Dot3(const float4& v1, const float4& v2)
	{
		return _mm_dp_ps(v1.m, v2.m, 0x7F);
	}

	inline static float Norm(const float4& p_Vec)
	{
		return (float)sqrt(DotProduct(p_Vec, p_Vec));
	}

	inline static float Distance(const float4& p_From, const float4& p_To)
	{
		return Norm(p_From - p_To);
	}
	
	union
	{
		__m128 m;

		struct
		{
			float x;
			float y;
			float z;
			float w;
		};
	};
};

inline std::ostream& operator<<(std::ostream& p_Stream, const float4& p_Value)
{
	return p_Stream << "(" << p_Value.x << ", " << p_Value.y << ", " << p_Value.z << ", " << p_Value.w << ")";
}

struct SMatrix
{
	SMatrix() {}

	float4 operator*(const float4& p_Other) const
	{
		const auto s_XAxis = XAxis * p_Other.x;
		const auto s_YAxis = YAxis * p_Other.y;
		const auto s_ZAxis = ZAxis * p_Other.z;
		const auto s_Trans = Trans * p_Other.w;

		return {
			s_XAxis.x + s_YAxis.x + s_ZAxis.x + s_Trans.x,
			s_XAxis.y + s_YAxis.y + s_ZAxis.y + s_Trans.y,
			s_XAxis.z + s_YAxis.z + s_ZAxis.z + s_Trans.z,
			s_XAxis.w + s_YAxis.w + s_ZAxis.w + s_Trans.w
		};
	}

	union
	{
		float4 mat[4];
		float flt[4 * 4];

		struct
		{
			float4 XAxis;
			float4 YAxis;
			float4 ZAxis;
			float4 Trans;
		};
	};
};

inline std::ostream& operator<<(std::ostream& p_Stream, const SMatrix& p_Value)
{
	return p_Stream << "[ " << p_Value.XAxis << ", " << p_Value.YAxis << ", " << p_Value.ZAxis << ", " << p_Value.Trans << " ]";
}

class SMatrix44
{
public:
	float32 m11; // 0x0
	float32 m12; // 0x4
	float32 m13; // 0x8
	float32 m14; // 0xC
	float32 m21; // 0x10
	float32 m22; // 0x14
	float32 m23; // 0x18
	float32 m24; // 0x1C
	float32 m31; // 0x20
	float32 m32; // 0x24
	float32 m33; // 0x28
	float32 m34; // 0x2C
	float32 m41; // 0x30
	float32 m42; // 0x34
	float32 m43; // 0x38
	float32 m44; // 0x3C
};

inline void Matrix43ToSMatrix(const SMatrix43& p_Input, SMatrix* p_Output)
{
	static auto v8 = _mm_set_ps(1.f, 0.f, 0.f, 0.f);
	static __m128 v2 = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
	
	auto* s_Input = reinterpret_cast<const SSESMatrix43*>(&p_Input);

	auto v4 = s_Input->data[1];
	auto v6 = s_Input->data[2];

	auto v7 = _mm_shuffle_ps(s_Input->data[0], v4, 79);	

	p_Output->mat[0].m = _mm_and_ps(s_Input->data[0], v2);
	p_Output->mat[1].m = _mm_and_ps(_mm_shuffle_ps(v7, v7, 56), v2);
	p_Output->mat[2].m = _mm_and_ps(_mm_shuffle_ps(v4, v6, 78), v2);
	p_Output->mat[3].m = _mm_add_ps(_mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(v6), 4)), v8);
}
