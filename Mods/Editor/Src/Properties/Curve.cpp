#include "Curve.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <format>

#include "implot.h"

#include "IconsMaterialDesign.h"
#include "IPluginInterface.h"
#include "Widgets.h"

#include "imgui_internal.h"

namespace {
    constexpr ImU32 c_CurveColor = IM_COL32(0x42, 0x96, 0xfa, 0xff);
    constexpr ImU32 c_GridColor = IM_COL32(0x3b, 0x3b, 0x40, 0xff);
    constexpr ImU32 c_ZeroLineColor = IM_COL32(0x6e, 0x6e, 0x80, 0xff);
    constexpr ImU32 c_PlotBackgroundColor = IM_COL32(0x0f, 0x0d, 0x0d, 0xff);
    constexpr ImU32 c_KeyStrokeColor = IM_COL32(0x0f, 0x0d, 0x0d, 0xff);
    constexpr ImU32 c_SelectedKeyStrokeColor = IM_COL32(0xfa, 0x00, 0x0d, 0xff);
    constexpr ImU32 c_HandleLineColor = IM_COL32(0xff, 0xff, 0xff, 0xb3);
    constexpr ImU32 c_TabUnderlineColor = IM_COL32(0xfa, 0x00, 0x0d, 0xff);

    constexpr ImVec4 c_WarningColor = ImVec4(1.f, 0.659f, 0.239f, 1.f);

    constexpr int c_SamplesPerSegment = 64;

    struct CurveRange {
        double m_T0;
        double m_T1;
        double m_V0;
        double m_V1;
    };

    CurveRange AutoRange(const CurveKeys& p_Keys) {
        double s_T0 = p_Keys.front()[0];
        double s_T1 = p_Keys.front()[0];
        double s_V0 = p_Keys.front()[1];
        double s_V1 = p_Keys.front()[1];

        for (size_t i = 0; i < p_Keys.size(); ++i) {
            s_T0 = std::min(s_T0, p_Keys[i][0]);
            s_T1 = std::max(s_T1, p_Keys[i][0]);
            s_V0 = std::min(s_V0, p_Keys[i][1]);
            s_V1 = std::max(s_V1, p_Keys[i][1]);

            if (i + 1 == p_Keys.size()) {
                continue;
            }

            for (int s = 0; s <= c_SamplesPerSegment; ++s) {
                const double v = SampleSegment(p_Keys[i], p_Keys[i + 1], static_cast<double>(s) / c_SamplesPerSegment);

                s_V0 = std::min(s_V0, v);
                s_V1 = std::max(s_V1, v);
            }
        }

        if (s_T1 - s_T0 < 1e-6) {
            s_T1 = s_T0 + 1;
        }

        const double s_ValuePadding = s_V1 - s_V0 > 0 ? (s_V1 - s_V0) * 0.1 : 0.5;
        const double s_TimePadding = (s_T1 - s_T0) * 0.03;

        return {
            s_T0 - s_TimePadding,
            s_T1 + s_TimePadding,
            s_V0 - s_ValuePadding,
            s_V1 + s_ValuePadding,
        };
    }

    double NiceStep(const double p_Value) {
        const double e = std::pow(10.0, std::floor(std::log10(p_Value)));
        const double f = p_Value / e;

        return (f < 1.5 ? 1 : f < 3.5 ? 2 : f < 7.5 ? 5 : 10) * e;
    }

    std::string FormatThreeDecimals(const double p_Value) {
        double s_Rounded = std::round(p_Value * 1000) / 1000;

        // Turn -0 into 0 so it doesn't print as -0.000.
        if (s_Rounded == 0) {
            s_Rounded = 0;
        }

        return std::format("{:.3f}", s_Rounded);
    }

    const char* GetTangentModeName(const ECurveTangentMode p_Mode) {
        switch (p_Mode) {
            case ECurveTangentMode::Auto:
                return "auto";
            case ECurveTangentMode::Free:
                return "free";
            case ECurveTangentMode::Flat:
                return "flat";
            case ECurveTangentMode::Constant:
                return "constant";
        }

        return "";
    }

    std::string GetCurveEditorKey(const std::string& p_Id, const ZEntityRef p_Entity) {
        return std::format("{:x}:{}", std::hash<ZEntityRef>{}(p_Entity), p_Id);
    }

    std::string FormatCurveReadout(const CurveEditorState& p_State) {
        if (p_State.m_CursorFrame >= ImGui::GetFrameCount() - 1) {
            return std::format(
                "t {}   v {}",
                FormatThreeDecimals(p_State.m_Cursor.first),
                FormatThreeDecimals(p_State.m_Cursor.second)
            );
        }

        const CurveKey& s_Key = p_State.m_Keys[p_State.m_SelectedKey];

        return std::format(
            "Key {} \xc2\xb7 t {}  v {}",
            p_State.m_SelectedKey,
            FormatThreeDecimals(s_Key[0]),
            FormatThreeDecimals(s_Key[1])
        );
    }

