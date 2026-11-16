#include <Editor.h>
#include <numbers>

#include "Functions.h"
#include <Glacier/ZModule.h>
#include <Glacier/ZGeomEntity.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZResource.h>
#include "IconsMaterialDesign.h"
#include "Logging.h"
#include "Glacier/ZPhysics.h"
#include <ResourceLib_HM3.h>
#include "Glacier/ZRender.h"
#include "Util/ImGuiUtils.h"
#include "imgui_internal.h"


static SMatrix ToLocalTransform(ZEntityRef p_Entity, const ZSpatialEntity* p_Spatial, const SMatrix& p_World) {
    SMatrix s_ParentTrans;

    if (p_Spatial->m_eidParent.m_pInterfaceRef) {
        s_ParentTrans = p_Spatial->m_eidParent.m_pInterfaceRef->GetObjectToWorldMatrix();
    }
    else if (p_Entity.GetLogicalParent() && p_Entity.GetLogicalParent().QueryInterface<ZSpatialEntity>()) {
        s_ParentTrans = p_Entity.GetLogicalParent().QueryInterface<ZSpatialEntity>()->GetObjectToWorldMatrix();
    }
    else if (p_Entity.GetOwningEntity() && p_Entity.GetOwningEntity().QueryInterface<ZSpatialEntity>()) {
        s_ParentTrans = p_Entity.GetOwningEntity().QueryInterface<ZSpatialEntity>()->GetObjectToWorldMatrix();
    }

    const auto s_ParentTransInv = s_ParentTrans.Inverse();

    auto s_LocalTrans = s_ParentTransInv * p_World;
    s_LocalTrans.Trans = p_World.Trans - s_ParentTrans.Trans;
    s_LocalTrans.Trans.w = 1.f;

    return s_LocalTrans;
}

void Editor::DrawEntityPropertiesWindow() {
    BeginPanelWindow(c_EntityPropertiesWindowName, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_Docking);

    if (IsSpecialEntityTreeNode(m_SelectedEntity)) {
        ImGui::End();
        return;
    }

    const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

    const auto s_SelectedEntity = m_SelectedEntity;

    if (s_SceneCtx && s_SceneCtx->m_pScene && s_SelectedEntity) {
        const auto s_Spatial = s_SelectedEntity.QueryInterface<ZSpatialEntity>();

        BeginPanelPadding("##EntityHeader");

        if (DrawEntityToolbar(s_SelectedEntity, s_Spatial)) {
            // Prevent crash on destroy.
            EndPanelPadding();
            ImGui::End();
            return;
        }

        const EntityDetails s_Details = CollectEntityDetails(s_SelectedEntity);

        if (s_Details.HasTreeNode) {
            DrawEntityName(s_Details);
        }

        EndPanelPadding();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.f));

        if (!s_Details.Rows.empty() && DrawPropertySectionHeader("Details", s_Details.TypeName, false)) {
            DrawEntityDetails(s_Details);
        }

        if (s_Spatial) {
            const SMatrix s_World = s_Spatial->GetObjectToWorldMatrix();

            DrawEntityTransform(m_UseLocalTransforms ? ToLocalTransform(s_SelectedEntity, s_Spatial, s_World) : s_World);
        }

        ImGui::PopStyleVar();

        if (const auto s_CameraEntity = s_SelectedEntity.QueryInterface<ZCameraEntity>()) {
            BeginPanelPadding("##CameraToggle");

            if (ImGui::Button(ICON_MD_CAMERA "Toggle camera")) {
                m_CameraActive = !m_CameraActive;

                QueueTask([this, s_SelectedEntity, s_IsActive = m_CameraActive] {
                    if (!s_IsActive) {
                        DeactivateCamera();
                    }
                    else if (IsEntityInTree(s_SelectedEntity)) {
                        ZEntityRef s_CameraRef = s_SelectedEntity;
                        ActivateCamera(&s_CameraRef);
                    }
                });
            }

            EndPanelPadding();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.f));

        if (DrawPropertySectionHeader("Pins", "", true)) {
            ImGui::PopStyleVar();
            DrawEntityPins(s_SelectedEntity);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.f));
        }

        const bool s_IsKeywordsOpen = DrawPropertySectionHeader("Keywords", "", false);

        ImGui::PopStyleVar();

        if (s_IsKeywordsOpen) {
            DrawEntityKeywords(s_SelectedEntity);
        }

        DrawEntityPropertyTable(s_SelectedEntity, s_Details);
    }

    ImGui::End();
}

bool Editor::DrawEntityToolbar(ZEntityRef p_Entity, ZSpatialEntity* p_Spatial) {
    PushFrameHeight(20.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ScaledSize(5.f), ScaledSize(3.f)));

    const auto s_ToolbarButton = [&](const char* p_Icon, const char* p_Tooltip) {
        const bool s_IsClicked = IconButton(p_Icon, p_Tooltip);

        ImGui::SameLine();

        return s_IsClicked;
    };

    if (s_ToolbarButton(ICON_MD_SUPERVISOR_ACCOUNT, "Select logical parent")) {
        OnSelectEntity(p_Entity.GetLogicalParent(), true, std::nullopt);
    }

    if (s_ToolbarButton(ICON_MD_BRANDING_WATERMARK, "Select owning entity (brick)")) {
        OnSelectEntity(p_Entity.GetOwningEntity(), true, std::nullopt);
    }

    ZActor* s_Actor = p_Entity.QueryInterface<ZActor>();
    ZEntityRef s_LogicalParent = p_Entity.GetLogicalParent();
    ZActor* s_Actor2 = s_LogicalParent ? s_LogicalParent.QueryInterface<ZActor>() : nullptr;
    ZActor* s_TargetActor = s_Actor ? s_Actor : s_Actor2;

    if (s_TargetActor && s_ToolbarButton(ICON_MD_PEOPLE, "Select in actors window")) {
        m_SelectedActor = s_TargetActor;
        m_ScrollToActor = true;
        m_GlobalOutfitKit = {};
    }

    if (s_ToolbarButton(ICON_MD_DESELECT, "Deselect")) {
        OnSelectEntity({}, false, std::nullopt);
    }

    // Only spawned entities can be destroyed.
    if (m_SpawnedEntities.contains(p_Entity) && s_ToolbarButton(ICON_MD_DELETE, "Destroy entity")) {
        OnDestroyEntity(p_Entity, std::nullopt);
    }

    if (p_Spatial) {
        DrawToolbarSeparator();
        ImGui::SameLine();

        if (s_ToolbarButton(ICON_MD_PERSON_PIN, "Move to hitman")) {
            if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
                auto s_HitmanSpatial = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();

                OnEntityTransformChange(p_Entity, s_HitmanSpatial->GetObjectToWorldMatrix(), false, std::nullopt);
            }
        }

        if (s_ToolbarButton(ICON_MD_PERSON_PIN_CIRCLE, "Move hitman to entity")) {
            if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
                OnEntityTransformChange(s_LocalHitman.m_entityRef, p_Spatial->GetObjectToWorldMatrix(), false, std::nullopt);
            }
        }
    }

    ImGui::NewLine();

    ImGui::PopStyleVar(2);

    return false;
}

