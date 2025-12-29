#include "Outfits.h"

#include <filesystem>

#include <imgui_internal.h>

#include <IconsMaterialDesign.h>

#include <simdjson.h>

#include <Glacier/ZModule.h>
#include <Glacier/SExternalReferences.h>

#include <Util/ResourceUtils.h>
#include "Logging.h"
#include "Functions.h"

void Outfits::Init() {
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &Outfits::OnClearScene);

    Hooks::ZLevelManager_StartGame->AddDetour(this, &Outfits::ZLevelManager_StartGame);
}

void Outfits::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_MAN " OUTFITS")) {
        m_OutfitsMenuActive = !m_OutfitsMenuActive;
    }
}

void Outfits::OnDrawUI(const bool p_HasFocus) {
    if (!p_HasFocus || !m_OutfitsMenuActive) {
        return;
    }

    ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;

    auto s_LocalHitman = SDK()->GetLocalPlayer();

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("OUTFITS", &m_OutfitsMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing) {
        if (m_Scenes.empty()) {
            BuildSceneNamesToRuntimeResourceIds();
        }

        ImGui::BeginDisabled(m_IsGlobalDataSeason2BrickLoaded);

        if (ImGui::Checkbox("Season 2 Global Outfits", &m_IsGlobalDataSeason2BrickLoaded)) {
            if (m_IsGlobalDataSeason2BrickLoaded) {
                m_LoadedGlobalOutfitBricks.erase(
                    ResId<"[assembly:/_pro/scenes/bricks/globaldata_s2.brick].pc_entitytype">
                );
            }
            else {
                m_IsGlobalDataSeason2BrickLoaded = LoadGlobalDataBrick(
                    ResId<"[assembly:/_pro/scenes/bricks/globaldata_s2.brick].pc_entitytype">
                );
            }
        }

        ImGui::EndDisabled();

        if (m_IsGlobalDataSeason2BrickLoaded) {
            ImGui::SameLine();
            ImGui::TextDisabled("(loaded)");
        }

        ImGui::BeginDisabled(m_IsGlobalDataSeason3BrickLoaded);

        if (ImGui::Checkbox("Season 3 Global Outfits", &m_IsGlobalDataSeason3BrickLoaded)) {
            if (m_IsGlobalDataSeason3BrickLoaded) {
                m_LoadedGlobalOutfitBricks.erase(
                    ResId<"[assembly:/_pro/scenes/bricks/globaldata_s3.brick].pc_entitytype">
                );
            }
            else {
                m_IsGlobalDataSeason3BrickLoaded = LoadGlobalDataBrick(
                    ResId<"[assembly:/_pro/scenes/bricks/globaldata_s3.brick].pc_entitytype">
                );
            }
        }

        ImGui::EndDisabled();

        if (m_IsGlobalDataSeason3BrickLoaded) {
            ImGui::SameLine();
            ImGui::TextDisabled("(loaded)");
        }

        static int s_SelectedIndex = 0;

        ImGui::TextUnformatted("Scenes:");
        ImGui::BeginChild("SceneList", ImVec2(0, 250), true, ImGuiWindowFlags_HorizontalScrollbar);

        const ZRuntimeResourceID s_CurrentSceneRuntimeResourceId =
            Globals::Hitman5Module->m_pEntitySceneContext->m_SceneConfig.m_ridSceneFactory;

        for (const auto& [s_SceneName, s_SceneRuntimeResourceIds] : m_Scenes) {
            const bool s_IsMainScene = s_SceneRuntimeResourceIds.contains(s_CurrentSceneRuntimeResourceId);
            const bool s_IsLoaded = s_IsMainScene || m_LoadedScenes.contains(s_SceneName);
            bool s_IsSelected = s_IsMainScene || m_SelectedScenes.contains(s_SceneName);

            ImGui::BeginDisabled(s_IsMainScene);

            if (ImGui::Checkbox(s_SceneName.c_str(), &s_IsSelected)) {
                if (s_IsSelected) {
                    m_SelectedScenes.insert(s_SceneName);
                }
                else {
                    m_SelectedScenes.erase(s_SceneName);
                }
            }

            ImGui::EndDisabled();

            if (s_IsLoaded) {
                ImGui::SameLine();
                ImGui::TextDisabled("(loaded)");
            }
        }

        ImGui::EndChild();

        static char s_BrickResourceId[2048]{ "" };

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Brick Resource ID");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(-1);

        ImGui::InputText(
            "##BrickResourceId",
            s_BrickResourceId,
            sizeof(s_BrickResourceId)
        );

        ImGui::TextUnformatted("Outfit Bricks:");
        ImGui::BeginChild("OutfitBrickList", ImVec2(0, 250), true, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& [s_OutfitBrickRuntimeResourceId, _] : m_AdditionalLoadedOutfitBricks) {
            const bool s_IsLoaded = m_AdditionalLoadedOutfitBricks.contains(s_OutfitBrickRuntimeResourceId);
            bool s_IsSelected = m_SelectedOutfitBricks.contains(s_OutfitBrickRuntimeResourceId);
            const std::string s_OutfitBrickLabel = fmt::format("{:016X}", s_OutfitBrickRuntimeResourceId.GetID());

            if (ImGui::Checkbox(s_OutfitBrickLabel.c_str(), &s_IsSelected)) {
                if (s_IsSelected) {
                    m_SelectedOutfitBricks.insert(s_OutfitBrickRuntimeResourceId);
                }
                else {
                    m_SelectedOutfitBricks.erase(s_OutfitBrickRuntimeResourceId);
                }
            }

            if (s_IsLoaded) {
                ImGui::SameLine();
                ImGui::TextDisabled("(loaded)");
            }
        }

        ImGui::EndChild();

        ImGui::Spacing();

        if (ImGui::Button("Load/Unload Outfits")) {
            if (m_ChunkIndexToResourcePackageCount.empty()) {
                BuildChunkIndexToResourcePackageCount();
            }

            std::vector<std::string> s_ScenesToLoad;
            std::unordered_set<std::string> s_ScenesToUnload;
            std::vector<ZRuntimeResourceID> s_OutfitBricksToUnload;

            for (const auto& s_SceneName : m_SelectedScenes) {
                if (!m_LoadedScenes.contains(s_SceneName)) {
                    s_ScenesToLoad.push_back(s_SceneName);
                }
            }

            for (const auto& s_SceneName : m_LoadedScenes) {
                if (!m_SelectedScenes.contains(s_SceneName)) {
                    s_ScenesToUnload.insert(s_SceneName);
                }
            }

            for (const auto& [s_OutfitBrickRuntimeResourceId, _] : m_AdditionalLoadedOutfitBricks) {
                if (!m_SelectedOutfitBricks.contains(s_OutfitBrickRuntimeResourceId)) {
                    s_OutfitBricksToUnload.push_back(s_OutfitBrickRuntimeResourceId);
                }
            }

            if (!s_ScenesToUnload.empty()) {
                UnloadOutfits(s_ScenesToUnload);
            }

            if (!s_OutfitBricksToUnload.empty()) {
                UnloadOutfits(s_OutfitBricksToUnload);
            }

            m_PendingChunks.clear();

            for (auto& s_PartitionInfo : (*Globals::PackageManager)->m_aPartitionInfos) {
                if (SDK()->IsChunkMounted(s_PartitionInfo->m_nIndex)) {
                    m_PendingChunks.insert(s_PartitionInfo->m_nIndex);
                }
            }

            ZRuntimeResourceID s_BrickRuntimeResourceID;

            if (s_BrickResourceId[0] == '\0') {
                for (const auto& s_SelectedScene : s_ScenesToLoad) {
                    for (const auto& s_SceneRuntimeResourceId : m_Scenes[s_SelectedScene]) {
                        const auto& s_ChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(s_SceneRuntimeResourceId);

                        for (uint32_t s_ChunkIndex : s_ChunkIndices) {
                            m_PendingChunks.insert(s_ChunkIndex);
                        }
                    }
                }

            }
            else {
                s_BrickRuntimeResourceID = ZRuntimeResourceID::FromString(s_BrickResourceId);
                const auto& s_ChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(s_BrickRuntimeResourceID);

                for (uint32_t s_ChunkIndex : s_ChunkIndices) {
                    m_PendingChunks.insert(s_ChunkIndex);
                }
            }

            m_PendingChunkCount = m_PendingChunks.size();
            m_PendingResourcePackageCount = 0;

            for (uint32_t s_ChunkIndex : m_PendingChunks) {
                auto s_Iterator = m_ChunkIndexToResourcePackageCount.find(s_ChunkIndex);

                if (s_Iterator != m_ChunkIndexToResourcePackageCount.end()) {
                    m_PendingResourcePackageCount += s_Iterator->second;
                }
            }

            if (m_PendingResourcePackageCount > MAX_RESOURCE_PACKAGES) {
                m_ShowResourcePackageLimitPopup = true;
            }
            else {
                if (s_BrickRuntimeResourceID.GetID() == -1) {
                    LoadOutfits(s_ScenesToLoad);

                    for (const auto& s_SceneName : s_ScenesToLoad) {
                        m_LoadedScenes.insert(s_SceneName);
                    }
                }
                else {
                    LoadOutfits(s_BrickRuntimeResourceID);

                    s_BrickResourceId[0] = '\0';
                }
            }
        }

        if (m_ShowResourcePackageLimitPopup && !m_ResourcePackageLimitPopupOpened) {
            ImGui::OpenPopup("Resource Package Limit Exceeded");

            m_ResourcePackageLimitPopupOpened = true;
        }

        ImGuiStyle& s_Style = ImGui::GetStyle();

        ImGui::PushStyleColor(ImGuiCol_PopupBg, s_Style.Colors[ImGuiCol_WindowBg]);
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0, 0, 0, 0));

        if (ImGui::BeginPopupModal(
            "Resource Package Limit Exceeded",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        )) {
            ImGui::Text(
                "The selected scenes require %zu chunks and %zu resource packages.\n"
                "The engine supports a maximum of %d resource packages.",
                m_PendingChunkCount,
                m_PendingResourcePackageCount,
                MAX_RESOURCE_PACKAGES
            );

            ImGui::Dummy(ImVec2(0.f, 14.f));

            if (ImGui::Button("Cancel")) {
                m_ShowResourcePackageLimitPopup = false;
                m_ResourcePackageLimitPopupOpened = false;

                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(2);
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

void Outfits::BuildSceneNamesToRuntimeResourceIds() {
    const ZRuntimeResourceID s_ConfigRuntimeResourceID = ResId<"[assembly:/_pro/online/default/offlineconfig/config.contracts].pc_contracts">;
    ZResourcePtr s_ConfigResourcePtr;

    Globals::ResourceManager->GetResourcePtr(s_ConfigResourcePtr, s_ConfigRuntimeResourceID, 0);

    const ZResourceContainer::SResourceInfo& s_ConfigResourceInfo = s_ConfigResourcePtr.GetResourceInfo();

    for (size_t i = 0; i < s_ConfigResourceInfo.numReferences; ++i) {
        const uint32_t s_JsonReferenceIndex = (*Globals::ResourceContainer)->m_references[s_ConfigResourceInfo.firstReferenceIndex + i].index;
        const ZResourceContainer::SResourceInfo& s_JsonReferenceInfo = (*Globals::ResourceContainer)->m_resources[s_JsonReferenceIndex];

        // Scene path and location don't match in this json
        if (s_JsonReferenceInfo.rid == ResId<
            "[assembly:/_pro/online/default/contracts/seed/whitespider/"
            "c_ws_group_3d407b2b-e2f2-4204-9c08-7da67baa78fd.contract.json]"
            "([assembly:/_pro/online/default/offlineconfig/config.unlockables]"
            ".pc_unlockables).pc_json">) {
            continue;
        }

        ZResourcePtr s_JsonResourcePtr;

        Globals::ResourceManager->LoadResource(s_JsonResourcePtr, s_JsonReferenceInfo.rid);

        if (!s_JsonReferenceInfo.resourceData) {
            Logger::Error("{:016x} JSON resource isn't installed!", s_JsonReferenceInfo.rid.GetID());

            continue;
        }

        ZResourceReader* s_JsonResourceReader = *reinterpret_cast<ZResourceReader**>(s_JsonReferenceInfo.resourceData);
        ZResourceDataBuffer* s_DataBuffer = s_JsonResourceReader->m_pResourceData.m_pObject;

        if (!s_DataBuffer || !s_DataBuffer->m_pData) {
            Logger::Error("{:016x} JSON resource has no data buffer!", s_JsonReferenceInfo.rid.GetID());

            continue;
        }

        const char* s_JsonData = static_cast<const char*>(s_DataBuffer->m_pData);
        size_t s_JsonSize = s_DataBuffer->m_nSize;

        simdjson::padded_string s_PaddedJson(s_JsonData, s_JsonSize);

        simdjson::ondemand::parser s_Parser;
        auto s_Document = s_Parser.iterate(s_PaddedJson);

        auto s_ParseErrorCode = s_Document.error();

        if (s_ParseErrorCode) {
            Logger::Error("Failed to parse JSON: {}!", simdjson::error_message(s_ParseErrorCode));

            continue;
        }

        simdjson::ondemand::object s_Metadata = s_Document["Metadata"];
        const std::string_view s_CodeNameHint = s_Metadata["CodeName_Hint"];
        const std::string_view s_ScenePath = s_Metadata["ScenePath"];
        const std::string_view s_LocationKey = s_Metadata["Location"];

        std::string s_LocationKey2 = std::format("UI_{}_CITY", s_LocationKey);
        const uint32_t s_LocationHash = Hash::Crc32(s_LocationKey2.data(), s_LocationKey2.size());

        ZString s_SceneName;
        int s_OutMarkupResult;

        const bool s_TextFound = Hooks::ZUIText_TryGetTextFromNameHash->Call(
            Globals::UIText,
            s_LocationHash,
            s_SceneName,
            s_OutMarkupResult
        );

        if (!s_TextFound) {
            Logger::Error(
                "Missing UI text for location key: {} (Runtime Resource ID: {:016x})!",
                s_LocationKey2,
                s_JsonReferenceInfo.rid.GetID()
            );

            continue;
        }

        if (!s_ScenePath.empty()) {
            const std::string s_EntityTemplatePath = ToEntityTemplatePath(s_ScenePath);
            const ZRuntimeResourceID s_SceneRuntimeResourceId = ZRuntimeResourceID::FromString(s_EntityTemplatePath);

            m_Scenes[s_SceneName.c_str()].insert(s_SceneRuntimeResourceId);
        }
    }
}

void Outfits::BuildSceneToOutfitBrickRuntimeResourceIds(const std::string& p_SceneName, const ZRuntimeResourceID& p_SceneRuntimeResourceId) {
    std::unordered_set<uint64_t> s_Visited;
    std::unordered_set<ZRuntimeResourceID> s_FoundOutfits;

    bool s_HasOutfit = FindOutfitReferencesRecursive(p_SceneRuntimeResourceId, s_Visited, s_FoundOutfits);

    if (s_HasOutfit && !s_FoundOutfits.empty()) {
        for (const auto& s_OutfitBrickRuntimeResourceId : s_FoundOutfits) {
            if (s_OutfitBrickRuntimeResourceId == ResId<
                "[assembly:/_pro/scenes/missions/bangkok/outfits_zika.brick].pc_entitytype"> &&
                p_SceneName != "Bangkok") {
                continue;
            }
            else if (s_OutfitBrickRuntimeResourceId == ResId<
                "[assembly:/_pro/scenes/missions/colorado_2/colorado_outfits.brick].pc_entitytype"> &&
                p_SceneName != "Colorado") {
                continue;
            }

            m_SceneToOutfitBrickIds[p_SceneName].insert(s_OutfitBrickRuntimeResourceId);
        }
    }
    else {
        Logger::Warn("No outfit reference found in dependency tree for scene: {}", p_SceneRuntimeResourceId);
    }
}

void Outfits::BuildChunkIndexToResourcePackageCount() {
    m_ChunkIndexToResourcePackageCount.clear();

    const std::filesystem::path s_RuntimeDirectory = GetRuntimeDirectory();

    if (!std::filesystem::exists(s_RuntimeDirectory)) {
        Logger::Error("Runtime directory not found: {}", s_RuntimeDirectory.string());
        return;
    }

    for (const auto& s_DirectoryEntry : std::filesystem::directory_iterator(s_RuntimeDirectory)) {
        if (!s_DirectoryEntry.is_regular_file()) {
            continue;
        }

        const auto& s_Path = s_DirectoryEntry.path();

        if (s_Path.extension() != ".rpkg") {
            continue;
        }

        const std::string s_FileName = s_Path.filename().string();
        uint32_t s_ChunkIndex;

        if (!Util::ResourceUtils::TryParseChunkIndexFromResourcePackageFileName(s_FileName, s_ChunkIndex)) {
            continue;
        }

        m_ChunkIndexToResourcePackageCount[s_ChunkIndex]++;
    }
}

bool Outfits::FindOutfitReferencesRecursive(
    const ZRuntimeResourceID& p_CurrentRuntimeResourceId,
    std::unordered_set<uint64_t>& p_Visited,
    std::unordered_set<ZRuntimeResourceID>& p_Found,
    int p_Depth
) {
    constexpr int MAX_DEPTH = 4;

    if (p_Depth > MAX_DEPTH) {
        return false;
    }

    if (!p_Visited.insert(p_CurrentRuntimeResourceId.GetID()).second) {
        return false;
    }

    ZMutex& s_ResourceManagerMutex = Globals::ResourceManager->GetMutex();

    s_ResourceManagerMutex.Lock();

    ZResourceIndex s_ResourceIndex;
    bool s_StartLoading;

    Functions::ZResourceManager_GetResourceIndex->Call(
        Globals::ResourceManager,
        s_ResourceIndex,
        p_CurrentRuntimeResourceId,
        0,
        s_StartLoading
    );

    ZResourcePtr s_ResourcePtr(s_ResourceIndex);

    s_ResourceManagerMutex.Unlock();

    const ZResourceContainer::SResourceInfo& s_ResourceInfo = s_ResourcePtr.GetResourceInfo();

    bool s_IsOutfitBrickFound = false;

    for (size_t i = 0; i < s_ResourceInfo.numReferences; ++i) {
        const auto& s_Reference = (*Globals::ResourceContainer)->m_references[s_ResourceInfo.firstReferenceIndex + i];
        const auto& s_ReferenceInfo = (*Globals::ResourceContainer)->m_resources[s_Reference.index];

        if (s_ReferenceInfo.resourceType == 'CPPT' &&
            s_ReferenceInfo.rid == ResId<"[modules:/zglobaloutfitkit.class].pc_entitytype">) {
            p_Found.insert(p_CurrentRuntimeResourceId);

            s_IsOutfitBrickFound = true;
            continue;
        }

        if (s_ReferenceInfo.resourceType == 'TEMP') {
            if (s_ReferenceInfo.rid == ResId<"[assembly:/_pro/scenes/bricks/globaldata.brick].pc_entitytype"> ||
                s_ReferenceInfo.rid == ResId<"[assembly:/_pro/scenes/bricks/globaldata_s2.brick].pc_entitytype"> ||
                s_ReferenceInfo.rid == ResId<"[assembly:/_pro/scenes/bricks/globaldata_s3.brick].pc_entitytype">) {
                continue;
            }

            s_IsOutfitBrickFound |=
                FindOutfitReferencesRecursive(s_ReferenceInfo.rid, p_Visited, p_Found, p_Depth + 1);
        }
    }

    return s_IsOutfitBrickFound;
}

bool Outfits::LoadBrick(
    const ZRuntimeResourceID& p_BrickRuntimeResourceId,
    TResourcePtr<ZTemplateEntityFactory>& p_ResourcePtr,
    ZEntityRef& p_EntityRef
) {
    Globals::ResourceManager->LoadResource(p_ResourcePtr, p_BrickRuntimeResourceId);

    if (!p_ResourcePtr) {
        Logger::Debug("Resource is not loaded.");

        return false;
    }

    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Debug("Scene not loaded.");
        return false;
    }

    SExternalReferences s_ExternalRefs;

    for (const auto& s_Brick : Globals::Hitman5Module->m_pEntitySceneContext->m_aLoadedBricks) {
        if (s_Brick.m_RuntimeResourceID == ResId<"[assembly:/_pro/scenes/bricks/globaldata.brick].pc_entitytype">) {
            s_ExternalRefs.externalScenes.insert(s_Brick.m_RuntimeResourceID, s_Brick.m_EntityRef);
            break;
        }
    }

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager, p_EntityRef, "", p_ResourcePtr, s_Scene.m_ref, s_ExternalRefs, -1
    );

    if (!p_EntityRef) {
        Logger::Debug("Failed to spawn entity.");
        return false;
    }

    return true;
}

