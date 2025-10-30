#pragma once

#include "ZPrimitives.h"
#include <emmintrin.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <directxmath.h>

struct SViewport {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
};

inline std::ostream& operator<<(std::ostream& p_Stream, const SViewport& p_Value) {
    return p_Stream << "(" << p_Value.x << ", " << p_Value.y << ", " << p_Value.w << ", " << p_Value.h << ")";
}

class SVector2 {
public:
    SVector2() :
        x(0.f), y(0.f) {}

    SVector2(float p_X, float p_Y) :
        x(p_X), y(p_Y) {}

    SVector2 operator*(const float p_Value) const {
        return SVector2(x * p_Value, y * p_Value);
    }

    SVector2 operator-() const {
        return SVector2(-x, -y);
    }

public:
    float32 x; // 0x0
    float32 y; // 0x4
};

inline std::ostream& operator<<(std::ostream& p_Stream, const SVector2& p_Value) {
    return p_Stream << "(" << p_Value.x << ", " << p_Value.y << ")";
}

struct float4;

class SVector3 {
public:
    SVector3() :
        x(0.f), y(0.f), z(0.f) {}

    SVector3(float p_X, float p_Y, float p_Z) :
        x(p_X), y(p_Y), z(p_Z) {}

    SVector3(const SVector2& p_Vec) :
        x(p_Vec.x), y(p_Vec.y), z(0.f) {}

    ZHMSDK_API SVector3(const float4& p_Vec);

    SVector3(const DirectX::XMVECTOR p_Vector) {
        DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(this), p_Vector);
    }

    SVector3 operator+(const SVector3& p_Other) const {
        return SVector3(x + p_Other.x, y + p_Other.y, z + p_Other.z);
    }

    SVector3 operator-(const SVector3& p_Other) const {
        return SVector3(x - p_Other.x, y - p_Other.y, z - p_Other.z);
    }

    SVector3 operator-() const {
        return SVector3(-x, -y, -z);
    }

    const bool operator==(const SVector3& p_Other) const {
        return x == p_Other.x && y == p_Other.y && z == p_Other.z;
    }

    float operator*(const SVector3& p_Other) const {
        return x * p_Other.x + y * p_Other.y + z * p_Other.z;
    }

    SVector3 operator*(const float p_Value) const {
        return SVector3(x * p_Value, y * p_Value, z * p_Value);
    }

    SVector3 operator/(const SVector3& p_Other) const {
        return SVector3(x / p_Other.x, y / p_Other.y, z / p_Other.z);
    }

    SVector3 operator/(const float p_Value) const {
        return SVector3(x / p_Value, y / p_Value, z / p_Value);
    }

    float Length() const {
        return sqrtf(x * x + y * y + z * z);
    }

    SVector3 Cross(const SVector3& p_Other) const {
        return SVector3(
            y * p_Other.z - z * p_Other.y,
            z * p_Other.x - x * p_Other.z,
            x * p_Other.y - y * p_Other.x
        );
    }

    float Dot(const SVector3& p_Other) const {
        return x * p_Other.x + y * p_Other.y + z * p_Other.z;
    }

    static SVector3 CrossProduct(const SVector3& p_Vector1, const SVector3& p_Vector2) {
        return SVector3(
            p_Vector1.y * p_Vector2.z - p_Vector1.z * p_Vector2.y,
            p_Vector1.z * p_Vector2.x - p_Vector1.x * p_Vector2.z,
            p_Vector1.x * p_Vector2.y - p_Vector1.y * p_Vector2.x
        );
    }

    static float DotProduct(const SVector3& p_Vector1, const SVector3& p_Vector2) {
        return p_Vector1.x * p_Vector2.x + p_Vector1.y * p_Vector2.y + p_Vector1.z * p_Vector2.z;
    }

    inline SVector3 Normalized() const {
        const float s_LengthSq = x * x + y * y + z * z;

        if (s_LengthSq > 1e-12f) {
            const float s_InvLength = 1.0f / std::sqrt(s_LengthSq);

            return *this * s_InvLength;
        }

        return SVector3();
    }

    inline void Normalize() {
        *this = Normalized();
    }

    SVector3 GetUnitVec() const {
        const float s_Length = Length();

        if (s_Length <= 0.f) {
            return SVector3();
        }

        const float s_InverseLength = 1.f / s_Length;

        return *this * s_InverseLength;
    }

    SVector3 SetLength(float p_Length) const {
        return Normalized() * p_Length;
    }

