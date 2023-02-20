#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZInput.h"

class Clumsy : public IPluginInterface
{
public:
    Clumsy();
    ~Clumsy() override;

    void OnEngineInitialized() override;
    void Init() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    ZEntityRef GetGetUpEntity();
    DEFINE_PLUGIN_DETOUR(Clumsy, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear);

private:
    double m_SlipStartTimer = -1;
    double m_SlipStopTimer = -1;
    bool m_DeactivateRagdollQueued = false;
    bool m_ShowBrickWarning = false;
};

DEFINE_ZHM_PLUGIN(Clumsy)