bool Outfits::LoadGlobalDataBrick(const ZRuntimeResourceID& p_BrickRuntimeResourceId) {
    TResourcePtr<ZTemplateEntityFactory> s_ResourcePtr;
    ZEntityRef s_EntityRef;

    if (LoadBrick(p_BrickRuntimeResourceId, s_ResourcePtr, s_EntityRef)) {
        m_LoadedGlobalOutfitBricks.insert(
            std::make_pair(p_BrickRuntimeResourceId, std::make_pair(s_ResourcePtr, s_EntityRef))
        );

        return true;
    }

    return false;
}

void Outfits::LoadOutfits(const std::string& p_SceneName, const ZRuntimeResourceID& p_OutfitsBrickRuntimeResourceId) {
    TResourcePtr<ZTemplateEntityFactory> s_ResourcePtr;

    Globals::ResourceManager->LoadResource(s_ResourcePtr, p_OutfitsBrickRuntimeResourceId);

    if (!s_ResourcePtr) {
        Logger::Debug("Resource is not loaded.");

        return;
    }

    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Debug("Scene not loaded.");
        return;
    }

    SExternalReferences s_ExternalRefs;

    for (const auto& s_Brick : Globals::Hitman5Module->m_pEntitySceneContext->m_aLoadedBricks) {
        if (s_Brick.m_RuntimeResourceID == ResId<"[assembly:/_pro/scenes/bricks/globaldata.brick].pc_entitytype">) {
            s_ExternalRefs.externalScenes.insert(s_Brick.m_RuntimeResourceID, s_Brick.m_EntityRef);
            break;
        }
    }

    ZEntityRef s_EntityRef;

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager, s_EntityRef, "", s_ResourcePtr, s_Scene.m_ref, s_ExternalRefs, -1
    );

    if (!s_EntityRef) {
        Logger::Debug("Failed to spawn entity.");
        return;
    }

    m_SceneToLoadedOutfitBricks[p_SceneName].push_back(std::make_pair(s_ResourcePtr, s_EntityRef));
}