public:
    float32 x; // 0x0
    float32 y; // 0x4
    float32 z; // 0x8
};

inline std::ostream& operator<<(std::ostream& p_Stream, const SVector3& p_Value) {
    return p_Stream << "(" << p_Value.x << ", " << p_Value.y << ", " << p_Value.z << ")";
}

class SVector4 {
public:
    SVector4() :
        x(0.f), y(0.f), z(0.f), w(0.f) {}

    SVector4(float p_X, float p_Y, float p_Z, float p_W) :
        x(p_X), y(p_Y), z(p_Z), w(p_W) {}

public:
    float32 x; // 0x0
    float32 y; // 0x4
    float32 z; // 0x8
    float32 w; // 0x10
};

inline std::ostream& operator<<(std::ostream& p_Stream, const SVector4& p_Value) {
    return p_Stream << "(" << p_Value.x << ", " << p_Value.y << ", " << p_Value.z << ", " << p_Value.w << ")";
}

class SMatrix43 {
public:
    SMatrix43() :
        XAxis(1.f, 0.f, 0.f),
        YAxis(0.f, 1.f, 0.f),
        ZAxis(0.f, 0.f, 1.f),
        Trans(0.f, 0.f, 0.f) {}

public:
    SVector3 XAxis; // 0x0
    SVector3 YAxis; // 0xC
    SVector3 ZAxis; // 0x18
    SVector3 Trans; // 0x24
};

class SMatrix44 {
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

struct alignas(16) float4 {
    float4() :
        m(_mm_setzero_ps()) {}

    float4(__m128 p_Value) :
        m(p_Value) {}

    float4(float p_X, float p_Y, float p_Z, float p_W) :
        x(p_X), y(p_Y), z(p_Z), w(p_W) {}

    float4(float p_Val) :
        x(p_Val), y(p_Val), z(p_Val), w(p_Val) {}

    float4(const SVector2& p_Vec)
        : x(p_Vec.x), y(p_Vec.y), z(0.f), w(0.f) {}

    float4(const SVector3& p_Vec, float p_W = 0.f)
        : x(p_Vec.x), y(p_Vec.y), z(p_Vec.z), w(p_W) {}

    float4 operator-(const float4& p_Vec) const {
        return _mm_sub_ps(m, p_Vec.m);
    }

    float4 operator+(const float4& p_Vec) const {
        return _mm_add_ps(m, p_Vec.m);
    }

    float4 operator*(const float4& p_Vec) const {
        return _mm_mul_ps(m, p_Vec.m);
    }

    float4 operator*(float p_Value) const {
        return _mm_mul_ps(m, _mm_load1_ps(&p_Value));
    }

    float4& operator*=(float p_Value) {
        m = _mm_mul_ps(m, _mm_set1_ps(p_Value));
        return *this;
    }

    float4 operator/(const float4& p_Vec) const {
        return _mm_div_ps(m, p_Vec.m);
    }

    float4 operator/(float p_Value) const {
        return _mm_div_ps(m, _mm_load1_ps(&p_Value));
    }

    bool operator==(const float4& p_Other) const {
        return (_mm_movemask_ps(_mm_cmpeq_ps(m, p_Other.m)) == 0xF) & 1;
    }

    inline bool operator!=(const float4& p_Other) const {
        return !(p_Other == (*this));
    }

    float4& operator+=(const float4& p_Other) {
        m = _mm_add_ps(m, p_Other.m);
        return *this;
    }

    float4& operator-=(const float4& p_Other) {
        m = _mm_sub_ps(m, p_Other.m);
        return *this;
    }

    float4 operator-() const {
        return float4(-x, -y, -z, -w);
    }

