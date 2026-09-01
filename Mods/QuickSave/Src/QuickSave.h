#pragma once

#include <unordered_map>

#include "IPluginInterface.h"

#include <Glacier/ZEntity.h>
#include <Glacier/ZInput.h>
#include <Glacier/ZCollision.h>

class QuickSave : public IPluginInterface {
public:
    QuickSave();
    ~QuickSave() override;

    void Init() override;
    void OnEngineInitialized() override;

private:
    ZEntityRef GetLevelControlEntity();
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

    DECLARE_PLUGIN_DETOUR(QuickSave, bool, OnLoadScene, ZEntitySceneContext*, SSceneInitParameters&);
    DECLARE_PLUGIN_DETOUR(QuickSave, void, OnClearScene, ZEntitySceneContext*, bool);

private:
    ZInputAction m_QuickSave;
    ZInputAction m_QuickLoad;
    ZEntityRef m_LevelControlEntity;
};

DECLARE_ZHM_PLUGIN(QuickSave)
