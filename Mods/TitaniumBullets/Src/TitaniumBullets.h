#pragma once

#include "IPluginInterface.h"

#include <Glacier/ZObject.h>
#include <Glacier/ZResource.h>
#include <Glacier/ZPrimitives.h>

#include <vector>

/**
 * TitaniumBullets - Wall Penetration Mod for HITMAN 3
 *
 * This ZHMModSDK plugin enables bullets to penetrate through walls,
 * replicating the functionality of the Simple Mod Framework's
 * TitaniumBullets mod without requiring SMF.
 *
 * Compatible with WeMod since it uses the same modding approach as
 * other ZHMModSDK plugins.
 *
 * How it works:
 * - Patches the in-memory repository (`[assembly:/repository/pro.repo].pc_repo`)
 * - For a set of ammo entries, overrides `AmmoConfig` to the "penetration" config
 * - Can be toggled on/off from the ZHMModSDK menu (restores originals when disabled)
 */
class TitaniumBullets : public IPluginInterface {
public:
    TitaniumBullets();
    ~TitaniumBullets();

    void Init() override;
    void OnEngineInitialized() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

    // Scene clear handler to reset state
    DECLARE_PLUGIN_DETOUR(TitaniumBullets, void, OnClearScene,
        ZEntitySceneContext* th, bool p_FullyUnloadScene);

    bool EnsureRepositoryLoaded();
    bool ApplyRepositoryPatch();
    void RestoreRepositoryPatch();

    bool m_Enabled = true;
    bool m_DebugWindowActive = false;
    bool m_PatchApplied = false;
    bool m_LogRepoNotReadyOnce = false;
    
    // Statistics for debugging
    uint32_t m_RepoEntriesPatched = 0;
    uint32_t m_RepoEntriesRestored = 0;

    // Repository resource handle (pro.repo)
    ZResourcePtr m_RepositoryResource = {};

    // Original AmmoConfig values for patched entries (for restoration)
    std::vector<std::pair<ZRepositoryID, ZDynamicObject>> m_OriginalAmmoConfigs;
};

DECLARE_ZHM_PLUGIN(TitaniumBullets)