static std::string GetTemplateFactoryType(const std::string_view p_BlueprintFactoryType) {
    static constexpr std::pair<std::string_view, std::string_view> c_TemplateFactoryTypes[] = {
        { "TBLU", "TEMP" },
        { "ASEB", "ASET" },
        { "CBLU", "CPPT" },
        { "ECPB", "ECPT" },
        { "UICB", "UICT" },
        { "MATB", "MATT" },
        { "AIBB", "AIBX" },
        { "WSWB", "WSWT" },
        { "WSGB", "WSGT" },
    };

    const auto s_It = std::ranges::find(c_TemplateFactoryTypes, p_BlueprintFactoryType, &std::pair<std::string_view, std::string_view>::first);

    return s_It != std::end(c_TemplateFactoryTypes) ? std::string(s_It->second) : std::string();
}

// Needs m_CachedEntityTreeMutex.
Editor::EntityDetails Editor::CollectEntityDetails(ZEntityRef p_Entity) {
    EntityDetails s_Details;

    {
        auto s_Iterator = m_CachedEntityTreeMap.find(p_Entity);

        if (s_Iterator != m_CachedEntityTreeMap.end()) {
            const std::shared_ptr<EntityTreeNode> s_EntityTreeNode = s_Iterator->second;

            s_Details.HasTreeNode = true;
            s_Details.Name = SplitEntityTreeNodeName(s_EntityTreeNode->Name).Name;
            s_Details.Id = fmt::format("{:016x}", s_EntityTreeNode->EntityId);
            s_Details.TypeName = s_EntityTreeNode->EntityType;

            s_Details.Rows.emplace_back("Type", s_Details.TypeName);

            {
                std::shared_lock s_FactoryLock(m_EntityRefToFactoryRuntimeResourceIDsMutex);
                auto s_Iterator2 = m_EntityRefToFactoryRuntimeResourceIDs.find(p_Entity);

                if (s_Iterator2 != m_EntityRefToFactoryRuntimeResourceIDs.end()) {
                    const auto [s_TemplateFactoryRuntimeResourceID, s_ParentTemplateFactoryRuntimeResourceID] = s_Iterator2->second;

                    s_Details.ReferencedTemplateFactoryType = GetTemplateFactoryType(
                        s_EntityTreeNode->ReferencedBlueprintFactoryType
                    );
                    s_Details.ReferencedTemplateFactory = s_TemplateFactoryRuntimeResourceID;

                    s_Details.Rows.emplace_back(
                        s_Details.ReferencedTemplateFactoryType,
                        fmt::format("{:016X}", s_TemplateFactoryRuntimeResourceID.GetID())
                    );

                    s_Details.Rows.emplace_back(
                        s_EntityTreeNode->ReferencedBlueprintFactoryType,
                        fmt::format("{:016X}", s_EntityTreeNode->ReferencedBlueprintFactory.GetID())
                    );

                    s_Details.Rows.emplace_back(
                        fmt::format("In {}", s_EntityTreeNode->BlueprintFactoryType == "TBLU" ? "TEMP" : "ASET"),
                        fmt::format("{:016X}", s_ParentTemplateFactoryRuntimeResourceID.GetID())
                    );
                }
                else if (!s_EntityTreeNode->ReferencedBlueprintFactoryType.empty()) {
                    s_Details.Rows.emplace_back(
                        s_EntityTreeNode->ReferencedBlueprintFactoryType,
                        fmt::format("{:016X}", s_EntityTreeNode->ReferencedBlueprintFactory.GetID())
                    );
                }

                s_Details.Rows.emplace_back(
                    fmt::format("In {}", s_EntityTreeNode->BlueprintFactoryType),
                    fmt::format("{:016X}", s_EntityTreeNode->BlueprintFactory.GetID())
                );
            }
        }
    }

    if (const ZGeomEntity* s_GeomEntity = p_Entity.QueryInterface<ZGeomEntity>()) {
        if (s_GeomEntity->m_ResourceID.m_nResourceIndex.val != -1) {
            const auto s_PrimResourceInfo = (*Globals::ResourceContainer)->m_resources[s_GeomEntity->m_ResourceID.
                m_nResourceIndex.val];

            s_Details.Rows.emplace_back("PRIM", fmt::format("{:016X}", s_PrimResourceInfo.rid.GetID()));
        }
    }

    return s_Details;
}

