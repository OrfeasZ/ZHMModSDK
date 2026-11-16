#include "Editor.h"

#include "IconsMaterialDesign.h"

#include "imgui_internal.h"

#include "Glacier/ZGameTime.h"
#include "Glacier/ZCurve.h"
#include "Glacier/ZResource.h"
#include "Glacier/SColorRGB.h"
#include "Glacier/SColorRGBA.h"
#include "Util/StringUtils.h"

#include "JsonHelpers.h"

static constexpr ImVec4 c_RowHoverColor = ImVec4(1.f, 1.f, 1.f, 0.04f);
static constexpr ImU32 c_BlockTintColor = IM_COL32(0xff, 0xff, 0xff, 0x05);
static constexpr float c_BlockPadding = 6.f;

static size_t GetPropertyNamePrefixLength(const std::string_view p_Name) {
    for (size_t i = 0; i < p_Name.size(); ++i) {
        if (std::isupper(static_cast<unsigned char>(p_Name[i]))) {
            return i;
        }
    }

    return 0;
}

void Editor::DrawPropertyName(const std::string& p_Name, const bool p_IsChild) {
    ImGui::AlignTextToFramePadding();

    if (p_IsChild) {
        ImGui::PushStyleColor(ImGuiCol_Text, c_SecondaryTextColor);
        ImGui::TextUnformatted(p_Name.c_str());
        ImGui::PopStyleColor();
        return;
    }

    const size_t s_PrefixLength = GetPropertyNamePrefixLength(p_Name);

    if (s_PrefixLength == 0) {
        ImGui::TextUnformatted(p_Name.c_str());
        return;
    }

    ImGui::BeginGroup();

    ImGui::PushStyleColor(ImGuiCol_Text, c_MutedTextColor);
    ImGui::TextUnformatted(p_Name.data(), p_Name.data() + s_PrefixLength);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.f, 0.f);
    ImGui::TextUnformatted(p_Name.data() + s_PrefixLength, p_Name.data() + p_Name.size());

    ImGui::EndGroup();
}

bool Editor::BeginPropertyTable(const char* p_Id, const float p_NameColumnWidth) {
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(ScaledSize(6.f), ScaledSize(2.f)));
    PushFrameHeight(20.f, 3.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ScaledSize(4.f), ScaledSize(3.f)));
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, ScaledSize(14.f));

    if (!ImGui::BeginTable(
        p_Id,
        2,
        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_PadOuterX
    )) {
        ImGui::PopStyleVar(4);
        return false;
    }

    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, ScaledSize(p_NameColumnWidth));
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

    m_PropertyTableTreeDepth = ImGui::GetCurrentWindow()->DC.TreeDepth;

    return true;
}

void Editor::EndPropertyTable() {
    ImGui::EndTable();
    ImGui::PopStyleVar(4);
}

void Editor::BeginPropertyTableRow() {
    ImGui::TableNextRow(ImGuiTableRowFlags_None, ScaledSize(24.f));

    if (ImGui::TableGetHoveredRow() == ImGui::TableGetRowIndex()) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(c_RowHoverColor));
    }
}

Editor::PropertyTableBlock Editor::BeginPropertyTableBlock() {
    ImGuiTable* s_Table = ImGui::GetCurrentTable();

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    const ImRect s_FirstCell = ImGui::TableGetCellBgRect(s_Table, 0);
    const ImRect s_LastCell = ImGui::TableGetCellBgRect(s_Table, 1);
    const ImRect& s_WindowClip = ImGui::GetCurrentWindow()->InnerClipRect;
    const float s_Padding = ScaledSize(c_BlockPadding);

    const PropertyTableBlock s_Block {
        .m_Min = ImVec2(s_FirstCell.Min.x, ImGui::GetCursorScreenPos().y - ImGui::GetStyle().CellPadding.y),
        .m_MaxX = s_LastCell.Max.x,
        .m_ContentWidth = s_LastCell.Max.x - s_FirstCell.Min.x - s_Padding * 2.f,
    };

    // Tables have no column spans, so we clip the block across the whole row.
    ImGui::PushClipRect(
        ImVec2(s_Block.m_Min.x, s_WindowClip.Min.y),
        ImVec2(s_Block.m_MaxX, s_WindowClip.Max.y),
        false
    );

    ImGui::SetCursorScreenPos(ImVec2(s_Block.m_Min.x + s_Padding, s_Block.m_Min.y + s_Padding));

    return s_Block;
}

