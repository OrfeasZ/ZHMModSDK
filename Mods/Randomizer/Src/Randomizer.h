#pragma once

#include "IPluginInterface.h"

#include <unordered_set>
#include <shared_mutex>

#include "Glacier/ZOutfit.h"

class Randomizer : public IPluginInterface {
public:
    Randomizer();

    void Init() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void DrawGeneralTab();
    void DrawCategoriesTab();
    void DrawPropsToSpawnTab();
    void DrawPropsToExcludeTab();
    void DrawOutfitsTab();
    void DrawOutfitsFromOtherScenesTab();
    void DrawOutfitsToSpawnTab();
    void DrawOutfitsToExcludeTab();

    void LoadRepositoryProps();
    void LoadRepositoryOutfits();
    void FilterRepositoryProps();
    void FilterRepositoryOutfits();

    static const ZRepositoryID& GetRandomRepositoryId(const std::vector<ZRepositoryID>& p_RepositoryIds);
    static const ZRepositoryID GetRandomEntranceId();
    static const int32_t GetRandomCharacterSetIndex(const TArray<TEntityRef<ZOutfitVariationCollection>>& p_CharSets);
    static const int32_t GetRandomOutfitVariationIndex(const TArray<TEntityRef<ZOutfitVariation>> p_Variations);

    void LoadCategoriesFromSettings();
    void LoadPropsToSpawnFromSettings();
    void LoadPropsToExcludeFromSettings();
    void LoadOutfitSettings();
    void LoadOutfitsFromOtherScenesFromSettings();
    void LoadOutfitsToSpawnFromSettings();
    void LoadOutfitsToExcludeFromSettings();
    void UpdatePropRepositoryIdListSetting(
        const ZString& p_Section,
        const ZString& p_Key
    );
    void UpdateOutfitRepositoryIdListSetting(
        const ZString& p_Section,
        const ZString& p_Key
    );
    static void ParseRepositoryIdCsv(
        const std::string& p_Value,
        std::unordered_set<ZRepositoryID>& p_RepositoryIds
    );

    void BuildCurrentSceneOutfitRepositoryIds();
    void BuildGlobalOutfitRepositoryIdsFromBricks();
    void BuildSceneNamesToRuntimeResourceIds();
    void BuildSceneToOutfitBrickRuntimeResourceIds(const std::string& p_SceneName, const ZRuntimeResourceID& p_SceneRuntimeResourceId);
    void BuildChunkIndexToResourcePackageCount();
    static bool FindOutfitReferencesRecursive(
        const ZRuntimeResourceID& p_CurrentRuntimeResourceId,
        std::unordered_set<uint64_t>& p_Visited,
        std::unordered_set<ZRuntimeResourceID>& p_Found,
        int p_Depth = 0
    );
    static bool LoadBrick(
        const ZRuntimeResourceID& p_BrickRuntimeResourceId,
        TResourcePtr<ZTemplateEntityFactory>& p_ResourcePtr,
        ZEntityRef& p_EntityRef
    );
    void LoadGlobalDataBrick(const ZRuntimeResourceID& p_BrickRuntimeResourceId);
    void LoadOutfits(const ZRuntimeResourceID& p_OutfitBrickRuntimeResourceId);
    void LoadOutfits(const std::unordered_set<std::string>& p_Scenes);
    void LoadOutfitsForSelectedScenes();
    void UnloadOutfits();
    static std::string ToEntityTemplatePath(const std::string_view p_ScenePath);
    static std::filesystem::path GetRuntimeDirectory();
    bool IsResourcePackageLimitExceeded();
    static void FindGlobalOutfitRepositoryIds(
        const ZEntityRef& p_BrickEntityRef,
        const ZRuntimeResourceID& p_BrickBlueprintRuntimeResourceId,
        std::unordered_set<ZRepositoryID>& m_GlobalOutfitRepositoryIds
    );
    bool TryFindOutfitWithBonesAndCollisionResourceProperty(
        const TArray<TEntityRef<ZOutfitVariationCollection>>& p_CharSets,
        int32_t& p_OutCharacterSetIndex,
        int32_t& p_OutVariationIndex
    ) const;
    static bool HasBonesAndCollisionResourceProperty(const ZRuntimeResourceID& s_OutfitRuntimeResourceID);
    void FindAndMountChunksForOutfitsToSpawn(const ZString& p_CurrentSceneResource);
    static bool ResourceContainsOutfitsToSpawn(
        const ZResourceContainer::SResourceInfo& p_ResourceInfo,
        const std::unordered_set<ZRepositoryID>& p_OutfitsToFind,
        size_t& p_FoundOutfitCount
    );

    static ZString GetCurrentSceneName();

    DECLARE_PLUGIN_DETOUR(Randomizer, bool, OnLoadScene, ZEntitySceneContext*, SSceneInitParameters& parameters);
    DECLARE_PLUGIN_DETOUR(Randomizer, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene);
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

    DECLARE_PLUGIN_DETOUR(
        Randomizer,
        void,
        ZActorInventoryHandler_StartItemStreamIn,
        ZActorInventoryHandler* th,
        TArray<TEntityRef<ZItemRepositoryKeyEntity>>& rInventoryKeys,
        TEntityRef<ZItemRepositoryKeyEntity>& rWeaponKey,
        TEntityRef<ZItemRepositoryKeyEntity>& rGrenadeKey
    );

    DECLARE_PLUGIN_DETOUR(
        Randomizer,
        void,
        ZHitman5_SetOutfit,
        ZHitman5* th,
        TEntityRef<ZGlobalOutfitKit> rOutfitKit,
        int32_t nCharset,
        int32_t nVariation,
        bool bEnableOutfitModifiers,
        bool bIgnoreOutifChange
    );

