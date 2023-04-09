#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

class WakingUpNpcs : public IPluginInterface
{
public:
    WakingUpNpcs();
    ~WakingUpNpcs() override;

    void Init() override;
    void OnEngineInitialized() override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

private:
    DECLARE_PLUGIN_DETOUR(WakingUpNpcs, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);
    DECLARE_PLUGIN_DETOUR(WakingUpNpcs, void, OnClearScene, ZEntitySceneContext*, bool);

private:
    std::unordered_map<ZActor*, double> m_PacifiedTimes;
    std::random_device m_RandomDevice;
    std::mt19937 m_Generator;
};

DECLARE_ZHM_PLUGIN(WakingUpNpcs)
