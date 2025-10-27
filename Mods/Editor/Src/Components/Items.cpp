#include "Editor.h"

#include <format>

#include <Glacier/ZAction.h>
#include <Glacier/ZItem.h>

void Editor::DrawItems(bool p_HasFocus) {
    if (!p_HasFocus || !m_ItemsMenuActive) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("ITEMS", &m_ItemsMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing) {
        const ZHM5ActionManager* s_Hm5ActionManager = Globals::HM5ActionManager;
        std::vector<ZHM5Action*> s_Actions;

        if (s_Hm5ActionManager->m_Actions.size() == 0) {
            ImGui::PopFont();
            ImGui::End();
            ImGui::PopFont();

            return;
        }

        for (unsigned int i = 0; i < s_Hm5ActionManager->m_Actions.size(); ++i) {
            ZHM5Action* s_Action = s_Hm5ActionManager->m_Actions[i];

            if (s_Action && s_Action->m_eActionType == EActionType::AT_PICKUP) {
                s_Actions.push_back(s_Action);
            }
        }

        static size_t s_Selected = 0;

        ImGui::BeginChild("left pane", ImVec2(300, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        for (size_t i = 0; i < s_Actions.size(); i++) {
            const ZHM5Action* s_Action = s_Actions[i];
            const ZHM5Item* s_Item = s_Action->m_Object.QueryInterface<ZHM5Item>();
            std::string s_Title = fmt::format(
                "{} ({:08x})###{}", s_Item->m_pItemConfigDescriptor->m_sTitle.c_str(),
                s_Action->m_Object->GetType()->m_nEntityId, i + 1
            );

            if (ImGui::Selectable(s_Title.c_str(), s_Selected == i)) {
                s_Selected = i;
            }
        }

        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

        const ZHM5Action* s_Action = s_Actions[s_Selected];
        const ZHM5Item* s_Item = s_Action->m_Object.QueryInterface<ZHM5Item>();

        if (s_Item) {
            if (ImGui::Button("Select In Entity Tree")) {
                if (!m_CachedEntityTree || !m_CachedEntityTree->Entity) {
                    UpdateEntities();
                }

                OnSelectEntity(s_Action->m_Object, std::nullopt);
            }
        }

        if (ImGui::Button("Teleport Item To Player")) {
            if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
                ZSpatialEntity* s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
                s_Item->m_rGeomentity.m_pInterfaceRef->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());
            }
        }

        if (ImGui::Button("Teleport Player To Item")) {
            if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
                ZSpatialEntity* s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
                s_HitmanSpatial->SetWorldMatrix(s_Item->m_rGeomentity.m_pInterfaceRef->GetWorldMatrix());
            }
        }

        ImGui::EndChild();
        ImGui::EndGroup();
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}