void Editor::DrawEntityName(const EntityDetails& p_Details) {
    PushFrameHeight(20.f);

    ImGui::PushFont(SDK()->GetImGuiBoldFont(), 0.f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(p_Details.Name.c_str());
    ImGui::PopFont();

    ImGui::SameLine(0.f, ScaledSize(2.f));

    if (GhostButton(ICON_MD_CONTENT_COPY, "##CopyEntityName", "Copy entity name")) {
        CopyToClipboard(p_Details.Name);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(c_MutedTextColor, "%s", p_Details.Id.c_str());
    ImGui::SameLine(0.f, ScaledSize(2.f));

    if (GhostButton(ICON_MD_CONTENT_COPY, "##CopyEntityId", "Copy entity ID")) {
        CopyToClipboard(p_Details.Id);
    }

    ImGui::PopStyleVar();
}

// Matches the outer padding of the property table, whose cells are padded by this much.
static constexpr float c_TableLeftPadding = 6.f;

void Editor::DrawEntityDetails(const EntityDetails& p_Details) {
    ImGui::Dummy(ImVec2(0.f, ScaledSize(4.f)));

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(ScaledSize(2.5f), ScaledSize(0.5f)));
    PushFrameHeight(20.f);

    if (ImGui::BeginTable("##Details", 3)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, ScaledSize(62.f + c_TableLeftPadding));
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Copy", ImGuiTableColumnFlags_WidthFixed, ScaledSize(20.f));

        for (size_t i = 0; i < p_Details.Rows.size(); ++i) {
            const auto& [s_Label, s_Value] = p_Details.Rows[i];

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ScaledSize(c_TableLeftPadding));
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(c_MutedTextColor, "%s", s_Label.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(s_Value.c_str());

            ImGui::TableSetColumnIndex(2);

            if (GhostButton(
                ICON_MD_CONTENT_COPY,
                fmt::format("##CopyDetail{}", i),
                fmt::format("Copy {}", s_Label).c_str()
            )) {
                CopyToClipboard(s_Value);
            }
        }

        ImGui::EndTable();
    }

    ImGui::PopStyleVar(2);

    ImGui::Dummy(ImVec2(0.f, ScaledSize(6.f)));
}

void Editor::DrawEntityKeywords(ZEntityRef p_Entity) {
    BeginPanelPadding("##Keywords");

    static char s_KeywordString[2048] = {};

    const float buttonWidth = ImGui::CalcTextSize(ICON_MD_ADD " Add keyword").x + ImGui::GetStyle().FramePadding.x * 2.0f;

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - buttonWidth - ImGui::GetStyle().ItemSpacing.x);

    ImGui::InputText("##Keyword", s_KeywordString, sizeof(s_KeywordString));

    ImGui::SameLine();

    if (ImGui::Button(ICON_MD_ADD " Add keyword")) {
        if (strlen(s_KeywordString) > 0) {
            const int32_t s_Keyword = Hash::Fnv1a(s_KeywordString);

            QueueTask([p_Entity, s_Keyword] {
                Functions::ZGameKeywordManager_AddKeyword->Call(Globals::GameKeywordManager, p_Entity, s_Keyword);
            });

            s_KeywordString[0] = '\0';
        }
    }

    ImGui::Separator();

    THashSet<int32_t, TDefaultHashSetPolicy<int32_t>> s_Keywords;

    Functions::ZGameKeywordManager_GetKeywords->Call(Globals::GameKeywordManager, p_Entity, s_Keywords);

    for (const auto s_Keyword : s_Keywords) {
        ZString s_KeywordString;

        Functions::ZGameKeywordManager_GetKeywordString->Call(
            Globals::GameKeywordManager,
            s_KeywordString,
            s_Keyword
        );

        ImGui::Text(s_KeywordString.c_str());

        ImGui::SameLine();

        ImGui::PushID(s_Keyword);

        if (ImGui::SmallButton(ICON_MD_CONTENT_COPY)) {
            CopyToClipboard(s_KeywordString.c_str());
        }

        ImGui::SameLine();

        if (ImGui::SmallButton(ICON_MD_DELETE)) {
            QueueTask([p_Entity, s_Keyword] {
                Functions::ZGameKeywordManager_RemoveKeyword->Call(Globals::GameKeywordManager, p_Entity, s_Keyword);
            });

            ImGui::PopID();

            break;
        }

        ImGui::PopID();

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Remove");
        }
    }

    EndPanelPadding();
}

static ZRenderMaterialInstance* LoadRenderMaterialInstance(const ZRuntimeResourceID& p_MaterialEntityFactory) {
    TResourcePtr<ZRenderMaterialEntityFactory> s_MaterialEntityResourcePtr;

    Globals::ResourceManager->LoadResource(s_MaterialEntityResourcePtr, p_MaterialEntityFactory);

    ZRenderMaterialEntityFactory* s_RenderMaterialEntityFactory = s_MaterialEntityResourcePtr.GetResource();

    if (!s_RenderMaterialEntityFactory) {
        return nullptr;
    }

    TResourcePtr<ZRenderMaterialInstance> s_RenderMaterialInstanceResourcePtr;

    Globals::ResourceManager->LoadResource(
        s_RenderMaterialInstanceResourcePtr,
        s_RenderMaterialEntityFactory->m_ridMaterialInstance
    );

    return s_RenderMaterialInstanceResourcePtr.GetResource();
}

static std::string GetPropertyName(const SPropertyData* p_Property, const SNamedPropertyInfo* p_PropertyInfo) {
    if (!p_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->IsResource() &&
        p_PropertyInfo->m_nPropertyID == p_Property->m_nPropertyID) {
        return p_PropertyInfo->m_pszPropertyName;
    }

    // Some properties don't have a name for some reason. Try to find using RL.
    const auto s_PropertyNameData = HM3_GetPropertyName(p_Property->m_nPropertyID);

    if (s_PropertyNameData.Size > 0) {
        return std::string(s_PropertyNameData.Data, s_PropertyNameData.Size);
    }

    return fmt::format("~{:08x}", p_Property->m_nPropertyID);
}

static bool IsActiveIdSubmittedSince(const ImGuiID p_ActiveIdAliveBefore) {
    const ImGuiContext& s_Context = *ImGui::GetCurrentContext();

    return s_Context.ActiveId != 0 &&
        s_Context.ActiveIdIsAlive == s_Context.ActiveId &&
        p_ActiveIdAliveBefore != s_Context.ActiveId;
}

