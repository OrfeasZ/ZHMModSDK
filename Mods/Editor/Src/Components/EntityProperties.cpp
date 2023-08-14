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

void Editor::DrawEntityProperties() {
	auto s_ImgGuiIO = ImGui::GetIO();

	ImGui::SetNextWindowPos({ s_ImgGuiIO.DisplaySize.x - 600, 110 }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({ 600, s_ImgGuiIO.DisplaySize.y - 110 }, ImGuiCond_FirstUseEver);
	ImGui::Begin(ICON_MD_TUNE " Entity Properties", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

	const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

	const auto s_SelectedEntity = m_SelectedEntity;

	if (s_SceneCtx && s_SceneCtx->m_pScene && s_SelectedEntity) {
		if (ImGui::Button(ICON_MD_SUPERVISOR_ACCOUNT)) {
			OnSelectEntity(s_SelectedEntity.GetLogicalParent(), std::nullopt);
		}

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Select Logical Parent");


		ImGui::SameLine(0, 5);

		if (ImGui::Button(ICON_MD_BRANDING_WATERMARK)) {
			OnSelectEntity(s_SelectedEntity.GetOwningEntity(), std::nullopt);
		}

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Select Owning Entity (Brick)");


		ImGui::SameLine(0, 5);

		if (ImGui::Button(ICON_MD_DESELECT))
			OnSelectEntity({}, std::nullopt);

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Deselect");

		ImGui::SameLine(0, 20);

		static bool s_LocalTransform = false;
		ImGui::Checkbox("Local Transforms", &s_LocalTransform);

		ImGui::Separator();

		// The way to get the factory here is probably wrong.
		auto s_Factory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_SelectedEntity.GetBlueprintFactory());

		if (s_SelectedEntity.GetOwningEntity())
			s_Factory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_SelectedEntity.GetOwningEntity().GetBlueprintFactory());

		if (s_Factory) {
			// This is also probably wrong.
			auto s_Index = s_Factory->GetSubEntityIndex(s_SelectedEntity->GetType()->m_nEntityId);

			if (s_Index != -1)
				ImGui::TextUnformatted(
					fmt::format(
						"Entity Name: {}",
						s_Factory->m_pTemplateEntityBlueprint->subEntities[s_Index].entityName
					).c_str());
		}

		ImGui::TextUnformatted(fmt::format("Entity ID: {:016x}", s_SelectedEntity->GetType()->m_nEntityId).c_str());

		if (ImGuiCopyWidget("EntId")) {
			CopyToClipboard(fmt::format("{:016x}", s_SelectedEntity->GetType()->m_nEntityId));
		}

		const auto& s_Interfaces = *s_SelectedEntity->GetType()->m_pInterfaces;
		ImGui::TextUnformatted(fmt::format("Entity Type: {}", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName).c_str());

		if (s_Factory) {
			ImGui::TextUnformatted(fmt::format("Contained TBLU: {:016X}", s_Factory->m_ridResource.GetID()).c_str());

			if (ImGuiCopyWidget("EntTblu")) {
				CopyToClipboard(fmt::format("{:016X}", s_Factory->m_ridResource.GetID()));
			}
		}

		if (const ZGeomEntity* s_GeomEntity = s_SelectedEntity.QueryInterface<ZGeomEntity>()) {
			if (s_GeomEntity->m_ResourceID.m_nResourceIndex != -1) {
				const auto s_PrimResourceInfo = (*Globals::ResourceContainer)->m_resources[s_GeomEntity->m_ResourceID.m_nResourceIndex];
				const auto s_PrimHash = s_PrimResourceInfo.rid.GetID();

				ImGui::TextUnformatted(fmt::format("Associated PRIM: {:016X}", s_PrimHash).c_str());

				const auto s_ContainedRpkg = (*Globals::ResourceContainer)->m_MountedPackages[s_PrimResourceInfo.packageId];

				if (ImGuiCopyWidget("EntPrim")) {
					CopyToClipboard(fmt::format("{:016X}", s_PrimHash));
				}
			}
		}

		if (const auto s_Spatial = s_SelectedEntity.QueryInterface<ZSpatialEntity>()) {
			ImGui::TextUnformatted("Entity Transform");

			auto s_Trans = s_Spatial->GetWorldMatrix();

			if (s_LocalTransform) {
				SMatrix s_ParentTrans;

				// Get parent entity.
				if (s_Spatial->m_eidParent.m_pInterfaceRef) {
					s_ParentTrans = s_Spatial->m_eidParent.m_pInterfaceRef->GetWorldMatrix();
				} else if (s_SelectedEntity.GetLogicalParent() && s_SelectedEntity.GetLogicalParent().QueryInterface<ZSpatialEntity>()) {
					s_ParentTrans = s_SelectedEntity.GetLogicalParent().QueryInterface<ZSpatialEntity>()->GetWorldMatrix();
				} else if (s_SelectedEntity.GetOwningEntity() && s_SelectedEntity.GetOwningEntity().QueryInterface<ZSpatialEntity>()) {
					s_ParentTrans = s_SelectedEntity.GetOwningEntity().QueryInterface<ZSpatialEntity>()->GetWorldMatrix();
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
						"{{\"XAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"YAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"ZAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"Trans\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
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
					));
			}

			ImGui::SameLine();

			if (ImGui::Button(ICON_MD_CONTENT_COPY " QN JSON##EntQN")) {
				// This is adapted from QN: https://github.com/atampy25/quickentity-rs/blob/b94dbab32c91ccd0e1612a2e5dda8fb83eb2a8e9/src/lib.rs#L243
				constexpr double c_RAD2DEG = 180.0 / std::numbers::pi;

				double s_RotationX = abs(s_Trans.XAxis.z) < 0.9999999f
				                     ? atan2f(-s_Trans.YAxis.z, s_Trans.ZAxis.z) * c_RAD2DEG
				                     : atan2f(s_Trans.ZAxis.y, s_Trans.YAxis.y) * c_RAD2DEG;

				double s_RotationY = asinf(min(max(-1.f, s_Trans.XAxis.z), 1.f)) * c_RAD2DEG;

				double s_RotationZ = abs(s_Trans.XAxis.z) < 0.9999999f
				                     ? atan2f(-s_Trans.XAxis.y, s_Trans.XAxis.x) * c_RAD2DEG
				                     : 0.f;

				CopyToClipboard(
					fmt::format(
						"{{\"rotation\":{{\"x\":{},\"y\":{},\"z\":{}}},\"position\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
						s_RotationX, s_RotationY, s_RotationZ,
						s_Trans.Trans.x, s_Trans.Trans.y, s_Trans.Trans.z
					));
			}

			if (ImGui::Button(ICON_MD_PERSON_PIN " Move to Hitman")) {
				TEntityRef<ZHitman5> s_LocalHitman;
				Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

				auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

				OnEntityTransformChange(s_SelectedEntity, s_HitmanSpatial->GetWorldMatrix(), false, std::nullopt);
			}

			ImGui::SameLine();

			if (ImGui::Button(ICON_MD_PERSON_PIN_CIRCLE " Move Hitman to")) {
				TEntityRef<ZHitman5> s_LocalHitman;
				Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

				auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

				s_HitmanSpatial->SetWorldMatrix(s_Spatial->GetWorldMatrix());

				OnEntityTransformChange(s_LocalHitman.m_ref, s_Spatial->GetWorldMatrix(), false, std::nullopt);
			}
		}

        if (const auto s_CameraEntity = s_SelectedEntity.QueryInterface<ZCameraEntity>())
        {
            if (ImGui::Button(ICON_MD_CAMERA "Toggle Camera"))
            {
                ZEntityRef s_EntRef;
                auto s_Camera = s_CameraEntity->GetID(&s_EntRef);

                if (m_CameraActive)
                {
                    m_CameraActive = false;
                    Editor::DeactivateCamera();
                }
                else
                {
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

		if (ImGui::InputText("In", s_InputPinInput, IM_ARRAYSIZE(s_InputPinInput), ImGuiInputTextFlags_EnterReturnsTrue)) {
			OnSignalEntityPin(s_SelectedEntity, s_InputPinInput, false);
			s_InputPinInput[0] = '\0';
		}


		static char s_OutputPinInput[1024] = {};

		if (ImGui::Button(ICON_MD_BOLT "##fireOutputPin")) {
			OnSignalEntityPin(s_SelectedEntity, s_InputPinInput, true);
			s_OutputPinInput[0] = '\0';
		}

		ImGui::SameLine(0, 5);

		if (ImGui::InputText("Out", s_OutputPinInput, IM_ARRAYSIZE(s_OutputPinInput), ImGuiInputTextFlags_EnterReturnsTrue)) {
			OnSignalEntityPin(s_SelectedEntity, s_InputPinInput, true);
			s_OutputPinInput[0] = '\0';
		}

		ImGui::Separator();

		const auto s_EntityType = s_SelectedEntity->GetType();

		if (s_EntityType && s_EntityType->m_pProperties01) {
			for (uint32_t i = 0; i < s_EntityType->m_pProperties01->size(); ++i) {
				ZEntityProperty* s_Property = &s_EntityType->m_pProperties01->operator[](i);
				const auto* s_PropertyInfo = s_Property->m_pType->getPropertyInfo();

				if (!s_PropertyInfo || !s_PropertyInfo->m_pType)
					continue;

				const auto s_PropertyAddress = reinterpret_cast<uintptr_t>(s_SelectedEntity.m_pEntity) + s_Property->m_nOffset;
				const uint16_t s_TypeSize = s_PropertyInfo->m_pType->typeInfo()->m_nTypeSize;
				const uint16_t s_TypeAlignment = s_PropertyInfo->m_pType->typeInfo()->m_nTypeAlignment;

				// Get the value of the property.
				auto* s_Data = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(s_TypeSize, s_TypeAlignment);

				if (s_PropertyInfo->m_nFlags & EPropertyInfoFlags::E_HAS_GETTER_SETTER) {
					s_PropertyInfo->get(
						reinterpret_cast<void*>(s_PropertyAddress),
						s_Data,
						s_PropertyInfo->m_nOffset
					);
				} else {
					s_PropertyInfo->m_pType->typeInfo()->m_pTypeFunctions->copyConstruct(
						s_Data,
						reinterpret_cast<void*>(s_PropertyAddress)
					);
				}

				const std::string s_TypeName = s_PropertyInfo->m_pType->typeInfo()->m_pTypeName;
				const std::string s_InputId = std::format("##Property{}", i);

				// Render the name of the property.
				ImGui::PushFont(SDK()->GetImGuiBoldFont());

				if (s_PropertyInfo->m_pType->typeInfo()->isResource() || s_PropertyInfo->m_nPropertyID != s_Property->m_nPropertyId) {
					// Some properties don't have a name for some reason. Try to find using RL.
					const auto s_PropertyName = HM3_GetPropertyName(s_Property->m_nPropertyId);

					if (s_PropertyName.Size > 0) {
						ImGui::Text("%s", std::string(s_PropertyName.Data, s_PropertyName.Size).c_str());
					} else {
						ImGui::Text("~%08x", s_Property->m_nPropertyId);
					}
				} else {
					ImGui::Text("%s", s_PropertyInfo->m_pName);
				}

				ImGui::PopFont();
				ImGui::SameLine();

				// Make the next item fill the rest of the width.
				ImGui::PushItemWidth(-1);

				// Render the value of the property.
				if (s_TypeName == "ZString") {
					StringProperty(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "bool") {
					BoolProperty(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "uint8") {
					Uint8Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "int8") {
					Int8Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "uint16") {
					Uint16Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "int16") {
					Int16Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "uint32") {
					Uint32Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "int32") {
					Int32Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "uint64") {
					Uint64Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "int64") {
					Int64Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "float32") {
					Float32Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "float64") {
					Float64Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "SVector2") {
					SVector2Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "SVector3") {
					SVector3Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "SVector4") {
					SVector4Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_TypeName == "SMatrix43") {
					SMatrix43Property(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_PropertyInfo->m_pType->typeInfo()->isEnum()) {
					EnumProperty(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else if (s_PropertyInfo->m_pType->typeInfo()->isResource()) {
					ResourceProperty(s_InputId, s_SelectedEntity, s_Property, s_Data);
				} else {
					UnsupportedProperty(s_InputId, s_SelectedEntity, s_Property, s_Data);
				}

				ImGui::Separator();

				// Free the property data.
				(*Globals::MemoryManager)->m_pNormalAllocator->Free(s_Data);
			}
		}
	}

	ImGui::End();
}
