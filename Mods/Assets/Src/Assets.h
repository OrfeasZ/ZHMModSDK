#pragma once

#include "IPluginInterface.h"

#include "Glacier/ZResource.h"

class ZGlobalOutfitKit;

class Assets : public IPluginInterface
{
public:
    void Init() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    DECLARE_PLUGIN_DETOUR(Assets, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene);

    static void SpawnRepositoryProp(const ZRepositoryID& p_RepositoryId, const bool p_AddToWorld);
    static void SpawnNonRepositoryProp(const std::string& p_PropAssemblyPath);
    static void SpawnActor(
        const std::string& p_ActorName, const ZRepositoryID& p_RepositoryID,
        const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit, uint8_t p_CharacterSetIndex,
        const std::string& p_CharSetCharacterType, uint8_t p_OutfitVariationIndex
    );

    static void EquipOutfit(
        const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit, uint8_t p_CharSetIndex,
        const std::string& s_CharSetCharacterType, uint8_t n_OutfitVariationindex, ZActor* p_Actor
    );

    void LoadRepositoryProps();
    std::string ConvertDynamicObjectValueToString(const ZDynamicObject& p_DynamicObject);

    bool m_AssetsMenuActive = false;

    TResourcePtr<ZTemplateEntityFactory> m_RepositoryResource;
    std::vector<std::pair<ZRepositoryID, std::string>> m_RepositoryProps; // RepoId -> Title/Common Name

    const std::vector<std::string> m_CharSetCharacterTypes = { "Actor", "Nude", "HeroA" };
};

DECLARE_ZHM_PLUGIN(Assets)