ZRenderMaterialInstance* Editor::LoadSelectedMaterialInstance(ZEntityRef p_Entity) {
    ZRuntimeResourceID s_MaterialEntityFactory;

    {
        std::shared_lock s_TreeLock(m_CachedEntityTreeMutex);

        const auto s_NodeIt = m_CachedEntityTreeMap.find(p_Entity);

        if (s_NodeIt == m_CachedEntityTreeMap.end() ||
            GetTemplateFactoryType(s_NodeIt->second->ReferencedBlueprintFactoryType) != "MATT") {
            return nullptr;
        }

        std::shared_lock s_FactoryLock(m_EntityRefToFactoryRuntimeResourceIDsMutex);

        const auto s_It = m_EntityRefToFactoryRuntimeResourceIDs.find(p_Entity);

        if (s_It == m_EntityRefToFactoryRuntimeResourceIDs.end()) {
            return nullptr;
        }

        s_MaterialEntityFactory = s_It->second.first;
    }

    return LoadRenderMaterialInstance(s_MaterialEntityFactory);
}

// Needs m_CachedEntityTreeMutex.
void Editor::DrawEntityPropertyTable(ZEntityRef p_Entity, const EntityDetails& p_Details) {
    const auto s_EntityType = p_Entity->GetType();

    if (!s_EntityType || !s_EntityType->m_pPropertyData ||
        !DrawPropertySectionHeader("Properties", std::to_string(s_EntityType->m_pPropertyData->size())) ||
        !BeginPropertyTable("##Properties", 132.f)) {
        return;
    }

    // Widget IDs include the entity, so a drag ends when the selection changes.
    ImGui::PushID(p_Entity.m_pObj);

    for (uint32_t i = 0; i < s_EntityType->m_pPropertyData->size(); ++i) {
        SPropertyData* s_Property = &(*s_EntityType->m_pPropertyData)[i];
        const auto* s_PropertyInfo = s_Property->GetPropertyInfo();

        if (!s_PropertyInfo || !s_PropertyInfo->m_propertyInfo.m_Type)
            continue;

        const auto s_PropertyAddress = reinterpret_cast<uintptr_t>(p_Entity.m_pObj) + s_Property->
            m_nPropertyOffset;
        const uint16_t s_TypeSize = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_nTypeSize;
        const uint16_t s_TypeAlignment = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_nTypeAlignment;

        const ChangeKey s_Key { p_Entity, s_Property->m_nPropertyID };
        const bool s_IsEdited = m_PropertyEdit && m_PropertyEdit->Key == s_Key;

        // Get the value of the property.
        auto* s_Data = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
            s_TypeSize, s_TypeAlignment
        );

        // The update thread applies edits a frame later, and the drag widgets add to the value
        // we pass in, so we keep drawing the edited value until its commit is applied.
        if (s_IsEdited) {
            s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_pTypeFunctions->placementCopyConstruct(
                s_Data,
                m_PropertyEdit->Value.GetData()
            );
        }
        else if (s_PropertyInfo->m_propertyInfo.m_Flags & EPropertyInfoFlags::E_HAS_GETTER_SETTER) {
            s_PropertyInfo->m_propertyInfo.m_PropertyGetter(
                reinterpret_cast<void*>(s_PropertyAddress),
                s_Data,
                s_PropertyInfo->m_propertyInfo.m_nExtraData
            );
        }
        else {
            s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_pTypeFunctions->placementCopyConstruct(
                s_Data,
                reinterpret_cast<void*>(s_PropertyAddress)
            );
        }

        const std::string s_InputId = std::format("##Property{}", i);
        const std::string s_PropertyName = GetPropertyName(s_Property, s_PropertyInfo);
        const ImGuiID s_ActiveIdAliveBefore = ImGui::GetCurrentContext()->ActiveIdIsAlive;

        const PropertyRowResult s_Row = DrawPropertyRow(
            s_InputId,
            s_PropertyName,
            s_PropertyInfo->m_propertyInfo.m_Type,
            p_Entity,
            s_Property,
            s_Data,
            EPropertyRowButton::Copy
        );

        if (s_Row.m_IsNameHovered) {
            std::string s_Tooltip = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->pszTypeName;

            const ZRenderMaterialInstance* s_RenderMaterialInstance = m_SelectedMaterialInstance;

            if (s_RenderMaterialInstance && s_RenderMaterialInstance->m_pEffect) {
                for (const auto& s_SemanticStringPair : s_RenderMaterialInstance->m_pEffect->m_SemanticStringPairs) {
                    if (s_PropertyName == s_SemanticStringPair.m_MaterialPropertyName) {
                        s_Tooltip += "\n";
                        s_Tooltip += s_SemanticStringPair.m_ShaderParameterName.c_str();
                        break;
                    }
                }
            }

            ImGui::BeginTooltip();
            ImGui::PushFont(SDK()->GetImGuiBoldFont(), 0.f);
            ImGui::TextUnformatted(s_PropertyName.c_str());
            ImGui::PopFont();
            ImGui::TextUnformatted(s_Tooltip.c_str());
            ImGui::EndTooltip();
        }

        const bool s_IsActive = IsActiveIdSubmittedSince(s_ActiveIdAliveBefore);

        // A drag sends a preview each frame and commits once on release. Other edits commit right away.
        if (s_Row.m_Changed) {
            if (!s_IsEdited || m_PropertyEdit->IsCommitted()) {
                m_PropertyEdit = ActiveEdit { .Key = s_Key };
            }

            m_PropertyEdit->Value.Assign(s_PropertyInfo->m_propertyInfo.m_Type, s_Data);

            if (s_IsActive) {
                OnSetPropertyValue(
                    p_Entity, s_Property->m_nPropertyID, m_PropertyEdit->Value, std::nullopt, ChangeMode::Preview
                );
            }
            else {
                CommitPropertyEdit();
            }
        }
        else if (s_IsEdited && !m_PropertyEdit->IsCommitted() && !s_IsActive) {
            CommitPropertyEdit();
        }

        if (m_PropertyEdit && m_PropertyEdit->Key == s_Key) {
            m_PropertyEdit->IsDrawn = true;
        }

        s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_pTypeFunctions->destruct(s_Data);
        (*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);
    }

    ImGui::PopID();

    EndPropertyTable();
}

