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

void Editor::DrawEntityProperties() {
    auto s_ImgGuiIO = ImGui::GetIO();

    ImGui::SetNextWindowPos({s_ImgGuiIO.DisplaySize.x - 500, 110}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({500, s_ImgGuiIO.DisplaySize.y - 110}, ImGuiCond_FirstUseEver);
    ImGui::Begin(ICON_MD_TUNE " Entity Properties", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

    if (m_SelectedEntity == m_DynamicEntitiesNodeEntityRef ||
        m_SelectedEntity == m_UnparentedEntitiesNodeEntityRef
    ) {
        ImGui::End();

        return;
    }

    const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

    const auto s_SelectedEntity = m_SelectedEntity;

    if (s_SceneCtx && s_SceneCtx->m_pScene && s_SelectedEntity) {
        if (ImGui::Button(ICON_MD_SUPERVISOR_ACCOUNT)) {
            OnSelectEntity(s_SelectedEntity.GetLogicalParent(), true, std::nullopt);
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Select Logical Parent");

        ImGui::SameLine(0, 5);

        if (ImGui::Button(ICON_MD_BRANDING_WATERMARK)) {
            OnSelectEntity(s_SelectedEntity.GetOwningEntity(), true, std::nullopt);
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Select Owning Entity (Brick)");

        ImGui::SameLine(0, 5);

        if (ImGui::Button(ICON_MD_DESELECT))
            OnSelectEntity({}, false, std::nullopt);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Deselect");

        // Only show destroy button for custom entities.
        if (m_EntityNames.contains(s_SelectedEntity)) {
            ImGui::SameLine(0, 5);

            if (ImGui::Button(ICON_MD_DELETE)) {
                OnDestroyEntity(s_SelectedEntity, std::nullopt);

                // Prevent crash on destroy.
                ImGui::End();
                return;
            }

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Destroy Entity");
        }

        ZActor* s_Actor = s_SelectedEntity.QueryInterface<ZActor>();
        ZEntityRef s_LogicalParent = s_SelectedEntity.GetLogicalParent();
        ZActor* s_Actor2 = s_LogicalParent ? s_LogicalParent.QueryInterface<ZActor>() : nullptr;
        ZActor* s_TargetActor = s_Actor ? s_Actor : s_Actor2;

        if (s_TargetActor) {
            ImGui::SameLine(0, 5);

            if (ImGui::Button(ICON_MD_PEOPLE)) {
                m_SelectedActor = s_TargetActor;
                m_ScrollToActor = true;
                m_GlobalOutfitKit = {};
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Select In Actors Menu");
            }
        }

        static bool s_LocalTransform = false;
        ImGui::Checkbox("Local Transforms", &s_LocalTransform);

        ImGui::SameLine(0, 20);

        if (ImGui::Checkbox("QNE Transforms", &m_UseQneTransforms)) {
            SetSettingBool("general", "qne_transforms", m_UseQneTransforms);
        }

        if (ImGui::Checkbox("##UseSnap", &m_UseSnap)) {
            SetSettingBool("general", "snap", m_UseSnap);
        }

        ImGui::SameLine();

        if (ImGui::InputDouble("Snap", &m_SnapValue)) {
            SetSettingDouble("general", "snap_value", m_SnapValue);
        }

        if (ImGui::Checkbox("##UseAngleSnap", &m_UseAngleSnap)) {
            SetSettingBool("general", "angle_snap", m_UseAngleSnap);
        }

        ImGui::SameLine();

        if (ImGui::InputDouble("Angle Snap", &m_AngleSnapValue)) {
            SetSettingDouble("general", "angle_snap_value", m_AngleSnapValue);
        }

        if (ImGui::Checkbox("##UseScaleSnap", &m_UseScaleSnap)) {
            SetSettingBool("general", "scale_snap", m_UseScaleSnap);
        }

        ImGui::SameLine();

        if (ImGui::InputDouble("Scale Snap", &m_ScaleSnapValue)) {
            SetSettingDouble("general", "scale_snap_value", m_ScaleSnapValue);
        }

        ImGui::Separator();

        std::string s_ReferencedTemplateFactoryType;
        ZRuntimeResourceID s_ReferencedTemplateFactory;

        {
            std::shared_lock s_TreeLock(m_CachedEntityTreeMutex);

            auto s_Iterator = m_CachedEntityTreeMap.find(s_SelectedEntity);

            if (s_Iterator != m_CachedEntityTreeMap.end()) {
                const std::shared_ptr<EntityTreeNode> s_EntityTreeNode = s_Iterator->second;
                const std::string s_EntityName = s_EntityTreeNode->Name.substr(
                    0,
                    s_EntityTreeNode->Name.find_last_of(" (") - 1
                );

                ImGui::TextUnformatted(fmt::format("Entity Name: {}", s_EntityName).c_str());

                if (ImGuiCopyWidget("EntityName")) {
                    CopyToClipboard(s_EntityName);
                }

                ImGui::TextUnformatted(fmt::format("Entity ID: {:016x}", s_EntityTreeNode->EntityId).c_str());

                if (ImGuiCopyWidget("EntityID")) {
                    CopyToClipboard(fmt::format("{:016x}", s_EntityTreeNode->EntityId));
                }

                ImGui::TextUnformatted(fmt::format("Entity Type: {}", s_EntityTreeNode->EntityType).c_str());

                if (ImGuiCopyWidget("EntityType")) {
                    CopyToClipboard(s_EntityTreeNode->EntityType);
                }

                {
                    std::shared_lock s_FactoryLock(m_EntityRefToFactoryRuntimeResourceIDsMutex);
                    auto s_Iterator2 = m_EntityRefToFactoryRuntimeResourceIDs.find(s_SelectedEntity);

                    if (s_Iterator2 != m_EntityRefToFactoryRuntimeResourceIDs.end()) {
                        const auto [s_TemplateFactoryRuntimeResourceID, s_ParentTemplateFactoryRuntimeResourceID] = s_Iterator2->second;

                        if (s_EntityTreeNode->ReferencedBlueprintFactoryType == "TBLU") {
                            s_ReferencedTemplateFactoryType = "TEMP";
                        }
                        else if (s_EntityTreeNode->ReferencedBlueprintFactoryType == "ASEB") {
                            s_ReferencedTemplateFactoryType = "ASET";
                        }
                        else if (s_EntityTreeNode->ReferencedBlueprintFactoryType == "CBLU") {
                            s_ReferencedTemplateFactoryType = "CPPT";
                        }
                        else if (s_EntityTreeNode->ReferencedBlueprintFactoryType == "ECPB") {
                            s_ReferencedTemplateFactoryType = "ECPT";
                        }
                        else if (s_EntityTreeNode->ReferencedBlueprintFactoryType == "UICB") {
                            s_ReferencedTemplateFactoryType = "UICT";
                        }
                        else if (s_EntityTreeNode->ReferencedBlueprintFactoryType == "MATB") {
                            s_ReferencedTemplateFactoryType = "MATT";
                        }
                        else if (s_EntityTreeNode->ReferencedBlueprintFactoryType == "AIBB") {
                            s_ReferencedTemplateFactoryType = "AIBX";
                        }
                        else if (s_EntityTreeNode->ReferencedBlueprintFactoryType == "WSWB") {
                            s_ReferencedTemplateFactoryType = "WSWT";
                        }
                        else if (s_EntityTreeNode->ReferencedBlueprintFactoryType == "WSGB") {
                            s_ReferencedTemplateFactoryType = "WSGT";
                        }

                        s_ReferencedTemplateFactory = s_TemplateFactoryRuntimeResourceID;

                        ImGui::TextUnformatted(
                            fmt::format("{}: {:016X}", s_ReferencedTemplateFactoryType, s_TemplateFactoryRuntimeResourceID.GetID()
                            ).c_str());

                        if (ImGuiCopyWidget("ReferencedTemplateFactory")) {
                            CopyToClipboard(fmt::format("{:016X}", s_TemplateFactoryRuntimeResourceID.GetID()));
                        }

                        ImGui::TextUnformatted(
                            fmt::format("{}: {:016X}", s_EntityTreeNode->ReferencedBlueprintFactoryType, s_EntityTreeNode->ReferencedBlueprintFactory.GetID()
                            ).c_str());

                        if (ImGuiCopyWidget("ReferencedBlueprintFactory")) {
                            CopyToClipboard(fmt::format("{:016X}", s_EntityTreeNode->ReferencedBlueprintFactory.GetID()));
                        }

                        ImGui::TextUnformatted(fmt::format(
                            "Contained in {}: {:016X}",
                            s_EntityTreeNode->BlueprintFactoryType == "TBLU" ? "TEMP" : "ASET",
                            s_ParentTemplateFactoryRuntimeResourceID.GetID()
                        ).c_str());

                        if (ImGuiCopyWidget("ParentTemplateFactory")) {
                            CopyToClipboard(fmt::format("{:016X}", s_ParentTemplateFactoryRuntimeResourceID.GetID()));
                        }

                        ImGui::TextUnformatted(fmt::format(
                            "Contained in {}: {:016X}",
                            s_EntityTreeNode->BlueprintFactoryType,
                            s_EntityTreeNode->BlueprintFactory.GetID()
                        ).c_str());

                        if (ImGuiCopyWidget("ParentBlueprintFactory")) {
                            CopyToClipboard(fmt::format("{:016X}", s_EntityTreeNode->BlueprintFactory.GetID()));
                        }
                    }
                    else {
                        ImGui::TextUnformatted(
                            fmt::format("{}: {:016X}", s_EntityTreeNode->ReferencedBlueprintFactoryType, s_EntityTreeNode->ReferencedBlueprintFactory.GetID()
                            ).c_str());

                        if (ImGuiCopyWidget("ReferencedBlueprintFactory")) {
                            CopyToClipboard(fmt::format("{:016X}", s_EntityTreeNode->ReferencedBlueprintFactory.GetID()));
                        }

                        ImGui::TextUnformatted(fmt::format(
                            "Contained in {}: {:016X}",
                            s_EntityTreeNode->BlueprintFactoryType,
                            s_EntityTreeNode->BlueprintFactory.GetID()
                        ).c_str());

                        if (ImGuiCopyWidget("ParentBlueprintFactory")) {
                            CopyToClipboard(fmt::format("{:016X}", s_EntityTreeNode->BlueprintFactory.GetID()));
                        }
                    }
                }
            };
        }

        if (const ZGeomEntity* s_GeomEntity = s_SelectedEntity.QueryInterface<ZGeomEntity>()) {
            if (s_GeomEntity->m_ResourceID.m_nResourceIndex.val != -1) {
                const auto s_PrimResourceInfo = (*Globals::ResourceContainer)->m_resources[s_GeomEntity->m_ResourceID.
                    m_nResourceIndex.val];
                const auto s_PrimHash = s_PrimResourceInfo.rid.GetID();

                ImGui::TextUnformatted(fmt::format("Associated PRIM: {:016X}", s_PrimHash).c_str());

                const auto s_ContainedRpkg = (*Globals::ResourceContainer)->m_MountedPackages[s_PrimResourceInfo.
                    packageId];

                if (ImGuiCopyWidget("EntPrim")) {
                    CopyToClipboard(fmt::format("{:016X}", s_PrimHash));
                }
            }
        }

        if (const auto s_Spatial = s_SelectedEntity.QueryInterface<ZSpatialEntity>()) {
            ImGui::TextUnformatted("Entity Transform");

            auto s_Trans = s_Spatial->GetObjectToWorldMatrix();

            if (s_LocalTransform) {
                SMatrix s_ParentTrans;

                // Get parent entity.
                if (s_Spatial->m_eidParent.m_pInterfaceRef) {
                    s_ParentTrans = s_Spatial->m_eidParent.m_pInterfaceRef->GetObjectToWorldMatrix();
                }
                else if (s_SelectedEntity.GetLogicalParent() && s_SelectedEntity.GetLogicalParent().QueryInterface<
                    ZSpatialEntity>()) {
                    s_ParentTrans = s_SelectedEntity.GetLogicalParent().QueryInterface<ZSpatialEntity>()->
                        GetObjectToWorldMatrix();
                }
                else if (s_SelectedEntity.GetOwningEntity() && s_SelectedEntity.GetOwningEntity().QueryInterface<
                    ZSpatialEntity>()) {
                    s_ParentTrans = s_SelectedEntity.GetOwningEntity().QueryInterface<ZSpatialEntity>()->
                        GetObjectToWorldMatrix();
                }

                const auto s_ParentTransInv = s_ParentTrans.Inverse();

                auto s_LocalTrans = s_ParentTransInv * s_Trans;
                s_LocalTrans.Trans = s_Trans.Trans - s_ParentTrans.Trans;
                s_LocalTrans.Trans.w = 1.f;

                s_Trans = s_LocalTrans;
            }

            if (ImGuiCopyWidget("EntTrans")) {
                CopyToClipboard(fmt::format("{}", s_Trans));
            }

            ImGui::Separator();

            if (ImGui::BeginTable("DebugMod_HitmanPosition", 4)) {
                for (int i = 0; i < 4; ++i) {
                    ImGui::TableNextRow();

                    for (int j = 0; j < 4; ++j) {
                        ImGui::TableSetColumnIndex(j);
                        ImGui::Text("%f", s_Trans.flt[(i * 4) + j]);
                    }
                }

                ImGui::EndTable();
            }

            ImGui::Separator();

            if (ImGui::Button(ICON_MD_CONTENT_COPY " RT JSON##EntRT")) {
                CopyToClipboard(
                    fmt::format(
                        "{{"
                        "\"XAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},"
                        "\"YAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},"
                        "\"ZAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},"
                        "\"Trans\":{{\"x\":{},\"y\":{},\"z\":{}}}"
                        "}}",
                        s_Trans.XAxis.x,
                        s_Trans.XAxis.y,
                        s_Trans.XAxis.z,
                        s_Trans.YAxis.x,
                        s_Trans.YAxis.y,
                        s_Trans.YAxis.z,
                        s_Trans.ZAxis.x,
                        s_Trans.ZAxis.y,
                        s_Trans.ZAxis.z,
                        s_Trans.Trans.x,
                        s_Trans.Trans.y,
                        s_Trans.Trans.z
                    )
                );
            }

            ImGui::SameLine();

            if (ImGui::Button(ICON_MD_CONTENT_COPY " QN JSON##EntQN")) {
                const auto s_QneTrans = MatrixToQneTransform(s_Trans);

                CopyToClipboard(
                    fmt::format(
                        "{{\"rotation\":{{\"x\":{},\"y\":{},\"z\":{}}},\"position\":{{\"x\":{},\"y\":{},\"z\":{}}},"
                        "\"scale\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
                        s_QneTrans.Rotation.x, s_QneTrans.Rotation.y, s_QneTrans.Rotation.z,
                        s_QneTrans.Position.x, s_QneTrans.Position.y, s_QneTrans.Position.z,
                        s_QneTrans.Scale.x, s_QneTrans.Scale.y, s_QneTrans.Scale.z
                    )
                );
            }

            if (ImGui::Button(ICON_MD_PERSON_PIN " Move to Hitman")) {
                if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
                    auto s_HitmanSpatial = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();

                    OnEntityTransformChange(s_SelectedEntity, s_HitmanSpatial->GetObjectToWorldMatrix(), false, std::nullopt);
                }
            }

            ImGui::SameLine();

            if (ImGui::Button(ICON_MD_PERSON_PIN_CIRCLE " Move Hitman to")) {
                if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
                    auto s_HitmanSpatial = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();

                    s_HitmanSpatial->SetObjectToWorldMatrixFromEditor(s_Spatial->GetObjectToWorldMatrix());

                    OnEntityTransformChange(s_LocalHitman.m_entityRef, s_Spatial->GetObjectToWorldMatrix(), false, std::nullopt);
                }
            }
        }

        if (const auto s_CameraEntity = s_SelectedEntity.QueryInterface<ZCameraEntity>()) {
            if (ImGui::Button(ICON_MD_CAMERA "Toggle Camera")) {
                ZEntityRef s_EntRef;
                auto s_Camera = s_CameraEntity->GetID(s_EntRef);

                if (m_CameraActive) {
                    m_CameraActive = false;
                    Editor::DeactivateCamera();
                }
                else {
                    m_CameraActive = true;
                    Editor::ActivateCamera(s_Camera);
                }
            }
        }

        ImGui::Separator();

        static char s_InputPinInput[1024] = {};

        if (ImGui::Button(ICON_MD_BOLT "##fireInputPin")) {
            OnSignalEntityPin(s_SelectedEntity, s_InputPinInput, false);
            s_InputPinInput[0] = '\0';
        }

        ImGui::SameLine(0, 5);

        if (ImGui::InputText(
            "In", s_InputPinInput, IM_ARRAYSIZE(s_InputPinInput), ImGuiInputTextFlags_EnterReturnsTrue
        )) {
            OnSignalEntityPin(s_SelectedEntity, s_InputPinInput, false);
            s_InputPinInput[0] = '\0';
        }


        static char s_OutputPinInput[1024] = {};

        if (ImGui::Button(ICON_MD_BOLT "##fireOutputPin")) {
            OnSignalEntityPin(s_SelectedEntity, s_InputPinInput, true);
            s_OutputPinInput[0] = '\0';
        }

        ImGui::SameLine(0, 5);

        if (ImGui::InputText(
            "Out", s_OutputPinInput, IM_ARRAYSIZE(s_OutputPinInput), ImGuiInputTextFlags_EnterReturnsTrue
        )) {
            OnSignalEntityPin(s_SelectedEntity, s_InputPinInput, true);
            s_OutputPinInput[0] = '\0';
        }

        ImGui::Separator();

        ZRenderMaterialInstance* s_RenderMaterialInstance = nullptr;

        if (s_ReferencedTemplateFactoryType == "MATT") {
            TResourcePtr<ZRenderMaterialEntityFactory> s_MaterialEntityResourcePtr;

            Globals::ResourceManager->GetResourcePtr(s_MaterialEntityResourcePtr, s_ReferencedTemplateFactory, 0);

            ZRenderMaterialEntityFactory* s_RenderMaterialEntityFactory = s_MaterialEntityResourcePtr.GetResource();

            TResourcePtr<ZRenderMaterialInstance> s_RenderMaterialInstanceResourcePtr;

            Globals::ResourceManager->GetResourcePtr(
                s_RenderMaterialInstanceResourcePtr,
                s_RenderMaterialEntityFactory->m_ridMaterialInstance,
                0
            );

            s_RenderMaterialInstance = s_RenderMaterialInstanceResourcePtr.GetResource();
        }

        const auto s_EntityType = s_SelectedEntity->GetType();

        if (s_EntityType && s_EntityType->m_pPropertyData) {
            for (uint32_t i = 0; i < s_EntityType->m_pPropertyData->size(); ++i) {
                SPropertyData* s_Property = &(*s_EntityType->m_pPropertyData)[i];
                const auto* s_PropertyInfo = s_Property->GetPropertyInfo();

                if (!s_PropertyInfo || !s_PropertyInfo->m_propertyInfo.m_Type)
                    continue;

                const auto s_PropertyAddress = reinterpret_cast<uintptr_t>(s_SelectedEntity.m_pObj) + s_Property->
                    m_nPropertyOffset;
                const uint16_t s_TypeSize = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_nTypeSize;
                const uint16_t s_TypeAlignment = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->m_nTypeAlignment;

                // Get the value of the property.
                auto* s_Data = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
                    s_TypeSize, s_TypeAlignment
                );

                if (s_PropertyInfo->m_propertyInfo.m_Flags & EPropertyInfoFlags::E_HAS_GETTER_SETTER) {
                    s_PropertyInfo->m_propertyInfo.m_PropetyGetter(
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

                const std::string s_TypeName = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->pszTypeName;
                const std::string s_InputId = std::format("##Property{}", i);

                // Render the name of the property.
                ImGui::PushFont(SDK()->GetImGuiBoldFont());

                ImGui::AlignTextToFramePadding();

                std::string s_PropertyName;

                if (s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->IsResource() ||
                    s_PropertyInfo->m_nPropertyID != s_Property->m_nPropertyID
                    ) {
                    // Some properties don't have a name for some reason. Try to find using RL.
                    const auto s_PropertyNameData = HM3_GetPropertyName(s_Property->m_nPropertyID);

                    if (s_PropertyNameData.Size > 0) {
                        s_PropertyName.assign(s_PropertyNameData.Data, s_PropertyNameData.Size);
                    }
                    else {
                        s_PropertyName = fmt::format("~{:08x}", s_Property->m_nPropertyID);
                    }
                }
                else {
                    s_PropertyName = s_PropertyInfo->m_pszPropertyName;
                }

                if (!s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->IsArray() &&
                    !s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->IsFixedArray() &&
                    s_TypeName != "ZCurve") {
                    ImGui::Text("%s", s_PropertyName.c_str());
                }

                if (ImGui::IsItemHovered()) {
                    const char* s_TypeName = s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->pszTypeName;

                    const char* s_ShaderParameterName = nullptr;

                    if (s_RenderMaterialInstance && s_RenderMaterialInstance->m_pEffect) {
                        for (const auto& s_SemanticStringPair : s_RenderMaterialInstance->m_pEffect->m_SemanticStringPairs) {
                            if (s_PropertyName == s_SemanticStringPair.m_MaterialPropertyName) {
                                s_ShaderParameterName = s_SemanticStringPair.m_ShaderParameterName.c_str();
                                break;
                            }
                        }
                    }

                    if (s_ShaderParameterName) {
                        ImGui::SetTooltip("%s\n%s", s_TypeName, s_ShaderParameterName);
                    }
                    else {
                        ImGui::SetTooltip("%s", s_TypeName);
                    }
                }

                ImGui::PopFont();

                if (!s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->IsArray() &&
                    !s_PropertyInfo->m_propertyInfo.m_Type->GetTypeInfo()->IsFixedArray() &&
                    s_TypeName != "ZCurve") {
                    ImGui::SameLine();

                    // Make the next item fill the rest of the width.
                    ImGui::PushItemWidth(-1);
                }

                // Render the value of the property.
                bool s_IsChanged = DrawEntityPropertyValue(
                    s_InputId,
                    s_PropertyName,
                    s_TypeName,
                    s_PropertyInfo->m_propertyInfo.m_Type,
                    s_SelectedEntity,
                    s_Property,
                    s_Data
                );

                if (s_IsChanged) {
                    ZObjectRef s_ObjectRef;

                    s_ObjectRef.Assign(s_PropertyInfo->m_propertyInfo.m_Type, s_Data);

                    OnSetPropertyValue(s_SelectedEntity, s_Property->m_nPropertyID, s_ObjectRef, std::nullopt);
                }

                ImGui::Separator();

                // Free the property data.
                (*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);
            }
        }
    }

    ImGui::End();
}

bool Editor::DrawEntityPropertyValue(
    const std::string& p_Id,
    const std::string& p_PropertyName,
    const std::string& p_TypeName,
    const STypeID* p_TypeID,
    ZEntityRef p_Entity,
    SPropertyData* p_Property,
    void* p_Data
) {
    bool s_IsChanged = false;

    if (p_TypeName == "ZString") {
        s_IsChanged = StringProperty(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "bool") {
        s_IsChanged = BoolProperty(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "uint8") {
        s_IsChanged = Uint8Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "int8") {
        s_IsChanged = Int8Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "uint16") {
        s_IsChanged = Uint16Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "int16") {
        s_IsChanged = Int16Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "uint32") {
        s_IsChanged = Uint32Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "int32") {
        s_IsChanged = Int32Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "uint64") {
        s_IsChanged = Uint64Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "int64") {
        s_IsChanged = Int64Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "float32") {
        s_IsChanged = Float32Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "float64") {
        s_IsChanged = Float64Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "SVector2") {
        s_IsChanged = SVector2Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "SVector3") {
        s_IsChanged = SVector3Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "SVector4") {
        s_IsChanged = SVector4Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "SMatrix43") {
        s_IsChanged = SMatrix43Property(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "SColorRGB") {
        s_IsChanged = SColorRGBProperty(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "SColorRGBA") {
        s_IsChanged = SColorRGBAProperty(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeID->GetTypeInfo()->IsEnum()) {
        s_IsChanged = EnumProperty(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeID->GetTypeInfo()->IsResource()) {
        ResourcePtrProperty(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "ZRuntimeResourceID") {
        ZRuntimeResourceIDProperty(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName.starts_with("ZEntityRef")) {
        ZEntityRefProperty(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName.starts_with("TEntityRef")) {
        TEntityRefProperty(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "ZRepositoryID") {
        ZRepositoryIDProperty(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeName == "ZGuid") {
        ZGuidProperty(p_Id, p_Entity, p_Property, p_Data);
    }
    else if (p_TypeID->GetTypeInfo()->IsArray() || p_TypeID->GetTypeInfo()->IsFixedArray()) {
        s_IsChanged = ArrayProperty(p_Id, p_Entity, p_Property, p_Data, p_PropertyName, p_TypeID);
    }
    else if (p_TypeName == "ZCurve") {
        s_IsChanged = ZCurveProperty(p_Id, p_Entity, p_Property, p_Data, p_PropertyName, p_TypeID);
    }
    else if (p_TypeName == "ZGameTime") {
        s_IsChanged = ZGameTimeProperty(p_Id, p_Entity, p_Property, p_Data);
    }
    else {
        UnsupportedProperty(p_Id, p_Entity, p_Property, p_Data);
    }

    return s_IsChanged;
}
