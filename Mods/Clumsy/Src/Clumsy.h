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

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    ZEntityRef GetGetUpEntity();
    DEFINE_PLUGIN_DETOUR(Clumsy, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear);

private:
    float m_SlipStartTimer = -1.f;
    float m_SlipStopTimer = -1.f;
    bool m_DeactivateRagdollQueued = false;
};

DEFINE_ZHM_PLUGIN(Clumsy)