void Outfits::LoadOutfits(const std::vector<std::string>& p_Scenes) {
    for (const auto& s_SceneName : p_Scenes) {
        if (m_SceneToOutfitBrickIds.contains(s_SceneName)) {
            continue;
        }

        for (const auto& s_SceneRuntimeResourceId : m_Scenes[s_SceneName]) {
            const auto& s_ChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(s_SceneRuntimeResourceId);

            // Case like assembly:/_PRO/Scenes/Missions/TheFacility/_Scene_Mission_Polarbear_Module_002_C.entity
            if (s_ChunkIndices.size() == 0) {
                return;
            }

            m_SceneToChunkIndex[s_SceneName] = s_ChunkIndices[0];

            if (!SDK()->IsChunkMounted(s_ChunkIndices[0])) {
                SDK()->MountChunk(s_ChunkIndices[0]);
            }

            BuildSceneToOutfitBrickRuntimeResourceIds(s_SceneName, s_SceneRuntimeResourceId);
        }

        for (const auto& s_OutfitBrickRuntimeResourceId : m_SceneToOutfitBrickIds[s_SceneName]) {
            LoadOutfits(s_SceneName, s_OutfitBrickRuntimeResourceId);
        }
    }
}

void Outfits::LoadOutfits(const ZRuntimeResourceID& p_OutfitBrickRuntimeResourceId) {
    const auto& s_ChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(p_OutfitBrickRuntimeResourceId);

    if (!SDK()->IsChunkMounted(s_ChunkIndices[0])) {
        SDK()->MountChunk(s_ChunkIndices[0]);
    }

    TResourcePtr<ZTemplateEntityFactory> s_ResourcePtr;
    ZEntityRef s_EntityRef;

    if (LoadBrick(p_OutfitBrickRuntimeResourceId, s_ResourcePtr, s_EntityRef)) {
        m_AdditionalLoadedOutfitBricks.insert(
            std::make_pair(p_OutfitBrickRuntimeResourceId, std::make_pair(s_ResourcePtr, s_EntityRef))
        );
        m_SelectedOutfitBricks.insert(p_OutfitBrickRuntimeResourceId);
    }

    Logger::Error("Failed to load outfit brick!");
}

