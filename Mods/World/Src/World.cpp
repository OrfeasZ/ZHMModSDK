#include "World.h"

#include "IconsMaterialDesign.h"

#include "Glacier/ZGameLoopManager.h"
#include "Glacier/ZGameTime.h"

World::~World() {
    const ZMemberDelegate<World, void(const SGameUpdateEvent&)> s_Delegate(this, &World::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void World::OnEngineInitialized() {
    const ZMemberDelegate<World, void(const SGameUpdateEvent&)> s_Delegate(this, &World::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void World::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_MAN " WORLD")) {
        m_WorldMenuActive = !m_WorldMenuActive;
    }
}

void World::OnDrawUI(const bool p_HasFocus) {
    if (!p_HasFocus || !m_WorldMenuActive) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("WORLD", &m_WorldMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Game Time Multiplier");
        ImGui::SameLine();

        ImGui::Checkbox("##EnableGameTimeMultiplier", &m_IsTimeMultiplierEnabled);
        ImGui::SameLine();

        ImGui::BeginDisabled(!m_IsTimeMultiplierEnabled);

        ImGui::SetNextItemWidth(ImGui::GetFrameHeight() * 5.f);

        constexpr float s_Min = 0.001f;
        constexpr float s_Max = 10.f;

        if (ImGui::DragScalar("##GameTimeMultiplier", ImGuiDataType_Float, &m_GameTimeMultiplier, 0.1f, &s_Min, &s_Max)) {
            if (m_IsTimeMultiplierEnabled) {
                Globals::GameTimeManager->m_fGameTimeMultiplier = m_GameTimeMultiplier;
            }
        }

        ImGui::EndDisabled();
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

void World::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    if (m_IsTimeMultiplierEnabled && Globals::GameTimeManager->m_fGameTimeMultiplier != m_GameTimeMultiplier) {
        Globals::GameTimeManager->m_fGameTimeMultiplier = m_GameTimeMultiplier;
    }
}

DEFINE_ZHM_PLUGIN(World);