#pragma once

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

enum class ECurveTangentMode {
    Auto,
    Free,
    Flat,
    Constant
};

// Handle tip offset from the key, in time (length) and value (height).
struct CurveHandle {
    double m_Length = 0.0;
    double m_Height = 0.0;
};

struct CurveKeyModel {
    ECurveTangentMode m_Mode = ECurveTangentMode::Flat;
    CurveHandle m_In;
    CurveHandle m_Out;
    bool m_IsMirrored = true;

    std::array<double, 6> m_OutResidual = {};
};

// [T, V, c5, c4, c3, c2, c1, c0], with the coefficients over segment time 0 to 1.
using CurveKey = std::array<double, 8>;
using CurveKeys = std::vector<CurveKey>;

struct CurveModel {
    CurveKeys m_Keys;
    std::vector<CurveKeyModel> m_KeyModels;
    size_t m_SelectedKey = 0;
};

struct SegmentDerivatives {
    double m_EndValue;
    double m_StartSlope;
    double m_EndSlope;
    double m_StartCurvature;
    double m_EndCurvature;
};

inline constexpr double c_DefaultWeight = 1.0 / 3.0;

bool IsNear(double a, double b);
double SampleSegment(const CurveKey& p_Key, const CurveKey& p_NextKey, double u);
SegmentDerivatives DeriveSegment(const CurveKeys& p_Keys, size_t p_Segment);
double SideDuration(const CurveKeys& p_Keys, size_t i, bool p_IsOut);
double ClampKeyTime(const CurveKeys& p_Keys, size_t i, double t);

void BuildModel(CurveModel& p_Curve);

bool HasHandle(const CurveModel& p_Curve, size_t i, bool p_IsOut);
CurveHandle HandleOffset(const CurveModel& p_Curve, size_t i, bool p_IsOut);
std::pair<double, double> HandleTip(const CurveModel& p_Curve, size_t i, bool p_IsOut);

void EditHandle(CurveModel& p_Curve, size_t i, bool p_IsOut, CurveHandle p_Handle);
bool SetMirrored(CurveModel& p_Curve, size_t i, bool p_IsMirrored);
void ResetResidual(CurveModel& p_Curve, size_t i);
void SetTangentMode(CurveModel& p_Curve, size_t i, ECurveTangentMode p_Mode);
void MoveKey(CurveModel& p_Curve, size_t i, double t, double v);
void SplitSegment(CurveModel& p_Curve, size_t i, double s);
void RemoveKey(CurveModel& p_Curve, size_t i);
