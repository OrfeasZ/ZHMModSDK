#include "CurveModel.h"

#include <algorithm>
#include <cmath>
#include <limits>

static constexpr double c_Binomials[6][6] = {
    { 1 },
    { 1, 1 },
    { 1, 2, 1 },
    { 1, 3, 3, 1 },
    { 1, 4, 6, 4, 1 },
    { 1, 5, 10, 10, 5, 1 },
};

static double EvaluateSegment(const CurveKey& p_Key, const double p_U) {
    const double u = p_U;

    // f(t) = c5*t^5 + c4*t^4 + c3*t^3 + c2*t^2 + c1*t + c0
    return p_Key[2] * u * u * u * u * u +
        p_Key[3] * u * u * u * u +
        p_Key[4] * u * u * u +
        p_Key[5] * u * u +
        p_Key[6] * u +
        p_Key[7];
}

SegmentDerivatives DeriveSegment(const CurveKeys& p_Keys, const size_t p_Segment) {
    const CurveKey& k = p_Keys[p_Segment];
    const double s_Duration = p_Keys[p_Segment + 1][0] - k[0];
    const double D = s_Duration > 1e-9 ? s_Duration : 1e-9;

    const double c5 = k[2], c4 = k[3], c3 = k[4], c2 = k[5], c1 = k[6], c0 = k[7];

    return {
        .m_EndValue = c5 + c4 + c3 + c2 + c1 + c0,
        .m_StartSlope = c1 / D,
        .m_EndSlope = (5 * c5 + 4 * c4 + 3 * c3 + 2 * c2 + c1) / D,
        .m_StartCurvature = 2 * c2 / (D * D),
        .m_EndCurvature = (20 * c5 + 12 * c4 + 6 * c3 + 2 * c2) / (D * D),
    };
}

static bool IsConstantSegment(const CurveKey& p_Key) {
    for (size_t i = 2; i < 7; ++i) {
        if (std::abs(p_Key[i]) >= 1e-6) {
            return false;
        }
    }

    return true;
}

bool IsNear(const double a, const double b) {
    return std::abs(a - b) <= 1e-4 * std::max({ 1.0, std::abs(a), std::abs(b) });
}

constexpr int c_FitSamples = 64;

using Polynomial = std::array<double, 6>;

static double Bernstein5(const int i, const double u) {
    return c_Binomials[5][i] * std::pow(u, i) * std::pow(1 - u, 5 - i);
}

// Fit a quintic in Bernstein form to the cubic Bezier through the keys and handle tips.
// We clamp the inner coefficients to the value range of those points, so short or
// vertical handles don't make the curve oscillate.
static Polynomial FitSegment(
    const double v0, const double v1, const double w0, const double h0, const double w1, const double h1
) {
    const double s_Xs[4] = { 0, w0, 1 - w1, 1 };
    const double s_Ys[4] = { v0, v0 + h0, v1 - h1, v1 };
    const double s_Low = std::min({ s_Ys[0], s_Ys[1], s_Ys[2], s_Ys[3] });
    const double s_High = std::max({ s_Ys[0], s_Ys[1], s_Ys[2], s_Ys[3] });

    auto Bezier = [&](const double* p_Points, const double s) {
        const double t = 1 - s;
        return t * t * t * p_Points[0] + 3 * s * t * t * p_Points[1] + 3 * s * s * t * p_Points[2] +
            s * s * s * p_Points[3];
    };

    std::array<std::array<double, 4>, 4> A = {};
    std::array<double, 4> b = {};

    // Samples are uniform in time, so find each one's Bezier parameter by bisection.
    for (int k = 0; k <= c_FitSamples; ++k) {
        const double u = static_cast<double>(k) / c_FitSamples;
        double s_Min = 0.0;
        double s_Max = 1.0;

        for (int s_Iteration = 0; s_Iteration < 40; ++s_Iteration) {
            const double s_Mid = (s_Min + s_Max) / 2;
            (Bezier(s_Xs, s_Mid) < u ? s_Min : s_Max) = s_Mid;
        }

        const double y = Bezier(s_Ys, (s_Min + s_Max) / 2);
        const double s_Residual = y - v0 * Bernstein5(0, u) - v1 * Bernstein5(5, u);
        double f[4];

        for (int i = 0; i < 4; ++i) {
            f[i] = Bernstein5(i + 1, u);
        }

        for (int i = 0; i < 4; ++i) {
            b[i] += f[i] * s_Residual;

            for (int j = 0; j < 4; ++j) {
                A[i][j] += f[i] * f[j];
            }
        }
    }

    // Solve the bounded least squares with projected Gauss-Seidel.
    std::array<double, 4> s_Best;
    s_Best.fill((v0 + v1) / 2);

    const double s_Tolerance = 1e-14 * std::max(1.0, s_High - s_Low);

    for (int s_Sweep = 0; s_Sweep < 2000; ++s_Sweep) {
        double s_Change = 0.0;

        for (int i = 0; i < 4; ++i) {
            double s_Value = b[i];

            for (int j = 0; j < 4; ++j) {
                if (j != i) {
                    s_Value -= A[i][j] * s_Best[j];
                }
            }

            s_Value = std::clamp(s_Value / A[i][i], s_Low, s_High);
            s_Change = std::max(s_Change, std::abs(s_Value - s_Best[i]));
            s_Best[i] = s_Value;
        }

        if (s_Change <= s_Tolerance) {
            break;
        }
    }

    // Convert from Bernstein to monomial form.
    const double s_Bernstein[6] = { v0, s_Best[0], s_Best[1], s_Best[2], s_Best[3], v1 };
    Polynomial s_Result = {};

    for (int i = 0; i <= 5; ++i) {
        for (int k = i; k <= 5; ++k) {
            const double s_Sign = (k - i) % 2 == 0 ? 1.0 : -1.0;
            s_Result[k] += s_Bernstein[i] * c_Binomials[5][i] * c_Binomials[5 - i][k - i] * s_Sign;
        }
    }

    return s_Result;
}