void Outfits::UnloadOutfits(const std::unordered_set<std::string>& p_Scenes) {
    std::unordered_set<uint32_t> s_ChunksToUnmount;

    for (const auto& s_SceneName : p_Scenes) {
        for (const auto& s_SceneRuntimeResourceId : m_Scenes[s_SceneName]) {
            const auto& s_ChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(s_SceneRuntimeResourceId);

            if (s_ChunkIndices.size() > 0) {
                if (s_ChunkIndices[0] == 0) {
                    continue;
                }

                s_ChunksToUnmount.insert(s_ChunkIndices[0]);
                break;
            }
        }
    }

    ZResourceContainer* s_ResourceContainer = *Globals::ResourceContainer;
    uint32_t s_EarliestPackageId = UINT32_MAX;
    uint32_t s_EarliestChunkIndex = UINT32_MAX;

    for (uint32_t s_PackageId = 0; s_PackageId < s_ResourceContainer->m_MountedPackages.size(); ++s_PackageId) {
        const ZString& s_PackagePath = s_ResourceContainer->m_MountedPackages[s_PackageId];
        uint32_t s_ChunkIndex;

        if (!Util::ResourceUtils::TryParseChunkIndexFromResourcePackagePath(s_PackagePath, s_ChunkIndex)) {
            continue;
        }

        if (s_ChunksToUnmount.contains(s_ChunkIndex)) {
            s_EarliestPackageId = s_PackageId;
            s_EarliestChunkIndex = s_ChunkIndex;
            break;
        }
    }

    for (uint32_t s_PackageId = s_EarliestPackageId; s_PackageId < s_ResourceContainer->m_MountedPackages.size(); ++s_PackageId) {
        const ZString& s_PackagePath = s_ResourceContainer->m_MountedPackages[s_PackageId];
        uint32_t s_ChunkIndex;

        if (!Util::ResourceUtils::TryParseChunkIndexFromResourcePackagePath(s_PackagePath, s_ChunkIndex)) {
            continue;
        }

        s_ChunksToUnmount.insert(s_ChunkIndex);
    }

    UnloadOutfits(s_ChunksToUnmount);
    SDK()->UnmountChunk(s_EarliestChunkIndex, false);

    for (const auto& s_SceneName : p_Scenes) {
        m_LoadedScenes.erase(s_SceneName);
    }

    for (const auto& s_SceneName : m_LoadedScenes) {
        const uint32_t s_ChunkIndex = m_SceneToChunkIndex[s_SceneName];

        if (!s_ChunksToUnmount.contains(s_ChunkIndex)) {
            continue;
        }

        if (!SDK()->IsChunkMounted(s_ChunkIndex)) {
            SDK()->MountChunk(s_ChunkIndex);
        }

        for (const auto& s_OutfitBrickRuntimeResourceId : m_SceneToOutfitBrickIds[s_SceneName]) {
            LoadOutfits(s_SceneName, s_OutfitBrickRuntimeResourceId);
        }
    }

    for (const auto& s_OutfitBrickRuntimeResourceId : m_SelectedOutfitBricks) {
        const auto& s_ChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(s_OutfitBrickRuntimeResourceId);

        if (!s_ChunksToUnmount.contains(s_ChunkIndices[0])) {
            continue;
        }

        if (!SDK()->IsChunkMounted(s_ChunkIndices[0])) {
            SDK()->MountChunk(s_ChunkIndices[0]);
        }

        LoadOutfits(s_OutfitBrickRuntimeResourceId);
    }
}

