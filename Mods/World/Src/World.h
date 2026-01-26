#pragma once

#include "IPluginInterface.h"

class World : public IPluginInterface {
public:
    ~World() override;

    void OnEngineInitialized() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

    bool m_WorldMenuActive = false;
    bool m_IsTimeMultiplierEnabled = false;

    float m_GameTimeMultiplier = 1.f;
};

DECLARE_ZHM_PLUGIN(World)
