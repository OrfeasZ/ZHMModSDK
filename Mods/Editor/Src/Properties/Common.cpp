#include "Editor.h"

#include "implot.h"

#include "IconsMaterialDesign.h"

#include "imgui_internal.h"

#include "Glacier/ZGameTime.h"
#include "Glacier/ZCurve.h"

void Editor::OnSetPropertyValue(
    ZEntityRef p_Entity,
    uint32_t p_PropertyId,
    const ZObjectRef& p_Value,
    std::optional<std::string> p_ClientId
) {
    Hooks::SetPropertyValue->Call(p_Entity, p_PropertyId, p_Value, true);
    m_Server.OnEntityPropertySet(p_Entity, p_PropertyId, std::move(p_ClientId));
}

void Editor::OnSignalEntityPin(ZEntityRef p_Entity, uint32_t p_PinId, bool p_Output) {
    if (p_Output) {
        p_Entity.SignalOutputPin(p_PinId);
    } else {
        p_Entity.SignalInputPin(p_PinId);
    }
}

void Editor::OnSignalEntityPin(ZEntityRef p_Entity, const std::string& p_Pin, bool p_Output) {
    OnSignalEntityPin(p_Entity, Hash::Crc32(p_Pin.c_str(), p_Pin.size()), p_Output);
}

void Editor::UnsupportedProperty(
    const std::string& p_Id,
    ZEntityRef p_Entity,
    ZEntityProperty* p_Property,
    void* p_Data
) {
    const auto s_PropertyInfo = p_Property->m_pType->getPropertyInfo();
    const std::string s_TypeName = s_PropertyInfo->m_pType->typeInfo()->m_pTypeName;

    constexpr auto s_TextColor = ImVec4(1.f, 1.f, 1.f, 0.5f);
    ImGui::TextColored(s_TextColor, "(Unsupported)", s_TypeName.c_str());
}

void Editor::ZEntityRefProperty(
    const std::string& p_Id,
    ZEntityRef p_Entity,
    ZEntityProperty* p_Property,
    void* p_Data
) {
    if (auto s_EntityRef = reinterpret_cast<ZEntityRef*>(p_Data)) {
        EntityRefProperty(p_Id, *s_EntityRef);
    }
    else {
        EntityRefProperty(p_Id, ZEntityRef{});
    }
}

void Editor::TEntityRefProperty(
    const std::string& p_Id,
    ZEntityRef p_Entity,
    ZEntityProperty* p_Property,
    void* p_Data
) {
    if (auto s_EntityRef = reinterpret_cast<TEntityRef<void*>*>(p_Data)) {
        EntityRefProperty(p_Id, s_EntityRef->m_ref);
    }
    else {
        EntityRefProperty(p_Id, ZEntityRef{});
    }
}

void Editor::EntityRefProperty(const std::string& p_Id, ZEntityRef p_Entity) {
    if (!p_Entity) {
        constexpr ImVec4 s_TextColor = ImVec4(1.f, 1.f, 1.f, 0.5f);

        ImGui::TextColored(s_TextColor, "(%s)", "null");

        return;
    }

    ImVec4 s_LinkColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, s_LinkColor);
    ImGui::Text("%s", fmt::format("{:016x}", p_Entity->GetType()->m_nEntityId).c_str());
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered()) {
        ImVec2 s_Min = ImGui::GetItemRectMin();
        ImVec2 s_Max = ImGui::GetItemRectMax();

        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(s_Min.x, s_Max.y),
            ImVec2(s_Max.x, s_Max.y),

            ImGui::GetColorU32(s_LinkColor)
        );

        auto s_Iterator = m_CachedEntityTreeMap.find(p_Entity);

        if (s_Iterator != m_CachedEntityTreeMap.end()) {
            ImGui::SetTooltip("%s", s_Iterator->second->Name.c_str());
        }
        else {
            ImGui::SetTooltip("%s", "Entity tree not loaded, rebuild the entity tree");
        };
    }

    if (ImGui::IsItemClicked()) {
        OnSelectEntity(p_Entity, true, std::nullopt);
    }

    if (ImGuiCopyWidget(("EntityRef_" + p_Id).c_str())) {
        CopyToClipboard(fmt::format("{:016x}", p_Entity->GetType()->m_nEntityId));
    }
}