// Keep what the floats have on top of the handle fit as the residual, so the first edit
// doesn't change the segment's shape.
static void LoadSegmentHandles(CurveModel& p_Curve, const size_t j) {
    const CurveKey& k = p_Curve.m_Keys[j];
    const CurveKey& s_NextKey = p_Curve.m_Keys[j + 1];
    const Polynomial s_Target = { k[7], k[6], k[5], k[4], k[3], k[2] };
    const double S0 = s_Target[1];
    const double S1 = s_Target[1] + 2 * s_Target[2] + 3 * s_Target[3] + 4 * s_Target[4] + 5 * s_Target[5];

    const double D = s_NextKey[0] - k[0];

    CurveKeyModel& s_Start = p_Curve.m_KeyModels[j];

    s_Start.m_Out = { D * c_DefaultWeight, S0 * c_DefaultWeight };
    s_Start.m_OutResidual = {};
    p_Curve.m_KeyModels[j + 1].m_In = { D * c_DefaultWeight, S1 * c_DefaultWeight };

    if (s_Start.m_Mode == ECurveTangentMode::Constant) {
        return;
    }

    const Polynomial s_Fit = FitSegment(
        k[1], s_NextKey[1], c_DefaultWeight, S0 * c_DefaultWeight, c_DefaultWeight, S1 * c_DefaultWeight
    );

    for (int d = 0; d < 6; ++d) {
        s_Start.m_OutResidual[d] = s_Target[d] - s_Fit[d];
    }
}

void BuildModel(CurveModel& p_Curve) {
    const CurveKeys& s_Keys = p_Curve.m_Keys;
    const size_t n = s_Keys.size();

    p_Curve.m_KeyModels.assign(n, CurveKeyModel());

    if (n < 2) {
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        const double s_InSlope = i > 0 ? DeriveSegment(s_Keys, i - 1).m_EndSlope : DeriveSegment(s_Keys, i).m_StartSlope;
        const double s_OutSlope = i < n - 1 ? DeriveSegment(s_Keys, i).m_StartSlope : s_InSlope;

        const bool s_IsConstant = i < n - 1 &&
            IsConstantSegment(s_Keys[i]) &&
            std::abs(s_Keys[i + 1][1] - s_Keys[i][1]) > 1e-6;

        CurveKeyModel& s_Key = p_Curve.m_KeyModels[i];

        s_Key.m_Mode = ECurveTangentMode::Free;
        s_Key.m_IsMirrored = IsNear(s_InSlope, s_OutSlope);

        if (s_IsConstant) {
            s_Key.m_Mode = ECurveTangentMode::Constant;
        }
        else if (std::abs(s_InSlope) < 1e-4 && std::abs(s_OutSlope) < 1e-4) {
            s_Key.m_Mode = ECurveTangentMode::Flat;
        }
    }

    for (size_t j = 0; j + 1 < n; ++j) {
        LoadSegmentHandles(p_Curve, j);
    }
}