static void SetPinArgumentType(ZObjectRef& p_Value, STypeID* p_TypeID) {
    p_Value.Clear();

    if (!p_TypeID) {
        return;
    }

    p_Value.UNSAFE_SetType(p_TypeID);

    void* s_Data = p_Value.AllocateMemory();

    if (std::string_view(p_TypeID->GetTypeInfo()->pszTypeName) == "SMatrix43") {
        new (s_Data) SMatrix43 {};
    }
    else {
        p_TypeID->GetTypeInfo()->m_pTypeFunctions->placementConstruct(s_Data);
    }
}

void Editor::DrawEntityPins(ZEntityRef p_Entity) {
    static char s_InputPinName[1024] = {};
    static char s_OutputPinName[1024] = {};

    static std::string s_LastInputPinName;
    static std::string s_LastOutputPinName;

    if (m_ShouldClearRecentPinFires.exchange(false)) {
        m_RecentPinFires.clear();
    }

    char* s_PinName = m_IsFiringOutputPin ? s_OutputPinName : s_InputPinName;
    std::string& s_LastPinName = m_IsFiringOutputPin ? s_LastOutputPinName : s_LastInputPinName;
    ZObjectRef& s_PinValue = m_IsFiringOutputPin ? m_OutputPinValue : m_InputPinValue;

    BeginPanelPadding("##PinInputs");

    PushFrameHeight(20.f, 3.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ScaledSize(4.f), ScaledSize(4.f)));

    ImGui::Dummy(ImVec2(0.f, 0.f));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ScaledSize(7.f), ImGui::GetStyle().FramePadding.y));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ScaledSize(2.f), ScaledSize(4.f)));

    for (const bool s_IsOutput : { false, true }) {
        const bool s_IsOn = m_IsFiringOutputPin == s_IsOutput;

        if (s_IsOn) {
            PushSelectedButtonStyle();
        }

        if (ImGui::Button(s_IsOutput ? "Out##PinDirection" : "In##PinDirection")) {
            m_IsFiringOutputPin = s_IsOutput;
        }

        if (s_IsOn) {
            PopSelectedButtonStyle();
        }

        ImGui::SameLine();
    }

    ImGui::PopStyleVar(2);
    ImGui::SameLine();

    const auto s_Pins = GetPins(p_Entity, m_IsFiringOutputPin);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    Util::ImGuiUtils::InputWithAutocomplete(
        m_IsFiringOutputPin ? "##OutputPinsPopup" : "##InputPinsPopup",
        s_PinName,
        sizeof(s_InputPinName),
        s_Pins,
        [](const PinInfo& s_PinInfo) -> std::string {
            return s_PinInfo.m_Name;
        },
        [](const PinInfo& s_PinInfo) -> std::string {
            if (s_PinInfo.m_Description.empty()) {
                return s_PinInfo.m_Name;
            }

            return s_PinInfo.m_Name + " - " + s_PinInfo.m_Description;
        },
        [&](const std::string&, const std::string&, const std::string& p_Value) {
            strcpy_s(s_PinName, sizeof(s_InputPinName), p_Value.c_str());
        },
        [](const PinInfo& s_PinInfo) -> std::string {
            return s_PinInfo.m_Name;
        }
    );

    bool s_ShouldFire = ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Enter);

    if (s_LastPinName != s_PinName) {
        s_LastPinName = s_PinName;

        if (const auto s_It = m_LastPinArgumentTypes.find(s_LastPinName); s_It != m_LastPinArgumentTypes.end() &&
            s_It->second != s_PinValue.GetTypeID()) {
            SetPinArgumentType(s_PinValue, s_It->second);
        }
    }

    STypeID* s_ArgumentType = s_PinValue.GetTypeID();

    ImGui::SetNextItemWidth(ScaledSize(104.f));

    if (ImGui::BeginCombo(
        "##PinArgumentType", s_ArgumentType ? s_ArgumentType->GetTypeInfo()->pszTypeName : "No argument"
    )) {
        if (ImGui::Selectable("No argument", !s_ArgumentType)) {
            SetPinArgumentType(s_PinValue, nullptr);
        }

        for (const auto& [s_TypeName, s_TypeID] : m_PinDataTypes) {
            if (ImGui::Selectable(s_TypeName.c_str(), s_TypeID == s_ArgumentType)) {
                SetPinArgumentType(s_PinValue, s_TypeID);
            }
        }

        ImGui::EndCombo();
    }

    ImGui::SameLine();

    ImGui::PushFont(SDK()->GetImGuiBoldFont(), 0.f);
    const float s_FireWidth = ImGui::CalcTextSize(ICON_MD_BOLT " Fire").x + ImGui::GetStyle().FramePadding.x * 2.f;
    ImGui::PopFont();

    const float s_ValueWidth = ImGui::GetContentRegionAvail().x - s_FireWidth - ImGui::GetStyle().ItemSpacing.x;

    s_ArgumentType = s_PinValue.GetTypeID();

    if (!s_ArgumentType || !s_PinValue.GetData()) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(c_MutedTextColor, "No argument");
    }
    else {
        const std::string_view s_TypeName = s_ArgumentType->GetTypeInfo()->pszTypeName;

        ImGui::PushItemWidth(s_ValueWidth);
        ImGui::BeginGroup();

        if (s_TypeName == "SMatrix43") {
            DrawEntityPinValue("##PinArgument", static_cast<SMatrix43*>(s_PinValue.GetData()));
        }
        else {
            DrawEntityPropertyValue("##PinArgument", s_ArgumentType, s_PinValue.GetData());
        }

        ImGui::EndGroup();
        ImGui::PopItemWidth();

        s_ShouldFire |= ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter);
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.f, ImGui::GetContentRegionAvail().x - s_FireWidth));

    ImGui::BeginDisabled(s_PinName[0] == '\0');
    PushSelectedButtonStyle();

    s_ShouldFire |= ImGui::Button(ICON_MD_BOLT " Fire##FirePin");

    PopSelectedButtonStyle();
    ImGui::EndDisabled();

    if (s_ShouldFire) {
        FireEntityPin(p_Entity, m_IsFiringOutputPin, s_PinName, s_PinValue);
    }

    ImGui::Dummy(ImVec2(0.f, ScaledSize(2.f)));

    ImGui::PopStyleVar(2);

    EndPanelPadding();

    if (const auto s_FireAgain = DrawRecentPinFires(p_Entity)) {
        FireEntityPin(p_Entity, s_FireAgain->m_IsOutput, s_FireAgain->m_Name, s_FireAgain->m_Value);
    }
}

