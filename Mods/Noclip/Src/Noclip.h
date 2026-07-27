#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZInput.h"

class Noclip : public IPluginInterface {
public:
    Noclip();
    ~Noclip() override;

    void OnEngineInitialized() override;
    void Init() override;
    void OnDrawMenu() override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

    void TogglePlayerMovement();
    bool CreateHM5CrippleBoxEntity();

    DECLARE_PLUGIN_DETOUR(Noclip, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene);

private:
    bool m_NoclipEnabled = false;
    SMatrix m_PlayerPosition = {};
    ZInputAction m_ToggleNoclipAction;
    ZInputAction m_ForwardAction;
    ZInputAction m_BackwardAction;
    ZInputAction m_LeftAction;
    ZInputAction m_RightAction;
    ZInputAction m_FastAction;

    ZEntityRef m_HM5CrippleBoxEntity;
};

DECLARE_ZHM_PLUGIN(Noclip)