    static float4 CrossProduct(float4& v1, float4& v2) {
        return _mm_sub_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(v1.m, v1.m, _MM_SHUFFLE(3, 0, 2, 1)),
                _mm_shuffle_ps(v2.m, v2.m, _MM_SHUFFLE(3, 1, 0, 2))
            ),
            _mm_mul_ps(
                _mm_shuffle_ps(v1.m, v1.m, _MM_SHUFFLE(3, 1, 0, 2)),
                _mm_shuffle_ps(v2.m, v2.m, _MM_SHUFFLE(3, 0, 2, 1))
            )
        );
    }

    inline static float DotProduct(const float4& v1, const float4& v2) {
        return _mm_cvtss_f32(_mm_dp_ps(v1.m, v2.m, 0x71));
    }

    inline static float4 Dot3(const float4& v1, const float4& v2) {
        return _mm_dp_ps(v1.m, v2.m, 0x7F);
    }

    inline static float Norm(const float4& p_Vec) {
        return sqrt(DotProduct(p_Vec, p_Vec));
    }

    inline static float Distance(const float4& p_From, const float4& p_To) {
        return Norm(p_From - p_To);
    }

    inline float Length() const {
        return sqrtf(x * x + y * y + z * z + w * w);
    }

    inline float4 Normalized() const {
        const float s_LengthSq = DotProduct(*this, *this);

        if (s_LengthSq > 1e-12f) {
            __m128 s_InvLength = _mm_rsqrt_ps(_mm_set1_ps(s_LengthSq));
            return _mm_mul_ps(m, s_InvLength);
        }

        return _mm_setzero_ps();
    }

    inline void Normalize() {
        *this = Normalized();
    }

    union {
        __m128 m;

        struct {
            float x;
            float y;
            float z;
            float w;
        };
    };
};

struct SQuaternion {
    float4 w128;
};

class SQV {
public:
    SQuaternion m_Rotation;
    float4 m_Translation;
};

class EulerAngles {
public:
    EulerAngles() :
        yaw(0.f), pitch(0.f), roll(0.f) {}

    EulerAngles(float p_Yaw, float p_Pitch, float p_Roll) :
        yaw(p_Yaw), pitch(p_Pitch), roll(p_Roll) {}

public:
    float yaw;
    float pitch;
    float roll;
};

class Quat {
public:
    Quat() :
        m(0.f, 0.f, 0.f, 1.f) {}

    Quat(float4 p_Vec) :
        m(p_Vec) {}

    Quat(float p_X, float p_Y, float p_Z, float p_W) :
        m(p_X, p_Y, p_Z, p_W) {}

    EulerAngles ToEuler() const {
        // Adapted from DirectXTK.
        const float xx = m.x * m.x;
        const float yy = m.y * m.y;
        const float zz = m.z * m.z;

        const float m31 = 2.f * m.x * m.z + 2.f * m.y * m.w;
        const float m32 = 2.f * m.y * m.z - 2.f * m.x * m.w;
        const float m33 = 1.f - 2.f * xx - 2.f * yy;

        const float cy = sqrtf(m33 * m33 + m31 * m31);
        const float cx = atan2f(-m32, cy);
        if (cy > 16.f * FLT_EPSILON) {
            const float m12 = 2.f * m.x * m.y + 2.f * m.z * m.w;
            const float m22 = 1.f - 2.f * xx - 2.f * zz;

            return {cx, atan2f(m31, m33), atan2f(m12, m22)};
        }
        else {
            const float m11 = 1.f - 2.f * yy - 2.f * zz;
            const float m21 = 2.f * m.x * m.y - 2.f * m.z * m.w;

            return {cx, 0.f, atan2f(-m21, m11)};
        }
    }

    Quat operator*(Quat p_Other) {
        return Quat(
            m.w * p_Other.m.x + m.x * p_Other.m.w + m.y * p_Other.m.z - m.z * p_Other.m.y,
            m.w * p_Other.m.y - m.x * p_Other.m.z + m.y * p_Other.m.w + m.z * p_Other.m.x,
            m.w * p_Other.m.z + m.x * p_Other.m.y - m.y * p_Other.m.x + m.z * p_Other.m.w,
            m.w * p_Other.m.w - m.x * p_Other.m.x - m.y * p_Other.m.y - m.z * p_Other.m.z
        );
    }

public:
    float4 m;
};

inline std::ostream& operator<<(std::ostream& p_Stream, const float4& p_Value) {
    return p_Stream << "(" << p_Value.x << ", " << p_Value.y << ", " << p_Value.z << ", " << p_Value.w << ")";
}

template <>
struct fmt::formatter<float4> {
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
        return ctx.begin();
    }

    auto format(const float4& m, format_context& ctx) const -> format_context::iterator {
        return fmt::format_to(ctx.out(), "({}, {}, {}, {})", m.x, m.y, m.z, m.w);
    }
};