void Editor::EndPropertyTableBlock(const PropertyTableBlock& p_Block) {
    const float s_Padding = ScaledSize(c_BlockPadding);
    const float s_Bottom = ImGui::GetCursorScreenPos().y - ImGui::GetStyle().ItemSpacing.y + s_Padding;

    ImGui::SetCursorScreenPos(ImVec2(p_Block.m_Min.x + s_Padding, s_Bottom - ImGui::GetStyle().CellPadding.y));
    ImGui::Dummy(ImVec2(0.f, 0.f));

    ImGui::PopClipRect();

    // The background channel sits above the column borders, so an opaque fill there
    // hides the border behind the block.
    ImDrawList* s_DrawList = ImGui::GetWindowDrawList();
    const ImVec2 s_Max(p_Block.m_MaxX, s_Bottom);

    ImGui::TablePushBackgroundChannel();
    s_DrawList->AddRectFilled(p_Block.m_Min, s_Max, ImGui::GetColorU32(ImGuiCol_WindowBg));
    s_DrawList->AddRectFilled(p_Block.m_Min, s_Max, c_BlockTintColor);
    ImGui::TablePopBackgroundChannel();
}

bool Editor::IsReadOnlyProperty(const SPropertyData* p_Property) {
    const SPropertyInfo* s_PropertyInfo = p_Property->m_pPropertyInfo;

    if (!s_PropertyInfo) {
        return false;
    }

    return (s_PropertyInfo->m_Flags & E_HAS_GETTER_SETTER) && !s_PropertyInfo->m_PropertySetCallBack;
}

Editor::PropertyRowResult Editor::DrawPropertyRow(
    const std::string& p_Id,
    const std::string& p_Name,
    const STypeID* p_TypeID,
    ZEntityRef p_Entity,
    SPropertyData* p_Property,
    void* p_Data,
    const EPropertyRowButton p_Button
) {
    PropertyRowResult s_Result;

    const IType* s_TypeInfo = p_TypeID->GetTypeInfo();
    const std::string_view s_TypeName = s_TypeInfo->pszTypeName;

    const bool s_IsMatrix = s_TypeName == "SMatrix43";
    const bool s_IsCurve = s_TypeName == "ZCurve";
    const bool s_IsArray = s_TypeInfo->IsArray() || s_TypeInfo->IsFixedArray();
    const bool s_HasChildRows = s_IsMatrix || s_IsCurve || s_IsArray;
    const bool s_IsChild = ImGui::GetCurrentWindow()->DC.TreeDepth > m_PropertyTableTreeDepth;
    const bool s_IsReadOnly = p_Property && IsReadOnlyProperty(p_Property);

    ImGui::PushID(p_Id.c_str());

    BeginPropertyTableRow();
    ImGui::TableSetColumnIndex(0);

    bool s_IsOpen = false;

    if (s_HasChildRows) {
        const float s_StartX = ImGui::GetCursorPosX();

        ImGui::AlignTextToFramePadding();

        s_IsOpen = ImGui::TreeNodeEx(
            "##Node",
            ImGuiTreeNodeFlags_SpanAvailWidth | (s_IsMatrix ? ImGuiTreeNodeFlags_DefaultOpen : 0)
        );

        ImGui::SameLine();
        ImGui::SetCursorPosX(s_StartX + ImGui::GetTreeNodeToLabelSpacing());
    }

    DrawPropertyName(p_Name, s_IsChild);

    s_Result.m_IsNameHovered = ImGui::IsItemHovered();

    ImGui::TableSetColumnIndex(1);

    const bool s_HasCopyButton = p_Button == EPropertyRowButton::Copy && CanCopyPropertyValue(s_TypeInfo);
    const bool s_HasTrailingSlot = s_IsReadOnly || s_HasCopyButton || p_Button == EPropertyRowButton::Remove;

    const float s_ButtonSize = ScaledSize(20.f);
    const float s_ValueStartX = ImGui::GetCursorPosX();
    const float s_ValueEndX = s_ValueStartX + ImGui::GetContentRegionAvail().x;
    const float s_ButtonX = s_ValueEndX - s_ButtonSize;

    ImGui::BeginDisabled(s_IsReadOnly);
    ImGui::PushItemWidth(
        s_HasTrailingSlot
            ? s_ButtonX - s_ValueStartX - ImGui::GetStyle().ItemSpacing.x
            : s_ValueEndX - s_ValueStartX
    );

    if (s_IsCurve) {
        s_Result.m_Changed = m_CurveEditor.DrawSummary(p_Id, p_Entity, static_cast<ZCurve*>(p_Data));
    }
    else if (s_IsArray) {
        s_Result.m_Changed = ArrayProperty(p_Data, p_TypeID);
    }
    else if (!s_IsMatrix) {
        s_Result.m_Changed = DrawEntityPropertyValue(p_Id, p_TypeID, p_Data);
    }

    ImGui::PopItemWidth();
    ImGui::EndDisabled();

    if (s_HasTrailingSlot) {
        ImGui::SameLine();
        ImGui::SetCursorPosX(s_ButtonX);
    }

    if (s_IsReadOnly) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(c_MutedTextColor, "%s", ICON_MD_LOCK);

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Read only");
        }
    }
    else if (s_HasCopyButton) {
        if (GhostButton(ICON_MD_CONTENT_COPY, "##Copy", "Copy to clipboard")) {
            CopyToClipboard(FormatPropertyValueForClipboard(p_TypeID, p_Data));
        }
    }
    else if (p_Button == EPropertyRowButton::Remove) {
        s_Result.m_RemoveClicked = GhostButton(ICON_MD_REMOVE, "##Remove", "Remove element");
    }

    if (s_IsOpen) {
        ImGui::BeginDisabled(s_IsReadOnly);

        if (s_IsMatrix) {
            s_Result.m_Changed |= SMatrix43Property(p_Id, p_Data);
        }
        else if (s_IsCurve) {
            const PropertyTableBlock s_Block = BeginPropertyTableBlock();

            s_Result.m_Changed |= m_CurveEditor.DrawBlock(
                p_Id, p_Entity, static_cast<ZCurve*>(p_Data), s_Block.m_ContentWidth
            );

            EndPropertyTableBlock(s_Block);
        }
        else {
            s_Result.m_Changed |= DrawArrayElements(
                p_Id,
                p_Entity,
                p_Property,
                p_Data,
                reinterpret_cast<const IArrayType*>(s_TypeInfo)
            );
        }

        ImGui::EndDisabled();
        ImGui::TreePop();
    }

    ImGui::PopID();

    return s_Result;
}

