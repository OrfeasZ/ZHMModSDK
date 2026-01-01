#pragma once

#include "IPluginInterface.h"

#include <unordered_set>
#include <shared_mutex>

class Randomizer : public IPluginInterface {
public:
    void Init() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void DrawGeneralTab();
    void DrawCategoriesTab();
    void DrawPropsToSpawnTab();
    void DrawPropsToExcludeTab();

    void LoadRepositoryProps();
    void FilterRepositoryProps();

    const ZRepositoryID& GetRandomRepositoryId(const std::vector<ZRepositoryID>& p_Props);

    void LoadCategoriesFromSettings();
    void LoadPropsToSpawnFromSettings();

    DECLARE_PLUGIN_DETOUR(Randomizer, bool, OnLoadScene, ZEntitySceneContext*, SSceneInitParameters& parameters);
    DECLARE_PLUGIN_DETOUR(Randomizer, void, ZLevelManager_StartGame, ZLevelManager* th);

    DECLARE_PLUGIN_DETOUR(Randomizer, void, ZItemSpawner_RequestContentLoad, ZItemSpawner* th);

    DECLARE_PLUGIN_DETOUR(
        Randomizer,
        ZCharacterSubcontrollerInventory::SCreateItem*,
        ZCharacterSubcontrollerInventory_CreateItem,
        ZCharacterSubcontrollerInventory* th,
        ZRepositoryID& repId,
        const ZString& sOnlineInstanceId,
        const TArray<ZRepositoryID>& instanceModifiersToApply,
        ZCharacterSubcontrollerInventory::ECreateItemType createItemType
    );

    DECLARE_PLUGIN_DETOUR(Randomizer, bool, ZActorInventoryHandler_RequestItem, ZActorInventoryHandler* th, ZRepositoryID& id);

    bool m_IsRandomizerEnabled = true;
    bool m_IsRandomizerAllowedForScene = true;

    bool m_RandomizerMenuActive = false;

    bool m_RandomizeWorldProps = true;
    bool m_RandomizeStashProps = true;
    bool m_RandomizePlayerInventory = true;
    bool m_RandomizeActorInventory = true;

    bool m_RandomizeItems = true;
    bool m_RandomizeWeapons = true;

    int32_t m_RepositoryPropSpawnCount = 1;

    TResourcePtr<ZTemplateEntityFactory> m_RepositoryResource;
    std::vector<std::tuple<ZRepositoryID, std::string, bool, std::string>> m_AllRepositoryProps;
    std::unordered_set<ZRepositoryID> m_RepositoryWeapons;

    std::map<std::string, bool> m_InventoryCategoryToState;
    std::vector<std::tuple<ZRepositoryID, std::string, bool, bool, bool, bool>> m_PropsToSpawn;
    std::vector<ZRepositoryID> m_PropsToSpawnInWorld;
    std::vector<ZRepositoryID> m_PropsToSpawnInStash;
    std::vector<ZRepositoryID> m_PropsToSpawnInPlayerInventory;
    std::vector<ZRepositoryID> m_PropsToSpawnInActorInventory;
    std::vector<ZRepositoryID> m_WeaponsToSpawnInActorInventory;
    std::vector<std::pair<ZRepositoryID, std::string>> m_PropsToExclude;
    std::unordered_set<ZRepositoryID> m_ExcludedPropRepositoryIds;

    bool m_SpawnInWorld = false;
    bool m_SpawnInStash = false;
    bool m_SpawnInPlayerInventory = false;
    bool m_SpawnInActorInventory = false;
};

DECLARE_ZHM_PLUGIN(Randomizer)