double SideDuration(const CurveKeys& p_Keys, const size_t i, const bool p_IsOut) {
    return p_IsOut ? p_Keys[i + 1][0] - p_Keys[i][0] : p_Keys[i][0] - p_Keys[i - 1][0];
}

static double AutoSlope(const CurveKeys& p_Keys, const size_t i) {
    const CurveKey& a = p_Keys[i > 0 ? i - 1 : 0];
    const CurveKey& b = p_Keys[std::min(p_Keys.size() - 1, i + 1)];

    return b[0] - a[0] > 1e-9 ? (b[1] - a[1]) / (b[0] - a[0]) : 0.0;
}

bool HasHandle(const CurveModel& p_Curve, const size_t i, const bool p_IsOut) {
    if (!p_IsOut) {
        return i > 0;
    }

    return i + 1 < p_Curve.m_Keys.size() && p_Curve.m_KeyModels[i].m_Mode != ECurveTangentMode::Constant;
}

static CurveHandle StoredHandle(const CurveModel& p_Curve, const size_t i, const bool p_IsOut) {
    const CurveKeyModel& s_Key = p_Curve.m_KeyModels[i];
    const CurveHandle& s_Handle = p_IsOut ? s_Key.m_Out : s_Key.m_In;

    switch (s_Key.m_Mode) {
        case ECurveTangentMode::Flat:
            return { s_Handle.m_Length, 0.0 };
        case ECurveTangentMode::Auto:
            return { s_Handle.m_Length, AutoSlope(p_Curve.m_Keys, i) * s_Handle.m_Length };
        case ECurveTangentMode::Free:
        case ECurveTangentMode::Constant:
            break;
    }

    return s_Handle;
}

CurveHandle HandleOffset(const CurveModel& p_Curve, const size_t i, const bool p_IsOut) {
    const CurveHandle s_Handle = StoredHandle(p_Curve, i, p_IsOut);
    const double s_Duration = SideDuration(p_Curve.m_Keys, i, p_IsOut);

    if (s_Handle.m_Length <= s_Duration) {
        return s_Handle;
    }

    return { s_Duration, s_Handle.m_Height * s_Duration / s_Handle.m_Length };
}

std::pair<double, double> HandleTip(const CurveModel& p_Curve, const size_t i, const bool p_IsOut) {
    const CurveHandle s_Offset = HandleOffset(p_Curve, i, p_IsOut);
    const CurveKey& k = p_Curve.m_Keys[i];

    return p_IsOut
        ? std::pair { k[0] + s_Offset.m_Length, k[1] + s_Offset.m_Height }
        : std::pair { k[0] - s_Offset.m_Length, k[1] - s_Offset.m_Height };
}

// Store the handles flat and auto keys work out, so they stay put when the key becomes free.
static void StoreHandles(CurveModel& p_Curve, const size_t i) {
    if (i > 0) {
        p_Curve.m_KeyModels[i].m_In = StoredHandle(p_Curve, i, false);
    }

    if (i + 1 < p_Curve.m_Keys.size()) {
        p_Curve.m_KeyModels[i].m_Out = StoredHandle(p_Curve, i, true);
    }
}

static void SetHandle(CurveModel& p_Curve, const size_t i, const bool p_IsOut, const CurveHandle p_Handle) {
    StoreHandles(p_Curve, i);

    CurveKeyModel& s_Key = p_Curve.m_KeyModels[i];
    const bool s_StaysConstant = s_Key.m_Mode == ECurveTangentMode::Constant && !p_IsOut;

    s_Key.m_Mode = s_StaysConstant ? ECurveTangentMode::Constant : ECurveTangentMode::Free;
    (p_IsOut ? s_Key.m_Out : s_Key.m_In) = p_Handle;

    const bool s_HasOtherSide = p_IsOut ? i > 0 : i + 1 < p_Curve.m_Keys.size();

    if (!s_Key.m_IsMirrored || !s_HasOtherSide) {
        return;
    }

    CurveHandle& s_Other = p_IsOut ? s_Key.m_In : s_Key.m_Out;

    if (p_Handle.m_Length < 1e-9) {
        const double s_Size = std::abs(s_Other.m_Height) > 1e-9
            ? std::abs(s_Other.m_Height)
            : std::abs(p_Handle.m_Height);

        s_Other = { 0.0, p_Handle.m_Height >= 0 ? s_Size : -s_Size };
        return;
    }

    if (s_Other.m_Length < 1e-9) {
        s_Other.m_Length = SideDuration(p_Curve.m_Keys, i, !p_IsOut) * c_DefaultWeight;
    }

    s_Other.m_Height = p_Handle.m_Height / p_Handle.m_Length * s_Other.m_Length;
}

