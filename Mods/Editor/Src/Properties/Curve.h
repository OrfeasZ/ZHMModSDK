#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "CurveModel.h"

#include "Glacier/ZCurve.h"
#include "Glacier/ZEntity.h"

struct CurveEditorState : CurveModel {
    bool m_IsSnapOn = false;
    bool m_IsAdvancedOpen = false;
    bool m_IsLargeEditorOpen = false;
    bool m_ShouldFitLarge = true;
    std::pair<double, double> m_Cursor;
    int m_CursorFrame = -1;
    float m_KeyPanelHeight = 0.f;
};

class CurveEditor {
public:
    bool DrawSummary(const std::string& p_Id, ZEntityRef p_Entity, ZCurve* p_Curve);
    bool DrawBlock(const std::string& p_Id, ZEntityRef p_Entity, ZCurve* p_Curve, float p_Width);

private:
    std::unordered_map<std::string, CurveEditorState> m_States;
};
