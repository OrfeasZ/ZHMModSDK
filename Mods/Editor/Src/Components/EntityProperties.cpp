#include <Editor.h>
#include <numbers>

#include "Functions.h"
#include <Glacier/ZModule.h>
#include <Glacier/ZGeomEntity.h>
#include "IconsMaterialDesign.h"
#include "Glacier/ZPhysics.h"

void Editor::DrawEntityProperties()
{
    auto s_ImgGuiIO = ImGui::GetIO();

    ImGui::SetNextWindowPos({ s_ImgGuiIO.DisplaySize.x - 400, 110 });
    ImGui::SetNextWindowSize({ 400, 600 });
    ImGui::Begin(ICON_MD_TUNE " Entity Properties", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

    const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

    if (s_SceneCtx && s_SceneCtx->m_pScene && m_SelectedEntity)
    {
        // The way to get the factory here is probably wrong.
        auto s_Factory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(m_SelectedEntity.GetBlueprintFactory());

        if (m_SelectedEntity.GetOwningEntity())
            s_Factory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(m_SelectedEntity.GetOwningEntity().GetBlueprintFactory());

        if (s_Factory)
        {
            // This is also probably wrong.
            auto s_Index = s_Factory->GetSubEntityIndex(m_SelectedEntity->GetType()->m_nEntityId);

            if (s_Index != -1)
                ImGui::TextUnformatted(fmt::format("Entity Name: {}", s_Factory->m_pTemplateEntityBlueprint->subEntities[s_Index].entityName).c_str());
        }

        ImGui::TextUnformatted(fmt::format("Entity ID: {:016x}", m_SelectedEntity->GetType()->m_nEntityId).c_str());

        if (ImGuiCopyWidget("EntId"))
        {
            CopyToClipboard(fmt::format("{:016x}", m_SelectedEntity->GetType()->m_nEntityId));
        }

        const auto& s_Interfaces = *m_SelectedEntity->GetType()->m_pInterfaces;
        ImGui::TextUnformatted(fmt::format("Entity Type: {}", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName).c_str());

        if (s_Factory)
        {
            ImGui::TextUnformatted(fmt::format("Contained TBLU: {:016X}", s_Factory->m_ridResource.GetID()).c_str());

            if (ImGuiCopyWidget("EntTblu"))
            {
                CopyToClipboard(fmt::format("{:016X}", s_Factory->m_ridResource.GetID()));
            }
        }

        if (const ZGeomEntity* s_GeomEntity = m_SelectedEntity.QueryInterface<ZGeomEntity>())
        {
            if (s_GeomEntity->m_ResourceID.m_nResourceIndex != -1)
            {
                const auto s_PrimResourceInfo = (*Globals::ResourceContainer)->m_resources[s_GeomEntity->m_ResourceID.m_nResourceIndex];
                const auto s_PrimHash = s_PrimResourceInfo.rid.GetID();

                ImGui::TextUnformatted(fmt::format("Associated PRIM: {:016X}", s_PrimHash).c_str());

                if (ImGuiCopyWidget("EntPrim"))
                {
                    CopyToClipboard(fmt::format("{:016X}", s_PrimHash));
                }
            }
        }

        if (const auto s_Spatial = m_SelectedEntity.QueryInterface<ZSpatialEntity>())
        {
            const auto s_Trans = s_Spatial->GetWorldMatrix();

            ImGui::TextUnformatted("Entity Transform");

            if (ImGuiCopyWidget("EntTrans"))
            {
                CopyToClipboard(fmt::format("{}", s_Trans));
            }

            if (ImGui::BeginTable("DebugMod_HitmanPosition", 4))
            {
                for (int i = 0; i < 4; ++i)
                {
                    ImGui::TableNextRow();

                    for (int j = 0; j < 4; ++j)
                    {
                        ImGui::TableSetColumnIndex(j);
                        ImGui::Text("%f", s_Trans.flt[(i * 4) + j]);
                    }
                }

                ImGui::EndTable();
            }

            if (ImGui::Button(ICON_MD_CONTENT_COPY " RT JSON##EntRT"))
            {
                CopyToClipboard(fmt::format(
                    "{{\"XAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"YAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"ZAxis\":{{\"x\":{},\"y\":{},\"z\":{}}},\"Trans\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
                    s_Trans.XAxis.x, s_Trans.XAxis.y, s_Trans.XAxis.z,
                    s_Trans.YAxis.x, s_Trans.YAxis.y, s_Trans.YAxis.z,
                    s_Trans.ZAxis.x, s_Trans.ZAxis.y, s_Trans.ZAxis.z,
                    s_Trans.Trans.x, s_Trans.Trans.y, s_Trans.Trans.z
                ));
            }

            ImGui::SameLine();

            if (ImGui::Button(ICON_MD_CONTENT_COPY " QN JSON##EntQN"))
            {
                // This is adapted from QN: https://github.com/atampy25/quickentity-rs/blob/b94dbab32c91ccd0e1612a2e5dda8fb83eb2a8e9/src/lib.rs#L243
                constexpr double c_RAD2DEG = 180.0 / std::numbers::pi;

                double s_RotationX = abs(s_Trans.XAxis.z) < 0.9999999f
                    ? atan2f(-s_Trans.YAxis.z, s_Trans.ZAxis.z) * c_RAD2DEG
                    : atan2f(s_Trans.ZAxis.y, s_Trans.YAxis.y) * c_RAD2DEG;

                double s_RotationY = asinf(min(max(-1.f, s_Trans.XAxis.z), 1.f)) * c_RAD2DEG;

                double s_RotationZ = abs(s_Trans.XAxis.z) < 0.9999999f
                    ? atan2f(-s_Trans.XAxis.y, s_Trans.XAxis.x) * c_RAD2DEG
                    : 0.f;

                CopyToClipboard(fmt::format(
                    "{{\"rotation\":{{\"x\":{},\"y\":{},\"z\":{}}},\"position\":{{\"x\":{},\"y\":{},\"z\":{}}}}}",
                    s_RotationX, s_RotationY, s_RotationZ,
                    s_Trans.Trans.x, s_Trans.Trans.y, s_Trans.Trans.z
                ));
            }

            if (ImGui::Button(ICON_MD_PERSON_PIN " Move to Hitman"))
            {
                TEntityRef<ZHitman5> s_LocalHitman;
                Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

                auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

                s_Spatial->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());

                if (const auto s_PhysicsAspect = m_SelectedEntity.QueryInterface<ZStaticPhysicsAspect>())
                    s_PhysicsAspect->m_pPhysicsObject->SetTransform(s_Spatial->GetWorldMatrix());
            }

            if (ImGui::Button(ICON_MD_PERSON_PIN_CIRCLE " Move Hitman to"))
            {
                TEntityRef<ZHitman5> s_LocalHitman;
                Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

                auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

                s_HitmanSpatial->SetWorldMatrix(s_Spatial->GetWorldMatrix());
            }
        }

        static char s_InputPinInput[1024] = {};

        if (ImGui::Button(ICON_MD_BOLT "##fireInputPin"))
        {
            m_SelectedEntity.SignalInputPin(s_InputPinInput);
            s_InputPinInput[0] = '\0';
        }

        ImGui::SameLine(0, 5);

        if (ImGui::InputText("In", s_InputPinInput, IM_ARRAYSIZE(s_InputPinInput), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            m_SelectedEntity.SignalInputPin(s_InputPinInput);
            s_InputPinInput[0] = '\0';
        }


        static char s_OutputPinInput[1024] = {};

        if (ImGui::Button(ICON_MD_BOLT "##fireOutputPin"))
        {
            m_SelectedEntity.SignalOutputPin(s_OutputPinInput);
            s_OutputPinInput[0] = '\0';
        }

        ImGui::SameLine(0, 5);

        if (ImGui::InputText("Out", s_OutputPinInput, IM_ARRAYSIZE(s_OutputPinInput), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            m_SelectedEntity.SignalOutputPin(s_OutputPinInput);
            s_OutputPinInput[0] = '\0';
        }


        if (ImGui::Button(ICON_MD_SUPERVISOR_ACCOUNT))
            m_SelectedEntity = m_SelectedEntity.GetLogicalParent();

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Select Logical Parent");


        ImGui::SameLine(0, 5);

        if (ImGui::Button(ICON_MD_BRANDING_WATERMARK))
            m_SelectedEntity = m_SelectedEntity.GetOwningEntity();

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Select Owning Entity (Brick)");


        ImGui::SameLine(0, 5);

        if (ImGui::Button(ICON_MD_DESELECT))
            m_SelectedEntity = {};

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Deselect");
    }

    ImGui::End();
}
