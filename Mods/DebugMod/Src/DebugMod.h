#pragma once

#include <shared_mutex>
#include <random>
#include <unordered_map>
#include <map>
#include <set>

#include "IPluginInterface.h"

#include <Glacier/ZInput.h>
#include <Glacier/ZEntity.h>
#include <Glacier/ZResource.h>

#include "Glacier/ZScene.h"

class DebugMod : public IPluginInterface {
public:
    ~DebugMod() override;

    void Init() override;
    void OnEngineInitialized() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;
    void OnDraw3D(IRenderer* p_Renderer) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    static void CopyToClipboard(const std::string& p_String);

private:
    void DrawOptions(bool p_HasFocus);
    void DrawPositionBox(bool p_HasFocus);

    static std::string BehaviorToString(ECompiledBehaviorType p_Type);

    DECLARE_PLUGIN_DETOUR(DebugMod, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);
    DECLARE_PLUGIN_DETOUR(DebugMod, void, OnClearScene, ZEntitySceneContext* th, bool forReload);

private:
    bool m_DebugMenuActive = false;
    bool m_PositionsMenuActive = false;
    bool m_RenderNpcBoxes = false;
    bool m_RenderNpcNames = false;
    bool m_RenderNpcRepoIds = false;
    bool m_RenderNpcBehaviors = false;
};

DECLARE_ZHM_PLUGIN(DebugMod)