void Outfits::UnloadOutfits(const std::vector<ZRuntimeResourceID>& p_OutfitBrickRuntimeResourceIds) {
    std::unordered_set<uint32_t> s_ChunksToUnmount;

    for (const auto& OutfitBrickRuntimeResourceId : p_OutfitBrickRuntimeResourceIds) {
        const auto& s_ChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(OutfitBrickRuntimeResourceId);

        if (s_ChunkIndices.size() > 0) {
            if (s_ChunkIndices[0] == 0) {
                continue;
            }

            s_ChunksToUnmount.insert(s_ChunkIndices[0]);
        }
    }

    ZResourceContainer* s_ResourceContainer = *Globals::ResourceContainer;
    uint32_t s_EarliestPackageId = UINT32_MAX;
    uint32_t s_EarliestChunkIndex = UINT32_MAX;

    for (uint32_t s_PackageId = 0; s_PackageId < s_ResourceContainer->m_MountedPackages.size(); ++s_PackageId) {
        const ZString& s_PackagePath = s_ResourceContainer->m_MountedPackages[s_PackageId];
        uint32_t s_ChunkIndex;

        if (!Util::ResourceUtils::TryParseChunkIndexFromResourcePackagePath(s_PackagePath, s_ChunkIndex)) {
            continue;
        }

        if (s_ChunksToUnmount.contains(s_ChunkIndex)) {
            bool s_IsChunkUsedByLoadedScene = false;

            for (const auto& s_SceneName : m_LoadedScenes) {
                if (s_ChunkIndex == m_SceneToChunkIndex[s_SceneName]) {
                    s_IsChunkUsedByLoadedScene = true;
                    break;
                }
            }

            if (s_IsChunkUsedByLoadedScene) {
                continue;
            }

            s_EarliestPackageId = s_PackageId;
            s_EarliestChunkIndex = s_ChunkIndex;
            break;
        }
    }

    if (s_EarliestPackageId != UINT32_MAX) {
        for (uint32_t s_PackageId = s_EarliestPackageId; s_PackageId < s_ResourceContainer->m_MountedPackages.size(); ++s_PackageId) {
            const ZString& s_PackagePath = s_ResourceContainer->m_MountedPackages[s_PackageId];
            uint32_t s_ChunkIndex;

            if (!Util::ResourceUtils::TryParseChunkIndexFromResourcePackagePath(s_PackagePath, s_ChunkIndex)) {
                continue;
            }

            s_ChunksToUnmount.insert(s_ChunkIndex);
        }

        UnloadOutfits(s_ChunksToUnmount);
        SDK()->UnmountChunk(s_EarliestChunkIndex, false);

        for (const auto& s_SceneName : m_LoadedScenes) {
            const uint32_t s_ChunkIndex = m_SceneToChunkIndex[s_SceneName];

            if (!s_ChunksToUnmount.contains(s_ChunkIndex)) {
                continue;
            }

            if (!SDK()->IsChunkMounted(s_ChunkIndex)) {
                SDK()->MountChunk(s_ChunkIndex);
            }

            for (const auto& s_OutfitBrickRuntimeResourceId : m_SceneToOutfitBrickIds[s_SceneName]) {
                LoadOutfits(s_SceneName, s_OutfitBrickRuntimeResourceId);
            }
        }

        for (const auto& s_OutfitBrickRuntimeResourceId : m_SelectedOutfitBricks) {
            const auto& s_ChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(s_OutfitBrickRuntimeResourceId);

            if (!s_ChunksToUnmount.contains(s_ChunkIndices[0])) {
                continue;
            }

            if (!SDK()->IsChunkMounted(s_ChunkIndices[0])) {
                SDK()->MountChunk(s_ChunkIndices[0]);
            }

            LoadOutfits(s_OutfitBrickRuntimeResourceId);
        }
    }
    else {
        SExternalReferences s_ExternalRefs;

        for (const auto& s_Brick : Globals::Hitman5Module->m_pEntitySceneContext->m_aLoadedBricks) {
            if (s_Brick.m_RuntimeResourceID == ResId<"[assembly:/_pro/scenes/bricks/globaldata.brick].pc_entitytype">) {
                s_ExternalRefs.externalScenes.insert(s_Brick.m_RuntimeResourceID, s_Brick.m_EntityRef);
                break;
            }
        }

        for (const auto& s_OutfitBrickRuntimeResourceId : p_OutfitBrickRuntimeResourceIds) {
            auto s_Iterator = m_AdditionalLoadedOutfitBricks.find(s_OutfitBrickRuntimeResourceId);

            if (s_Iterator == m_AdditionalLoadedOutfitBricks.end()) {
                continue;
            }

            Functions::ZEntityManager_DeleteEntity->Call(
                Globals::EntityManager,
                s_Iterator->second.second,
                s_ExternalRefs
            );

            m_AdditionalLoadedOutfitBricks.erase(s_Iterator);
        }
    }
}