void Editor::ZRepositoryIDProperty(
    const std::string& p_Id,
    ZEntityRef p_Entity,
    ZEntityProperty* p_Property,
    void* p_Data
) {
    if (auto s_RepositoryId = reinterpret_cast<ZRepositoryID*>(p_Data)) {
        const auto& s_RepositoryIdString = s_RepositoryId->ToString();

        ImGui::Text("%s", s_RepositoryIdString.c_str());

        if (ImGuiCopyWidget(("RepositoryId_" + p_Id).c_str())) {
            CopyToClipboard(s_RepositoryIdString.c_str());
        }
    }
    else {
        constexpr auto s_TextColor = ImVec4(1.f, 1.f, 1.f, 0.5f);

        ImGui::TextColored(s_TextColor, "(%s)", "null");
    }
}

void Editor::ZGuidProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
    if (auto s_Guid = reinterpret_cast<ZGuid*>(p_Data)) {
        const auto& s_GuidString = s_Guid->ToString();

        ImGui::Text("%s", s_GuidString.c_str());

        if (ImGuiCopyWidget(("Guid_" + p_Id).c_str())) {
            CopyToClipboard(s_GuidString.c_str());
        }
    }
    else {
        constexpr auto s_TextColor = ImVec4(1.f, 1.f, 1.f, 0.5f);

        ImGui::TextColored(s_TextColor, "(%s)", "null");
    }
}

bool Editor::ZGameTimeProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data) {
    bool s_IsChanged = false;
    auto s_Value = static_cast<ZGameTime*>(p_Data);

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::DragScalar(p_Id.c_str(), ImGuiDataType_S64, &s_Value->m_nTicks)) {
        s_IsChanged = true;
    }

    if (ImGuiCopyWidget(("GameTime_" + p_Id).c_str())) {
        CopyToClipboard(std::to_string(s_Value->m_nTicks));
    }

    return s_IsChanged;
}

