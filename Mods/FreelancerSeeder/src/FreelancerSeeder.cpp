#include "FreelancerSeeder.h"

#include <Logging.h>
#include <IconsMaterialDesign.h>

#include <cerrno>
#include <limits>
#include <random>
#include <regex>

#include "Functions.h"
#include <Glacier/ZEvergreenCampaignManager.h>

void FreelancerSeeder::Init() {
    Hooks::ZEvergreenCampaignManager_OnGenerate->AddDetour(this, &FreelancerSeeder::ZEvergreenCampaignManager_OnGenerate);
}

int FreelancerSeeder::GenerateRandomSeed() {
    static std::mt19937 s_Rng(std::random_device{}());
    std::uniform_int_distribution<int> s_Dist(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    return s_Dist(s_Rng);
}

DEFINE_PLUGIN_DETOUR(FreelancerSeeder,  ZEvergreenCampaignManager*, ZEvergreenCampaignManager_OnGenerate, ZEvergreenCampaignManager* th) {
    Logger::Info("A new Freelancer campaign is being generated with seed: {}.", th->m_rSeed.m_entityRef.GetProperty<int>("m_nValue").Get());

    if (m_EnableCustomSeed) {
        th->m_rSeed.m_entityRef.SignalInputPin("SetValue", ZObjectRef::From(m_Seed));
        Logger::Info("Freelancer seed overridden to: {}", th->m_rSeed.m_entityRef.GetProperty<int>("m_nValue").Get());
    }
    else {
        m_Seed = th->m_rSeed.m_entityRef.GetProperty<int>("m_nValue").Get();
        snprintf(m_SeedInput, sizeof(m_SeedInput), "%d", m_Seed);
    }

    return { HookAction::Continue() };
}


void FreelancerSeeder::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_ECO " FL SEED"))
        m_ShowSettings = !m_ShowSettings;
}

void FreelancerSeeder::OnDrawUI(bool p_HasFocus) {
    if (p_HasFocus && m_ShowSettings) {
        const ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
        ImGui::SetNextWindowSize(ImVec2(viewportSize.x * 0.15f, viewportSize.y * 0.11f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(ICON_MD_ECO " Freelance Seeder", &m_ShowSettings)) {
            ImGui::Checkbox("Enable custom seed", &m_EnableCustomSeed);

            ImGuiInputTextFlags s_SeedFlags = ImGuiInputTextFlags_CharsDecimal;
            if (!m_EnableCustomSeed)
                s_SeedFlags |= ImGuiInputTextFlags_ReadOnly;

            // Live-update the seed while typing, but only when the field holds a
            // valid integer. Invalid intermediate input (empty, "-", overflow) is
            // ignored so m_Seed keeps its last good value.
            if (ImGui::InputText("##FlSeed", m_SeedInput, IM_ARRAYSIZE(m_SeedInput), s_SeedFlags)) {
                char* s_End = nullptr;
                errno = 0;

                if (const long s_Parsed = std::strtol(m_SeedInput, &s_End, 10);
                    s_End != m_SeedInput && *s_End == '\0' && errno != ERANGE &&
                    s_Parsed >= std::numeric_limits<int>::min() &&
                    s_Parsed <= std::numeric_limits<int>::max()) {
                    m_Seed = static_cast<int>(s_Parsed);
                }
            }

            // On commit, re-sync the field to the accepted seed to strip invalid text.
            if (ImGui::IsItemDeactivatedAfterEdit())
                snprintf(m_SeedInput, sizeof(m_SeedInput), "%d", m_Seed);

            ImGui::SameLine();
            ImGui::BeginDisabled(!m_EnableCustomSeed);
            if (ImGui::Button(ICON_MD_REFRESH)) {
                m_Seed = GenerateRandomSeed();
                snprintf(m_SeedInput, sizeof(m_SeedInput), "%d", m_Seed);
            }
            ImGui::EndDisabled();

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Generate a random seed.");
        }

        ImGui::End();
    }
}

DECLARE_ZHM_PLUGIN(FreelancerSeeder);