    void SyncState(CurveEditorState& p_State, const ZCurve* p_Curve) {
        const size_t n = p_Curve->data.size();
        bool s_IsInSync = p_State.m_Keys.size() == n && p_State.m_KeyModels.size() == n;

        for (size_t i = 0; s_IsInSync && i < n; ++i) {
            for (size_t j = 0; j < 8; ++j) {
                const float s_Value = static_cast<float>(p_State.m_Keys[i][j]);

                if (std::memcmp(&s_Value, &p_Curve->data[i][j], sizeof(float)) != 0) {
                    s_IsInSync = false;
                    break;
                }
            }
        }

        if (s_IsInSync) {
            return;
        }

        p_State.m_Keys.resize(n);

        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < 8; ++j) {
                p_State.m_Keys[i][j] = p_Curve->data[i][j];
            }
        }

        BuildModel(p_State);

        p_State.m_SelectedKey = std::min(p_State.m_SelectedKey, n > 0 ? n - 1 : 0);
    }

    void WriteCurve(CurveEditorState& p_State, ZCurve* p_Curve) {
        const size_t n = p_State.m_Keys.size();

        if (p_Curve->data.size() != n) {
            p_Curve->data.clear();

            for (size_t i = 0; i < n; ++i) {
                p_Curve->data.push_back(TFixedArray<float, 8>());
            }
        }

        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < 8; ++j) {
                // Round the state to floats too, so SyncState sees it as in sync.
                const float s_Value = static_cast<float>(p_State.m_Keys[i][j]);

                p_State.m_Keys[i][j] = s_Value;
                p_Curve->data[i][j] = s_Value;
            }
        }
    }

    void DrawMutedTextEllipsis(const ImVec2 p_Pos, const float p_MaxX, const char* p_Text) {
        ImGui::PushStyleColor(ImGuiCol_Text, c_MutedTextColor);
        ImGui::RenderTextEllipsis(
            ImGui::GetWindowDrawList(),
            p_Pos,
            ImVec2(p_MaxX, p_Pos.y + ImGui::GetFontSize()),
            p_MaxX,
            p_Text,
            nullptr,
            nullptr
        );
        ImGui::PopStyleColor();
    }

    void DrawTangentModeShape(
        ImDrawList* p_DrawList, const ImVec2 p_Center, const ECurveTangentMode p_Mode, const ImU32 p_Fill,
        const ImU32 p_Stroke, const float p_Scale
    ) {
        const float s_Thickness = 2.f * p_Scale;
        const float x = p_Center.x;
        const float y = p_Center.y;

        switch (p_Mode) {
            case ECurveTangentMode::Auto:
                p_DrawList->AddCircleFilled(p_Center, 5.5f * p_Scale, p_Fill);
                p_DrawList->AddCircle(p_Center, 5.5f * p_Scale, p_Stroke, 0, s_Thickness);
                break;

            case ECurveTangentMode::Flat: {
                const float r = 6.5f * p_Scale;
                const ImVec2 a(x, y - r), b(x + r, y), c(x, y + r), d(x - r, y);

                p_DrawList->AddQuadFilled(a, b, c, d, p_Fill);
                p_DrawList->AddQuad(a, b, c, d, p_Stroke, s_Thickness);
                break;
            }

            case ECurveTangentMode::Constant:
            case ECurveTangentMode::Free: {
                const float s_HalfWidth = (p_Mode == ECurveTangentMode::Constant ? 7.f : 5.f) * p_Scale;
                const float s_HalfHeight = (p_Mode == ECurveTangentMode::Constant ? 3.5f : 5.f) * p_Scale;
                const ImVec2 s_Min(x - s_HalfWidth, y - s_HalfHeight);
                const ImVec2 s_Max(x + s_HalfWidth, y + s_HalfHeight);

                p_DrawList->AddRectFilled(s_Min, s_Max, p_Fill);
                p_DrawList->AddRect(s_Min, s_Max, p_Stroke, 0.f, 0, s_Thickness);
                break;
            }
        }
    }

    // Split on double-click last, so a double-click on a handle or key doesn't split.
    bool DragHandlesAndKeys(CurveEditorState& p_State, const ImPlotRect& p_Limits, const float p_Scale) {
        bool s_IsChanged = false;

        const ImPlotPoint s_Mouse = ImPlot::GetPlotMousePos();
        const ImVec2 s_MouseDelta = ImGui::GetIO().MouseDelta;
        const bool s_HasMouseMoved = s_MouseDelta.x != 0.f || s_MouseDelta.y != 0.f;

        const double s_TimeStep = NiceStep(p_Limits.X.Size() / 20);
        const double s_ValueStep = NiceStep(p_Limits.Y.Size() / 20);

        auto SnapTime = [&](const double t) {
            return p_State.m_IsSnapOn ? std::round(t / s_TimeStep) * s_TimeStep : t;
        };

        auto SnapValue = [&](const double v) {
            return p_State.m_IsSnapOn ? std::round(v / s_ValueStep) * s_ValueStep : v;
        };

        bool s_IsOverTool = false;

        // Submit handles before keys so a key wins where they overlap.
        for (const bool s_IsOut : { false, true }) {
            const size_t s_Selected = p_State.m_SelectedKey;

            if (!HasHandle(p_State, s_Selected, s_IsOut)) {
                continue;
            }

            const auto [s_TipT, s_TipV] = HandleTip(p_State, s_Selected, s_IsOut);

            ImPlotPoint s_HandlePoint(s_TipT, s_TipV);
            bool s_IsHovered = false;
            bool s_IsHeld = false;

            ImPlot::DragPoint(
                s_IsOut ? 1 : 0,
                &s_HandlePoint.x,
                &s_HandlePoint.y,
                ImVec4(0.f, 0.f, 0.f, 0.f),
                6.f * p_Scale,
                ImPlotDragToolFlags_NoFit,
                nullptr,
                &s_IsHovered,
                &s_IsHeld
            );

            s_IsOverTool |= s_IsHovered || s_IsHeld;

            if (s_IsHovered && !s_IsHeld) {
                // Show the curve's own slope, which is shallower than the handle's when the handle is short.
                const double s_CurveSlope = s_IsOut
                    ? DeriveSegment(p_State.m_Keys, s_Selected).m_StartSlope
                    : DeriveSegment(p_State.m_Keys, s_Selected - 1).m_EndSlope;

                ImGui::SetTooltip(
                    "Curve slope %s, handle length %s in time",
                    FormatThreeDecimals(s_CurveSlope).c_str(),
                    FormatThreeDecimals(HandleOffset(p_State, s_Selected, s_IsOut).m_Length).c_str()
                );
            }

            if (s_IsHeld && s_HasMouseMoved) {
                const CurveKey& k = p_State.m_Keys[s_Selected];
                const double s_Dt = s_IsOut ? s_Mouse.x - k[0] : k[0] - s_Mouse.x;
                const double s_Length = std::clamp(s_Dt, 0.0, SideDuration(p_State.m_Keys, s_Selected, s_IsOut));
                const double s_Height = s_IsOut ? s_Mouse.y - k[1] : k[1] - s_Mouse.y;

                EditHandle(p_State, s_Selected, s_IsOut, { s_Length, s_Height });
                s_IsChanged = true;
            }
        }

        for (size_t i = 0; i < p_State.m_Keys.size(); ++i) {
            double x = p_State.m_Keys[i][0];
            double y = p_State.m_Keys[i][1];
            bool s_IsClicked = false;
            bool s_IsHovered = false;
            bool s_IsHeld = false;

            ImPlot::DragPoint(
                2 + static_cast<int>(i),
                &x,
                &y,
                ImVec4(0.f, 0.f, 0.f, 0.f),
                7.f * p_Scale,
                ImPlotDragToolFlags_NoFit,
                &s_IsClicked,
                &s_IsHovered,
                &s_IsHeld
            );

            s_IsOverTool |= s_IsHovered || s_IsHeld;

            if (s_IsClicked || s_IsHeld) {
                p_State.m_SelectedKey = i;
            }

            if (s_IsHovered && !s_IsHeld) {
                ImGui::SetTooltip(
                    "Key %zu \xc2\xb7 %s \xc2\xb7 t %s, v %s",
                    i,
                    GetTangentModeName(p_State.m_KeyModels[i].m_Mode),
                    FormatThreeDecimals(p_State.m_Keys[i][0]).c_str(),
                    FormatThreeDecimals(p_State.m_Keys[i][1]).c_str()
                );
            }

            if (s_IsHeld && s_HasMouseMoved) {
                const bool s_IsEndpoint = i == 0 || i + 1 == p_State.m_Keys.size();
                const double t = s_IsEndpoint
                    ? p_State.m_Keys[i][0]
                    : ClampKeyTime(p_State.m_Keys, i, SnapTime(s_Mouse.x));

                MoveKey(p_State, i, t, SnapValue(s_Mouse.y));
                s_IsChanged = true;
            }
        }

        if (ImPlot::IsPlotHovered()) {
            p_State.m_Cursor = { s_Mouse.x, s_Mouse.y };
            p_State.m_CursorFrame = ImGui::GetFrameCount();

            if (!s_IsOverTool && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                const CurveKeys& s_Keys = p_State.m_Keys;

                for (size_t i = 0; i + 1 < s_Keys.size(); ++i) {
                    if (s_Mouse.x > s_Keys[i][0] && s_Mouse.x < s_Keys[i + 1][0]) {
                        SplitSegment(p_State, i, (s_Mouse.x - s_Keys[i][0]) / (s_Keys[i + 1][0] - s_Keys[i][0]));
                        s_IsChanged = true;
                        break;
                    }
                }
            }
        }

        return s_IsChanged;
    }

    void DrawCurve(const CurveEditorState& p_State, const ImPlotRect& p_Limits, const float p_Scale, const bool p_IsLarge) {
        const ImVec2 s_PlotPos = ImPlot::GetPlotPos();
        const ImVec2 s_PlotSize = ImPlot::GetPlotSize();

        const CurveKeys& s_Keys = p_State.m_Keys;
        const size_t n = s_Keys.size();
        const size_t s_Selected = p_State.m_SelectedKey;

        ImDrawList* s_DrawList = ImPlot::GetPlotDrawList();

        ImPlot::PushPlotClipRect();

        if (p_Limits.Y.Min < 0 && p_Limits.Y.Max > 0) {
            const float s_ZeroY = ImPlot::PlotToPixels(0.0, 0.0).y;

            s_DrawList->AddLine(
                ImVec2(s_PlotPos.x, s_ZeroY),
                ImVec2(s_PlotPos.x + s_PlotSize.x, s_ZeroY),
                c_ZeroLineColor,
                p_Scale
            );
        }

        const float s_CurveThickness = (p_IsLarge ? 2.5f : 2.2f) * p_Scale;

        for (size_t j = 0; j + 1 < n; ++j) {
            ImVec2 s_Points[c_SamplesPerSegment + 1];

            for (int s = 0; s <= c_SamplesPerSegment; ++s) {
                const double u = static_cast<double>(s) / c_SamplesPerSegment;
                const double t = s_Keys[j][0] + (s_Keys[j + 1][0] - s_Keys[j][0]) * u;

                s_Points[s] = ImPlot::PlotToPixels(t, SampleSegment(s_Keys[j], s_Keys[j + 1], u));
            }

            s_DrawList->AddPolyline(s_Points, c_SamplesPerSegment + 1, c_CurveColor, 0, s_CurveThickness);

            const double s_EndValue = SampleSegment(s_Keys[j], s_Keys[j + 1], 1.0);

            if (!IsNear(s_EndValue, s_Keys[j + 1][1])) {
                const ImVec2 s_From = ImPlot::PlotToPixels(s_Keys[j + 1][0], s_EndValue);
                const float s_To = ImPlot::PlotToPixels(s_Keys[j + 1][0], s_Keys[j + 1][1]).y;
                const float s_Direction = s_To > s_From.y ? 1.f : -1.f;
                const float s_Dash = (p_IsLarge ? 4.f : 3.f) * p_Scale;
                const float s_Gap = 3.f * p_Scale;

                for (float y = s_From.y; (s_To - y) * s_Direction > 0; y += (s_Dash + s_Gap) * s_Direction) {
                    const float s_DashEnd = s_Direction > 0 ? std::min(y + s_Dash, s_To) : std::max(y - s_Dash, s_To);

                    s_DrawList->AddLine(
                        ImVec2(s_From.x, y),
                        ImVec2(s_From.x, s_DashEnd),
                        c_CurveColor,
                        1.5f * p_Scale
                    );
                }
            }
        }

        const ImVec2 s_KeyPixel = ImPlot::PlotToPixels(s_Keys[s_Selected][0], s_Keys[s_Selected][1]);

        for (const bool s_IsOut : { false, true }) {
            if (!HasHandle(p_State, s_Selected, s_IsOut)) {
                continue;
            }

            const auto [s_TipT, s_TipV] = HandleTip(p_State, s_Selected, s_IsOut);
            const ImVec2 s_HandlePixel = ImPlot::PlotToPixels(s_TipT, s_TipV);
            const float s_Radius = (p_IsLarge ? 6.f : 5.f) * p_Scale;

            s_DrawList->AddLine(s_KeyPixel, s_HandlePixel, c_HandleLineColor, 1.2f * p_Scale);
            s_DrawList->AddCircleFilled(s_HandlePixel, s_Radius, c_CurveColor);
            s_DrawList->AddCircle(s_HandlePixel, s_Radius, IM_COL32_WHITE, 0, 1.5f * p_Scale);
        }

        for (size_t i = 0; i < n; ++i) {
            DrawTangentModeShape(
                s_DrawList,
                ImPlot::PlotToPixels(s_Keys[i][0], s_Keys[i][1]),
                p_State.m_KeyModels[i].m_Mode,
                IM_COL32_WHITE,
                i == s_Selected ? c_SelectedKeyStrokeColor : c_KeyStrokeColor,
                p_Scale
            );
        }

        ImPlot::PopPlotClipRect();
    }

    bool DrawCurvePlot(CurveEditorState& p_State, const float p_Width, const float p_Height, const bool p_IsLarge) {
        bool s_IsChanged = false;
        const float s_Scale = ScaledSize(1.f);

        // Double-click adds a key, so fit with the middle button instead.
        ImPlotInputMap& s_InputMap = ImPlot::GetInputMap();
        const ImPlotInputMap s_InputMapBackup = s_InputMap;
        s_InputMap.Fit = ImGuiMouseButton_Middle;

        ImPlot::PushStyleColor(ImPlotCol_FrameBg, IM_COL32(0, 0, 0, 0));
        ImPlot::PushStyleColor(ImPlotCol_PlotBg, c_PlotBackgroundColor);
        ImPlot::PushStyleColor(ImPlotCol_PlotBorder, IM_COL32(0, 0, 0, 0));
        ImPlot::PushStyleColor(ImPlotCol_AxisGrid, c_GridColor);
        ImPlot::PushStyleColor(ImPlotCol_AxisText, ImVec4(1.f, 1.f, 1.f, 0.6f));
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0.f, 0.f));
        ImGui::PushFont(nullptr, FontSize(p_IsLarge ? 12.f : 11.f));

        if (ImPlot::BeginPlot(p_IsLarge ? "##ZCurveLarge" : "##ZCurveInline", ImVec2(p_Width, p_Height), ImPlotFlags_CanvasOnly)) {
            const ImPlotAxisFlags s_AxisFlags = ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_NoSideSwitch |
                ImPlotAxisFlags_NoHighlight | (p_IsLarge ? 0 : ImPlotAxisFlags_Lock);

            ImPlot::SetupAxes(p_IsLarge ? "Time" : nullptr, p_IsLarge ? "Value" : nullptr, s_AxisFlags, s_AxisFlags);

            // Don't fit while the mouse is down, so the view doesn't move under a dragged key.
            const bool s_ShouldFit = p_IsLarge ? p_State.m_ShouldFitLarge : !ImGui::IsMouseDown(ImGuiMouseButton_Left);

            if (s_ShouldFit) {
                const CurveRange s_Range = AutoRange(p_State.m_Keys);

                ImPlot::SetupAxesLimits(s_Range.m_T0, s_Range.m_T1, s_Range.m_V0, s_Range.m_V1, ImPlotCond_Always);

                if (p_IsLarge) {
                    p_State.m_ShouldFitLarge = false;
                }
            }

            ImPlot::SetupFinish();

            const ImPlotRect s_Limits = ImPlot::GetPlotLimits();

            s_IsChanged |= DragHandlesAndKeys(p_State, s_Limits, s_Scale);
            DrawCurve(p_State, s_Limits, s_Scale, p_IsLarge);

            ImPlot::EndPlot();
        }

        ImGui::PopFont();
        ImPlot::PopStyleVar();
        ImPlot::PopStyleColor(5);

        s_InputMap = s_InputMapBackup;

        return s_IsChanged;
    }

    constexpr float c_KeyPanelRowHeight = 24.f;
    constexpr ImGuiSliderFlags c_ExactFlags = ImGuiSliderFlags_NoRoundToFormat;

    struct KeyPanelRow {
        float m_X;
        float m_Width;
        float m_Top;
        float m_ValueWidth;
    };

    KeyPanelRow BeginKeyPanelRow(const char* p_Label, const float p_X, const float p_Width) {
        const float s_LabelWidth = ScaledSize(104.f);
        const float s_Top = ImGui::GetCursorScreenPos().y;
        const float s_FrameY = s_Top + (ScaledSize(c_KeyPanelRowHeight) - ImGui::GetFrameHeight()) * 0.5f;

        ImGui::SetCursorScreenPos(ImVec2(p_X + ScaledSize(6.f), s_FrameY));
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(p_Label);

        ImGui::SetCursorScreenPos(ImVec2(p_X + s_LabelWidth, s_FrameY));

        return { p_X, p_Width, s_Top, p_Width - s_LabelWidth - ScaledSize(4.f) };
    }

    void EndKeyPanelRow(const KeyPanelRow& p_Row) {
        const float s_Bottom = p_Row.m_Top + ScaledSize(c_KeyPanelRowHeight);
        const float s_BorderY = s_Bottom - ScaledSize(0.5f);

        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(p_Row.m_X, s_BorderY),
            ImVec2(p_Row.m_X + p_Row.m_Width, s_BorderY),
            ImGui::GetColorU32(c_RowBorderColor),
            ScaledSize(1.f)
        );

        ImGui::SetCursorScreenPos(ImVec2(p_Row.m_X, s_Bottom));
    }

    bool DrawKeyTabs(CurveEditorState& p_State, const float p_Width) {
        bool s_IsChanged = false;

        const float s_FrameHeight = ImGui::GetFrameHeight();
        const float s_X0 = ImGui::GetCursorScreenPos().x;
        const float s_X1 = s_X0 + p_Width;
        const float s_Gap = ScaledSize(2.f);
        const size_t n = p_State.m_Keys.size();

        float x = s_X0;
        float y = ImGui::GetCursorScreenPos().y;

        ImGui::PushStyleVarX(ImGuiStyleVar_FramePadding, ScaledSize(6.f));

        for (size_t i = 0; i < n; ++i) {
            const bool s_IsSelected = i == p_State.m_SelectedKey;
            const std::string s_Label = std::format("Key {}##CurveKeyTab{}", i, i);

            if (s_IsSelected) {
                PushSelectedButtonStyle();
            }

            const float s_TabWidth = ImGui::CalcTextSize(s_Label.c_str(), nullptr, true).x + ScaledSize(12.f);

            if (i > 0 && x + s_Gap + s_TabWidth > s_X1 - ScaledSize(44.f)) {
                x = s_X0;
                y += s_FrameHeight + s_Gap;
            }
            else if (i > 0) {
                x += s_Gap;
            }

            ImGui::SetCursorScreenPos(ImVec2(x, y));

            if (ImGui::Button(s_Label.c_str(), ImVec2(s_TabWidth, s_FrameHeight))) {
                p_State.m_SelectedKey = i;
            }

            if (s_IsSelected) {
                PopSelectedButtonStyle();
            }

            x += s_TabWidth;
        }

        ImGui::PopStyleVar();

        ImGui::SetCursorScreenPos(ImVec2(x + s_Gap, y));

        if (GhostButton(ICON_MD_ADD, "##CurveAddKey", "Add key after the selected key")) {
            const size_t s_Segment = p_State.m_SelectedKey < n - 1 ? p_State.m_SelectedKey : n - 2;

            SplitSegment(p_State, s_Segment, 0.5);
            s_IsChanged = true;
        }

        ImGui::SetCursorScreenPos(ImVec2(s_X1 - ScaledSize(20.f), y));
        ImGui::BeginDisabled(n <= 2);

        if (GhostButton(ICON_MD_DELETE, "##CurveDeleteKey", "Delete selected key")) {
            RemoveKey(p_State, p_State.m_SelectedKey);
            s_IsChanged = true;
        }

        ImGui::EndDisabled();

        const float s_UnderlineY = y + s_FrameHeight - ScaledSize(0.5f);

        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(s_X0, s_UnderlineY), ImVec2(s_X1, s_UnderlineY), c_TabUnderlineColor, ScaledSize(1.f)
        );

        ImGui::SetCursorScreenPos(ImVec2(s_X0, y + s_FrameHeight + ScaledSize(6.f)));

        return s_IsChanged;
    }

    bool DrawTangentRow(CurveEditorState& p_State, const float p_Width, const bool p_IsLarge) {
        struct ModeButton {
            ECurveTangentMode m_Mode;
            const char* m_Label;
            const char* m_Tooltip;
        };

        static constexpr ModeButton c_Modes[] = {
            { ECurveTangentMode::Auto, "Auto", "Slope follows the neighbouring keys" },
            { ECurveTangentMode::Flat, "Flat", "Zero slope: ease in and out" },
            { ECurveTangentMode::Constant, "Constant", "Hold this value until the next key" },
        };

        bool s_IsChanged = false;

        ImDrawList* s_DrawList = ImGui::GetWindowDrawList();
        const float s_FrameHeight = ImGui::GetFrameHeight();
        const float s_Gap = ScaledSize(2.f);
        const size_t s_Selected = p_State.m_SelectedKey;

        const KeyPanelRow s_Row = BeginKeyPanelRow("Tangent", ImGui::GetCursorScreenPos().x, p_Width);
        const float s_ValueWidth = p_IsLarge ? std::min(s_Row.m_ValueWidth, ScaledSize(420.f)) : s_Row.m_ValueWidth;

        const float s_MirrorWidth = ScaledSize(20.f);
        const float s_ButtonWidth = (s_ValueWidth - s_MirrorWidth - s_Gap * 3) / 3;
        const ECurveTangentMode s_CurrentMode = p_State.m_KeyModels[s_Selected].m_Mode;

        for (size_t m = 0; m < std::size(c_Modes); ++m) {
            const ECurveTangentMode s_Mode = c_Modes[m].m_Mode;
            const char* s_Label = c_Modes[m].m_Label;
            const bool s_IsOn = s_Mode == s_CurrentMode;

            if (m > 0) {
                ImGui::SameLine(0.f, s_Gap);
            }

            if (s_IsOn) {
                PushSelectedButtonStyle();
            }

            const bool s_IsClicked = ImGui::Button(
                std::format("##CurveMode{}", m).c_str(),
                ImVec2(s_ButtonWidth, s_FrameHeight)
            );

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", c_Modes[m].m_Tooltip);
            }

            const ImVec2 s_Min = ImGui::GetItemRectMin();
            const float s_IconSize = ScaledSize(10.f);
            const float s_IconGap = ScaledSize(4.f);
            const float s_ContentWidth = std::min(s_ButtonWidth, s_IconSize + s_IconGap + ImGui::CalcTextSize(s_Label).x);
            const float s_ContentX = s_Min.x + (s_ButtonWidth - s_ContentWidth) * 0.5f;

            ImGui::PushClipRect(s_Min, ImGui::GetItemRectMax(), true);

            DrawTangentModeShape(
                s_DrawList,
                ImVec2(s_ContentX + s_IconSize * 0.5f, s_Min.y + s_FrameHeight * 0.5f),
                s_Mode,
                ImGui::GetColorU32(ImGuiCol_Text),
                IM_COL32_BLACK_TRANS,
                ScaledSize(0.8f)
            );

            s_DrawList->AddText(
                ImVec2(s_ContentX + s_IconSize + s_IconGap, s_Min.y + (s_FrameHeight - ImGui::GetFontSize()) * 0.5f),
                ImGui::GetColorU32(ImGuiCol_Text),
                s_Label
            );

            ImGui::PopClipRect();

            if (s_IsOn) {
                PopSelectedButtonStyle();
            }

            if (s_IsClicked) {
                SetTangentMode(p_State, s_Selected, s_Mode);
                s_IsChanged = true;
            }
        }

        ImGui::SameLine(0.f, s_Gap);

        const bool s_IsMirrored = p_State.m_KeyModels[s_Selected].m_IsMirrored;

        if (GhostButton(ICON_MD_LINK, "##CurveMirror", "Mirror handles: both sides keep the same angle", s_IsMirrored)) {
            s_IsChanged |= SetMirrored(p_State, s_Selected, !s_IsMirrored);
        }

        EndKeyPanelRow(s_Row);

        return s_IsChanged;
    }

    // We draw this by hand so it fits the panel's width and both editors share its open state.
    void DrawAdvancedHeader(CurveEditorState& p_State, const float p_Width) {
        ImDrawList* s_DrawList = ImGui::GetWindowDrawList();
        const ImVec2 s_Min = ImGui::GetCursorScreenPos();
        const ImVec2 s_Max(s_Min.x + p_Width, s_Min.y + ScaledSize(22.f));

        if (ImGui::InvisibleButton("##CurveAdvanced", ImVec2(p_Width, s_Max.y - s_Min.y))) {
            p_State.m_IsAdvancedOpen = !p_State.m_IsAdvancedOpen;
        }

        s_DrawList->AddRectFilled(
            s_Min,
            s_Max,
            ImGui::GetColorU32(ImGui::IsItemHovered() ? c_SectionHeaderHoverColor : c_SectionHeaderColor)
        );
        s_DrawList->AddLine(s_Min, ImVec2(s_Max.x, s_Min.y), ImGui::GetColorU32(c_SectionBorderColor), ScaledSize(1.f));

        const float s_TextY = s_Min.y + (s_Max.y - s_Min.y - ImGui::GetFontSize()) * 0.5f;

        ImGui::RenderArrow(
            s_DrawList,
            ImVec2(s_Min.x + ScaledSize(6.f), s_TextY),
            ImGui::GetColorU32(ImGuiCol_Text),
            p_State.m_IsAdvancedOpen ? ImGuiDir_Down : ImGuiDir_Right,
            0.7f
        );

        ImGui::PushFont(SDK()->GetImGuiBoldFont(), 0.f);
        s_DrawList->AddText(
            ImVec2(s_Min.x + ScaledSize(6.f) + ImGui::GetFontSize() * 0.7f + ScaledSize(6.f), s_TextY),
            ImGui::GetColorU32(ImGuiCol_Text),
            "Advanced"
        );
        ImGui::PopFont();

        ImGui::SetCursorScreenPos(ImVec2(s_Min.x, s_Max.y));
    }

    bool DrawKeyFields(CurveEditorState& p_State, const float p_Width) {
        bool s_IsChanged = false;

        const CurveKeys& s_Keys = p_State.m_Keys;
        const size_t s_Selected = p_State.m_SelectedKey;
        const float s_X0 = ImGui::GetCursorScreenPos().x;

        {
            double s_Time = s_Keys[s_Selected][0];
            const KeyPanelRow s_Row = BeginKeyPanelRow("Time", s_X0, p_Width);

            // Endpoint times are locked, but the raw T field can still change them.
            const bool s_IsEndpoint = s_Selected == 0 || s_Selected + 1 == s_Keys.size();

            ImGui::BeginDisabled(s_IsEndpoint);

            const bool s_IsTimeChanged = TaggedDragScalar(
                "##CurveTime", "T", c_AxisTagColors[2], ImGuiDataType_Double, &s_Time, s_Row.m_ValueWidth, 0.01f,
                "%.3f", c_ExactFlags
            );

            ImGui::EndDisabled();

            if (s_IsEndpoint && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("The first and last key times are locked");
            }

            if (s_IsTimeChanged) {
                MoveKey(p_State, s_Selected, ClampKeyTime(s_Keys, s_Selected, s_Time), s_Keys[s_Selected][1]);
                s_IsChanged = true;
            }

            EndKeyPanelRow(s_Row);
        }

        {
            double s_Value = s_Keys[s_Selected][1];
            const KeyPanelRow s_Row = BeginKeyPanelRow("Value", s_X0, p_Width);

            if (TaggedDragScalar(
                "##CurveValue", "V", c_AxisTagColors[1], ImGuiDataType_Double, &s_Value, s_Row.m_ValueWidth, 0.01f,
                "%.3f", c_ExactFlags
            )) {
                MoveKey(p_State, s_Selected, s_Keys[s_Selected][0], s_Value);
                s_IsChanged = true;
            }

            EndKeyPanelRow(s_Row);
        }

        const bool s_HasIn = s_Selected > 0;
        const bool s_HasOut = s_Selected + 1 < s_Keys.size();
        const SegmentDerivatives s_InSegment = s_HasIn ? DeriveSegment(s_Keys, s_Selected - 1) : SegmentDerivatives {};
        const SegmentDerivatives s_OutSegment = s_HasOut ? DeriveSegment(s_Keys, s_Selected) : SegmentDerivatives {};

        // Returns the edited side (0 in, 1 out, -1 none) and its new value.
        auto DrawInOutRow = [&](const char* p_Label, const char* p_Id, std::array<double, 2> p_Values, const bool p_IsReadOnly) {
            const KeyPanelRow s_Row = BeginKeyPanelRow(p_Label, s_X0, p_Width);
            const float s_FieldSpacing = ScaledSize(3.f);
            const float s_FieldWidth = (s_Row.m_ValueWidth - s_FieldSpacing) / 2;
            int s_EditedSide = -1;

            for (int s_Side = 0; s_Side < 2; ++s_Side) {
                const bool s_HasSide = s_Side == 1 ? s_HasOut : s_HasIn;

                if (s_Side == 1) {
                    ImGui::SameLine(0.f, s_FieldSpacing);
                }

                ImGui::BeginDisabled(!s_HasSide);

                const bool s_IsEdited = TaggedDragScalar(
                    std::format("##{}{}", p_Id, s_Side),
                    s_Side == 1 ? "out" : "in",
                    c_ValueTagColor,
                    ImGuiDataType_Double,
                    &p_Values[s_Side],
                    s_FieldWidth,
                    p_IsReadOnly ? 0.f : 0.01f,
                    s_HasSide ? "%.3f" : "\xe2\x80\x94",
                    p_IsReadOnly ? ImGuiSliderFlags_ReadOnly : c_ExactFlags
                );

                ImGui::EndDisabled();

                if (s_IsEdited && s_HasSide) {
                    s_EditedSide = s_Side;
                }
            }

            EndKeyPanelRow(s_Row);

            return std::pair { s_EditedSide, s_EditedSide >= 0 ? p_Values[s_EditedSide] : 0.0 };
        };

        const auto [s_SlopeSide, s_Slope] = DrawInOutRow(
            "Slope", "CurveSlope", { s_InSegment.m_EndSlope, s_OutSegment.m_StartSlope }, false
        );

        if (s_SlopeSide >= 0) {
            const bool s_IsOut = s_SlopeSide == 1;
            double s_Length = HandleOffset(p_State, s_Selected, s_IsOut).m_Length;

            if (s_Length < 1e-9) {
                s_Length = SideDuration(s_Keys, s_Selected, s_IsOut) * c_DefaultWeight;
            }

            EditHandle(p_State, s_Selected, s_IsOut, { s_Length, s_Slope * s_Length });
            s_IsChanged = true;
        }

        DrawInOutRow(
            "Curvature", "CurveCurvature", { s_InSegment.m_EndCurvature, s_OutSegment.m_StartCurvature }, true
        );

        const KeyPanelRow s_ShapeRow = BeginKeyPanelRow("Shape", s_X0, p_Width);
        const std::array<double, 6>& s_Residual = p_State.m_KeyModels[s_Selected].m_OutResidual;

        ImGui::AlignTextToFramePadding();

        if (!s_HasOut) {
            ImGui::TextColored(c_MutedTextColor, "\xe2\x80\x94");
        }
        else if (s_Residual == std::array<double, 6> {}) {
            ImGui::TextColored(c_MutedTextColor, "Follows the handles");
        }
        else {
            if (ImGui::Button("Reset to handles")) {
                ResetResidual(p_State, s_Selected);

                s_IsChanged = true;
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "The segment's floats have detail the handles don't describe, which edits keep. Reset to shape "
                    "the segment from its handles only."
                );
            }
        }

        EndKeyPanelRow(s_ShapeRow);

        return s_IsChanged;
    }

    bool DrawRawFloats(CurveEditorState& p_State, const float p_Width) {
        static constexpr const char* c_RawLabels[] = { "T", "V", "c5", "c4", "c3", "c2", "c1", "c0" };

        bool s_IsChanged = false;

        const CurveKeys& s_Keys = p_State.m_Keys;
        const size_t s_Selected = p_State.m_SelectedKey;
        const bool s_IsLast = s_Selected + 1 == s_Keys.size();
        const float s_X = ImGui::GetCursorScreenPos().x;
        const float s_FieldSpacing = ScaledSize(3.f);
        const float s_CellWidth = (p_Width - s_FieldSpacing * 3) / 4;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(s_FieldSpacing, ScaledSize(5.f)));
        ImGui::PushTextWrapPos(s_X + p_Width);

        ImGui::TextColored(
            c_MutedTextColor,
            "%s",
            s_IsLast
                ? "Raw floats. Last key: c5\xe2\x80\xa6" "c0 are never evaluated and are kept as a constant."
                : "Raw floats. c5\xe2\x80\xa6" "c0 shape the segment from this key to the next."
        );

        for (size_t j = 0; j < 8; ++j) {
            ImGui::SetCursorScreenPos(
                ImVec2(
                    s_X + (j % 4) * (s_CellWidth + s_FieldSpacing),
                    j % 4 == 0 ? ImGui::GetCursorScreenPos().y : ImGui::GetItemRectMin().y
                )
            );

            const ImU32 s_TagColor = j == 0 ? c_AxisTagColors[2] : j == 1 ? c_AxisTagColors[1] : c_ValueTagColor;
            const bool s_IsUnused = s_IsLast && j > 1;
            double s_Raw = s_Keys[s_Selected][j];

            ImGui::BeginDisabled(s_IsUnused);

            const bool s_IsRawChanged = TaggedDragScalar(
                std::format("##CurveRaw{}", j),
                c_RawLabels[j],
                s_TagColor,
                ImGuiDataType_Double,
                &s_Raw,
                s_CellWidth,
                0.01f,
                "%g",
                c_ExactFlags
            );

            ImGui::EndDisabled();

            if (!s_IsRawChanged) {
                continue;
            }

            s_IsChanged = true;

            if (j == 0) {
                MoveKey(p_State, s_Selected, s_Raw, s_Keys[s_Selected][1]);
            }
            else if (j == 1) {
                MoveKey(p_State, s_Selected, s_Keys[s_Selected][0], s_Raw);
            }
            else {
                p_State.m_Keys[s_Selected][j] = s_Raw;
                BuildModel(p_State);
            }
        }

        const auto s_DrawWarning = [&](const char* p_Warning) {
            ImGui::SetCursorScreenPos(ImVec2(s_X, ImGui::GetCursorScreenPos().y));
            ImGui::PushStyleColor(ImGuiCol_Text, c_WarningColor);
            ImGui::TextUnformatted(ICON_MD_WARNING);
            ImGui::SameLine(0.f, ScaledSize(5.f));
            ImGui::TextUnformatted(p_Warning);
            ImGui::PopStyleColor();
        };

        if (!s_IsLast && !IsNear(s_Keys[s_Selected][1], s_Keys[s_Selected][7])) {
            s_DrawWarning("Value [1] differs from c0 of the outgoing segment.");
        }

        if (s_Selected > 0 &&
            p_State.m_KeyModels[s_Selected - 1].m_Mode != ECurveTangentMode::Constant &&
            !IsNear(DeriveSegment(s_Keys, s_Selected - 1).m_EndValue, s_Keys[s_Selected][1])) {
            s_DrawWarning("The incoming segment does not end on this key.");
        }

        ImGui::PopTextWrapPos();
        ImGui::PopStyleVar();

        return s_IsChanged;
    }

    bool DrawCurveKeyPanel(CurveEditorState& p_State, const float p_Width, const bool p_IsLarge) {
        const float s_X0 = ImGui::GetCursorScreenPos().x;

        bool s_IsChanged = DrawKeyTabs(p_State, p_Width);

        s_IsChanged |= DrawTangentRow(p_State, p_Width, p_IsLarge);

        DrawAdvancedHeader(p_State, p_Width);

        if (!p_State.m_IsAdvancedOpen) {
            ImGui::Dummy(ImVec2(p_Width, 0.f));
            return s_IsChanged;
        }

        const float s_ColumnGap = ScaledSize(12.f);
        const float s_FieldsWidth = p_IsLarge ? (p_Width - s_ColumnGap) / 2 : p_Width;
        const float s_FieldsTop = ImGui::GetCursorScreenPos().y;

        s_IsChanged |= DrawKeyFields(p_State, s_FieldsWidth);

        const float s_FieldsBottom = ImGui::GetCursorScreenPos().y;
        const float s_RawX = p_IsLarge ? s_X0 + s_FieldsWidth + s_ColumnGap : s_X0 + ScaledSize(6.f);
        const float s_RawWidth = p_IsLarge ? p_Width - s_FieldsWidth - s_ColumnGap : p_Width - ScaledSize(12.f);

        ImGui::SetCursorScreenPos(ImVec2(s_RawX, (p_IsLarge ? s_FieldsTop : s_FieldsBottom) + ScaledSize(6.f)));

        s_IsChanged |= DrawRawFloats(p_State, s_RawWidth);

        ImGui::SetCursorScreenPos(ImVec2(s_X0, std::max(s_FieldsBottom, ImGui::GetCursorScreenPos().y)));
        ImGui::Dummy(ImVec2(p_Width, 0.f));

        return s_IsChanged;
    }

    bool DrawLargeEditor(CurveEditorState& p_State, const std::string& p_Key) {
        bool s_IsChanged = false;
        bool s_IsOpen = true;

        const std::string s_Title = std::format(
            "{} ZCurve  {} keys###ZCurveEditor{}", ICON_MD_SHOW_CHART, p_State.m_Keys.size(), p_Key
        );

        ImGui::SetNextWindowSize(ImVec2(ScaledSize(872.f), ScaledSize(760.f)), ImGuiCond_FirstUseEver);

        if (ImGui::Begin(s_Title.c_str(), &s_IsOpen)) {
            const float s_Width = ImGui::GetContentRegionAvail().x;
            const float s_StartX = ImGui::GetCursorScreenPos().x;

            if (ImGui::Button(ICON_MD_FIT_SCREEN " Fit")) {
                p_State.m_ShouldFitLarge = true;
            }

            ImGui::SameLine(0.f, ScaledSize(3.f));

            if (GhostButton(ICON_MD_GRID_4X4, "##CurveSnap", "Snap to grid", p_State.m_IsSnapOn)) {
                p_State.m_IsSnapOn = !p_State.m_IsSnapOn;
            }

            ImGui::SameLine(0.f, ScaledSize(4.f));
            DrawToolbarSeparator();
            ImGui::SameLine(0.f, ScaledSize(4.f));

            const std::string s_Readout = FormatCurveReadout(p_State);
            const float s_ReadoutTextWidth = ImGui::CalcTextSize(s_Readout.c_str()).x;
            const float s_ReadoutRight = s_StartX + s_Width - ScaledSize(4.f);
            const float s_HintMaxX = s_ReadoutRight - std::max(ScaledSize(150.f), s_ReadoutTextWidth) - ScaledSize(6.f);

            static constexpr const char* c_Hint =
                "Drag keys and handles \xc2\xb7 double-click the curve to add a key \xc2\xb7 scroll to zoom \xc2\xb7 drag "
                "empty space to pan";

            const ImVec2 s_HintPos(
                ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y + ImGui::GetStyle().FramePadding.y
            );

            DrawMutedTextEllipsis(s_HintPos, s_HintMaxX, c_Hint);

            ImGui::SameLine();
            ImGui::SetCursorScreenPos(ImVec2(s_ReadoutRight - s_ReadoutTextWidth, ImGui::GetCursorScreenPos().y));
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(c_MutedTextColor, "%s", s_Readout.c_str());

            // Use the height the key panel left over last frame.
            const float s_PlotHeight = std::max(
                ScaledSize(200.f),
                ImGui::GetContentRegionAvail().y - p_State.m_KeyPanelHeight - ImGui::GetStyle().ItemSpacing.y
            );

            s_IsChanged |= DrawCurvePlot(p_State, s_Width, s_PlotHeight, true);

            const float s_PanelTop = ImGui::GetCursorScreenPos().y;

            s_IsChanged |= DrawCurveKeyPanel(p_State, s_Width, true);

            p_State.m_KeyPanelHeight = ImGui::GetCursorScreenPos().y - s_PanelTop;
        }

        ImGui::End();

        if (!s_IsOpen) {
            p_State.m_IsLargeEditorOpen = false;
        }

        return s_IsChanged;
    }
}