void Outfits::UnloadOutfits(const std::unordered_set<uint32_t>& p_Chunks) {
    SExternalReferences s_ExternalRefs;

    for (const auto& s_Brick : Globals::Hitman5Module->m_pEntitySceneContext->m_aLoadedBricks) {
        if (s_Brick.m_RuntimeResourceID == ResId<"[assembly:/_pro/scenes/bricks/globaldata.brick].pc_entitytype">) {
            s_ExternalRefs.externalScenes.insert(s_Brick.m_RuntimeResourceID, s_Brick.m_EntityRef);
            break;
        }
    }

    for (auto s_Iterator = m_SceneToLoadedOutfitBricks.begin(); s_Iterator != m_SceneToLoadedOutfitBricks.end();) {
        if (p_Chunks.contains(m_SceneToChunkIndex[s_Iterator->first])) {
            for (const auto& [s_ResourcePtr, s_EntityRef] : s_Iterator->second) {
                Functions::ZEntityManager_DeleteEntity->Call(
                    Globals::EntityManager,
                    s_EntityRef,
                    s_ExternalRefs
                );
            }

            s_Iterator->second.clear();
            s_Iterator = m_SceneToLoadedOutfitBricks.erase(s_Iterator);
        }
        else {
            ++s_Iterator;
        }
    }

    for (auto s_Iterator = m_AdditionalLoadedOutfitBricks.begin(); s_Iterator != m_AdditionalLoadedOutfitBricks.end();) {
        const auto& s_ChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(s_Iterator->first);

        if (p_Chunks.contains(s_ChunkIndices[0])) {
            Functions::ZEntityManager_DeleteEntity->Call(
                Globals::EntityManager,
                s_Iterator->second.second,
                s_ExternalRefs
            );

            s_Iterator = m_AdditionalLoadedOutfitBricks.erase(s_Iterator);
        }
        else {
            ++s_Iterator;
        }
    }
}