struct DecomposedTransform {
    SVector3 Position;
    Quat Quaternion;
    SVector3 Scale;
};

struct alignas(16) SMatrix {
    SMatrix() :
        XAxis(1.f, 0.f, 0.f, 0.f),
        YAxis(0.f, 1.f, 0.f, 0.f),
        ZAxis(0.f, 0.f, 1.f, 0.f),
        Trans(0.f, 0.f, 0.f, 1.f) {}

    SMatrix(
        float4 p_XAxis,
        float4 p_YAxis,
        float4 p_ZAxis,
        float4 p_Trans
    ) :
        XAxis(p_XAxis),
        YAxis(p_YAxis),
        ZAxis(p_ZAxis),
        Trans(p_Trans) {}

    SMatrix(SMatrix43 p_Other) :
        XAxis(p_Other.XAxis.x, p_Other.XAxis.y, p_Other.XAxis.z, 0.f),
        YAxis(p_Other.YAxis.x, p_Other.YAxis.y, p_Other.YAxis.z, 0.f),
        ZAxis(p_Other.ZAxis.x, p_Other.ZAxis.y, p_Other.ZAxis.z, 0.f),
        Trans(p_Other.Trans.x, p_Other.Trans.y, p_Other.Trans.z, 1.f) {}

    SMatrix(DirectX::XMMATRIX p_Other) :
        XAxis(p_Other.r[0]),
        YAxis(p_Other.r[1]),
        ZAxis(p_Other.r[2]),
        Trans(p_Other.r[3]) {}

    SMatrix(SMatrix44 p_Other) :
        XAxis(p_Other.m11, p_Other.m12, p_Other.m13, p_Other.m14),
        YAxis(p_Other.m21, p_Other.m22, p_Other.m23, p_Other.m24),
        ZAxis(p_Other.m31, p_Other.m32, p_Other.m33, p_Other.m34),
        Trans(p_Other.m41, p_Other.m42, p_Other.m43, p_Other.m44) {}

    DirectX::XMMATRIX& DX() {
        return *reinterpret_cast<DirectX::XMMATRIX*>(this);
    }

    const DirectX::XMMATRIX& DX() const {
        return *reinterpret_cast<const DirectX::XMMATRIX*>(this);
    }

    float4 operator*(const float4& p_Other) const {
        return DirectX::XMVector4Transform(p_Other.m, DX());
    }

    SMatrix operator*(const SMatrix& p_Other) const {
        return DirectX::XMMatrixMultiply(DX(), p_Other.DX());
    }

    SMatrix Inverse() const {
        return DirectX::XMMatrixInverse(nullptr, DX());
    }

    float Determinant() const {
        const auto s_Determinant = DirectX::XMMatrixDeterminant(DX());
        return DirectX::XMVectorGetX(s_Determinant);
    }

    SMatrix Transposed() const {
        return DirectX::XMMatrixTranspose(DX());
    }

    DecomposedTransform Decompose() const {
        DirectX::XMVECTOR s_Scale;
        DirectX::XMVECTOR s_RotQuat;
        DirectX::XMVECTOR s_Trans;

        DirectX::XMMatrixDecompose(&s_Scale, &s_RotQuat, &s_Trans, DX());

        return {
            s_Trans,
            Quat(float4(s_RotQuat)),
            s_Scale
        };
    }

    void ScaleTransform(const SVector3& p_Scale) {
        XAxis *= p_Scale.x;
        YAxis *= p_Scale.y;
        ZAxis *= p_Scale.z;
    }

    static SMatrix ScaleTransform(const SVector3& p_Scale, SMatrix& p_Transform) {
        return SMatrix(
            p_Transform.XAxis * p_Scale.x,
            p_Transform.YAxis * p_Scale.y,
            p_Transform.ZAxis * p_Scale.z,
            p_Transform.Trans
        );
    }

    SVector3 GetScale() const {
        return SVector3(XAxis.Length(), YAxis.Length(), ZAxis.Length());
    }

    [[nodiscard]] SMatrix43 ToMatrix43() const {
        SMatrix43 s_Matrix;
        s_Matrix.XAxis = {XAxis.x, XAxis.y, XAxis.z};
        s_Matrix.YAxis = {YAxis.x, YAxis.y, YAxis.z};
        s_Matrix.ZAxis = {ZAxis.x, ZAxis.y, ZAxis.z};
        s_Matrix.Trans = {Trans.x, Trans.y, Trans.z};
        return s_Matrix;
    }