void Editor::FireEntityPin(
    ZEntityRef p_Entity, const bool p_IsOutput, const std::string& p_Name, const ZObjectRef& p_Value
) {
    if (p_Name.empty()) {
        return;
    }

    QueueTask([this, p_Entity, p_Name, p_IsOutput, p_Value] {
        if (IsEntityInTree(p_Entity)) {
            OnSignalEntityPin(p_Entity, p_Name, p_IsOutput, p_Value);
        }
    });

    m_LastPinArgumentTypes[p_Name] = p_Value.GetTypeID();

    const auto s_FormatArgument = [&](const ZObjectRef& p_Argument) -> std::string {
        if (!p_Argument.GetTypeID() || !p_Argument.GetData()) {
            return {};
        }

        return FormatPropertyValueForClipboard(p_Argument.GetTypeID(), p_Argument.GetData());
    };

    auto& s_RecentFires = m_RecentPinFires[p_Entity];
    const std::string s_Argument = s_FormatArgument(p_Value);

    std::erase_if(s_RecentFires, [&](const RecentPinFire& p_Fire) {
        return p_Fire.m_IsOutput == p_IsOutput && p_Fire.m_Name == p_Name &&
            p_Fire.m_Value.GetTypeID() == p_Value.GetTypeID() && s_FormatArgument(p_Fire.m_Value) == s_Argument;
    });

    s_RecentFires.push_front({ p_IsOutput, p_Name, p_Value, ImGui::GetTime() });

    if (s_RecentFires.size() > 4) {
        s_RecentFires.pop_back();
    }
}

static constexpr float c_PinFireFlashSeconds = 0.6f;

std::optional<Editor::RecentPinFire> Editor::DrawRecentPinFires(ZEntityRef p_Entity) {
    const auto s_RecentIt = m_RecentPinFires.find(p_Entity);

    if (s_RecentIt == m_RecentPinFires.end() || s_RecentIt->second.empty()) {
        return std::nullopt;
    }

    ImDrawList* s_DrawList = ImGui::GetWindowDrawList();
    const float s_Width = ImGui::GetContentRegionAvail().x;
    const float s_RowHeight = ScaledSize(22.f);
    const float s_Padding = ScaledSize(6.f);
    const float s_TextOffsetY = (s_RowHeight - ImGui::GetFontSize()) * 0.5f;
    const ImU32 s_MutedColor = ImGui::GetColorU32(c_MutedTextColor);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.f));

    const ImVec2 s_HeaderMin = ImGui::GetCursorScreenPos();

    DrawPanelHeader("Recent", "click to fire again", s_Width);

    s_DrawList->AddLine(
        s_HeaderMin, ImVec2(s_HeaderMin.x + s_Width, s_HeaderMin.y), ImGui::GetColorU32(c_SectionBorderColor), ScaledSize(1.f)
    );

    std::optional<RecentPinFire> s_FireAgain;

    for (size_t i = 0; i < s_RecentIt->second.size(); ++i) {
        const RecentPinFire& s_Fire = s_RecentIt->second[i];
        const ImVec2 s_Min = ImGui::GetCursorScreenPos();
        const ImVec2 s_Max(s_Min.x + s_Width, s_Min.y + s_RowHeight);

        ImGui::PushID(static_cast<int>(i));

        if (ImGui::InvisibleButton("##RecentPinFire", ImVec2(s_Width, s_RowHeight))) {
            s_FireAgain = s_Fire;
        }

        if (ImGui::IsItemHovered()) {
            s_DrawList->AddRectFilled(s_Min, s_Max, IM_COL32(0xff, 0xff, 0xff, 0x14));
            ImGui::SetTooltip("Fire again");
        }

        ImGui::PopID();

        const float s_Flash = std::max(
            0.f, 1.f - static_cast<float>(ImGui::GetTime() - s_Fire.m_FireTime) / c_PinFireFlashSeconds
        );

        const auto s_AccentColor = [&](const float p_Alpha) {
            return ImGui::GetColorU32(ImVec4(c_AccentColor.x, c_AccentColor.y, c_AccentColor.z, p_Alpha));
        };

        if (s_Flash > 0.f) {
            s_DrawList->AddRectFilled(s_Min, s_Max, s_AccentColor(s_Flash * 0.35f));
        }

        s_DrawList->AddLine(
            ImVec2(s_Min.x, s_Max.y - 1.f), ImVec2(s_Max.x, s_Max.y - 1.f), ImGui::GetColorU32(c_RowBorderColor)
        );

        const float s_TextY = s_Min.y + s_TextOffsetY;
        float s_X = s_Min.x + s_Padding;

        s_DrawList->AddText(
            ImVec2(s_X, s_TextY), s_MutedColor, s_Fire.m_IsOutput ? ICON_MD_ARROW_BACK : ICON_MD_ARROW_FORWARD
        );
        s_X += ScaledSize(14.f) + s_Padding;

        s_DrawList->AddText(ImVec2(s_X, s_TextY), ImGui::GetColorU32(ImGuiCol_Text), s_Fire.m_Name.c_str());
        s_X += ImGui::CalcTextSize(s_Fire.m_Name.c_str()).x + s_Padding;

        const float s_BoltX = s_Max.x - s_Padding - ImGui::CalcTextSize(ICON_MD_BOLT).x;

        if (s_Fire.m_Value.GetTypeID() && s_Fire.m_Value.GetData()) {
            const std::string s_Argument = FormatPropertyValueForClipboard(
                s_Fire.m_Value.GetTypeID(), s_Fire.m_Value.GetData()
            );

            s_DrawList->PushClipRect(ImVec2(s_X, s_Min.y), ImVec2(s_BoltX - s_Padding, s_Max.y), true);
            s_DrawList->AddText(ImVec2(s_X, s_TextY), s_MutedColor, s_Argument.c_str());
            s_DrawList->PopClipRect();
        }

        s_DrawList->AddText(ImVec2(s_BoltX, s_TextY), s_MutedColor, ICON_MD_BOLT);

        if (s_Flash > 0.f) {
            s_DrawList->AddText(ImVec2(s_BoltX, s_TextY), s_AccentColor(s_Flash), ICON_MD_BOLT);
        }
    }

    ImGui::PopStyleVar();

    return s_FireAgain;
}