static void RebuildSegment(CurveModel& p_Curve, const size_t j) {
    CurveKeys& s_Keys = p_Curve.m_Keys;
    const double v0 = s_Keys[j][1];
    const double v1 = s_Keys[j + 1][1];
    const double D = s_Keys[j + 1][0] - s_Keys[j][0];

    Polynomial s_Polynomial = { v0, 0, 0, 0, 0, 0 };

    if (p_Curve.m_KeyModels[j].m_Mode != ECurveTangentMode::Constant && D > 1e-9) {
        const CurveHandle s_Out = HandleOffset(p_Curve, j, true);
        const CurveHandle s_In = HandleOffset(p_Curve, j + 1, false);

        s_Polynomial = FitSegment(v0, v1, s_Out.m_Length / D, s_Out.m_Height, s_In.m_Length / D, s_In.m_Height);

        for (int d = 0; d < 6; ++d) {
            s_Polynomial[d] += p_Curve.m_KeyModels[j].m_OutResidual[d];
        }
    }

    for (int d = 0; d < 6; ++d) {
        s_Keys[j][7 - d] = s_Polynomial[d];
    }
}

// The last key's coefficients are never evaluated, so we keep them constant.
static void ResetLastKey(CurveKeys& p_Keys) {
    CurveKey& s_LastKey = p_Keys.back();

    s_LastKey = { s_LastKey[0], s_LastKey[1], 0, 0, 0, 0, 0, s_LastKey[1] };
}

static void RebuildKeySegments(CurveModel& p_Curve, const size_t i, const bool p_In, const bool p_Out) {
    if (p_In && i > 0) {
        RebuildSegment(p_Curve, i - 1);
    }

    if (p_Out && i + 1 < p_Curve.m_Keys.size()) {
        RebuildSegment(p_Curve, i);
    }

    ResetLastKey(p_Curve.m_Keys);
}

void EditHandle(CurveModel& p_Curve, const size_t i, const bool p_IsOut, const CurveHandle p_Handle) {
    SetHandle(p_Curve, i, p_IsOut, p_Handle);

    const bool s_IsMirrored = p_Curve.m_KeyModels[i].m_IsMirrored;

    RebuildKeySegments(p_Curve, i, !p_IsOut || s_IsMirrored, p_IsOut || s_IsMirrored);
}

bool SetMirrored(CurveModel& p_Curve, const size_t i, const bool p_IsMirrored) {
    CurveKeyModel& s_Key = p_Curve.m_KeyModels[i];
    s_Key.m_IsMirrored = p_IsMirrored;

    const bool s_CanDiffer = i > 0 && i + 1 < p_Curve.m_Keys.size() &&
        (s_Key.m_Mode == ECurveTangentMode::Free || s_Key.m_Mode == ECurveTangentMode::Constant);

    if (!p_IsMirrored || !s_CanDiffer) {
        return false;
    }

    SetHandle(p_Curve, i, true, HandleOffset(p_Curve, i, true));
    RebuildKeySegments(p_Curve, i, true, false);

    return true;
}

void ResetResidual(CurveModel& p_Curve, const size_t i) {
    p_Curve.m_KeyModels[i].m_OutResidual = {};
    RebuildKeySegments(p_Curve, i, false, true);
}

void SetTangentMode(CurveModel& p_Curve, const size_t i, const ECurveTangentMode p_Mode) {
    const CurveKeys& s_Keys = p_Curve.m_Keys;
    double s_Shortest = std::numeric_limits<double>::infinity();
    double s_InSlope = 0.0;

    if (i > 0) {
        const CurveHandle s_In = StoredHandle(p_Curve, i, false);

        s_Shortest = SideDuration(s_Keys, i, false);
        s_InSlope = s_In.m_Length > 1e-9 ? s_In.m_Height / s_In.m_Length : 0.0;
    }

    if (i + 1 < s_Keys.size()) {
        s_Shortest = std::min(s_Shortest, SideDuration(s_Keys, i, true));
    }

    const double s_Length = s_Shortest * c_DefaultWeight;
    CurveKeyModel& s_Key = p_Curve.m_KeyModels[i];

    s_Key.m_Mode = p_Mode;
    s_Key.m_In = { s_Length, s_InSlope * s_Length };
    s_Key.m_Out = s_Key.m_In;

    RebuildKeySegments(p_Curve, i, true, true);
}