std::string Outfits::ToEntityTemplatePath(const std::string_view p_ScenePath) {
    std::string s_NormalizedPath(p_ScenePath);

    std::transform(s_NormalizedPath.begin(), s_NormalizedPath.end(), s_NormalizedPath.begin(), ::tolower);

    return std::format("[{}].pc_entitytemplate", s_NormalizedPath);
}

std::filesystem::path Outfits::GetRuntimeDirectory() {
    char s_ExePathStr[MAX_PATH]{};

    GetModuleFileNameA(nullptr, s_ExePathStr, MAX_PATH);

    std::filesystem::path s_ExePath(s_ExePathStr);

    return s_ExePath.parent_path().parent_path() / "Runtime";
}

DEFINE_PLUGIN_DETOUR(Outfits, void, ZLevelManager_StartGame, ZLevelManager* th) {
    for (const auto& s_Brick : Globals::Hitman5Module->m_pEntitySceneContext->m_aLoadedBricks) {
        if (s_Brick.m_RuntimeResourceID == ResId<"[assembly:/_pro/scenes/bricks/globaldata_s2.brick].pc_entitytype">) {
            m_IsGlobalDataSeason2BrickLoaded = true;
            break;
        }
        else if (s_Brick.m_RuntimeResourceID == ResId<"[assembly:/_pro/scenes/bricks/globaldata_s3.brick].pc_entitytype">) {
            m_IsGlobalDataSeason3BrickLoaded = true;
            break;
        }
    }

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Outfits, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene) {
    m_SelectedScenes.clear();
    m_LoadedGlobalOutfitBricks.clear();

    if (!m_LoadedScenes.empty()) {
        UnloadOutfits(m_LoadedScenes);
    }

    std::vector<ZRuntimeResourceID> s_OutfitBricksToUnload;

    s_OutfitBricksToUnload.reserve(m_AdditionalLoadedOutfitBricks.size());

    for (const auto& [s_OutfitBrickRuntimeResourceId, _] : m_AdditionalLoadedOutfitBricks) {
        s_OutfitBricksToUnload.push_back(s_OutfitBrickRuntimeResourceId);
    }

    if (!s_OutfitBricksToUnload.empty()) {
        UnloadOutfits(s_OutfitBricksToUnload);
    }

    return HookResult<void>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(Outfits);