void Editor::DrawEntityTransform(const SMatrix& p_Transform) {
    const float s_HeaderX = ImGui::GetCursorScreenPos().x;

    // Anchoring to the header frame widens the content every frame when there's a scrollbar.
    const float s_ContentRight = s_HeaderX + ImGui::GetContentRegionAvail().x;
    const bool s_IsOpen = DrawPropertySectionHeader("Transform", "", true);
    const ImVec2 s_HeaderMin = ImGui::GetItemRectMin();
    const ImVec2 s_HeaderMax = ImGui::GetItemRectMax();
    const ImVec2 s_ContentPos = ImGui::GetCursorScreenPos();

    static constexpr float c_SmallButtonFontSize = 12.f;
    static constexpr const char* c_CopyMatrixLabel = ICON_MD_CONTENT_COPY " Matrix##EntityTransformMatrix";
    static constexpr const char* c_CopyRtLabel = "RT##EntityTransformRT";
    static constexpr const char* c_CopyQnLabel = "QN##EntityTransformQN";

    const char* s_SpaceLabel = m_UseLocalTransforms ? "Local##EntityTransformSpace" : "World##EntityTransformSpace";

    const float s_SmallButtonPadding = ScaledSize(5.f);
    const float s_SmallButtonHeight = ScaledSize(16.f);
    const float s_SmallButtonSpacing = ScaledSize(2.f);

    ImGui::PushFont(nullptr, FontSize(c_SmallButtonFontSize));

    float s_ButtonsWidth = s_SmallButtonSpacing * 3.f;

    for (const char* s_Label : { s_SpaceLabel, c_CopyMatrixLabel, c_CopyRtLabel, c_CopyQnLabel }) {
        s_ButtonsWidth += ImGui::CalcTextSize(s_Label, nullptr, true).x + s_SmallButtonPadding * 2.f;
    }

    PushFrameHeight(16.f, 5.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(s_SmallButtonSpacing, 0.f));

    ImGui::SetCursorScreenPos(
        ImVec2(
            s_ContentRight - s_ButtonsWidth,
            s_HeaderMin.y + (s_HeaderMax.y - s_HeaderMin.y - s_SmallButtonHeight) * 0.5f
        )
    );

    if (ImGui::Button(s_SpaceLabel)) {
        m_UseLocalTransforms = !m_UseLocalTransforms;
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "%s",
            m_UseLocalTransforms
                ? "Showing the local transform, relative to the parent.\nClick to show the world transform."
                : "Showing the world transform.\nClick to show the local transform, relative to the parent."
        );
    }

    ImGui::SameLine();

    if (ImGui::Button(c_CopyMatrixLabel)) {
        CopyToClipboard(fmt::format("{}", p_Transform));
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Copy matrix");
    }

    ImGui::SameLine();

    if (ImGui::Button(c_CopyRtLabel)) {
        CopyToClipboard(FormatMatrixAsRtJson(p_Transform.ToMatrix43(), false, 0));
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Copy as RT JSON");
    }

    ImGui::SameLine();

    if (ImGui::Button(c_CopyQnLabel)) {
        CopyToClipboard(FormatQneTransformAsJson(MatrixToQneTransform(p_Transform), false, 0));
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Copy as QN JSON");
    }

    ImGui::PopStyleVar(2);
    ImGui::PopFont();

    ImGui::SetCursorScreenPos(s_ContentPos);

    if (!s_IsOpen) {
        return;
    }

    struct TransformRow {
        const char* m_Label;
        const char* m_Suffix;
        SVector3 m_Values;
    };

    std::vector<TransformRow> s_Rows;

    if (m_UseQneTransforms) {
        const auto s_QneTransform = MatrixToQneTransform(p_Transform);

        s_Rows.push_back({ "Position", nullptr, s_QneTransform.Position });
        s_Rows.push_back({ "Rotation", "\xC2\xB0", s_QneTransform.Rotation });
    }
    else {
        s_Rows.push_back({ "X axis", nullptr, SVector3(p_Transform.XAxis.x, p_Transform.XAxis.y, p_Transform.XAxis.z) });
        s_Rows.push_back({ "Y axis", nullptr, SVector3(p_Transform.YAxis.x, p_Transform.YAxis.y, p_Transform.YAxis.z) });
        s_Rows.push_back({ "Z axis", nullptr, SVector3(p_Transform.ZAxis.x, p_Transform.ZAxis.y, p_Transform.ZAxis.z) });
        s_Rows.push_back({ "Position", nullptr, SVector3(p_Transform.Trans.x, p_Transform.Trans.y, p_Transform.Trans.z) });
    }

    const float s_RowHeight = ScaledSize(17.f);

    ImGui::Dummy(ImVec2(0.f, ScaledSize(5.f)));

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(ScaledSize(3.f), 0.f));
    PushFrameHeight(17.f);

    if (ImGui::BeginTable("##EntityTransform", 4)) {
        ImGui::TableSetupColumn("Row", ImGuiTableColumnFlags_WidthFixed, ScaledSize(64.f + c_TableLeftPadding));
        ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow(ImGuiTableRowFlags_None, s_RowHeight);

        const float s_TagHeight = ScaledSize(14.f);

        PushFrameHeight(14.f);

        for (int i = 0; i < 3; ++i) {
            ImGui::TableSetColumnIndex(i + 1);

            const float s_TagWidth = CalcValueTagWidth(c_VectorTags[i]);

            ImGui::SetCursorPos(
                ImVec2(
                    ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - s_TagWidth,
                    ImGui::GetCursorPosY() + (s_RowHeight - s_TagHeight) * 0.5f
                )
            );

            DrawValueTag(c_VectorTags[i], c_AxisTagColors[i]);
        }

        ImGui::PopStyleVar();

        for (size_t s_RowIndex = 0; s_RowIndex < s_Rows.size(); ++s_RowIndex) {
            const auto& s_Row = s_Rows[s_RowIndex];

            ImGui::TableNextRow(ImGuiTableRowFlags_None, s_RowHeight);

            if (s_RowIndex % 2 == 1) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(c_AlternateRowColor));
            }

            ImGui::TableSetColumnIndex(0);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ScaledSize(c_TableLeftPadding));
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(c_SecondaryTextColor, "%s", s_Row.m_Label);

            if (s_Row.m_Suffix) {
                ImGui::SameLine(0.f, ScaledSize(5.f));
                ImGui::TextColored(c_MutedTextColor, "%s", s_Row.m_Suffix);
            }

            const float s_Values[] = { s_Row.m_Values.x, s_Row.m_Values.y, s_Row.m_Values.z };

            for (int i = 0; i < 3; ++i) {
                ImGui::TableSetColumnIndex(i + 1);

                const std::string s_Value = fmt::format("{:.3f}", s_Values[i]);

                ImGui::SetCursorPosX(
                    ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(s_Value.c_str()).x
                );
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(s_Value.c_str());
            }
        }

        ImGui::EndTable();
    }

    ImGui::PopStyleVar(2);

    ImGui::Dummy(ImVec2(0.f, ScaledSize(6.f)));
}