double ClampKeyTime(const CurveKeys& p_Keys, const size_t i, double t) {
    if (i > 0) {
        t = std::max(t, p_Keys[i - 1][0] + 0.001);
    }

    if (i + 1 < p_Keys.size()) {
        t = std::min(t, p_Keys[i + 1][0] - 0.001);
    }

    return t;
}

void MoveKey(CurveModel& p_Curve, const size_t i, const double t, const double v) {
    p_Curve.m_Keys[i][0] = t;
    p_Curve.m_Keys[i][1] = v;

    RebuildKeySegments(p_Curve, i, true, true);

    if (i > 0 && p_Curve.m_KeyModels[i - 1].m_Mode == ECurveTangentMode::Auto) {
        RebuildKeySegments(p_Curve, i - 1, true, false);
    }

    if (i + 1 < p_Curve.m_Keys.size() && p_Curve.m_KeyModels[i + 1].m_Mode == ECurveTangentMode::Auto) {
        RebuildKeySegments(p_Curve, i + 1, false, true);
    }
}

void SplitSegment(CurveModel& p_Curve, const size_t i, const double s) {
    CurveKeys& s_Keys = p_Curve.m_Keys;
    const CurveKey k = s_Keys[i];
    const CurveKey& s_NextKey = s_Keys[i + 1];

    // Reparametrise p(u) as p(s u) for the first part and p(s + (1 - s) u) for the second.
    const double p[6] = { k[7], k[6], k[5], k[4], k[3], k[2] };
    double q[6] = {};
    double h[6] = {};

    for (int d = 0; d <= 5; ++d) {
        q[d] = p[d] * std::pow(s, d);

        for (int j = 0; j <= d; ++j) {
            h[j] += p[d] * c_Binomials[d][j] * std::pow(s, d - j) * std::pow(1 - s, j);
        }
    }

    const double t = k[0] + s * (s_NextKey[0] - k[0]);

    s_Keys[i] = { k[0], k[1], q[5], q[4], q[3], q[2], q[1], q[0] };
    s_Keys.insert(
        s_Keys.begin() + i + 1,
        { t, EvaluateSegment(k, s), h[5], h[4], h[3], h[2], h[1], h[0] }
    );

    ECurveTangentMode s_Mode = ECurveTangentMode::Constant;

    if (p_Curve.m_KeyModels[i].m_Mode != ECurveTangentMode::Constant) {
        s_Mode = std::abs(DeriveSegment(s_Keys, i + 1).m_StartSlope) < 1e-4
            ? ECurveTangentMode::Flat
            : ECurveTangentMode::Free;
    }

    p_Curve.m_KeyModels.insert(p_Curve.m_KeyModels.begin() + i + 1, CurveKeyModel { .m_Mode = s_Mode });

    LoadSegmentHandles(p_Curve, i);
    LoadSegmentHandles(p_Curve, i + 1);

    p_Curve.m_SelectedKey = i + 1;
}

void RemoveKey(CurveModel& p_Curve, const size_t i) {
    const size_t n = p_Curve.m_Keys.size();

    if (n <= 2) {
        return;
    }

    p_Curve.m_Keys.erase(p_Curve.m_Keys.begin() + i);
    p_Curve.m_KeyModels.erase(p_Curve.m_KeyModels.begin() + i);

    if (i > 0 && i < n - 1) {
        p_Curve.m_KeyModels[i - 1].m_OutResidual = {};
        RebuildSegment(p_Curve, i - 1);
    }

    ResetLastKey(p_Curve.m_Keys);

    p_Curve.m_SelectedKey = i > 0 ? i - 1 : 0;
}

double SampleSegment(const CurveKey& p_Key, const CurveKey& p_NextKey, const double u) {
    return p_NextKey[0] <= p_Key[0] ? p_Key[1] : EvaluateSegment(p_Key, u);
}
