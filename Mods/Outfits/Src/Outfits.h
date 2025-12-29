#pragma once

#include <unordered_set>

#include "IPluginInterface.h"

#include <Glacier/ZScene.h>

class Outfits : public IPluginInterface {
public:
    void Init() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
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
    bool LoadGlobalDataBrick(const ZRuntimeResourceID& p_BrickRuntimeResourceId);
    void LoadOutfits(const std::string& p_SceneName, const ZRuntimeResourceID& p_OutfitsBrickRuntimeResourceId);
    void LoadOutfits(const std::vector<std::string>& p_Scenes);
    void LoadOutfits(const ZRuntimeResourceID& p_OutfitBrickRuntimeResourceId);
    void UnloadOutfits(const std::unordered_set<std::string>& p_Scenes);
    void UnloadOutfits(const std::vector<ZRuntimeResourceID>& p_OutfitBrickRuntimeResourceIds);
    void UnloadOutfits(const std::unordered_set<uint32_t>& p_Chunks);
    static std::string ToEntityTemplatePath(const std::string_view p_ScenePath);
    static std::filesystem::path GetRuntimeDirectory();

    DECLARE_PLUGIN_DETOUR(Outfits, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene);

    DECLARE_PLUGIN_DETOUR(Outfits, void, ZLevelManager_StartGame, ZLevelManager* th);

    bool m_OutfitsMenuActive = false;

    std::map<std::string, std::unordered_set<ZRuntimeResourceID>> m_Scenes;
    std::unordered_map<std::string, uint32_t> m_SceneToChunkIndex;
    std::unordered_map<std::string, std::unordered_set<ZRuntimeResourceID>> m_SceneToOutfitBrickIds;
    std::unordered_set<std::string> m_SelectedScenes;
    std::unordered_set<std::string> m_LoadedScenes;
    std::unordered_map<uint32_t, size_t> m_ChunkIndexToResourcePackageCount;
    std::unordered_set<uint32_t> m_PendingChunks;
    std::unordered_map<std::string, std::vector<std::pair<ZResourcePtr, ZEntityRef>>> m_SceneToLoadedOutfitBricks;
    std::unordered_map<ZRuntimeResourceID, std::pair<ZResourcePtr, ZEntityRef>> m_LoadedGlobalOutfitBricks;
    std::unordered_map<ZRuntimeResourceID, std::pair<ZResourcePtr, ZEntityRef>> m_AdditionalLoadedOutfitBricks;
    std::unordered_set<ZRuntimeResourceID> m_SelectedOutfitBricks;
    bool m_ShowResourcePackageLimitPopup = false;
    bool m_ResourcePackageLimitPopupOpened = false;
    size_t m_PendingChunkCount = 0;
    size_t m_PendingResourcePackageCount = 0;
    const size_t MAX_RESOURCE_PACKAGES = 128;
    bool m_IsGlobalDataSeason2BrickLoaded = false;
    bool m_IsGlobalDataSeason3BrickLoaded = false;
};

DECLARE_ZHM_PLUGIN(Outfits)
