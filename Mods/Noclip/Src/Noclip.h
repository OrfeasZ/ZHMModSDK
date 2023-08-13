#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZInput.h"

class Noclip : public IPluginInterface
{
public:
    Noclip();
    ~Noclip() override;

    void OnEngineInitialized() override;
    void Init() override;
    void OnDrawMenu() override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    DECLARE_PLUGIN_DETOUR(Noclip, void, OnClearScene, ZEntitySceneContext* th, bool forReload);

private:
    bool m_NoclipEnabled = false;
    SMatrix m_PlayerPosition = {};
};

DECLARE_ZHM_PLUGIN(Noclip)