bool Editor::DrawVectorChildRow(const std::string& p_Id, const char* p_Label, const char* p_Suffix, float* p_Values) {
    BeginPropertyTableRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(c_SecondaryTextColor, "%s", p_Label);

    if (p_Suffix) {
        ImGui::SameLine(0.f, ScaledSize(5.f));
        ImGui::TextColored(c_MutedTextColor, "%s", p_Suffix);
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

    const bool s_IsChanged = DragFloatComponents(p_Id, p_Values, 3, c_VectorTags, 0.1f);

    ImGui::PopItemWidth();

    return s_IsChanged;
}

bool Editor::CanCopyPropertyValue(const IType* p_Type) {
    static constexpr std::string_view c_CopyableTypeNames[] = {
        "ZString", "SVector2", "SVector3", "SVector4", "SMatrix43", "SColorRGB", "SColorRGBA", "ZRuntimeResourceID",
        "ZRepositoryID", "ZGuid", "ZGameTime", "ZCurve",
    };

    const std::string_view s_TypeName = p_Type->pszTypeName;

    if (p_Type->IsEnum() || p_Type->IsResource() || p_Type->IsArray() || p_Type->IsFixedArray() ||
        FindScalarPropertyType(s_TypeName)) {
        return true;
    }

    return std::ranges::find(c_CopyableTypeNames, s_TypeName) != std::end(c_CopyableTypeNames);
}

int32_t Editor::GetEnumValue(const IEnumType* p_Type, const void* p_Data) {
    switch (p_Type->type.m_nTypeSize) {
        case 1:
            return *static_cast<const int8_t*>(p_Data);
        case 2:
            return *static_cast<const int16_t*>(p_Data);
        case 4:
            return *static_cast<const int32_t*>(p_Data);
        default:
            return 0;
    }
}

void Editor::SetEnumValue(const IEnumType* p_Type, void* p_Data, const int32_t p_Value) {
    switch (p_Type->type.m_nTypeSize) {
        case 1:
            *static_cast<int8_t*>(p_Data) = static_cast<int8_t>(p_Value);
            break;
        case 2:
            *static_cast<int16_t*>(p_Data) = static_cast<int16_t>(p_Value);
            break;
        case 4:
            *static_cast<int32_t*>(p_Data) = p_Value;
            break;
        default:
            break;
    }
}

std::string Editor::GetEnumValueName(const IEnumType* p_Type, const void* p_Data) {
    const int32_t s_Value = GetEnumValue(p_Type, p_Data);

    for (const auto& s_EnumValue : p_Type->items) {
        if (s_EnumValue.nValue == s_Value) {
            return s_EnumValue.szName;
        }
    }

    if (const auto s_It = m_SmfEnumVariants.find(p_Type->type.pszTypeName); s_It != m_SmfEnumVariants.end()) {
        for (const auto& [s_Name, s_VariantValue] : s_It->second) {
            if (s_VariantValue == s_Value) {
                return s_Name;
            }
        }
    }

    return std::to_string(s_Value);
}

std::string Editor::FormatMatrixAsRtJson(const SMatrix43& p_Matrix, const bool p_Round, const uint32_t p_Decimals) {
    const auto s_Format = [&](const float p_Value) {
        return FormatFloat(p_Value, p_Round, p_Decimals);
    };

    return fmt::format(
        "{{"
        "\"XAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},"
        "\"YAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},"
        "\"ZAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},"
        "\"Trans\":{{\"x\":{},\"y\":{},\"z\":{}}}"
        "}}",
        s_Format(p_Matrix.XAxis.x),
        s_Format(p_Matrix.XAxis.y),
        s_Format(p_Matrix.XAxis.z),
        s_Format(p_Matrix.YAxis.x),
        s_Format(p_Matrix.YAxis.y),
        s_Format(p_Matrix.YAxis.z),
        s_Format(p_Matrix.ZAxis.x),
        s_Format(p_Matrix.ZAxis.y),
        s_Format(p_Matrix.ZAxis.z),
        s_Format(p_Matrix.Trans.x),
        s_Format(p_Matrix.Trans.y),
        s_Format(p_Matrix.Trans.z)
    );
}

std::string Editor::FormatQneTransformAsJson(const QneTransform& p_Transform, const bool p_Round, const uint32_t p_Decimals) {
    const auto s_Format = [&](const float p_Value) {
        return FormatFloat(p_Value, p_Round, p_Decimals);
    };

    return fmt::format(
        "{{\"rotation\":{{\"x\":{},\"y\":{},\"z\":{}}},"
        "\"position\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
        s_Format(p_Transform.Rotation.x),
        s_Format(p_Transform.Rotation.y),
        s_Format(p_Transform.Rotation.z),
        s_Format(p_Transform.Position.x),
        s_Format(p_Transform.Position.y),
        s_Format(p_Transform.Position.z)
    );
}

std::string Editor::FormatPropertyValueForClipboard(const STypeID* p_TypeID, void* p_Data) {
    const IType* s_TypeInfo = p_TypeID->GetTypeInfo();
    const std::string_view s_TypeName = s_TypeInfo->pszTypeName;

    if (s_TypeName == "ZString") {
        return static_cast<ZString*>(p_Data)->c_str();
    }

    if (s_TypeName == "bool") {
        return *static_cast<bool*>(p_Data) ? "true" : "false";
    }

    if (const ScalarPropertyType* s_ScalarType = FindScalarPropertyType(s_TypeName)) {
        // Copy floats with more decimals than the "%.3f" we show.
        const char* s_Format = s_ScalarType->m_DataType == ImGuiDataType_Float
            ? "%f"
            : ImGui::DataTypeGetInfo(s_ScalarType->m_DataType)->PrintFmt;

        char s_Buffer[64];
        ImGui::DataTypeFormatString(s_Buffer, sizeof(s_Buffer), s_ScalarType->m_DataType, p_Data, s_Format);

        return s_Buffer;
    }

    if (s_TypeName == "SVector2") {
        const auto* s_Value = static_cast<SVector2*>(p_Data);

        return fmt::format("{{\"x\":{},\"y\":{}}}", s_Value->x, s_Value->y);
    }

    if (s_TypeName == "SVector3") {
        const auto* s_Value = static_cast<SVector3*>(p_Data);

        return fmt::format("{{\"x\":{},\"y\":{},\"z\":{}}}", s_Value->x, s_Value->y, s_Value->z);
    }

    if (s_TypeName == "SVector4") {
        const auto* s_Value = static_cast<SVector4*>(p_Data);

        return fmt::format(
            "{{\"x\":{},\"y\":{},\"z\":{},\"w\":{}}}", s_Value->x, s_Value->y, s_Value->z, s_Value->w
        );
    }

    if (s_TypeName == "SMatrix43") {
        const auto* s_Value = static_cast<SMatrix43*>(p_Data);

        return m_UseQneTransforms
            ? FormatQneTransformAsJson(MatrixToQneTransform(*s_Value), m_RoundCopiedMatrixValues, m_CopyDecimalPlaces)
            : FormatMatrixAsRtJson(*s_Value, m_RoundCopiedMatrixValues, m_CopyDecimalPlaces);
    }

    if (s_TypeName == "SColorRGB") {
        const uint32_t s_Color = static_cast<SColorRGB*>(p_Data)->GetAsUInt32();

        return fmt::format("#{0:02X}{1:02X}{2:02X}", s_Color & 0xFF, (s_Color >> 8) & 0xFF, (s_Color >> 16) & 0xFF);
    }

    if (s_TypeName == "SColorRGBA") {
        const uint32_t s_Color = static_cast<SColorRGBA*>(p_Data)->GetAsUInt32();

        return fmt::format(
            "#{0:02X}{1:02X}{2:02X}{3:02X}",
            s_Color & 0xFF,
            (s_Color >> 8) & 0xFF,
            (s_Color >> 16) & 0xFF,
            (s_Color >> 24) & 0xFF
        );
    }

    if (s_TypeInfo->IsEnum()) {
        return GetEnumValueName(reinterpret_cast<const IEnumType*>(s_TypeInfo), p_Data);
    }

    if (s_TypeInfo->IsResource()) {
        const auto* s_ResourcePtr = static_cast<ZResourcePtr*>(p_Data);

        if (s_ResourcePtr->m_nResourceIndex.val < 0) {
            return "null";
        }

        const auto& s_RuntimeResourceID = s_ResourcePtr->GetResourceInfo().rid;

        return fmt::format("{:08X}{:08X}", s_RuntimeResourceID.m_IDHigh, s_RuntimeResourceID.m_IDLow);
    }

    if (s_TypeName == "ZRuntimeResourceID") {
        const auto* s_RuntimeResourceID = static_cast<ZRuntimeResourceID*>(p_Data);

        if (*s_RuntimeResourceID == -1) {
            return "null";
        }

        return fmt::format("{:08X}{:08X}", s_RuntimeResourceID->m_IDHigh, s_RuntimeResourceID->m_IDLow);
    }

    if (s_TypeName == "ZRepositoryID") {
        return static_cast<ZRepositoryID*>(p_Data)->ToString().c_str();
    }

    if (s_TypeName == "ZGuid") {
        return Util::StringUtils::ToLowerCase(static_cast<ZGuid*>(p_Data)->ToString().c_str());
    }

    if (s_TypeName == "ZGameTime") {
        return std::to_string(static_cast<ZGameTime*>(p_Data)->m_nTicks);
    }

    if (s_TypeName == "ZCurve") {
        return FormatZCurveForClipboard(static_cast<ZCurve*>(p_Data));
    }

    if (s_TypeInfo->IsArray() || s_TypeInfo->IsFixedArray()) {
        const auto* s_ArrayType = reinterpret_cast<const IArrayType*>(s_TypeInfo);
        const STypeID* s_ElementTypeID = s_ArrayType->containerType.elementType;
        const IType* s_ElementType = s_ElementTypeID->GetTypeInfo();
        const std::string_view s_ElementTypeName = s_ElementType->pszTypeName;

        // Quote anything that isn't a JSON number, boolean, object or array.
        const bool s_IsQuoted = s_ElementTypeName == "ZString" || s_ElementTypeName == "SColorRGB" ||
            s_ElementTypeName == "SColorRGBA" || s_ElementTypeName == "ZRuntimeResourceID" ||
            s_ElementTypeName == "ZRepositoryID" || s_ElementTypeName == "ZGuid" || s_ElementType->IsEnum() ||
            s_ElementType->IsResource();

        std::string s_Result = "[";

        void* s_Iterator = s_ArrayType->containerType.pVTab->begin(p_Data);
        void* s_End = s_ArrayType->containerType.pVTab->end(p_Data);

        for (; s_Iterator != s_End; s_Iterator = s_ArrayType->containerType.pVTab->next(p_Data, s_Iterator)) {
            if (s_Result.size() > 1) {
                s_Result += ",";
            }

            std::string s_Element = FormatPropertyValueForClipboard(s_ElementTypeID, s_Iterator);

            if (s_Element.empty()) {
                s_Element = "null";
            }

            s_Result += s_IsQuoted ? write_json(s_Element) : s_Element;
        }

        return s_Result + "]";
    }

    return {};
}

void Editor::OnSetPropertyValue(
    ZEntityRef p_Entity,
    uint32_t p_PropertyId,
    const ZObjectRef& p_Value,
    std::optional<std::string> p_ClientId
) {
    Hooks::SetPropertyValue->Call(p_Entity, p_PropertyId, p_Value, true);
    RefreshPathfinderObstacle(p_Entity, true);
    m_Server.OnEntityPropertySet(p_Entity, p_PropertyId, std::move(p_ClientId));
}

void Editor::OnSignalEntityPin(
    ZEntityRef p_Entity,
    uint32_t p_PinId,
    bool p_Output,
    const ZObjectRef& p_Data
) {
    if (p_Output) {
        p_Entity.SignalOutputPin(p_PinId, p_Data);
    }
    else {
        p_Entity.SignalInputPin(p_PinId, p_Data);
    }
}

void Editor::OnSignalEntityPin(
    ZEntityRef p_Entity,
    const std::string& p_Pin,
    bool p_Output,
    const ZObjectRef& p_Data
) {
    OnSignalEntityPin(p_Entity, Hash::Crc32(p_Pin.c_str(), p_Pin.size()), p_Output, p_Data);
}

void Editor::UnsupportedProperty() {
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(c_MutedTextColor, "%s", ICON_MD_BLOCK);
    ImGui::SameLine(0.f, ScaledSize(4.f));
    ImGui::TextColored(c_MutedTextColor, "Can't be edited here");
}

bool Editor::TEntityRefProperty(const std::string& p_Id, void* p_Data, const STypeID* p_TypeID) {
    auto* s_EntityRef = static_cast<TEntityRef<void>*>(p_Data);

    if (!EntityRefProperty(p_Id, s_EntityRef->m_entityRef)) {
        return false;
    }

    s_EntityRef->m_pInterfaceRef = nullptr;

    if (s_EntityRef->m_entityRef) {
        const std::string_view s_TypeName = p_TypeID->GetTypeInfo()->pszTypeName;
        const size_t s_Start = s_TypeName.find('<') + 1;
        const std::string s_InterfaceTypeName(s_TypeName.substr(s_Start, s_TypeName.rfind('>') - s_Start));

        if (STypeID* s_InterfaceTypeID = (*Globals::TypeRegistry)->GetTypeID(ZString(s_InterfaceTypeName))) {
            s_EntityRef->m_pInterfaceRef = s_EntityRef->m_entityRef.QueryInterface(s_InterfaceTypeID);
        }
    }

    return true;
}

bool Editor::EntityRefProperty(const std::string& p_Id, ZEntityRef& p_EntityRef) {
    bool s_IsChanged = false;
    const bool s_IsPicking = m_EntityRefPickTarget == p_Id;

    if (s_IsPicking && m_PickedEntityRef) {
        p_EntityRef = *m_PickedEntityRef;
        s_IsChanged = true;

        m_PickedEntityRef.reset();
        m_EntityRefPickTarget.clear();
    }
    else if (s_IsPicking && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_EntityRefPickTarget.clear();
    }

    std::string s_Name;
    std::string s_ShortId;
    std::string s_EntityId;

    if (p_EntityRef) {
        s_EntityId = fmt::format("{:016x}", p_EntityRef->GetType()->m_nEntityID);
        s_Name = s_EntityId;

        std::shared_lock s_TreeLock(m_CachedEntityTreeMutex);

        if (const auto s_Iterator = m_CachedEntityTreeMap.find(p_EntityRef); s_Iterator != m_CachedEntityTreeMap.end()) {
            s_Name = SplitEntityTreeNodeName(s_Iterator->second->Name).Name;
            s_ShortId = s_EntityId.substr(0, 8) + "\xE2\x80\xA6";
        }
    }

    const float s_ButtonWidth = ScaledSize(20.f) + ImGui::GetStyle().ItemSpacing.x;
    const int s_ButtonCount = p_EntityRef ? 2 : 1;

    DrawReferenceWell(ICON_MD_CATEGORY, ImGui::CalcItemWidth() - s_ButtonCount * s_ButtonWidth, [&] {
        if (!p_EntityRef) {
            ImGui::TextColored(c_MutedTextColor, "None");
            return;
        }

        if (DrawLinkText(s_Name)) {
            OnSelectEntity(p_EntityRef, true, std::nullopt);
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Select %s (%s)", s_Name.c_str(), s_EntityId.c_str());
        }

        if (!s_ShortId.empty()) {
            ImGui::SameLine(0.f, ScaledSize(4.f));
            ImGui::TextColored(c_MutedTextColor, "%s", s_ShortId.c_str());
        }
    });

    if (p_EntityRef) {
        ImGui::SameLine();

        if (GhostButton(ICON_MD_CLOSE, "##ClearEntityRef", "Clear reference")) {
            p_EntityRef = {};
            s_IsChanged = true;
        }
    }

    ImGui::SameLine();

    if (GhostButton(
        ICON_MD_ADS_CLICK, "##PickEntity", "Pick entity in the tree or the game (Escape cancels)", s_IsPicking
    )) {
        m_PickedEntityRef.reset();

        if (s_IsPicking) {
            m_EntityRefPickTarget.clear();
        }
        else {
            m_EntityRefPickTarget = p_Id;
        }
    }

    return s_IsChanged;
}

void Editor::ZRepositoryIDProperty(void* p_Data) {
    const auto* s_RepositoryId = static_cast<ZRepositoryID*>(p_Data);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(s_RepositoryId->ToString().c_str());

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", GetNameFromRepository(*s_RepositoryId).c_str());
    }
}