bool Editor::ZCurveProperty(const std::string& p_Id, ZEntityRef p_Entity, ZEntityProperty* p_Property, void* p_Data,
    const std::string& s_PropertyName, const STypeID* p_TypeID) {
    bool s_IsChanged = false;
    auto* s_Curve = static_cast<ZCurve*>(p_Data);

    IType* s_ArrayTypeInfo = (*Globals::TypeRegistry)->GetTypeID("TArray<TFixedArray<float32>>")->typeInfo();
    const IArrayType* s_ArrayType = static_cast<IArrayType*>(s_ArrayTypeInfo);
    size_t s_ArraySize = s_ArrayType->m_pArrayFunctions->size(&s_Curve->data);

    ImGui::SetNextItemAllowOverlap();

    ImGui::PushFont(SDK()->GetImGuiBoldFont());
    ImGui::AlignTextToFramePadding();

    const bool s_IsTreeNodeOpen = ImGui::TreeNodeEx(
        fmt::format("{} ({} {})##{}",
            s_PropertyName,
            s_ArraySize,
            s_ArraySize == 1 ? "element" : "elements",
            p_Id
        ).c_str()
    );

    ImGui::PopFont();

    ImGui::PushID(p_Id.c_str());

    ImGui::SameLine(0, 10.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.5, 0.5 });
    ImGui::SetWindowFontScale(0.6);

    const auto s_Result = ImGui::ButtonEx(
        (std::string(ICON_MD_SHOW_CHART) + "##" + p_Id).c_str(),
        { m_CopyWidgetButtonSize, m_CopyWidgetButtonSize },
        ImGuiButtonFlags_AlignTextBaseLine
    );

    ImGui::SetWindowFontScale(1.0);
    ImGui::PopStyleVar(2);

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Show curve plot");
    }

    if (ImGuiCopyWidget(("Curve_" + p_Id).c_str())) {
        std::string s_Text = FormatZCurveForClipboard(s_Curve, p_Data, s_ArrayType);

        CopyToClipboard(s_Text.c_str());
    }

    if (s_IsTreeNodeOpen) {
        s_IsChanged = DrawArrayElements(
            p_Id,
            p_Entity,
            p_Property,
            &s_Curve->data,
            s_ArrayType
        );

        ImGui::TreePop();
    }

    if (s_Result) {
        ImGui::OpenPopup("CurvePopup");
    }

    if (ImGui::BeginPopup("CurvePopup")) {
        ImPlot::SetNextAxesToFit();

        if (ImPlot::BeginPlot("##ZCurve", ImVec2(600, 320))) {

            ImPlot::SetupAxes("Time", "Value");

            PlotZCurve(s_Curve);

            ImPlot::EndPlot();
        }

        if (ImGui::Button("Fit")) {
            ImPlot::SetNextAxesToFit();
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();

    return s_IsChanged;
}

void Editor::PlotZCurve(const ZCurve* p_Curve) {
    const int32_t s_KeyCount = static_cast<int32_t>(p_Curve->data.size());

    if (s_KeyCount == 0) {
        return;
    }

    constexpr int32_t s_SamplesPerSegment = 64;

    std::vector<double> s_Xs;
    std::vector<double> s_Ys;

    for (int32_t i = 0; i < s_KeyCount - 1; ++i) {
        const auto& s_Key = p_Curve->data[i];
        const auto& s_NextKey = p_Curve->data[i + 1];

        s_Xs.clear();
        s_Ys.clear();

        const float s_T0 = s_Key[0];
        const float s_T1 = s_NextKey[0];

        for (int32_t s = 0; s < s_SamplesPerSegment; ++s) {
            const float s_A = static_cast<float>(s) / (s_SamplesPerSegment - 1);
            const float s_Time = s_T0 + (s_T1 - s_T0) * s_A;

            s_Xs.push_back(s_Time);
            s_Ys.push_back(EvaluateZCurveSegment(s_Key, s_NextKey, s_Time));
        }

        ImPlot::PlotLine(
            ("Segment##" + std::to_string(i)).c_str(),
            s_Xs.data(),
            s_Ys.data(),
            (int)s_Xs.size()
        );
    }

    if (s_KeyCount > 0) {
        const float s_X = p_Curve->data.front()[0];
        const float s_Y = p_Curve->data.front()[1];

        ImPlot::PlotScatter("Start", &s_X, &s_Y, 1);
    }

    if (s_KeyCount > 0) {
        const float s_X = p_Curve->data.back()[0];
        const float s_Y = p_Curve->data.back()[1];

        ImPlot::PlotScatter("End", &s_X, &s_Y, 1);
    }

    for (int32_t i = 0; i < s_KeyCount; ++i) {
        const double s_X = p_Curve->data[i][0];
        const double s_Y = p_Curve->data[i][1];

        ImPlot::PlotScatter(
            ("Key##" + std::to_string(i)).c_str(),
            &s_X,
            &s_Y,
            1
        );
    }
}

float Editor::EvaluateZCurveSegment(
    const TFixedArray<float, 8>& p_Key,
    const TFixedArray<float, 8>& p_NextKey,
    float p_Time
) {
    const float s_T0 = p_Key[0];
    const float s_T1 = p_NextKey[0];

    if (s_T1 <= s_T0) {
        return p_Key[1];
    }

    float s_T = (p_Time - s_T0) / (s_T1 - s_T0);
    s_T = std::clamp(s_T, 0.0f, 1.0f);

    // f(t) = c5*t^5 + c4*t^4 + c3*t^3 + c2*t^2 + c1*t + c0
    return
        p_Key[2] * s_T * s_T * s_T * s_T * s_T +
        p_Key[3] * s_T * s_T * s_T * s_T +
        p_Key[4] * s_T * s_T * s_T +
        p_Key[5] * s_T * s_T +
        p_Key[6] * s_T +
        p_Key[7];
}

std::string Editor::FormatZCurveForClipboard(ZCurve* p_Curve, void* p_Data, const IArrayType* p_ArrayType) {
    std::string s_Result;

    s_Result += "[\n";

    void* s_Iterator = p_ArrayType->m_pArrayFunctions->begin(&p_Curve->data);
    void* s_End = p_ArrayType->m_pArrayFunctions->end(&p_Curve->data);

    size_t s_Index = 0;
    size_t s_ArraySize = p_ArrayType->m_pArrayFunctions->size(&p_Curve->data);

    for (; s_Iterator != s_End; s_Iterator = p_ArrayType->m_pArrayFunctions->next(p_Data, s_Iterator), ++s_Index) {
        auto* s_FixedArray = static_cast<TFixedArray<float32, 8>*>(s_Iterator);

        s_Result += "\t[\n";

        for (size_t i = 0; i < s_FixedArray->size(); ++i) {
            s_Result += fmt::format(
                "\t\t{}{}",
                (*s_FixedArray)[i],
                (i + 1 < s_FixedArray->size()) ? "," : ""
            );

            s_Result += "\n";
        }

        s_Result += "\t]";

        if (s_Index + 1 < s_ArraySize) {
            s_Result += ",";
        }

        s_Result += "\n";
    }

    s_Result += "]";

    return s_Result;
}