    static SMatrix ScaleTranslate(const float4& p_Scale, const float4& p_Translate) {
        return SMatrix(
            { p_Scale.x, 0.f, 0.f, 0.f},
            {0.f, p_Scale.y, 0.f, 0.f},
            {0.f, 0.f, p_Scale.z, 0.f},
            { p_Translate.x, p_Translate.y, p_Translate.z, 1.f}
        );
    }

    SMatrix AffineMultiply(const SMatrix& p_Other) const {
        SMatrix s_Matrix;

        s_Matrix.XAxis = XAxis * p_Other.XAxis.x + YAxis * p_Other.XAxis.y + ZAxis * p_Other.XAxis.z;
        s_Matrix.YAxis = XAxis * p_Other.YAxis.x + YAxis * p_Other.YAxis.y + ZAxis * p_Other.YAxis.z;
        s_Matrix.ZAxis = XAxis * p_Other.ZAxis.x + YAxis * p_Other.ZAxis.y + ZAxis * p_Other.ZAxis.z;
        s_Matrix.Trans = XAxis * p_Other.Trans.x + YAxis * p_Other.Trans.y + ZAxis * p_Other.Trans.z + Trans;

        return s_Matrix;
    }

    float4 WVectorTransform(const float4& p_Vector) const {
        return XAxis * p_Vector.x + YAxis * p_Vector.y + ZAxis * p_Vector.z + Trans;
    }

    float4 WVectorTransformH(const float4& p_Vector) const {
        return XAxis * p_Vector.x + YAxis * p_Vector.y + ZAxis * p_Vector.z + Trans * p_Vector.w;
    }

    float4 WVectorRotate(const float4& p_Vector) const {
        return XAxis * p_Vector.x + YAxis * p_Vector.y + ZAxis * p_Vector.z;
    }

    static SMatrix RotationAxisAngle(const float4& p_Axis, const float p_Angle) {
        const float x = p_Axis.x;
        const float y = p_Axis.y;
        const float z = p_Axis.z;

        const float c = cosf(p_Angle);
        const float s = sinf(p_Angle);

        SMatrix s_Result;

        s_Result.XAxis = float4(
            c + (1.f - c) * x * x,
            (1.f - c) * y * x - s * z,
            (1.f - c) * z * x + s * y,
            0.f
        );

        s_Result.YAxis = float4(
            (1.f - c) * x * y + s * z,
            c + (1.f - c) * y * y,
            (1.f - c) * z * y - s * x,
            0.f
        );

        s_Result.ZAxis = float4(
            (1.f - c) * x * z - s * y,
            (1.f - c) * y * z + s * x,
            c + (1.f - c) * z * z,
            0.f
        );

        s_Result.Trans = float4(0.f, 0.f, 0.f, 1.f);

        return s_Result;
    }

    union {
        float4 mat[4];
        float flt[4 * 4];

        struct {
            float4 XAxis;
            float4 YAxis;
            float4 ZAxis;
            float4 Trans;
        };

        struct {
            float4 Right;
            float4 Backward;
            float4 Up;
            float4 Pos;
        };

        struct {
            float4 CameraRight;
            float4 CameraUp;
            float4 CameraBackward;
            float4 CameraPosition;
        };
    };
};

static_assert(sizeof(SMatrix) == sizeof(DirectX::XMMATRIX));
static_assert(alignof(SMatrix) == alignof(DirectX::XMMATRIX));

inline std::ostream& operator<<(std::ostream& p_Stream, const SMatrix& p_Value) {
    return p_Stream << "[ " << p_Value.XAxis << ", " << p_Value.YAxis << ", " << p_Value.ZAxis << ", " << p_Value.Trans
            << " ]";
}

template <>
struct fmt::formatter<SMatrix> {
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
        return ctx.begin();
    }

    auto format(const SMatrix& m, format_context& ctx) const -> format_context::iterator {
        return fmt::format_to(ctx.out(), "[{}, {}, {}, {}]", m.XAxis, m.YAxis, m.ZAxis, m.Trans);
    }
};

static_assert(alignof(SMatrix) == 16);
static_assert(alignof(SMatrix43) == 4);
static_assert(alignof(SMatrix44) == 4);