bool Editor::ZGuidProperty(const std::string& p_Id, void* p_Data) {
    auto* s_Guid = static_cast<ZGuid*>(p_Data);
    const ZString s_GuidString = s_Guid->ToString();

    char s_Buffer[64] = {};
    std::snprintf(s_Buffer, sizeof(s_Buffer), "%s", s_GuidString.c_str());

    // Apply on Enter only, because a partly typed GUID parses to a different one.
    const bool s_IsChanged = ImGui::InputText(
        p_Id.c_str(), s_Buffer, sizeof(s_Buffer), ImGuiInputTextFlags_EnterReturnsTrue
    );

    if (s_IsChanged) {
        s_Guid->FromString(ZString(s_Buffer));
    }

    DrawEditingOutline();

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", GetNameFromRepository(ZRepositoryID(s_GuidString)).c_str());
    }

    return s_IsChanged;
}

bool Editor::ZGameTimeProperty(const std::string& p_Id, void* p_Data) {
    auto* s_Value = static_cast<ZGameTime*>(p_Data);

    return TaggedDragScalar(
        p_Id, ICON_MD_SCHEDULE, c_ValueTagColor, ImGuiDataType_S64, &s_Value->m_nTicks, ImGui::CalcItemWidth()
    );
}

std::string Editor::FormatZCurveForClipboard(ZCurve* p_Curve) {
    std::string s_Result;

    s_Result += "[\n";

    for (size_t s_Index = 0; s_Index < p_Curve->data.size(); ++s_Index) {
        const auto& s_Key = p_Curve->data[s_Index];

        s_Result += "\t[\n";

        for (size_t i = 0; i < s_Key.size(); ++i) {
            s_Result += fmt::format("\t\t{}{}", s_Key[i], (i + 1 < s_Key.size()) ? "," : "");
            s_Result += "\n";
        }

        s_Result += "\t]";

        if (s_Index + 1 < p_Curve->data.size()) {
            s_Result += ",";
        }

        s_Result += "\n";
    }

    s_Result += "]";

    return s_Result;
}
