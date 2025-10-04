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
        const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit, uint8_t n_CurrentCharSetIndex,
        const std::string& s_CurrentCharSetCharacterType, uint8_t n_CurrentOutfitVariationIndex, ZHitman5* p_LocalHitman
    );

    static void EnableInfiniteAmmo();

    DECLARE_PLUGIN_DETOUR(Player, void, OnClearScene, ZEntitySceneContext* th, bool forReload);

    bool m_PlayerMenuActive = false;

    const std::vector<std::string> m_CharSetCharacterTypes = { "Actor", "Nude", "HeroA" };

    ZHM5CrippleBox* m_Hm5CrippleBox = nullptr;

    TEntityRef<ZGlobalOutfitKit>* m_GlobalOutfitKit = nullptr;
};

DECLARE_ZHM_PLUGIN(Player)
