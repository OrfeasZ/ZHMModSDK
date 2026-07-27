#pragma once

#include <IPluginInterface.h>

class FreelancerSeeder : public IPluginInterface {
public:
    void Init() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    // Hooks
    DECLARE_PLUGIN_DETOUR(FreelancerSeeder, ZEvergreenCampaignManager*, ZEvergreenCampaignManager_OnGenerate, ZEvergreenCampaignManager* th);

    int GenerateRandomSeed();

private:
    int m_Seed = 0;
    char m_SeedInput[12] = "0";
    bool m_EnableCustomSeed = false;
    bool m_ShowSettings = false;
};

DEFINE_ZHM_PLUGIN(FreelancerSeeder)
