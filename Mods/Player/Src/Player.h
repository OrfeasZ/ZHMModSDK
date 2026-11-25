#pragma once

#include "IPluginInterface.h"

#include <Glacier/ZEntity.h>
#include <Glacier/ZInput.h>

#include <Util/ActorTeleporter.h>

#include <string>
#include <vector>

#include <atomic>

struct SGameUpdateEvent;

class ZGlobalOutfitKit;
class ZHitman5;
class ZHM5CrippleBox;
class ZGameStatsManager;
class ZOutfitVariationCollection;

class Player : public IPluginInterface {
public:
    ~Player() override;

    void Init() override;
    void OnEngineInitialized() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    static void EquipOutfit(
        const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit, uint8_t n_CharSetIndex,
        const std::string& s_CharSetCharacterType, uint8_t n_OutfitVariationIndex, ZHitman5* p_Hitman
    );

    static bool HasOutfit(
        ZOutfitVariationCollection* p_Collection, const std::string& p_CharSetCharacterType, uint8_t p_VariationIndex
    );

    // Invisibility: m_bIsHiding alone left 47 "partially visible". The AI is also told through
    // the cutscene signal of a ZAICrippleEntity (SetHeroHidden / SetHeroVisible), sent from the
    // game update.
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    bool CreateAICrippleEntity();

    // Options remembered between missions, ZHitman5 layout check on mission start, and
    // "Teleport all actors" done one actor per frame.
    void LoadOptions();
    void SaveOptions();
    void ApplyHitmanState(ZHitman5* p_Hitman, bool p_OnLoad);
    bool CheckLayout(const TEntityRef<ZHitman5>& p_Hitman);
    void TeleportAimedActor();
    void BuildTeleportAllQueue();

    void ToggleInfiniteAmmo();
    bool CreateHM5CrippleBoxEntity();

    DECLARE_PLUGIN_DETOUR(Player, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene);

    DECLARE_PLUGIN_DETOUR(
        Player,
        void,
        ZSecuritySystemCameraManager_OnFrameUpdate,
        ZSecuritySystemCameraManager* th,
        const SGameUpdateEvent& updateEvent
    );
    DECLARE_PLUGIN_DETOUR(
        Player,
        void,
        ZSecuritySystemCamera_FrameUpdate,
        ZSecuritySystemCamera* th,
        const SGameUpdateEvent& updateEvent
    );

    DECLARE_PLUGIN_DETOUR(Player, void, ZHM5ItemWeapon_SetBulletsInMagazine, IFirearm* th, int32_t nBullets);

    DECLARE_PLUGIN_DETOUR(
        Player,
        void,
        ZHitmanMorphemePostProcessor_UpdateWeaponRecoil,
        ZHitmanMorphemePostProcessor* th,
        float fDeltaTime,
        const THashMap<int32_t, int32_t, TDefaultHashMapPolicy<int32_t>>& charboneMap,
        TArrayRef<int32_t> hierarchy
    );

    DECLARE_PLUGIN_DETOUR(
        Player,
        void,
        ZHM5WeaponRecoilController_RecoilWeapon,
        ZHM5WeaponRecoilController* th,
        const TEntityRef<ZHM5ItemWeapon>& rWeapon
    );

    DECLARE_PLUGIN_DETOUR(Player, bool, ZHM5ItemWeapon_FireProjectiles, ZHM5ItemWeapon* th, bool bMayStartSound);

    DECLARE_PLUGIN_DETOUR(Player, bool, ZHM5ItemWeapon_IsFiring, IFirearm* th);

    DECLARE_PLUGIN_DETOUR(Player, bool, ZActor_YouGotHit, IBaseCharacter* th, const SHitInfo& hitInfo);

    DECLARE_PLUGIN_DETOUR(Player, void, ZGameStatsManager_SendAISignals, ZGameStatsManager* th);

    bool m_ShowPlayerWindow = false;
    bool m_IsInfiniteAmmoEnabled = false;
    bool m_IsNoReloadEnabled = false;
    bool m_IsNoRecoilEnabled = false;
    bool m_IsSuperAccuracyEnabled = false;
    bool m_IsRapidFireEnabled = false;
    bool m_IsOneHitKillEnabled = false;

    const std::vector<std::string> m_CharSetCharacterTypes = { "Actor", "Nude", "HeroA" };

    TEntityRef<ZGlobalOutfitKit> m_GlobalOutfitKit = {};

    ZEntityRef m_HM5CrippleBoxEntity;

    ZEntityRef m_AICrippleEntity;
    // -1: nothing to do, 0: make 47 visible, 1: hide him.
    std::atomic<int> m_PendingHeroHidden = -1;
    bool m_IsInvisible = false;
    bool m_IsInvincible = false;

    bool m_RememberOptions = true;
    bool m_LayoutOk = true;
    std::string m_LayoutError;

    ZHitman5* m_LastHitman = nullptr;
    int m_ApplyDelay = 0;
    int m_EnforceTimer = 0;
    std::atomic<bool> m_StateDirty = false;
    std::atomic<bool> m_TeleportAllRequested = false;

    // Freeze all NPCs (ZActorManager::m_bDisableAIBehavior).
    // -1 nothing to do, 0 unfreeze, 1 freeze, 2 toggle; applied in OnFrameUpdate.
    std::atomic<int> m_FreezeNpcsRequest = -1;
    static bool AreNpcsFrozen();
    static void SetNpcsFrozen(bool p_Frozen);

    struct PendingTeleport {
        ZEntityRef Entity;
        void* Actor;
        SMatrix Target;
    };

    std::vector<PendingTeleport> m_TeleportAllQueue;
    ActorTeleporter m_ActorTeleporter;
};

DECLARE_ZHM_PLUGIN(Player)
