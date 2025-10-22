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

    void EnableInfiniteAmmo();

    DECLARE_PLUGIN_DETOUR(Player, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene);

    bool m_PlayerMenuActive = false;

    const std::vector<std::string> m_CharSetCharacterTypes = { "Actor", "Nude", "HeroA" };

    TEntityRef<ZGlobalOutfitKit>* m_GlobalOutfitKit = nullptr;

    ZEntityRef m_HM5CrippleBoxEntity;
};

DECLARE_ZHM_PLUGIN(Player)
