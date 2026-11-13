#include "Editor.h"
#include "imgui.h"
#include "Logging.h"

#include <algorithm>
#include <filesystem>
#include <span>

#include <simdjson.h>

void Editor::LoadSmfEnumVariants() {
    ZString s_DeploymentPath;
    if (!SDK()->GetSmfDeploymentPath(s_DeploymentPath)) {
        return;
    }

    const auto s_SummaryPath = std::filesystem::path(
        std::u8string_view(reinterpret_cast<const char8_t*>(s_DeploymentPath.c_str()), s_DeploymentPath.size())
    ) / "summary.json";

    simdjson::padded_string s_Json;

    if (simdjson::padded_string::load(s_SummaryPath.wstring()).get(s_Json)) {
        Logger::Error("Could not read SMF deployment summary at '{}'.", s_DeploymentPath.c_str());
        return;
    }

    try {
        simdjson::ondemand::parser s_Parser;
        simdjson::ondemand::document s_Document = s_Parser.iterate(s_Json);
        simdjson::ondemand::object s_Enums = s_Document["enums"].get_object();

        for (auto s_Enum : s_Enums) {
            auto& s_Variants = m_SmfEnumVariants[std::string(s_Enum.unescaped_key().value())];

            for (auto s_Variant : s_Enum.value().get_object()) {
                std::string_view s_Name = s_Variant.unescaped_key().value();
                int64_t s_Value = s_Variant.value().get_int64();

                s_Variants.emplace_back(std::string(s_Name), s_Value);
            }

            std::ranges::sort(s_Variants);
        }
    }
    catch (const simdjson::simdjson_error& p_Error) {
        Logger::Error("Could not parse SMF deployment summary: {}", p_Error.what());
        m_SmfEnumVariants.clear();
        return;
    }

    Logger::Info("Loaded SMF enum variants for {} enum(s).", m_SmfEnumVariants.size());
}

bool Editor::EnumProperty(const std::string& p_Id, ZEntityRef p_Entity, SPropertyData* p_Property, void* p_Data, const STypeID* p_TypeID) {
    bool s_IsChanged = false;
    const auto s_PropertyInfo = p_Property->GetPropertyInfo();
    auto s_Type = reinterpret_cast<IEnumType*>(p_TypeID->GetTypeInfo());
    int32_t s_Value = 0;

    switch (s_Type->type.m_nTypeSize) {
        case 1:
            s_Value = *static_cast<int8_t*>(p_Data);
            break;
        case 2:
            s_Value = *static_cast<int16_t*>(p_Data);
            break;
        case 4:
            s_Value = *static_cast<int32_t*>(p_Data);
            break;
    }

    const auto s_SetValue = [&](int32_t p_NewValue) {
        switch (s_Type->type.m_nTypeSize) {
            case 1:
                *static_cast<int8_t*>(p_Data) = static_cast<int8_t>(p_NewValue);
                break;
            case 2:
                *static_cast<int16_t*>(p_Data) = static_cast<int16_t>(p_NewValue);
                break;
            case 4:
                *static_cast<int32_t*>(p_Data) = static_cast<int32_t>(p_NewValue);
                break;
            default:
                break;
        }

        s_Value = p_NewValue;

        s_IsChanged = true;
    };

    std::span<const std::pair<std::string, int64_t>> s_SmfVariants;

    if (const auto s_It = m_SmfEnumVariants.find(s_Type->type.pszTypeName); s_It != m_SmfEnumVariants.end()) {
        s_SmfVariants = s_It->second;
    }

    std::string s_CurrentValue;

    for (auto& s_EnumValue : s_Type->items) {
        if (s_EnumValue.nValue == s_Value) {
            s_CurrentValue = s_EnumValue.szName;
            break;
        }
    }

    if (s_CurrentValue.empty()) {
        for (const auto& [s_Name, s_VariantValue] : s_SmfVariants) {
            if (s_VariantValue == s_Value) {
                s_CurrentValue = s_Name;
                break;
            }
        }
    }

    if (s_CurrentValue.empty()) {
        s_CurrentValue = std::to_string(s_Value);
    }

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - m_CopyWidgetWidth);

    if (ImGui::BeginCombo(p_Id.c_str(), s_CurrentValue.c_str())) {
        for (auto& s_EnumValue : s_Type->items) {
            if (ImGui::Selectable(s_EnumValue.szName, s_EnumValue.nValue == s_Value)) {
                s_SetValue(s_EnumValue.nValue);
            }
        }

        if (!s_SmfVariants.empty()) {
            ImGui::Separator();

            for (const auto& [s_Name, s_VariantValue] : s_SmfVariants) {
                if (ImGui::Selectable(s_Name.c_str(), s_VariantValue == s_Value)) {
                    s_SetValue(static_cast<int32_t>(s_VariantValue));
                }
            }
        }

        ImGui::EndCombo();
    }

    if (ImGuiCopyWidget(("Enum_" + p_Id).c_str())) {
        CopyToClipboard(s_CurrentValue);
    }

    return s_IsChanged;
}