bool Editor::DrawEntityPropertyValue(const std::string& p_Id, const STypeID* p_TypeID, void* p_Data) {
    bool s_IsChanged = false;

    const IType* s_TypeInfo = p_TypeID->GetTypeInfo();
    const std::string_view s_TypeName = s_TypeInfo->pszTypeName;

    if (s_TypeName == "ZString") {
        s_IsChanged = StringProperty(p_Id, p_Data);
    }
    else if (s_TypeName == "bool") {
        s_IsChanged = BoolProperty(p_Id, p_Data);
    }
    else if (const ScalarPropertyType* s_ScalarType = FindScalarPropertyType(s_TypeName)) {
        s_IsChanged = TaggedDragScalar(
            p_Id,
            s_ScalarType->m_Tag,
            c_ValueTagColor,
            s_ScalarType->m_DataType,
            p_Data,
            ImGui::CalcItemWidth(),
            s_ScalarType->m_Speed
        );
    }
    else if (s_TypeName == "SVector2") {
        s_IsChanged = SVector2Property(p_Id, p_Data);
    }
    else if (s_TypeName == "SVector3") {
        s_IsChanged = SVector3Property(p_Id, p_Data);
    }
    else if (s_TypeName == "SVector4") {
        s_IsChanged = SVector4Property(p_Id, p_Data);
    }
    else if (s_TypeName == "SColorRGB") {
        s_IsChanged = SColorRGBProperty(p_Id, p_Data);
    }
    else if (s_TypeName == "SColorRGBA") {
        s_IsChanged = SColorRGBAProperty(p_Id, p_Data);
    }
    else if (s_TypeInfo->IsEnum()) {
        s_IsChanged = EnumProperty(p_Id, p_Data, p_TypeID);
    }
    else if (s_TypeInfo->IsResource()) {
        ResourcePtrProperty(p_Data);
    }
    else if (s_TypeName == "ZRuntimeResourceID") {
        ZRuntimeResourceIDProperty(p_Data);
    }
    else if (s_TypeName.starts_with("ZEntityRef")) {
        s_IsChanged = EntityRefProperty(p_Id, *static_cast<ZEntityRef*>(p_Data));
    }
    else if (s_TypeName.starts_with("TEntityRef")) {
        s_IsChanged = TEntityRefProperty(p_Id, p_Data, p_TypeID);
    }
    else if (s_TypeName == "ZRepositoryID") {
        ZRepositoryIDProperty(p_Data);
    }
    else if (s_TypeName == "ZGuid") {
        s_IsChanged = ZGuidProperty(p_Id, p_Data);
    }
    else if (s_TypeName == "ZGameTime") {
        s_IsChanged = ZGameTimeProperty(p_Id, p_Data);
    }
    else {
        UnsupportedProperty();
    }

    return s_IsChanged;
}

void Editor::DrawEntityPinValue(const std::string& p_Id, SMatrix43* p_Value) {
    if (m_UseQneTransforms) {
        auto s_QneTransform = MatrixToQneTransform(*p_Value);

        if (ImGui::DragFloat3((p_Id + "p").c_str(), &s_QneTransform.Position.x, 0.1f)) {
            *p_Value = QneTransformToMatrix(s_QneTransform).ToMatrix43();
        }

        if (ImGui::DragFloat3((p_Id + "r").c_str(), &s_QneTransform.Rotation.x, 0.1f)) {
            *p_Value = QneTransformToMatrix(s_QneTransform).ToMatrix43();
        }
    }
    else {
        ImGui::DragFloat3((p_Id + "x").c_str(), &p_Value->XAxis.x, 0.1f);
        ImGui::DragFloat3((p_Id + "y").c_str(), &p_Value->YAxis.x, 0.1f);
        ImGui::DragFloat3((p_Id + "z").c_str(), &p_Value->ZAxis.x, 0.1f);
        ImGui::DragFloat3((p_Id + "t").c_str(), &p_Value->Trans.x, 0.1f);
    }
}