bool CurveEditor::DrawSummary(const std::string& p_Id, const ZEntityRef p_Entity, ZCurve* p_Curve) {
    const size_t s_KeyCount = p_Curve->data.size();

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(c_MutedTextColor, "%zu %s", s_KeyCount, s_KeyCount == 1 ? "key" : "keys");

    const std::string s_Key = GetCurveEditorKey(p_Id, p_Entity);
    const auto s_It = m_States.find(s_Key);

    if (s_It == m_States.end() || !s_It->second.m_IsLargeEditorOpen || s_KeyCount < 2) {
        return false;
    }

    CurveEditorState& s_State = s_It->second;

    SyncState(s_State, p_Curve);

    if (!DrawLargeEditor(s_State, s_Key)) {
        return false;
    }

    WriteCurve(s_State, p_Curve);

    return true;
}

bool CurveEditor::DrawBlock(const std::string& p_Id, const ZEntityRef p_Entity, ZCurve* p_Curve, const float p_Width) {
    const auto [s_It, s_IsNew] = m_States.try_emplace(GetCurveEditorKey(p_Id, p_Entity));
    CurveEditorState& s_State = s_It->second;

    SyncState(s_State, p_Curve);

    if (s_State.m_Keys.size() < 2) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(c_MutedTextColor, "A curve needs at least two keys to be edited here.");

        return false;
    }

    if (s_IsNew && s_State.m_Keys.size() > 2) {
        s_State.m_SelectedKey = 1;
    }

    const float s_Padding = ScaledSize(6.f);
    const float s_X0 = ImGui::GetCursorScreenPos().x;

    if (GhostButton(ICON_MD_GRID_4X4, "##CurveSnap", "Snap to grid", s_State.m_IsSnapOn)) {
        s_State.m_IsSnapOn = !s_State.m_IsSnapOn;
    }

    ImGui::SameLine(0.f, ScaledSize(6.f));

    const std::string s_Readout = FormatCurveReadout(s_State);
    const ImVec2 s_ReadoutPos(
        ImGui::GetCursorScreenPos().x,
        ImGui::GetCursorScreenPos().y + ImGui::GetStyle().FramePadding.y
    );
    const float s_LargeButtonX = s_X0 + p_Width - ScaledSize(20.f);

    DrawMutedTextEllipsis(s_ReadoutPos, s_LargeButtonX - ScaledSize(3.f), s_Readout.c_str());

    ImGui::SetCursorScreenPos(ImVec2(s_LargeButtonX, s_ReadoutPos.y - ImGui::GetStyle().FramePadding.y));

    if (GhostButton(ICON_MD_OPEN_IN_FULL, "##CurveLarge", "Large editor", s_State.m_IsLargeEditorOpen)) {
        s_State.m_IsLargeEditorOpen = !s_State.m_IsLargeEditorOpen;
    }

    ImGui::SetCursorScreenPos(ImVec2(s_X0, ImGui::GetItemRectMax().y + s_Padding));

    bool s_IsChanged = DrawCurvePlot(s_State, p_Width, ScaledSize(214.f), false);

    ImGui::SetCursorScreenPos(ImVec2(s_X0, ImGui::GetItemRectMax().y + s_Padding));

    s_IsChanged |= DrawCurveKeyPanel(s_State, p_Width, false);

    if (s_IsChanged) {
        WriteCurve(s_State, p_Curve);
    }

    return s_IsChanged;
}
