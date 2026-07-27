#pragma once

#include "IPluginInterface.h"

#include <Glacier/ZEntity.h>

class ZGlobalOutfitKit;
class ZHitman5;
class ZHM5CrippleBox;

class Player : public IPluginInterface {
public:
    void Init() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    static void EquipOutfit(
        const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit, uint8_t n_CharSetIndex,
        const std::string& s_CharSetCharacterType, uint8_t n_OutfitVariationIndex, ZHitman5* p_Hitman
    );

    void ToggleInvincibility();
    void ToggleInvisibility();
    void ToggleInfiniteAmmo();
    bool CreateAICrippleEntity();
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

    bool m_PlayerMenuActive = false;
    bool m_IsInvincible = false;
    bool m_IsInvisible = false;
    bool m_IsInfiniteAmmoEnabled = false;
    bool m_IsNoReloadEnabled = false;
    bool m_IsNoRecoilEnabled = false;
    bool m_IsSuperAccuracyEnabled = false;
    bool m_IsRapidFireEnabled = false;
    bool m_IsOneHitKillEnabled = false;

    const std::vector<std::string> m_CharSetCharacterTypes = { "Actor", "Nude", "HeroA" };

    TEntityRef<ZGlobalOutfitKit> m_GlobalOutfitKit = {};

    ZEntityRef m_AICrippleEntity;
    ZEntityRef m_HM5CrippleBoxEntity;
};

DECLARE_ZHM_PLUGIN(Player)