    DECLARE_PLUGIN_DETOUR(
        Randomizer,
        void,
        ZActor_SetOutfit,
        ZActor* th,
        TEntityRef<ZGlobalOutfitKit> rOutfit,
        int32_t charset,
        int32_t variation,
        bool bNude
    );

    DECLARE_PLUGIN_DETOUR(
        Randomizer,
        TEntityRef<ZClothBundleEntity>*,
        ZClothBundleEntity_CreateClothBundle,
        TEntityRef<ZClothBundleEntity>& result,
        const SMatrix& mat,
        ZRepositoryID id,
        int32_t nOutfitVariation,
        int32_t nOutfitCharset,
        bool bSpawnedByHitman,
        bool bEnableOutfitModifiers
    );

    bool m_IsRandomizerEnabled = true;
    bool m_IsRandomizerAllowedForScene = true;

    bool m_RandomizerMenuActive = false;

    bool m_RandomizeProps = true;
    bool m_RandomizeOutfits = true;
    bool m_RandomizeEntrance = true;

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
    std::vector<std::pair<ZRepositoryID, std::string>> m_AllRepositoryOutfits;
    std::unordered_set<ZRepositoryID> m_PlayerOutfits;
    std::unordered_set<ZRepositoryID> m_ActorOutfits;

    std::map<std::string, bool> m_InventoryCategoryToState;
    std::vector<std::tuple<ZRepositoryID, std::string, bool, bool, bool, bool>> m_PropsToSpawn;
    std::vector<ZRepositoryID> m_PropsToSpawnInWorld;
    std::vector<ZRepositoryID> m_PropsToSpawnInStash;
    std::vector<ZRepositoryID> m_PropsToSpawnInPlayerInventory;
    std::vector<ZRepositoryID> m_PropsToSpawnInActorInventory;
    std::vector<ZRepositoryID> m_WeaponsToSpawnInActorInventory;
    std::vector<std::pair<ZRepositoryID, std::string>> m_PropsToExclude;
    std::unordered_set<ZRepositoryID> m_ExcludedPropRepositoryIds;

    bool m_SpawnInWorld = true;
    bool m_SpawnInStash = true;
    bool m_SpawnInPlayerInventory = true;
    bool m_SpawnInActorInventory = true;

    bool m_RandomizePlayerOutfit = true;
    bool m_RandomizeActorOutfit = true;
    bool m_RandomizeClothBundleOutfit = true;

    bool m_RandomizeCivilianOutfit = true;
    bool m_RandomizeGuardOutfit = true;

    bool m_RandomizeCharacterSetIndex = true;
    bool m_RandomizeOutfitVariation = true;

    bool m_RandomizeOutfitsFromOtherScenes = false;
    bool m_RandomizeSeason2GlobalOutfits = true;
    bool m_RandomizeSeason3GlobalOutfits = true;

    std::unordered_set<ZRepositoryID> m_CurrentSceneOutfitRepositoryIds;
    std::unordered_set<ZRepositoryID> m_GlobalOutfitRepositoryIds;
    std::unordered_set<ZRepositoryID> m_Season2GlobalOutfitRepositoryIds;
    std::unordered_set<ZRepositoryID> m_Season3GlobalOutfitRepositoryIds;

    std::map<std::string, std::unordered_set<ZRuntimeResourceID>> m_Scenes;
    std::unordered_map<std::string, uint32_t> m_SceneToChunkIndex;
    std::unordered_map<std::string, std::unordered_set<ZRuntimeResourceID>> m_SceneToOutfitBrickIds;
    std::unordered_set<std::string> m_SelectedScenes;
    std::unordered_set<std::string> m_LoadedScenes;
    std::unordered_map<uint32_t, size_t> m_ChunkIndexToResourcePackageCount;
    std::unordered_set<uint32_t> m_PendingChunks;
    std::vector<std::pair<ZResourcePtr, ZEntityRef>> m_LoadedOutfitBricks;
    std::unordered_map<ZRuntimeResourceID, std::pair<ZResourcePtr, ZEntityRef>> m_LoadedGlobalOutfitBricks;
    bool m_ShowResourcePackageLimitPopup = false;
    bool m_ResourcePackageLimitPopupOpened = false;
    size_t m_PendingChunkCount = 0;
    size_t m_PendingResourcePackageCount = 0;
    const size_t MAX_RESOURCE_PACKAGES = 128;

    bool m_SpawnForPlayer = true;
    bool m_SpawnForActor = true;
    bool m_SpawnForClothBundle = true;

    std::vector<std::tuple<ZRepositoryID, std::string, bool, bool, bool>> m_OutfitsToSpawn;
    std::vector<ZRuntimeResourceID> m_OutfitBricksToLoad;
    std::vector<ZRepositoryID> m_OutfitsToSpawnForPlayer;
    std::vector<ZRepositoryID> m_MaleCivilianOutfitsToSpawnForActor;
    std::vector<ZRepositoryID> m_FemaleCivilianOutfitsToSpawnForActor;
    std::vector<ZRepositoryID> m_MaleGuardOutfitsToSpawnForActor;
    std::vector<ZRepositoryID> m_FemaleGuardOutfitsToSpawnForActor;
    std::vector<ZRepositoryID> m_OutfitsToSpawnForClothBundle;
    std::vector<std::pair<ZRepositoryID, std::string>> m_OutfitsToExclude;
    std::unordered_set<ZRepositoryID> m_ExcludedOutfitRepositoryIds;
    bool m_AreOutfitsFiltered = false;
};

DECLARE_ZHM_PLUGIN(Randomizer)
