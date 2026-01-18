#include "Randomizer.h"

#include <random>
#include <filesystem>

#include "IconsMaterialDesign.h"

#include "simdjson.h"

#include "ResourceLib_HM3.h"

#include "Glacier/ZItem.h"
#include "Glacier/ZActor.h"
#include "Glacier/ZUIMapTracker.h"
#include "Glacier/ZStash.h"
#include "Glacier/ZContract.h"
#include "Glacier/ZScene.h"
#include "Glacier/IEnumType.h"
#include "Glacier/ZContentKitManager.h"
#include "Glacier/ZModule.h"
#include "Glacier/SExternalReferences.h"
#include "Glacier/ZUIMap.h"

#include "Util/ImGuiUtils.h"
#include "Util/ResourceUtils.h"

#include "Glacier/ZWorldInventory.h"
#include "Glacier/ZGameLoopManager.h"

Randomizer::Randomizer() {
    uint8_t s_Nop[0x42] = {};
    memset(s_Nop, 0x90, sizeof(s_Nop));

    if (!SDK()->PatchCode(
        "\x48\x85\xFF\x74\x00\xE8\x00\x00\x00\x00\x48\x8B\x08\x48\x85\xC9\x75\x00\xE8\x00\x00\x00\x00\x48\x8B\x48\x10\x48\x8B\x01\xEB\x00\x48\x8B\x01\x48\x8B\xD7\xFF\x50\x48\x48\x8B\xC8\x48\x85\xC0\x74\x00\x48\x8B\x00\x48\x8B\xD7\xFF\x50\x48\x48\xC7\x43\x08",
        "xxxx?x????xxxxxxx?x????xxxxxxxx?xxxxxxxxxxxxxxxx?xxxxxxxxxxxxx",
        s_Nop,
        sizeof(s_Nop),
        0
    )) {
        Logger::Error("Could not patch ZTemplateEntityFactory brick data freeing.");
    }
}

void Randomizer::Init() {
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &Randomizer::OnLoadScene);
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &Randomizer::OnClearScene);
    Hooks::ZLevelManager_StartGame->AddDetour(this, &Randomizer::ZLevelManager_StartGame);

    Hooks::ZItemSpawner_RequestContentLoad->AddDetour(this, &Randomizer::ZItemSpawner_RequestContentLoad);
    Hooks::ZCharacterSubcontrollerInventory_CreateItem->AddDetour(
        this,
        &Randomizer::ZCharacterSubcontrollerInventory_CreateItem
    );
    Hooks::ZActorInventoryHandler_RequestItem->AddDetour(this, &Randomizer::ZActorInventoryHandler_RequestItem);
    Hooks::ZHitman5_SetOutfit->AddDetour(this, &Randomizer::ZHitman5_SetOutfit);
    Hooks::ZActor_SetOutfit->AddDetour(this, &Randomizer::ZActor_SetOutfit);
    Hooks::ZClothBundleEntity_CreateClothBundle->AddDetour(this, &Randomizer::ZClothBundleEntity_CreateClothBundle);

    if (!HasSetting("general", "enable_randomizer")) {
        SetSettingBool("general", "enable_randomizer", true);
    }

    if (!HasSetting("general", "randomize_props")) {
        SetSettingBool("general", "randomize_props", true);
    }

    if (!HasSetting("general", "randomize_outfits")) {
        SetSettingBool("general", "randomize_outfits", true);
    }

    if (!HasSetting("general", "randomize_entrance")) {
        SetSettingBool("general", "randomize_entrance", true);
    }

    if (!HasSetting("general", "randomize_world_props")) {
        SetSettingBool("general", "randomize_world_props", true);
    }

    if (!HasSetting("general", "randomize_stash_props")) {
        SetSettingBool("general", "randomize_stash_props", true);
    }

    if (!HasSetting("general", "randomize_player_inventory")) {
        SetSettingBool("general", "randomize_player_inventory", true);
    }

    if (!HasSetting("general", "randomize_actor_inventory")) {
        SetSettingBool("general", "randomize_actor_inventory", true);
    }

    if (!HasSetting("general", "randomize_items")) {
        SetSettingBool("general", "randomize_items", true);
    }

    if (!HasSetting("general", "randomize_weapons")) {
        SetSettingBool("general", "randomize_weapons", true);
    }

    if (!HasSetting("general", "number_of_props_to_spawn_in_stashes")) {
        SetSettingInt("general", "number_of_props_to_spawn_in_stashes", 1);
    }

    m_IsRandomizerEnabled = GetSettingBool("general", "enable_randomizer", true);

    m_RandomizeProps = GetSettingBool("general", "randomize_props", true);
    m_RandomizeOutfits = GetSettingBool("general", "randomize_outfits", true);
    m_RandomizeEntrance = GetSettingBool("general", "randomize_entrance", true);

    m_RandomizeWorldProps = GetSettingBool("general", "randomize_world_props", true);
    m_RandomizeStashProps = GetSettingBool("general", "randomize_stash_props", true);
    m_RandomizePlayerInventory = GetSettingBool("general", "randomize_player_inventory", true);
    m_RandomizeActorInventory = GetSettingBool("general", "randomize_actor_inventory", true);

    m_RandomizeItems = GetSettingBool("general", "randomize_items", true);
    m_RandomizeWeapons = GetSettingBool("general", "randomize_weapons", true);

    m_RepositoryPropSpawnCount = GetSettingInt("general", "number_of_props_to_spawn_in_stashes", 1);
}

void Randomizer::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_MAN " RANDOMIZER")) {
        m_RandomizerMenuActive = !m_RandomizerMenuActive;
    }
}

void Randomizer::OnDrawUI(const bool p_HasFocus) {
    if (!p_HasFocus || !m_RandomizerMenuActive) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("RANDOMIZER", &m_RandomizerMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing) {
        if (m_Scenes.empty()) {
            BuildSceneNamesToRuntimeResourceIds();

            LoadOutfitsFromOtherScenesFromSettings();
        }

        if (m_AllRepositoryProps.empty()) {
            LoadRepositoryProps();
            LoadRepositoryOutfits();

            LoadCategoriesFromSettings();
            LoadPropsToSpawnFromSettings();
            LoadPropsToExcludeFromSettings();

            LoadOutfitSettings();
            LoadOutfitsToSpawnFromSettings();
            LoadOutfitsToExcludeFromSettings();
        }

        if (ImGui::BeginTabBar("RandomizerTabs")) {
            if (ImGui::BeginTabItem("General")) {
                DrawGeneralTab();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Categories")) {
                DrawCategoriesTab();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Props To Spawn")) {
                DrawPropsToSpawnTab();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Props To Exclude")) {
                DrawPropsToExcludeTab();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Outfits")) {
                DrawOutfitsTab();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Outfits From Other Scenes")) {
                DrawOutfitsFromOtherScenesTab();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Outfits To Spawn")) {
                DrawOutfitsToSpawnTab();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Outfits To Exclude")) {
                DrawOutfitsToExcludeTab();

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

void Randomizer::DrawGeneralTab() {
    if (ImGui::Checkbox("Enable Randomizer", &m_IsRandomizerEnabled)) {
        SetSettingBool("general", "enable_randomizer", m_IsRandomizerEnabled);
    }

    ImGui::Separator();

    if (ImGui::Checkbox("Randomize Props", &m_RandomizeProps)) {
        SetSettingBool("general", "randomize_props", m_RandomizeProps);
    }

    if (ImGui::Checkbox("Randomize Outfits", &m_RandomizeOutfits)) {
        SetSettingBool("general", "randomize_outfits", m_RandomizeOutfits);
    }

    if (ImGui::Checkbox("Randomize Entrance", &m_RandomizeEntrance)) {
        SetSettingBool("general", "randomize_entrance", m_RandomizeEntrance);
    }

    ImGui::Separator();

    ImGui::Text("Inventories");
    ImGui::Spacing();

    ImGui::BeginDisabled(!m_RandomizeProps);

    if (ImGui::Checkbox("Randomize World Props", &m_RandomizeWorldProps)) {
        SetSettingBool("general", "randomize_world_props", m_RandomizeWorldProps);
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeProps);

    if (ImGui::Checkbox("Randomize Stash Props", &m_RandomizeStashProps)) {
        SetSettingBool("general", "randomize_stash_props", m_RandomizeStashProps);
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeProps);

    if (ImGui::Checkbox("Randomize Player Inventory", &m_RandomizePlayerInventory)) {
        SetSettingBool("general", "randomize_player_inventory", m_RandomizePlayerInventory);
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeProps);

    if (ImGui::Checkbox("Randomize Actor Inventory", &m_RandomizeActorInventory)) {
        SetSettingBool("general", "randomize_actor_inventory", m_RandomizeActorInventory);
    }

    ImGui::EndDisabled();

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "At least one weapon category must be selected,\n"
            "or at least one weapon must be added to the props to spawn list."
        );
    }

    ImGui::Separator();

    ImGui::BeginDisabled(!m_RandomizeProps);

    if (ImGui::Checkbox("Randomize Items", &m_RandomizeItems)) {
        SetSettingBool("general", "randomize_items", m_RandomizeItems);
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeProps);

    if (ImGui::Checkbox("Randomize Weapons", &m_RandomizeWeapons)) {
        SetSettingBool("general", "randomize_weapons", m_RandomizeWeapons);
    }

    ImGui::EndDisabled();

    ImGui::Separator();

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Number Of Props To Spawn In Stashes");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetFrameHeight() * 5.f);

    ImGui::BeginDisabled(!m_RandomizeProps);

    if (ImGui::InputInt("##RepositoryPropSpawnCount", &m_RepositoryPropSpawnCount)) {
        SetSettingInt("general", "number_of_props_to_spawn_in_stashes", m_RepositoryPropSpawnCount);
    }

    ImGui::EndDisabled();
}

void Randomizer::DrawCategoriesTab() {
    for (auto& [s_InventoryCategory, s_IsEnabled] : m_InventoryCategoryToState) {
        ImGui::BeginDisabled(!m_RandomizeProps);

        if (ImGui::Checkbox(s_InventoryCategory.c_str(), &s_IsEnabled)) {
            SetSettingBool("categories", Util::StringUtils::ToLowerCase(s_InventoryCategory), s_IsEnabled);
        }

        ImGui::EndDisabled();
    }
}

void Randomizer::DrawPropsToSpawnTab() {
    ImGui::BeginDisabled(!m_RandomizeProps || !m_RandomizeWorldProps);
    ImGui::Checkbox("Spawn In World", &m_SpawnInWorld);
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeProps || !m_RandomizeStashProps);
    ImGui::Checkbox("Spawn In Stash", &m_SpawnInStash);
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeProps || !m_RandomizePlayerInventory);
    ImGui::Checkbox("Spawn In PlayerInventory", &m_SpawnInPlayerInventory);
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeProps || !m_RandomizeActorInventory);
    ImGui::Checkbox("Spawn In ActorInventory", &m_SpawnInActorInventory);
    ImGui::EndDisabled();

    static char s_PropTitle[2048]{ "" };

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Prop Title");
    ImGui::SameLine();

    ImGui::BeginDisabled(!m_RandomizeProps);

    Util::ImGuiUtils::InputWithAutocomplete(
        "##RepositoryProps",
        s_PropTitle,
        sizeof(s_PropTitle),
        m_AllRepositoryProps,
        [](auto& p_Tuple) -> const ZRepositoryID& { return std::get<0>(p_Tuple); },
        [](auto& p_Tuple) -> const std::string& { return std::get<1>(p_Tuple); },
        [&](const ZRepositoryID& p_Id, const std::string& p_Name, const auto&) {
            const bool s_IsAlreadyAdded = std::ranges::any_of(m_PropsToSpawn, [&](const auto& p_Item) {
                return std::get<0>(p_Item) == p_Id;
            });

            if (s_IsAlreadyAdded) {
                return;
            }

            if (m_SpawnInWorld) {
                UpdatePropRepositoryIdListSetting("props_to_spawn", "spawn_in_world");
            }

            if (m_SpawnInStash) {
                UpdatePropRepositoryIdListSetting("props_to_spawn", "spawn_in_stash");
            }

            if (m_SpawnInPlayerInventory) {
                UpdatePropRepositoryIdListSetting("props_to_spawn", "spawn_in_player_inventory");
            }

            if (m_SpawnInActorInventory) {
                UpdatePropRepositoryIdListSetting("props_to_spawn", "spawn_in_actor_inventory");
            }

            m_PropsToSpawn.push_back(std::make_tuple(
                p_Id,
                p_Name,
                m_SpawnInWorld,
                m_SpawnInStash,
                m_SpawnInPlayerInventory,
                m_SpawnInActorInventory
            ));
        },
        nullptr,
        [&](const auto& p_Tuple) -> bool {
            const bool s_IsWeapon = std::get<2>(p_Tuple);

            if (m_RandomizeItems && m_RandomizeWeapons) {
                return true;
            }

            if (m_RandomizeItems && !s_IsWeapon) {
                return true;
            }

            if (m_RandomizeWeapons && s_IsWeapon) {
                return true;
            }

            const auto& s_PropTitle = std::get<1>(p_Tuple);

            if (!s_PropTitle.starts_with("Gadget_Camera") && !s_PropTitle.contains("Gadget_Camera_Tagging")) {
                return true;
            }

            return false;
        }
    );

    ImGui::EndDisabled();

    ImGui::TextUnformatted("Props:");
    ImGui::BeginChild("PropList", ImVec2(0, 250), true, ImGuiWindowFlags_HorizontalScrollbar);

    for (size_t i = 0; i < m_PropsToSpawn.size(); ++i) {
        auto& [
            s_RepositoryId,
            s_Name,
            s_SpawnInWorld,
            s_SpawnInStash,
            s_SpawnInPlayerInventory,
            s_SpawnInActorInventory
        ] = m_PropsToSpawn[i];

        ImGui::PushID(static_cast<int>(i));

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(s_Name.c_str());

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizeProps || !m_RandomizeWorldProps);

        if (ImGui::Checkbox("Spawn In World", &s_SpawnInWorld)) {
            UpdatePropRepositoryIdListSetting("props_to_spawn", "spawn_in_world");
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizeProps || !m_RandomizeStashProps);

        if (ImGui::Checkbox("Spawn In Stash", &s_SpawnInStash)) {
            UpdatePropRepositoryIdListSetting("props_to_spawn", "spawn_in_stash");
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizeProps || !m_RandomizePlayerInventory);

        if (ImGui::Checkbox("Spawn In Player Inventory", &s_SpawnInPlayerInventory)) {
            UpdatePropRepositoryIdListSetting("props_to_spawn", "spawn_in_player_inventory");
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizeProps || !m_RandomizeActorInventory);

        if (ImGui::Checkbox("Spawn In Actor Inventory", &s_SpawnInActorInventory)) {
            UpdatePropRepositoryIdListSetting("props_to_spawn", "spawn_in_actor_inventory");
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizeProps);

        if (ImGui::SmallButton(ICON_MD_DELETE)) {
            UpdatePropRepositoryIdListSetting("props_to_spawn", "spawn_in_world");
            UpdatePropRepositoryIdListSetting("props_to_spawn", "spawn_in_stash");
            UpdatePropRepositoryIdListSetting("props_to_spawn", "spawn_in_player_inventory");
            UpdatePropRepositoryIdListSetting("props_to_spawn", "spawn_in_actor_inventory");

            m_PropsToSpawn.erase(m_PropsToSpawn.begin() + i);

            ImGui::PopID();

            break;
        }

        ImGui::EndDisabled();

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Remove");
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    if (!m_PropsToSpawn.empty()) {
        ImGui::BeginDisabled(!m_RandomizeProps);

        if (ImGui::Button("Clear All")) {
            RemoveSetting("props_to_spawn", "spawn_in_world");
            RemoveSetting("props_to_spawn", "spawn_in_stash");
            RemoveSetting("props_to_spawn", "spawn_in_player_inventory");
            RemoveSetting("props_to_spawn", "spawn_in_actor_inventory");

            m_PropsToSpawn.clear();
        }

        ImGui::EndDisabled();
    }
}

void Randomizer::DrawPropsToExcludeTab() {
    static char s_PropTitle[2048]{ "" };

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Prop Title");
    ImGui::SameLine();

    ImGui::BeginDisabled(!m_RandomizeProps);

    Util::ImGuiUtils::InputWithAutocomplete(
        "##RepositoryProps",
        s_PropTitle,
        sizeof(s_PropTitle),
        m_AllRepositoryProps,
        [](auto& p_Tuple) -> const ZRepositoryID& { return std::get<0>(p_Tuple); },
        [](auto& p_Tuple) -> const std::string& { return std::get<1>(p_Tuple); },
        [&](const ZRepositoryID& p_Id, const std::string& p_Name, const auto&) {
            const bool s_IsAlreadyAdded = std::ranges::any_of(m_PropsToExclude, [&](const auto& p_Item) {
                return std::get<0>(p_Item) == p_Id;
            });

            if (s_IsAlreadyAdded) {
                return;
            }

            m_PropsToExclude.push_back(std::make_pair(p_Id, p_Name));
            m_ExcludedPropRepositoryIds.insert(p_Id);

            SetSettingBool("props_to_exclude", Util::StringUtils::ToLowerCase(p_Id.ToString().c_str()), true);
        },
        nullptr,
        [&](const auto& p_Tuple) -> bool {
            const bool s_IsWeapon = std::get<2>(p_Tuple);

            if (m_RandomizeItems && m_RandomizeWeapons) {
                return true;
            }

            if (m_RandomizeItems && !s_IsWeapon) {
                return true;
            }

            if (m_RandomizeWeapons && s_IsWeapon) {
                return true;
            }

            const auto& s_PropTitle = std::get<1>(p_Tuple);

            if (!s_PropTitle.starts_with("Gadget_Camera") && !s_PropTitle.contains("Gadget_Camera_Tagging")) {
                return true;
            }

            return false;
        }
    );

    ImGui::EndDisabled();

    ImGui::TextUnformatted("Props:");
    ImGui::BeginChild("PropList", ImVec2(0, 250), true, ImGuiWindowFlags_HorizontalScrollbar);

    for (size_t i = 0; i < m_PropsToExclude.size(); ++i) {
        auto& [s_RepositoryId, s_Name] = m_PropsToExclude[i];

        ImGui::PushID(static_cast<int>(i));

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(s_Name.c_str());

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizeProps);

        if (ImGui::SmallButton(ICON_MD_DELETE)) {
            RemoveSetting("props_to_exclude", Util::StringUtils::ToLowerCase(s_RepositoryId.ToString().c_str()));

            m_PropsToExclude.erase(m_PropsToExclude.begin() + i);
            m_ExcludedPropRepositoryIds.erase(s_RepositoryId);

            ImGui::PopID();

            break;
        }

        ImGui::EndDisabled();

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Remove");
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    if (!m_PropsToExclude.empty()) {
        ImGui::BeginDisabled(!m_RandomizeProps);

        if (ImGui::Button("Clear All")) {
            for (const auto& [s_RepositoryId, s_Name] : m_PropsToExclude) {
                RemoveSetting("props_to_exclude", Util::StringUtils::ToLowerCase(s_RepositoryId.ToString().c_str()));
            }

            m_PropsToExclude.clear();
            m_ExcludedPropRepositoryIds.clear();
        }

        ImGui::EndDisabled();
    }
}

void Randomizer::DrawOutfitsTab() {
    ImGui::BeginDisabled(!m_RandomizeOutfits);

    if (ImGui::Checkbox("Randomize Player Outfit", &m_RandomizePlayerOutfit)) {
        SetSettingBool("outfits", "randomize_player_outfit", m_RandomizePlayerOutfit);
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeOutfits);

    if (ImGui::Checkbox("Randomize Actor Oufit", &m_RandomizeActorOutfit)) {
        SetSettingBool("outfits", "randomize_actor_outfit", m_RandomizeActorOutfit);
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeOutfits);

    if (ImGui::Checkbox("Randomize Cloth Bundle Outfit", &m_RandomizeClothBundleOutfit)) {
        SetSettingBool("outfits", "randomize_cloth_budle_outfit", m_RandomizeClothBundleOutfit);
    }

    ImGui::EndDisabled();

    ImGui::Separator();

    ImGui::Text("Actor Types");

    ImGui::Spacing();

    ImGui::BeginDisabled(!m_RandomizeOutfits || !m_RandomizeActorOutfit);

    if (ImGui::Checkbox("Randomize Civilian Outfit", &m_RandomizeCivilianOutfit)) {
        SetSettingBool("outfits", "randomize_civilian_outfit", m_RandomizeCivilianOutfit);
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeOutfits || !m_RandomizeActorOutfit);

    if (ImGui::Checkbox("Randomize Guard Outfit", &m_RandomizeGuardOutfit)) {
        SetSettingBool("outfits", "randomize_guard_outfit", m_RandomizeGuardOutfit);
    }

    ImGui::EndDisabled();

    ImGui::Separator();

    ImGui::BeginDisabled(!m_RandomizeOutfits);

    if (ImGui::Checkbox("Randomize Character Set Index", &m_RandomizeCharacterSetIndex)) {
        SetSettingBool("outfits", "randomize_character_set_index", m_RandomizeCharacterSetIndex);
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeOutfits);

    if (ImGui::Checkbox("Randomize Outfit Variation", &m_RandomizeOutfitVariation)) {
        SetSettingBool("outfits", "randomize_outfit_variation", m_RandomizeOutfitVariation);
    }

    ImGui::EndDisabled();
}

void Randomizer::DrawOutfitsFromOtherScenesTab() {
    ImGui::BeginDisabled(!m_RandomizeOutfits);

    if (ImGui::Checkbox("Randomize Outfits From Other Scenes", &m_RandomizeOutfitsFromOtherScenes)) {
        SetSettingBool(
            "outfits_from_other_scenes",
            "randomize_outfits_from_other_scenes",
            m_RandomizeOutfitsFromOtherScenes
        );
    }

    ImGui::EndDisabled();

    ImGui::Separator();

    ImGui::BeginDisabled(!m_RandomizeOutfits || !m_RandomizeOutfitsFromOtherScenes);

    if (ImGui::Checkbox("Season 2 Global Outfits", &m_RandomizeSeason2GlobalOutfits)) {
        SetSettingBool(
            "outfits_from_other_scenes",
            "randomize_season_2_global_outfits",
            m_RandomizeSeason2GlobalOutfits
        );
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeOutfits || !m_RandomizeOutfitsFromOtherScenes);

    if (ImGui::Checkbox("Season 3 Global Outfits", &m_RandomizeSeason3GlobalOutfits)) {
        SetSettingBool(
            "outfits_from_other_scenes",
            "randomize_season_3_global_outfits",
            m_RandomizeSeason3GlobalOutfits
        );
    }

    ImGui::EndDisabled();

    ImGui::TextUnformatted("Scenes:");
    ImGui::BeginChild("SceneList", ImVec2(0, 250), true, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& [s_SceneName, s_SceneRuntimeResourceIds] : m_Scenes) {
        bool s_IsSelected = m_SelectedScenes.contains(s_SceneName);

        ImGui::BeginDisabled(!m_RandomizeOutfits || !m_RandomizeOutfitsFromOtherScenes);

        if (ImGui::Checkbox(s_SceneName.c_str(), &s_IsSelected)) {
            if (s_IsSelected) {
                m_SelectedScenes.insert(s_SceneName);

                m_ShowResourcePackageLimitPopup = IsResourcePackageLimitExceeded();
            }
            else {
                m_SelectedScenes.erase(s_SceneName);
            }

            SetSettingBool(
                "outfits_from_other_scenes",
                Util::StringUtils::ToLowerCase(s_SceneName),
                s_IsSelected
            );
        }

        ImGui::EndDisabled();
    }

    ImGui::EndChild();

    ImGui::Spacing();

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

void Randomizer::DrawOutfitsToSpawnTab() {
    ImGui::BeginDisabled(!m_RandomizeOutfits || !m_RandomizePlayerOutfit);
    ImGui::Checkbox("Spawn For Player", &m_SpawnForPlayer);
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeOutfits || !m_RandomizeActorOutfit);
    ImGui::Checkbox("Spawn For Actor", &m_SpawnForActor);
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeOutfits || !m_RandomizeClothBundleOutfit);
    ImGui::Checkbox("Spawn For Cloth Bundle", &m_SpawnForClothBundle);
    ImGui::EndDisabled();

    static char s_OutfitName[2048]{ "" };

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Outfit");
    ImGui::SameLine();

    ImGui::BeginDisabled(!m_RandomizeOutfits);

    Util::ImGuiUtils::InputWithAutocomplete(
        "##RepositoryOutfits",
        s_OutfitName,
        sizeof(s_OutfitName),
        m_AllRepositoryOutfits,
        [](auto& p_Pair) -> const ZRepositoryID& { return p_Pair.first; },
        [](auto& p_Pair) -> const std::string& { return p_Pair.second; },
        [&](const ZRepositoryID& p_Id, const std::string& p_Name, const auto&) {
            const bool s_IsAlreadyAdded = std::ranges::any_of(m_OutfitsToSpawn, [&](const auto& p_Item) {
                return std::get<0>(p_Item) == p_Id;
                });

            if (s_IsAlreadyAdded) {
                return;
            }

            bool s_SpawnForPlayer = m_SpawnForPlayer;
            bool s_SpawnForActor = m_SpawnForActor;
            bool s_SpawnForClothBundle = m_SpawnForClothBundle;

            if (!m_PlayerOutfits.contains(p_Id)) {
                s_SpawnForPlayer = false;
                s_SpawnForClothBundle = false;
            }

            if (!m_ActorOutfits.contains(p_Id)) {
                s_SpawnForActor = false;
            }

            m_OutfitsToSpawn.push_back(std::make_tuple(
                p_Id,
                p_Name,
                s_SpawnForPlayer,
                s_SpawnForActor,
                s_SpawnForClothBundle
            ));

            if (s_SpawnForPlayer) {
                UpdateOutfitRepositoryIdListSetting("outfits_to_spawn", "spawn_for_player");
            }

            if (s_SpawnForActor) {
                UpdateOutfitRepositoryIdListSetting("outfits_to_spawn", "spawn_for_actor");
            }

            if (s_SpawnForClothBundle) {
                UpdateOutfitRepositoryIdListSetting("outfits_to_spawn", "spawn_for_cloth_bundle");
            }
        },
        nullptr,
        [&](const auto& p_Pair) -> bool {
            if (!p_Pair.second.starts_with("Base") &&
                !p_Pair.second.starts_with("Base_BodyguardMale") &&
                !p_Pair.second.starts_with("Base_CivFem") &&
                !p_Pair.second.starts_with("Base_CivFemHeel") &&
                !p_Pair.second.starts_with("Base_CivMale") &&
                !p_Pair.second.starts_with("Base_CopMale") &&
                !p_Pair.second.starts_with("Base_GuardMale") &&
                !p_Pair.second.starts_with("Debug_Missing Outfit_M") &&
                !p_Pair.second.starts_with("Debug_Missing Outfit_M01") &&
                !p_Pair.second.starts_with("Debug_Missing Outfit_M02") &&
                !p_Pair.second.starts_with("Crowd_Civilian_European_Casual_Male")) {
                return true;
            }

            return false;
        }
    );

    ImGui::EndDisabled();

    ImGui::TextUnformatted("Outfits:");
    ImGui::BeginChild("OutfitList", ImVec2(0, 250), true, ImGuiWindowFlags_HorizontalScrollbar);

    for (size_t i = 0; i < m_OutfitsToSpawn.size(); ++i) {
        auto& [
            s_RepositoryId,
            s_Name,
            s_SpawnForPlayer,
            s_SpawnForActor,
            s_SpawnForClothBundle
        ] = m_OutfitsToSpawn[i];

        ImGui::PushID(static_cast<int>(i));

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(s_Name.c_str());

        ImGui::SameLine();

        const bool s_IsPlayerOutfit = m_PlayerOutfits.contains(s_RepositoryId);
        const bool s_IsActorOutfit = m_ActorOutfits.contains(s_RepositoryId);

        ImGui::BeginDisabled(!m_RandomizeOutfits || !m_RandomizePlayerOutfit || !s_IsPlayerOutfit);

        if (ImGui::Checkbox("Spawn For Player", &s_SpawnForPlayer)) {
            UpdateOutfitRepositoryIdListSetting("outfits_to_spawn", "spawn_for_player");
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizeOutfits || !m_RandomizeActorOutfit || !s_IsActorOutfit);

        if (ImGui::Checkbox("Spawn For Actor", &s_SpawnForActor)) {
            UpdateOutfitRepositoryIdListSetting("outfits_to_spawn", "spawn_for_actor");
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizeOutfits || !m_RandomizeClothBundleOutfit || !s_IsPlayerOutfit);

        if (ImGui::Checkbox("Spawn For Cloth Bundle", &s_SpawnForClothBundle)) {
            UpdateOutfitRepositoryIdListSetting("outfits_to_spawn", "spawn_for_cloth_bundle");
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizeOutfits);

        if (ImGui::SmallButton(ICON_MD_DELETE)) {
            UpdateOutfitRepositoryIdListSetting("outfits_to_spawn", "spawn_for_player");
            UpdateOutfitRepositoryIdListSetting("outfits_to_spawn", "spawn_for_actor");
            UpdateOutfitRepositoryIdListSetting("outfits_to_spawn", "spawn_for_cloth_bundle");

            m_OutfitsToSpawn.erase(m_OutfitsToSpawn.begin() + i);

            ImGui::PopID();

            break;
        }

        ImGui::EndDisabled();

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Remove");
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    if (!m_OutfitsToSpawn.empty()) {
        ImGui::BeginDisabled(!m_RandomizeOutfits);

        if (ImGui::Button("Clear All")) {
            RemoveSetting("outfits_to_spawn", "spawn_for_player");
            RemoveSetting("outfits_to_spawn", "spawn_for_actor");
            RemoveSetting("outfits_to_spawn", "spawn_for_cloth_bundle");

            m_OutfitsToSpawn.clear();
        }

        ImGui::EndDisabled();
    }
}

void Randomizer::DrawOutfitsToExcludeTab() {
    static char s_OutfitName[2048]{ "" };

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Outfit");
    ImGui::SameLine();

    ImGui::BeginDisabled(!m_RandomizeOutfits);

    Util::ImGuiUtils::InputWithAutocomplete(
        "##RepositoryOutfits",
        s_OutfitName,
        sizeof(s_OutfitName),
        m_AllRepositoryOutfits,
        [](auto& p_Pair) -> const ZRepositoryID& { return p_Pair.first; },
        [](auto& p_Pair) -> const std::string& { return p_Pair.second; },
        [&](const ZRepositoryID& p_Id, const std::string& p_Name, const auto&) {
            const bool s_IsAlreadyAdded = std::ranges::any_of(m_OutfitsToExclude, [&](const auto& p_Item) {
                return std::get<0>(p_Item) == p_Id;
                });

            if (s_IsAlreadyAdded) {
                return;
            }

            m_OutfitsToExclude.push_back(std::make_pair(p_Id, p_Name));
            m_ExcludedOutfitRepositoryIds.insert(p_Id);

            SetSettingBool("outfits_to_exclude", Util::StringUtils::ToLowerCase(p_Id.ToString().c_str()), true);
        },
        nullptr,
        [&](const auto& p_Pair) -> bool {
            if (!p_Pair.second.starts_with("Base") &&
                !p_Pair.second.starts_with("Base_BodyguardMale") &&
                !p_Pair.second.starts_with("Base_CivFem") &&
                !p_Pair.second.starts_with("Base_CivFemHeel") &&
                !p_Pair.second.starts_with("Base_CivMale") &&
                !p_Pair.second.starts_with("Base_CopMale") &&
                !p_Pair.second.starts_with("Base_GuardMale") &&
                !p_Pair.second.starts_with("Debug_Missing Outfit_M") &&
                !p_Pair.second.starts_with("Debug_Missing Outfit_M01") &&
                !p_Pair.second.starts_with("Debug_Missing Outfit_M02") &&
                !p_Pair.second.starts_with("Crowd_Civilian_European_Casual_Male")) {
                return true;
            }

            return false;
        }
    );

    ImGui::EndDisabled();

    ImGui::TextUnformatted("Outfits:");
    ImGui::BeginChild("OutfitList", ImVec2(0, 250), true, ImGuiWindowFlags_HorizontalScrollbar);

    for (size_t i = 0; i < m_OutfitsToExclude.size(); ++i) {
        auto& [s_RepositoryId, s_Name] = m_OutfitsToExclude[i];

        ImGui::PushID(static_cast<int>(i));

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(s_Name.c_str());

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizeOutfits);

        if (ImGui::SmallButton(ICON_MD_DELETE)) {
            RemoveSetting("outfits_to_exclude", Util::StringUtils::ToLowerCase(s_RepositoryId.ToString().c_str()));

            m_OutfitsToExclude.erase(m_OutfitsToExclude.begin() + i);
            m_ExcludedOutfitRepositoryIds.erase(s_RepositoryId);

            ImGui::PopID();

            break;
        }

        ImGui::EndDisabled();

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Remove");
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    if (!m_OutfitsToExclude.empty()) {
        ImGui::BeginDisabled(!m_RandomizeOutfits);

        if (ImGui::Button("Clear All")) {
            for (const auto& [s_RepositoryId, s_Name] : m_OutfitsToExclude) {
                RemoveSetting("outfits_to_exclude", Util::StringUtils::ToLowerCase(s_RepositoryId.ToString().c_str()));
            }

            m_OutfitsToExclude.clear();
            m_ExcludedOutfitRepositoryIds.clear();
        }

        ImGui::EndDisabled();
    }
}

void Randomizer::LoadRepositoryProps() {
    m_AllRepositoryProps.clear();

    if (m_RepositoryResource.m_nResourceIndex.val == -1) {
        const auto s_ID = ResId<"[assembly:/repository/pro.repo].pc_repo">;

        Globals::ResourceManager->LoadResource(m_RepositoryResource, s_ID);
    }

    if (m_RepositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID) {
        const auto s_RepositoryData = static_cast<THashMap<
            ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(m_RepositoryResource.
                GetResourceData());

        for (const auto& [s_RepositoryID, s_DynamicObject] : *s_RepositoryData) {
            TArray<SDynamicObjectKeyValuePair>* s_Entries = s_DynamicObject.As<TArray<
                SDynamicObjectKeyValuePair>>();

            ZString s_Id, s_Title, s_CommonName, s_Name, s_InventoryCategoryIcon;
            std::string s_FinalName;
            bool s_IsItem = false;
            bool s_IsWeapon = false;

            for (auto& s_Entry : *s_Entries) {
                if (s_Entry.sKey == "ID_") {
                    s_Id = *s_Entry.value.As<ZString>();
                }
                else if (s_Entry.sKey == "Title") {
                    s_Title = *s_Entry.value.As<ZString>();
                }
                else if (s_Entry.sKey == "CommonName") {
                    s_CommonName = *s_Entry.value.As<ZString>();
                }
                else if (s_Entry.sKey == "Name") {
                    s_Name = *s_Entry.value.As<ZString>();
                }
                else if (s_Entry.sKey == "InventoryCategoryIcon") {
                    s_InventoryCategoryIcon = *s_Entry.value.As<ZString>();

                    if (s_InventoryCategoryIcon == "INVALID_CATEGORY_ICON") {
                        s_InventoryCategoryIcon = "other";
                    }
                    else if (s_InventoryCategoryIcon == "questitem") {
                        s_InventoryCategoryIcon = "questItem";
                    }
                }
                else if (s_Entry.sKey == "ItemType") {
                    s_IsItem = true;
                }
                else if (s_Entry.sKey == "PrimaryConfiguration") {
                    s_IsWeapon = true;
                }
            }

            if (s_Id.IsEmpty() || !s_IsItem || s_InventoryCategoryIcon == "evergreen_payout") {
                continue;
            }

            if (s_Title.IsEmpty() && s_CommonName.IsEmpty() && s_Name.IsEmpty()) {
                s_FinalName = std::format("<unnamed> [{}]", s_Id.c_str());
            }
            else if (!s_Title.IsEmpty()) {
                s_FinalName = std::format("{} [{}]", s_Title.c_str(), s_Id.c_str());
            }
            else if (!s_CommonName.IsEmpty()) {
                s_FinalName = std::format("{} [{}]", s_CommonName.c_str(), s_Id.c_str());
            }
            else if (!s_Name.IsEmpty()) {
                s_FinalName = std::format("{} [{}]", s_Name.c_str(), s_Id.c_str());
            }

            std::string s_InventoryCategoryIcon2 = s_InventoryCategoryIcon.c_str();
            s_InventoryCategoryIcon2[0] = std::toupper(s_InventoryCategoryIcon2[0]);

            m_AllRepositoryProps.push_back(std::make_tuple(
                s_Id,
                s_FinalName,
                s_IsWeapon,
                s_InventoryCategoryIcon2
            ));

            if (s_IsWeapon) {
                m_RepositoryWeapons.insert(s_Id);
            }

            m_InventoryCategoryToState.insert(std::make_pair(s_InventoryCategoryIcon2, true));
        }
    }

    std::ranges::sort(
        m_AllRepositoryProps,
        [](const auto& a, const auto& b) {
            auto [s_RepositoryIdA, s_NameA, s_IsWeaponA, s_InventoryCategoryIconA] = a;
            auto [s_RepositoryIdB, s_NameB, s_IsWeaponB, s_InventoryCategoryIconB] = b;

            std::ranges::transform(s_NameA, s_NameA.begin(), [](unsigned char c) { return std::tolower(c); });
            std::ranges::transform(s_NameB, s_NameB.begin(), [](unsigned char c) { return std::tolower(c); });

            return s_NameA < s_NameB;
        }
    );
}

void Randomizer::LoadRepositoryOutfits() {
    m_AllRepositoryOutfits.clear();

    if (m_RepositoryResource.m_nResourceIndex.val == -1) {
        const auto s_ID = ResId<"[assembly:/repository/pro.repo].pc_repo">;

        Globals::ResourceManager->LoadResource(m_RepositoryResource, s_ID);
    }

    if (m_RepositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID) {
        const auto s_RepositoryData = static_cast<THashMap<
            ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(m_RepositoryResource.
                GetResourceData());

        for (const auto& [s_RepositoryID, s_DynamicObject] : *s_RepositoryData) {
            TArray<SDynamicObjectKeyValuePair>* s_Entries = s_DynamicObject.As<TArray<
                SDynamicObjectKeyValuePair>>();

            ZString s_Id, s_CommonName;
            std::string s_FinalName;
            bool s_IsOufit = false;
            bool s_HeroDisguiseAvailable = false;
            bool s_IsHitmanSuit = false;

            for (auto& s_Entry : *s_Entries) {
                if (s_Entry.sKey == "ID_") {
                    s_Id = *s_Entry.value.As<ZString>();
                }
                else if (s_Entry.sKey == "CommonName") {
                    s_CommonName = *s_Entry.value.As<ZString>();
                }
                else if (s_Entry.sKey == "HeroDisguiseAvailable") {
                    s_HeroDisguiseAvailable = *s_Entry.value.As<bool>();
                }
                else if (s_Entry.sKey == "IsHitmanSuit") {
                    s_IsHitmanSuit = *s_Entry.value.As<bool>();
                    s_IsOufit = true;
                }
            }

            if (s_Id.IsEmpty() || !s_IsOufit) {
                continue;
            }

            if (s_CommonName.IsEmpty()) {
                s_FinalName = std::format("<unnamed> [{}]", s_Id.c_str());
            }
            else {
                s_FinalName = std::format("{} [{}]", s_CommonName.c_str(), s_Id.c_str());
            }

            m_AllRepositoryOutfits.push_back(std::make_tuple(s_Id, s_FinalName));

            if (s_HeroDisguiseAvailable) {
                m_PlayerOutfits.insert(s_Id);
            }

            if (!s_IsHitmanSuit && s_CommonName != "CHAR_Hokkaido_Hero_NinjaSuit_M_HPA2700") {
                m_ActorOutfits.insert(s_Id);
            }
        }
    }

    std::ranges::sort(
        m_AllRepositoryOutfits,
        [](const auto& a, const auto& b) {
            auto [s_RepositoryIdA, s_NameA] = a;
            auto [s_RepositoryIdB, s_NameB] = b;

            std::ranges::transform(s_NameA, s_NameA.begin(), [](unsigned char c) { return std::tolower(c); });
            std::ranges::transform(s_NameB, s_NameB.begin(), [](unsigned char c) { return std::tolower(c); });

            return s_NameA < s_NameB;
        }
    );
}

void Randomizer::FilterRepositoryProps() {
    m_PropsToSpawnInWorld.clear();
    m_PropsToSpawnInStash.clear();
    m_PropsToSpawnInPlayerInventory.clear();
    m_PropsToSpawnInActorInventory.clear();
    m_WeaponsToSpawnInActorInventory.clear();
    ZWorldInventory* inv = Globals::WorldInventory;
    if (m_PropsToSpawn.empty()) {
        for (const auto& [s_RepositoryId, s_Name, s_IsWeapon, s_InventoryCategoryName] : m_AllRepositoryProps) {
            if (s_Name.starts_with("Gadget_Camera") || s_Name.contains("Gadget_Camera_Tagging")) {
                continue;
            }

            if (!m_InventoryCategoryToState[s_InventoryCategoryName]) {
                continue;
            }

            if (!s_IsWeapon && !m_RandomizeItems) {
                continue;
            }

            if (s_IsWeapon && !m_RandomizeWeapons) {
                continue;
            }

            if (m_ExcludedPropRepositoryIds.contains(s_RepositoryId)) {
                continue;
            }

            if (m_RandomizeWorldProps) {
                m_PropsToSpawnInWorld.push_back(s_RepositoryId);
            }

            if (m_RandomizeStashProps) {
                m_PropsToSpawnInStash.push_back(s_RepositoryId);
            }

            if (m_RandomizePlayerInventory) {
                m_PropsToSpawnInPlayerInventory.push_back(s_RepositoryId);
            }

            if (m_RandomizeActorInventory) {
                m_PropsToSpawnInActorInventory.push_back(s_RepositoryId);

                if (s_IsWeapon) {
                    m_WeaponsToSpawnInActorInventory.push_back(s_RepositoryId);
                }
            }
        }
    }
    else {
        for (const auto& s_PropToSpawn : m_PropsToSpawn) {
            auto& [
                s_RepositoryId,
                s_Name,
                s_SpawnInWorld,
                s_SpawnInStash,
                s_SpawnInPlayerInventory,
                s_SpawnInActorInventory
            ] = s_PropToSpawn;

            if (s_SpawnInWorld) {
                m_PropsToSpawnInWorld.push_back(s_RepositoryId);
            }

            if (s_SpawnInStash) {
                m_PropsToSpawnInStash.push_back(s_RepositoryId);
            }

            if (s_SpawnInPlayerInventory) {
                m_PropsToSpawnInPlayerInventory.push_back(s_RepositoryId);
            }

            if (s_SpawnInActorInventory) {
                m_PropsToSpawnInActorInventory.push_back(s_RepositoryId);

                if (m_RepositoryWeapons.contains(s_RepositoryId)) {
                    m_WeaponsToSpawnInActorInventory.push_back(s_RepositoryId);
                }
            }
        }
    }
}

void Randomizer::FilterRepositoryOutfits() {
    if (m_GlobalOutfitRepositoryIds.empty()) {
        BuildGlobalOutfitRepositoryIdsFromBricks();
    }

    if (m_CurrentSceneOutfitRepositoryIds.empty()) {
        BuildCurrentSceneOutfitRepositoryIds();

        if (m_OutfitsToSpawn.empty()) {
            LoadOutfitsForSelectedScenes();
        }
        else {
            for (const auto& s_OutfitBrickRuntimeResourceId : m_OutfitBricksToLoad) {
                LoadOutfits(s_OutfitBrickRuntimeResourceId);
            }
        }
    }

    m_OutfitsToSpawnForPlayer.clear();
    m_MaleCivilianOutfitsToSpawnForActor.clear();
    m_FemaleCivilianOutfitsToSpawnForActor.clear();
    m_MaleGuardOutfitsToSpawnForActor.clear();
    m_FemaleGuardOutfitsToSpawnForActor.clear();
    m_OutfitsToSpawnForClothBundle.clear();

    ZString s_CurrentSceneName = GetCurrentSceneName();

    ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;

    if (m_OutfitsToSpawn.empty()) {
        for (const auto& [s_RepositoryId, s_GlobalOutfitKit] : s_ContentKitManager->m_repositoryGlobalOutfitKits) {
            const ZString& s_CommonName = s_GlobalOutfitKit.m_pInterfaceRef->m_sCommonName;

            if (s_CommonName == "Base" ||
                s_CommonName == "Base_BodyguardMale" ||
                s_CommonName == "Base_CivFem" ||
                s_CommonName == "Base_CivFemHeel" ||
                s_CommonName == "Base_CivMale" ||
                s_CommonName == "Base_CopMale" ||
                s_CommonName == "Base_GuardMale" ||
                s_CommonName == "Debug_Missing Outfit_M" ||
                s_CommonName == "Debug_Missing Outfit_M01" ||
                s_CommonName == "Debug_Missing Outfit_M02" ||
                s_CommonName == "Crowd_Civilian_European_Casual_Male") {
                continue;
            }

            if (m_ExcludedOutfitRepositoryIds.contains(s_RepositoryId)) {
                continue;
            }

            if (m_RandomizeOutfitsFromOtherScenes &&
                !s_CurrentSceneName.IsEmpty() &&
                !m_SelectedScenes.contains(s_CurrentSceneName.c_str()) &&
                m_CurrentSceneOutfitRepositoryIds.contains(s_RepositoryId)) {
                continue;
            }

            if (m_RandomizePlayerOutfit) {
                m_OutfitsToSpawnForPlayer.push_back(s_RepositoryId);
            }

            if (m_RandomizeActorOutfit &&
                s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets.size() > 0 &&
                !s_GlobalOutfitKit.m_pInterfaceRef->m_bIsHitmanSuit) {
                ZOutfitVariationCollection* s_Collection =
                    s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[0].m_pInterfaceRef;
                ZCharsetCharacterType* s_CharsetCharacterType =
                    s_Collection->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_Actor)].
                    m_pInterfaceRef;

                if (s_CharsetCharacterType->m_aVariations[0].m_pInterfaceRef->m_Outfit != -1) {
                    if (s_GlobalOutfitKit.m_pInterfaceRef->m_bIsFemale) {
                        if (s_GlobalOutfitKit.m_pInterfaceRef->m_eActorType == EActorType::eAT_Civilian) {
                            m_FemaleCivilianOutfitsToSpawnForActor.push_back(s_RepositoryId);
                        }
                        else {
                            m_FemaleGuardOutfitsToSpawnForActor.push_back(s_RepositoryId);
                        }
                    }
                    else {
                        if (s_GlobalOutfitKit.m_pInterfaceRef->m_eActorType == EActorType::eAT_Civilian) {
                            m_MaleCivilianOutfitsToSpawnForActor.push_back(s_RepositoryId);
                        }
                        else {
                            m_MaleGuardOutfitsToSpawnForActor.push_back(s_RepositoryId);
                        }
                    }
                }
            }

            if (m_RandomizeClothBundleOutfit) {
                m_OutfitsToSpawnForClothBundle.push_back(s_RepositoryId);
            }
        }
    }
    else {
        for (const auto& s_OutfitToSpawn : m_OutfitsToSpawn) {
            auto& [
                s_RepositoryId,
                s_Name,
                s_SpawnForPlayer,
                s_SpawnForActor,
                s_SpawnForClothBundle
            ] = s_OutfitToSpawn;

            if (s_SpawnForPlayer) {
                m_OutfitsToSpawnForPlayer.push_back(s_RepositoryId);
            }

            if (s_SpawnForActor) {
                auto s_Iterator = s_ContentKitManager->m_repositoryGlobalOutfitKits.find(s_RepositoryId);

                if (s_Iterator != s_ContentKitManager->m_repositoryGlobalOutfitKits.end()) {
                    if (s_Iterator->second.m_pInterfaceRef->m_bIsFemale) {
                        if (s_Iterator->second.m_pInterfaceRef->m_eActorType == EActorType::eAT_Civilian) {
                            m_FemaleCivilianOutfitsToSpawnForActor.push_back(s_RepositoryId);
                        }
                        else {
                            m_FemaleGuardOutfitsToSpawnForActor.push_back(s_RepositoryId);
                        }
                    }
                    else {
                        if (s_Iterator->second.m_pInterfaceRef->m_eActorType == EActorType::eAT_Civilian) {
                            m_MaleCivilianOutfitsToSpawnForActor.push_back(s_RepositoryId);
                        }
                        else {
                            m_MaleGuardOutfitsToSpawnForActor.push_back(s_RepositoryId);
                        }
                    }
                }
            }

            if (s_SpawnForClothBundle) {
                m_OutfitsToSpawnForClothBundle.push_back(s_RepositoryId);
            }
        }
    }
}

const ZRepositoryID& Randomizer::GetRandomRepositoryId(const std::vector<ZRepositoryID>& p_RepositoryIds) {
    static const ZRepositoryID s_Empty {};

    if (p_RepositoryIds.empty()) {
        return s_Empty;
    }

    static std::mt19937 s_RandomEngine { std::random_device{}() };

    std::uniform_int_distribution<size_t> s_Distribution(
        0,
        p_RepositoryIds.size() - 1
    );

    return p_RepositoryIds[s_Distribution(s_RandomEngine)];
}

const ZRepositoryID Randomizer::GetRandomEntranceId() {
    static const ZRepositoryID s_Empty{};

    const auto s_EntitySceneContext = Globals::Hitman5Module->m_pEntitySceneContext;

    if (!s_EntitySceneContext) {
        return s_Empty;
    }

    const std::string s_CurrentScene = Util::StringUtils::ToLowerCase(
        s_EntitySceneContext->m_SceneInitParameters.m_SceneResource.c_str()
    );

    TArray<ZDynamicObject>* s_Entrances = nullptr;
    bool s_IsSceneFound = false;

    for (const auto& s_UIMapConfig : Globals::UIMapManager->m_aMapConfigs) {
        ZWorldMapMetaData& s_WorldMapMetaData = s_UIMapConfig.m_pInterfaceRef->m_metaData;
        const TArray<SDynamicObjectKeyValuePair>* s_Entries = s_WorldMapMetaData.m_data.As<TArray<
            SDynamicObjectKeyValuePair>>();

        if (!s_Entries) {
            continue;
        }

        for (const auto& s_Entry : *s_Entries) {
            if (s_Entry.sKey == "entrances" && s_IsSceneFound) {
                s_Entrances = s_Entry.value.As<TArray<ZDynamicObject>>();
                break;
            }
            else if (s_Entry.sKey == "scene") {
                ZString* s_ScenePath = s_Entry.value.As<ZString>();

                if (*s_ScenePath == s_CurrentScene) {
                    s_IsSceneFound = true;
                }
            }
        }

        if (s_IsSceneFound) {
            break;
        }
    }

    if (!s_Entrances) {
        return s_Empty;
    }

    static std::mt19937 s_RandomEngine{ std::random_device{}() };

    std::uniform_int_distribution<size_t> s_Distribution(
        0,
        s_Entrances->size() - 1
    );

    size_t s_Index = s_Distribution(s_RandomEngine);
    const auto s_Entries = (*s_Entrances)[s_Index].As<TArray<SDynamicObjectKeyValuePair>>();

    for (const auto& s_Entry : *s_Entries) {
        if (s_Entry.sKey == "id") {
            return ZRepositoryID(*s_Entry.value.As<ZString>());
        }
    }

    return s_Empty;
}

const int32_t Randomizer::GetRandomCharacterSetIndex(
    const TArray<TEntityRef<ZOutfitVariationCollection>>& p_CharSets
) {
    static std::mt19937 s_RandomEngine{ std::random_device{}() };

    std::uniform_int_distribution<size_t> s_Distribution(
        0,
        p_CharSets.size() - 1
    );

    return s_Distribution(s_RandomEngine);
}

const int32_t Randomizer::GetRandomOutfitVariationIndex(
    const TArray<TEntityRef<ZOutfitVariation>> p_Variations
) {
    std::vector<ZRuntimeResourceID> s_OutfitVariations;

    s_OutfitVariations.reserve(p_Variations.size());

    for (const auto& s_Variation : p_Variations) {
        if (s_Variation.m_pInterfaceRef->m_Outfit != -1) {
            s_OutfitVariations.push_back(s_Variation.m_pInterfaceRef->m_Outfit);
        }
    }

    static std::mt19937 s_RandomEngine{ std::random_device{}() };

    std::uniform_int_distribution<size_t> s_Distribution(
        0,
        s_OutfitVariations.size() - 1
    );

    return s_Distribution(s_RandomEngine);
}

void Randomizer::LoadCategoriesFromSettings() {
    for (auto& [s_InventoryCategory, s_IsEnabled] : m_InventoryCategoryToState) {
        const std::string s_InventoryCategory2 = Util::StringUtils::ToLowerCase(s_InventoryCategory);

        if (!HasSetting("categories", s_InventoryCategory2)) {
            SetSettingBool("categories", s_InventoryCategory2, true);
        }

        s_IsEnabled = GetSettingBool("categories", s_InventoryCategory2, true);
    }
}

void Randomizer::LoadPropsToSpawnFromSettings() {
    std::unordered_set<ZRepositoryID> s_PropsToSpawnInWorld;
    std::unordered_set<ZRepositoryID> s_PropsToSpawnInStash;
    std::unordered_set<ZRepositoryID> s_PropsToSpawnInPlayerInventory;
    std::unordered_set<ZRepositoryID> s_PropsToSpawnInActorInventory;

    if (HasSetting("props_to_spawn", "spawn_in_world")) {
        ParseRepositoryIdCsv(
            GetSetting("props_to_spawn", "spawn_in_world", "").c_str(),
            s_PropsToSpawnInWorld
        );
    }

    if (HasSetting("props_to_spawn", "spawn_in_stash")) {
        ParseRepositoryIdCsv(
            GetSetting("props_to_spawn", "spawn_in_stash", "").c_str(),
            s_PropsToSpawnInStash
        );
    }

    if (HasSetting("props_to_spawn", "spawn_in_player_inventory")) {
        ParseRepositoryIdCsv(
            GetSetting("props_to_spawn", "spawn_in_player_inventory", "").c_str(),
            s_PropsToSpawnInPlayerInventory
        );
    }

    if (HasSetting("props_to_spawn", "spawn_in_actor_inventory")) {
        ParseRepositoryIdCsv(
            GetSetting("props_to_spawn", "spawn_in_actor_inventory", "").c_str(),
            s_PropsToSpawnInActorInventory
        );
    }

    for (const auto& [s_RepositoryId, s_Name, s_IsWeapon, s_InventoryCategoryIcon] : m_AllRepositoryProps) {
        bool s_SpawnInWorld = false;
        bool s_SpawnInStash = false;
        bool s_SpawnInPlayerInventory = false;
        bool s_SpawnInActorInventory = false;

        if (s_PropsToSpawnInWorld.contains(s_RepositoryId)) {
            s_SpawnInWorld = true;
        }
        else if (s_PropsToSpawnInWorld.contains(s_RepositoryId)) {
            s_SpawnInStash = true;
        }
        else if (s_PropsToSpawnInWorld.contains(s_RepositoryId)) {
            s_SpawnInPlayerInventory = true;
        }
        else if (s_PropsToSpawnInWorld.contains(s_RepositoryId)) {
            s_SpawnInActorInventory = true;
        }
        else {
            continue;
        }

        m_PropsToSpawn.push_back(std::make_tuple(
            s_RepositoryId,
            s_Name,
            s_SpawnInWorld,
            s_SpawnInStash,
            s_SpawnInPlayerInventory,
            s_SpawnInActorInventory
        ));
    }
}

void Randomizer::LoadPropsToExcludeFromSettings() {
    for (const auto& [s_RepositoryId, s_Name, s_IsWeapon, s_InventoryCategoryIcon] : m_AllRepositoryProps) {
        const ZString s_SettingName = Util::StringUtils::ToLowerCase(s_RepositoryId.ToString().c_str());

        if (!HasSetting("props_to_exclude", s_SettingName)) {
            continue;
        }

        m_PropsToExclude.push_back(std::make_pair(s_RepositoryId, s_Name));
        m_ExcludedPropRepositoryIds.insert(s_RepositoryId);
    }
}

void Randomizer::LoadOutfitSettings() {
    if (!HasSetting("outfits", "randomize_player_outfit")) {
        SetSettingBool("outfits", "randomize_player_outfit", true);
    }

    if (!HasSetting("outfits", "randomize_actor_outfit")) {
        SetSettingBool("outfits", "randomize_actor_outfit", true);
    }

    if (!HasSetting("outfits", "randomize_cloth_budle_outfit")) {
        SetSettingBool("outfits", "randomize_cloth_budle_outfit", true);
    }

    if (!HasSetting("outfits", "randomize_civilian_outfit")) {
        SetSettingBool("outfits", "randomize_civilian_outfit", true);
    }

    if (!HasSetting("outfits", "randomize_guard_outfit")) {
        SetSettingBool("outfits", "randomize_guard_outfit", true);
    }

    if (!HasSetting("outfits", "randomize_character_set_index")) {
        SetSettingBool("outfits", "randomize_character_set_index", true);
    }

    if (!HasSetting("outfits", "randomize_outfit_variation")) {
        SetSettingBool("outfits", "randomize_outfit_variation", true);
    }

    m_RandomizePlayerOutfit = GetSettingBool("outfits", "randomize_player_outfit", true);
    m_RandomizeActorOutfit = GetSettingBool("outfits", "randomize_actor_outfit", true);
    m_RandomizeClothBundleOutfit = GetSettingBool("outfits", "randomize_cloth_budle_outfit", true);

    m_RandomizeCivilianOutfit = GetSettingBool("outfits", "randomize_civilian_outfit", true);
    m_RandomizeGuardOutfit = GetSettingBool("outfits", "randomize_guard_outfit", true);

    m_RandomizeCharacterSetIndex = GetSettingBool("outfits", "randomize_civilian_outfit", true);
    m_RandomizeOutfitVariation = GetSettingBool("outfits", "randomize_guard_outfit", true);
}

void Randomizer::LoadOutfitsFromOtherScenesFromSettings() {
    if (!HasSetting("outfits_from_other_scenes", "randomize_outfits_from_other_scenes")) {
        SetSettingBool("outfits_from_other_scenes", "randomize_outfits_from_other_scenes", false);
    }

    if (!HasSetting("outfits_from_other_scenes", "randomize_season_2_global_outfits")) {
        SetSettingBool("outfits_from_other_scenes", "randomize_season_2_global_outfits", true);
    }

    if (!HasSetting("outfits_from_other_scenes", "randomize_season_3_global_outfits")) {
        SetSettingBool("outfits_from_other_scenes", "randomize_season_3_global_outfits", true);
    }

    m_RandomizeOutfitsFromOtherScenes = GetSettingBool("outfits_from_other_scenes", "randomize_outfits_from_other_scenes", false);
    m_RandomizeSeason2GlobalOutfits = GetSettingBool("outfits_from_other_scenes", "randomize_season_2_global_outfits", true);
    m_RandomizeSeason3GlobalOutfits = GetSettingBool("outfits_from_other_scenes", "randomize_season_3_global_outfits", true);

    for (const auto& [s_SceneName, s_SceneRuntimeResourceIds] : m_Scenes) {
        if (!HasSetting("outfits_from_other_scenes", s_SceneName)) {
            SetSettingBool("outfits_from_other_scenes", s_SceneName, false);
        }

        bool s_IsSceneSelected = GetSettingBool(
            "outfits_from_other_scenes",
            Util::StringUtils::ToLowerCase(s_SceneName),
            false
        );

        if (s_IsSceneSelected) {
            m_SelectedScenes.insert(s_SceneName);
        }
    }
}

void Randomizer::LoadOutfitsToSpawnFromSettings() {
    std::unordered_set<ZRepositoryID> s_OufitsToSpawnForPlayer;
    std::unordered_set<ZRepositoryID> s_OufitsToSpawnForActor;
    std::unordered_set<ZRepositoryID> s_OufitsToSpawnForClothBundle;

    if (HasSetting("outfits_to_spawn", "spawn_for_player")) {
        ParseRepositoryIdCsv(
            GetSetting("outfits_to_spawn", "spawn_for_player", "").c_str(),
            s_OufitsToSpawnForPlayer
        );
    }

    if (HasSetting("outfits_to_spawn", "spawn_for_actor")) {
        ParseRepositoryIdCsv(
            GetSetting("outfits_to_spawn", "spawn_for_actor", "").c_str(),
            s_OufitsToSpawnForActor
        );
    }

    if (HasSetting("outfits_to_spawn", "spawn_for_cloth_bundle")) {
        ParseRepositoryIdCsv(
            GetSetting("outfits_to_spawn", "spawn_for_cloth_bundle", "").c_str(),
            s_OufitsToSpawnForClothBundle
        );
    }

    for (const auto& [s_RepositoryId, s_Name] : m_AllRepositoryOutfits) {
        bool s_SpawnForPlayer = false;
        bool s_SpawnForActor = false;
        bool s_SpawnForClothBundle = false;

        if (s_OufitsToSpawnForPlayer.contains(s_RepositoryId)) {
            s_SpawnForPlayer = true;
        }
        else if (s_OufitsToSpawnForActor.contains(s_RepositoryId)) {
            s_SpawnForActor = true;
        }
        else if (s_OufitsToSpawnForClothBundle.contains(s_RepositoryId)) {
            s_SpawnForClothBundle = true;
        }
        else {
            continue;
        }

        m_OutfitsToSpawn.push_back(std::make_tuple(
            s_RepositoryId,
            s_Name,
            s_SpawnForPlayer,
            s_SpawnForActor,
            s_SpawnForClothBundle
        ));
    }
}

void Randomizer::LoadOutfitsToExcludeFromSettings() {
    for (const auto& [s_RepositoryId, s_Name] : m_AllRepositoryOutfits) {
        const ZString s_SettingName = Util::StringUtils::ToLowerCase(s_RepositoryId.ToString().c_str());

        if (!HasSetting("outfits_to_exclude", s_SettingName)) {
            continue;
        }

        m_OutfitsToExclude.push_back(std::make_pair(s_RepositoryId, s_Name));
        m_ExcludedOutfitRepositoryIds.insert(s_RepositoryId);
    }
}

void Randomizer::UpdatePropRepositoryIdListSetting(
    const ZString& p_Section,
    const ZString& p_Key
) {
    std::string s_Value;

    for (const auto& m_PropToSpawn : m_PropsToSpawn) {
        auto& [
            s_RepositoryId,
            s_Name,
            s_SpawnInWorld,
            s_SpawnInStash,
            s_SpawnInPlayerInventory,
            s_SpawnInActorInventory
        ] = m_PropToSpawn;

        if (p_Key == "spawn_in_world" && !s_SpawnInWorld ||
            p_Key == "spawn_in_stash" && !s_SpawnInStash ||
            p_Key == "spawn_in_player_inventory" && !s_SpawnInPlayerInventory ||
            p_Key == "spawn_in_actor_inventory" && !s_SpawnInActorInventory) {
            continue;
        }

        if (!s_Value.empty()) {
            s_Value += ",";
        }

        s_Value += Util::StringUtils::ToLowerCase(s_RepositoryId.ToString().c_str());
    }

    if (s_Value.empty()) {
        RemoveSetting(p_Section, p_Key);
    }
    else {
        SetSetting(p_Section, p_Key, s_Value);
    }
}

void Randomizer::UpdateOutfitRepositoryIdListSetting(
    const ZString& p_Section,
    const ZString& p_Key
) {
    std::string s_Value;

    for (const auto& s_OutfitToSpawn : m_OutfitsToSpawn) {
        auto& [
            s_RepositoryId,
            s_Name,
            s_SpawnForPlayer,
            s_SpawnForActor,
            s_SpawnForClothBundle
        ] = s_OutfitToSpawn;

        if (p_Key == "spawn_for_player" && !s_SpawnForPlayer ||
            p_Key == "spawn_for_actor" && !s_SpawnForActor ||
            p_Key == "spawn_for_cloth_bundle" && !s_SpawnForClothBundle) {
            continue;
        }

        if (!s_Value.empty()) {
            s_Value += ",";
        }

        s_Value += Util::StringUtils::ToLowerCase(s_RepositoryId.ToString().c_str());
    }

    if (s_Value.empty()) {
        RemoveSetting(p_Section, p_Key);
    }
    else {
        SetSetting(p_Section, p_Key, s_Value);
    }
}

void Randomizer::ParseRepositoryIdCsv(const std::string& p_Value, std::unordered_set<ZRepositoryID>& p_RepositoryIds) {
    size_t s_Start = 0;

    while (s_Start < p_Value.size()) {
        size_t s_End = p_Value.find(',', s_Start);

        if (s_End == std::string::npos) {
            s_End = p_Value.size();
        }

        const std::string s_Token = p_Value.substr(s_Start, s_End - s_Start);

        if (!s_Token.empty()) {
            p_RepositoryIds.insert(ZRepositoryID(s_Token));
        }

        s_Start = s_End + 1;
    }
}

void Randomizer::BuildCurrentSceneOutfitRepositoryIds() {
    for (const auto& [s_RepositoryId, s_GlobalOutfitKit] : Globals::ContentKitManager->m_repositoryGlobalOutfitKits) {
        if (m_GlobalOutfitRepositoryIds.contains(s_RepositoryId) ||
            m_Season2GlobalOutfitRepositoryIds.contains(s_RepositoryId) ||
            m_Season3GlobalOutfitRepositoryIds.contains(s_RepositoryId)) {
            continue;
        }

        m_CurrentSceneOutfitRepositoryIds.insert(s_RepositoryId);
    }
}

void Randomizer::BuildGlobalOutfitRepositoryIdsFromBricks() {
    for (const auto& s_Brick : Globals::Hitman5Module->m_pEntitySceneContext->m_aLoadedBricks) {
        if (s_Brick.m_RuntimeResourceID == ResId<"[assembly:/_pro/scenes/bricks/globaldata.brick].pc_entitytype">) {
            FindGlobalOutfitRepositoryIds(
                s_Brick.m_EntityRef,
                ResId<"[assembly:/_pro/scenes/bricks/globaldata.brick].pc_entityblueprint">,
                m_GlobalOutfitRepositoryIds
            );
        }
        if (s_Brick.m_RuntimeResourceID == ResId<"[assembly:/_pro/scenes/bricks/globaldata_s2.brick].pc_entitytype">) {
            FindGlobalOutfitRepositoryIds(
                s_Brick.m_EntityRef,
                ResId<"[assembly:/_pro/scenes/bricks/globaldata_s2.brick].pc_entityblueprint">,
                m_Season2GlobalOutfitRepositoryIds
            );
        }
        else if (s_Brick.m_RuntimeResourceID == ResId<"[assembly:/_pro/scenes/bricks/globaldata_s3.brick].pc_entitytype">) {
            FindGlobalOutfitRepositoryIds(
                s_Brick.m_EntityRef,
                ResId<"[assembly:/_pro/scenes/bricks/globaldata_s3.brick].pc_entityblueprint">,
                m_Season3GlobalOutfitRepositoryIds
            );
        }
    }
}

void Randomizer::BuildSceneNamesToRuntimeResourceIds() {
    const ZRuntimeResourceID s_ConfigRuntimeResourceID =
        ResId<"[assembly:/_pro/online/default/offlineconfig/config.contracts].pc_contracts">;
    ZResourcePtr s_ConfigResourcePtr;

    Globals::ResourceManager->GetResourcePtr(s_ConfigResourcePtr, s_ConfigRuntimeResourceID, 0);

    const ZResourceContainer::SResourceInfo& s_ConfigResourceInfo = s_ConfigResourcePtr.GetResourceInfo();

    for (size_t i = 0; i < s_ConfigResourceInfo.numReferences; ++i) {
        const uint32_t s_JsonReferenceIndex =
            (*Globals::ResourceContainer)->m_references[s_ConfigResourceInfo.firstReferenceIndex + i].index;
        const ZResourceContainer::SResourceInfo& s_JsonReferenceInfo =
            (*Globals::ResourceContainer)->m_resources[s_JsonReferenceIndex];

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

void Randomizer::BuildSceneToOutfitBrickRuntimeResourceIds(const std::string& p_SceneName, const ZRuntimeResourceID& p_SceneRuntimeResourceId) {
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

void Randomizer::BuildChunkIndexToResourcePackageCount() {
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

        auto s_ChunkIndex = Util::ResourceUtils::TryParseChunkIndexFromResourcePackagePath(s_Path.string());

        if (!s_ChunkIndex) {
            continue;
        }

        m_ChunkIndexToResourcePackageCount[*s_ChunkIndex]++;
    }
}

bool Randomizer::FindOutfitReferencesRecursive(
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

bool Randomizer::LoadBrick(
    const ZRuntimeResourceID& p_BrickRuntimeResourceId,
    TResourcePtr<ZTemplateEntityFactory>& p_ResourcePtr,
    ZEntityRef& p_EntityRef
) {
    Globals::ResourceManager->LoadResource(p_ResourcePtr, p_BrickRuntimeResourceId);

    while (!Globals::ResourceManager->DoneLoading()) {
        Logger::Debug("Waiting for resources to load (left: {})!", Globals::ResourceManager->m_nNumProcessing);
        Globals::ResourceManager->Update(true);
    }

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

    while (!p_ResourcePtr.GetResource()->m_blueprintResource.GetResource()->AreAllResourcesReady(p_EntityRef.m_pEntity)) {
        Logger::Debug("Waiting for resources to load (left: {})!", Globals::ResourceManager->m_nNumProcessing);
        Globals::ResourceManager->Update(true);
    }

    return true;
}

void Randomizer::LoadGlobalDataBrick(const ZRuntimeResourceID& p_BrickRuntimeResourceId) {
    TResourcePtr<ZTemplateEntityFactory> s_ResourcePtr;
    ZEntityRef s_EntityRef;

    if (LoadBrick(p_BrickRuntimeResourceId, s_ResourcePtr, s_EntityRef)) {
        m_LoadedGlobalOutfitBricks.insert(
            std::make_pair(p_BrickRuntimeResourceId, std::make_pair(s_ResourcePtr, s_EntityRef))
        );
    }
}

void Randomizer::LoadOutfits(const ZRuntimeResourceID& p_OutfitsBrickRuntimeResourceId) {
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

    m_LoadedOutfitBricks.push_back(std::make_pair(s_ResourcePtr, s_EntityRef));
}

void Randomizer::LoadOutfits(const std::unordered_set<std::string>& p_Scenes) {
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
            LoadOutfits(s_OutfitBrickRuntimeResourceId);
        }
    }
}

void Randomizer::LoadOutfitsForSelectedScenes() {
    LoadOutfits(m_SelectedScenes);

    for (const auto& s_SceneName : m_SelectedScenes) {
        m_LoadedScenes.insert(s_SceneName);
    }
}

void Randomizer::UnloadOutfits() {
    SExternalReferences s_ExternalRefs;

    for (const auto& s_Brick : Globals::Hitman5Module->m_pEntitySceneContext->m_aLoadedBricks) {
        if (s_Brick.m_RuntimeResourceID == ResId<"[assembly:/_pro/scenes/bricks/globaldata.brick].pc_entitytype">) {
            s_ExternalRefs.externalScenes.insert(s_Brick.m_RuntimeResourceID, s_Brick.m_EntityRef);
            break;
        }
    }

    for (const auto& [s_ResourcePtr, s_EntityRef] : m_LoadedOutfitBricks) {
        Functions::ZEntityManager_DeleteEntity->Call(
            Globals::EntityManager,
            s_EntityRef,
            s_ExternalRefs
        );
    }
}

std::string Randomizer::ToEntityTemplatePath(const std::string_view p_ScenePath) {
    std::string s_NormalizedPath(p_ScenePath);

    std::transform(s_NormalizedPath.begin(), s_NormalizedPath.end(), s_NormalizedPath.begin(), ::tolower);

    return std::format("[{}].pc_entitytemplate", s_NormalizedPath);
}

std::filesystem::path Randomizer::GetRuntimeDirectory() {
    char s_ExePathStr[MAX_PATH]{};

    GetModuleFileNameA(nullptr, s_ExePathStr, MAX_PATH);

    std::filesystem::path s_ExePath(s_ExePathStr);

    return s_ExePath.parent_path().parent_path() / "Runtime";
}

bool Randomizer::IsResourcePackageLimitExceeded() {
    if (m_ChunkIndexToResourcePackageCount.empty()) {
        BuildChunkIndexToResourcePackageCount();
    }

    m_PendingChunks.clear();

    for (auto& s_PartitionInfo : (*Globals::PackageManager)->m_aPartitionInfos) {
        if (SDK()->IsChunkMounted(s_PartitionInfo->m_nIndex)) {
            m_PendingChunks.insert(s_PartitionInfo->m_nIndex);
        }
    }

    for (const auto& s_SelectedScene : m_SelectedScenes) {
        for (const auto& s_SceneRuntimeResourceId : m_Scenes[s_SelectedScene]) {
            const auto& s_ChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(s_SceneRuntimeResourceId);

            for (uint32_t s_ChunkIndex : s_ChunkIndices) {
                m_PendingChunks.insert(s_ChunkIndex);
            }
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
        return true;
    }

    return false;
}

void Randomizer::FindGlobalOutfitRepositoryIds(
    const ZEntityRef& p_BrickEntityRef,
    const ZRuntimeResourceID& p_BrickBlueprintRuntimeResourceId,
    std::unordered_set<ZRepositoryID>& m_GlobalOutfitRepositoryIds
) {
    TResourcePtr<ZTemplateEntityBlueprintFactory> p_ResourcePtr;

    Globals::ResourceManager->LoadResource(p_ResourcePtr, p_BrickBlueprintRuntimeResourceId);

    const auto s_SubEntityCount = p_ResourcePtr.GetResource()->GetSubEntitiesCount();

    for (int64_t i = 0; i < s_SubEntityCount; ++i) {
        const ZEntityRef s_SubEntity = p_ResourcePtr.GetResource()->GetSubEntity(p_BrickEntityRef.m_pEntity, i);
        ZGlobalOutfitKit* s_GlobalOutfitKit = s_SubEntity.QueryInterface<ZGlobalOutfitKit>();

        if (s_GlobalOutfitKit) {
            m_GlobalOutfitRepositoryIds.insert(s_GlobalOutfitKit->m_sId);
        }
    }
}

bool Randomizer::TryFindOutfitWithBonesAndCollisionResourceProperty(
    const TArray<TEntityRef<ZOutfitVariationCollection>>& p_CharSets,
    int32_t& p_OutCharacterSetIndex,
    int32_t& p_OutVariationIndex
) const {
    if (p_CharSets.size() == 0) {
        return false;
    }

    static std::mt19937 s_RandomEngine{ std::random_device{}() };

    std::vector<int32_t> s_RemainingCharSetIndices;

    s_RemainingCharSetIndices.reserve(p_CharSets.size());

    for (int32_t i = 0; i < p_CharSets.size(); ++i) {
        s_RemainingCharSetIndices.push_back(i);
    }

    while (!s_RemainingCharSetIndices.empty()) {
        std::uniform_int_distribution<size_t> s_Distribution(
            0,
            s_RemainingCharSetIndices.size() - 1
        );

        const size_t s_Index = s_Distribution(s_RandomEngine);
        const int32_t s_CharSetIndex = s_RemainingCharSetIndices[s_Index];

        ZOutfitVariationCollection* s_Collection = p_CharSets[s_CharSetIndex].m_pInterfaceRef;

        if (!s_Collection || s_Collection->m_aCharacters.size() == 0) {
            s_RemainingCharSetIndices[s_Index] = s_RemainingCharSetIndices.back();
            s_RemainingCharSetIndices.pop_back();
            continue;
        }

        const size_t s_ActorIndex = static_cast<size_t>(ECharSetCharacterType::ECSCT_Actor);
        auto* s_ActorCharsetCharacterType = s_Collection->m_aCharacters[s_ActorIndex].m_pInterfaceRef;

        if (!s_ActorCharsetCharacterType || s_ActorCharsetCharacterType->m_aVariations.size() == 0) {
            s_RemainingCharSetIndices[s_Index] = s_RemainingCharSetIndices.back();
            s_RemainingCharSetIndices.pop_back();
            continue;
        }

        int32_t s_VariationIndex = 0;

        if (m_RandomizeCharacterSetIndex) {
            s_VariationIndex = GetRandomOutfitVariationIndex(s_ActorCharsetCharacterType->m_aVariations);
        }

        const ZRuntimeResourceID s_OutfitRuntimeResourceID =
            s_ActorCharsetCharacterType->m_aVariations[s_VariationIndex]
            .m_pInterfaceRef->m_Outfit;

        if (HasBonesAndCollisionResourceProperty(s_OutfitRuntimeResourceID)) {
            p_OutCharacterSetIndex = s_CharSetIndex;
            p_OutVariationIndex = s_VariationIndex;
            return true;
        }

        s_RemainingCharSetIndices[s_Index] = s_RemainingCharSetIndices.back();
        s_RemainingCharSetIndices.pop_back();
    }

    return false;
}

bool Randomizer::HasBonesAndCollisionResourceProperty(const ZRuntimeResourceID& s_OutfitRuntimeResourceID) {
    TResourcePtr<ZTemplateEntityFactory> p_ResourcePtr;

    Globals::ResourceManager->LoadResource(p_ResourcePtr, s_OutfitRuntimeResourceID);

    ZTemplateEntityFactory* s_TemplateEntityFactory = p_ResourcePtr.GetResource();
    const STemplateFactorySubEntity& s_RootEntity = s_TemplateEntityFactory->m_pResourceData->subEntities
        [s_TemplateEntityFactory->m_rootEntityIndex];

    for (const auto& s_Property : s_RootEntity.propertyValues) {
        if (strcmp(s_Property.value.GetTypeID()->typeInfo()->m_pTypeName, "ZRuntimeResourceID") == 0) {
            std::string s_PropertyName;
            const auto s_PropertyNameData = HM3_GetPropertyName(s_Property.nPropertyID);

            if (s_PropertyNameData.Size > 0) {
                s_PropertyName.assign(s_PropertyNameData.Data, s_PropertyNameData.Size);
            }

            if (s_PropertyName == "m_pBonesAndCollisionResource") {
                return true;
            }
        }
    }

    return false;
}

void Randomizer::FindAndMountChunksForOutfitsToSpawn(const ZString& p_CurrentSceneResource) {
    std::unordered_set<ZRepositoryID> s_OutfitsToFind;

    for (const auto& s_OutfitToSpawn : m_OutfitsToSpawn) {
        auto& [
            s_RepositoryId,
            s_Name,
            s_SpawnForPlayer,
            s_SpawnForActor,
            s_SpawnForClothBundle
        ] = s_OutfitToSpawn;

        s_OutfitsToFind.insert(s_RepositoryId);
    }

    size_t s_FoundOutfitCount = 0;

    for (const auto& s_ResourceInfo : (*Globals::ResourceContainer)->m_resources) {
        if (ResourceContainsOutfitsToSpawn(
            s_ResourceInfo,
            s_OutfitsToFind,
            s_FoundOutfitCount
        )) {
            m_OutfitBricksToLoad.push_back(s_ResourceInfo.rid);
        }

        if (s_FoundOutfitCount == s_OutfitsToFind.size()) {
            return;
        }
    }

    const std::string s_EntityTemplatePath = ToEntityTemplatePath(p_CurrentSceneResource);
    const ZRuntimeResourceID s_CurrentSceneRuntimeResourceId = ZRuntimeResourceID::FromString(s_EntityTemplatePath);
    const auto& s_CurrentSceneChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(
        s_CurrentSceneRuntimeResourceId
    );

    size_t s_StartIndex = (*Globals::ResourceContainer)->m_resources.size();

    for (const auto& [s_SceneName, s_SceneRuntimeResourceIds] : m_Scenes) {
        const auto& s_ChunkIndices = SDK()->GetChunkIndicesForRuntimeResourceId(*s_SceneRuntimeResourceIds.begin());

        // Case like assembly:/_PRO/Scenes/Missions/TheFacility/_Scene_Mission_Polarbear_Module_002_C.entity
        if (s_ChunkIndices.size() == 0) {
            return;
        }

        if (!SDK()->IsChunkMounted(s_ChunkIndices[0])) {
            SDK()->MountChunk(s_ChunkIndices[0]);
        }

        bool s_ChunkContainsOutfitToSpawn = false;

        for (size_t i = s_StartIndex; i < (*Globals::ResourceContainer)->m_resources.size(); ++i) {
            if (ResourceContainsOutfitsToSpawn(
                (*Globals::ResourceContainer)->m_resources[i],
                s_OutfitsToFind,
                s_FoundOutfitCount
            )) {
                m_OutfitBricksToLoad.push_back((*Globals::ResourceContainer)->m_resources[i].rid);

                s_ChunkContainsOutfitToSpawn = true;
            }

            if (s_FoundOutfitCount == s_OutfitsToFind.size()) {
                break;
            }
        }

        if (!s_ChunkContainsOutfitToSpawn &&
            s_ChunkIndices[0] != s_CurrentSceneChunkIndices[0] &&
            s_ChunkIndices[0] != 0) {
            SDK()->UnmountChunk(s_ChunkIndices[0], false);
        }
        else {
            s_StartIndex = (*Globals::ResourceContainer)->m_resources.size();
        }

        if (s_FoundOutfitCount == s_OutfitsToFind.size()) {
            return;
        }
    }
}

bool Randomizer::ResourceContainsOutfitsToSpawn(
    const ZResourceContainer::SResourceInfo& p_ResourceInfo,
    const std::unordered_set<ZRepositoryID>& p_OutfitsToFind,
    size_t& p_FoundOutfitCount
) {
    bool s_IsOutfitBrickFound = false;

    for (size_t i = 0; i < p_ResourceInfo.numReferences; ++i) {
        const uint32_t s_ReferenceIndex =
            (*Globals::ResourceContainer)->m_references[p_ResourceInfo.firstReferenceIndex + i].index;
        const ZResourceContainer::SResourceInfo& s_ReferenceInfo =
            (*Globals::ResourceContainer)->m_resources[s_ReferenceIndex];

        if (s_ReferenceInfo.resourceType == 'CPPT' &&
            s_ReferenceInfo.rid == ResId<"[modules:/zglobaloutfitkit.class].pc_entitytype">) {
            s_IsOutfitBrickFound = true;
            break;
        }
    }

    if (!s_IsOutfitBrickFound) {
        return false;
    }

    TResourcePtr<ZTemplateEntityFactory> s_ResourcePtr;

    Globals::ResourceManager->LoadResource(s_ResourcePtr, p_ResourceInfo.rid);

    ZTemplateEntityFactory* s_TemplateEntityFactory = s_ResourcePtr.GetResource();

    bool s_IsAnyOutfitFound = false;

    for (const auto& s_SubEntity : s_TemplateEntityFactory->m_pResourceData->subEntities) {
        const uint32_t s_ReferenceIndex =
            (*Globals::ResourceContainer)->m_references
            [p_ResourceInfo.firstReferenceIndex + s_SubEntity.entityTypeResourceIndex].index;
        const ZResourceContainer::SResourceInfo& s_ReferenceInfo =
            (*Globals::ResourceContainer)->m_resources[s_ReferenceIndex];

        if (s_ReferenceInfo.resourceType == 'CPPT' &&
            s_ReferenceInfo.rid != ResId<"[modules:/zglobaloutfitkit.class].pc_entitytype">) {
            continue;
        }

        for (const auto& s_Property : s_SubEntity.propertyValues) {
            if (strcmp(s_Property.value.GetTypeID()->typeInfo()->m_pTypeName, "ZGuid") == 0) {
                std::string s_PropertyName;
                const auto s_PropertyNameData = HM3_GetPropertyName(s_Property.nPropertyID);

                if (s_PropertyNameData.Size > 0) {
                    s_PropertyName.assign(s_PropertyNameData.Data, s_PropertyNameData.Size);
                }

                const ZRepositoryID s_RepositoryId = ZRepositoryID(s_Property.value.As<ZGuid>()->ToString());

                if (s_PropertyName == "m_sId" && p_OutfitsToFind.contains(s_RepositoryId)) {
                    ++p_FoundOutfitCount;

                    s_IsAnyOutfitFound = true;
                }
            }
        }
    }

    return s_IsAnyOutfitFound;
}

ZString Randomizer::GetCurrentSceneName() {
    std::string s_LocationKey = std::format(
        "UI_{}_CITY",
        Globals::ContractsManager->m_contractContext.m_sLocationId.c_str()
    );
    const uint32_t s_LocationHash = Hash::Crc32(s_LocationKey.data(), s_LocationKey.size());

    ZString s_CurrentSceneName;
    int s_OutMarkupResult;

    Hooks::ZUIText_TryGetTextFromNameHash->Call(
        Globals::UIText,
        s_LocationHash,
        s_CurrentSceneName,
        s_OutMarkupResult
    );

    return s_CurrentSceneName;
}

DEFINE_PLUGIN_DETOUR(Randomizer, bool, OnLoadScene, ZEntitySceneContext* th, SSceneInitParameters& parameters) {
    if (!m_IsRandomizerEnabled) {
        return HookResult<bool>(HookAction::Continue());
    }

    m_IsRandomizerAllowedForScene = parameters.m_Type != "evergreen" &&
        parameters.m_SceneResource != "assembly:/_PRO/Scenes/Frontend/MainMenu.entity";

    if (m_AllRepositoryProps.empty()) {
        LoadRepositoryProps();
        LoadRepositoryOutfits();

        LoadCategoriesFromSettings();
        LoadPropsToSpawnFromSettings();
        LoadPropsToExcludeFromSettings();

        LoadOutfitSettings();
        LoadOutfitsToSpawnFromSettings();
        LoadOutfitsToExcludeFromSettings();
    }

    if (m_RandomizeProps &&
        (m_RandomizeWorldProps || m_RandomizeStashProps || m_RandomizePlayerInventory || m_RandomizeActorInventory)) {
        FilterRepositoryProps();
    }

    if (m_RandomizeOutfits && m_IsRandomizerAllowedForScene && !m_OutfitsToSpawn.empty()) {
        FindAndMountChunksForOutfitsToSpawn(parameters.m_SceneResource);
    }

    return HookResult<bool>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Randomizer, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene) {
    if (th->m_SceneInitParameters.m_SceneResource == "assembly:/_PRO/Scenes/Frontend/MainMenu.entity" &&
        m_Scenes.empty()) {
        BuildSceneNamesToRuntimeResourceIds();

        LoadOutfitsFromOtherScenesFromSettings();
    }

    m_CurrentSceneOutfitRepositoryIds.clear();
    m_LoadedGlobalOutfitBricks.clear();
    m_LoadedScenes.clear();

    if (!m_LoadedOutfitBricks.empty()) {
        UnloadOutfits();
    }

    m_AreOutfitsFiltered = false;

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Randomizer, void, ZLevelManager_StartGame, ZLevelManager* th) {
    if (!m_IsRandomizerEnabled || !m_IsRandomizerAllowedForScene) {
        return HookResult<void>(HookAction::Continue());
    }

    if (m_RandomizeEntrance) {
        Globals::ContractsManager->m_contractContext.m_EnabledEntranceId = GetRandomEntranceId();
    }

    if (m_RandomizeProps && m_RandomizeStashProps) {
        SAgencyPickupInfo& s_AgencyPickupInfo = Globals::ContractsManager->m_sAgencyPickup;

        for (const auto& s_UIMapTracker : Globals::UIMapTrackerManager->m_aUIMapTrackers) {
            auto s_GenericUIMapTrackerAspect = static_cast<ZGenericUIMapTrackerAspect*>(s_UIMapTracker.m_pInterfaceRef);

            if (s_GenericUIMapTrackerAspect->m_sIconId != "stashpoint") {
                continue;
            }

            auto s_StashPointTrackerAspect = static_cast<ZStashPointTrackerAspect*>(s_UIMapTracker.m_pInterfaceRef);
            auto s_StashPointEntity = s_StashPointTrackerAspect->m_pStashPoint.m_pInterfaceRef;

            s_AgencyPickupInfo.m_AgencyPickupId = s_StashPointEntity->m_sId;

            for (size_t i = 0; i < m_RepositoryPropSpawnCount; ++i) {
                s_AgencyPickupInfo.m_aItemIds.push_back(GetRandomRepositoryId(m_PropsToSpawnInStash));
            }

            Functions::ZStashPointEntity_RequestContentLoad->Call(s_StashPointEntity);

            Globals::ContractsManager->m_sAgencyPickup.m_aItemIds.clear();
        }

        s_AgencyPickupInfo.m_AgencyPickupId = ZRepositoryID();
        s_AgencyPickupInfo.m_aItemIds.clear();
    }

    if (m_RandomizeOutfitsFromOtherScenes && m_RandomizeSeason2GlobalOutfits || m_RandomizeSeason3GlobalOutfits) {
        bool m_IsGlobalDataSeason2BrickLoaded = false;
        bool m_IsGlobalDataSeason3BrickLoaded = false;

        for (const auto& s_Brick : Globals::Hitman5Module->m_pEntitySceneContext->m_aLoadedBricks) {
            if (m_RandomizeSeason2GlobalOutfits &&
                s_Brick.m_RuntimeResourceID == ResId<"[assembly:/_pro/scenes/bricks/globaldata_s2.brick].pc_entitytype">) {
                m_IsGlobalDataSeason2BrickLoaded = true;
                break;
            }
            else if (m_RandomizeSeason3GlobalOutfits &&
                s_Brick.m_RuntimeResourceID == ResId<"[assembly:/_pro/scenes/bricks/globaldata_s3.brick].pc_entitytype">) {
                m_IsGlobalDataSeason3BrickLoaded = true;
                break;
            }
        }

        if (m_RandomizeSeason2GlobalOutfits && !m_IsGlobalDataSeason2BrickLoaded) {
            LoadGlobalDataBrick(
                ResId<"[assembly:/_pro/scenes/bricks/globaldata_s2.brick].pc_entitytype">
            );
        }

        if (m_RandomizeSeason3GlobalOutfits && !m_IsGlobalDataSeason3BrickLoaded) {
            LoadGlobalDataBrick(
                ResId<"[assembly:/_pro/scenes/bricks/globaldata_s3.brick].pc_entitytype">
            );
        }
    }

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Randomizer, void, ZItemSpawner_RequestContentLoad, ZItemSpawner* th) {
    if (!m_IsRandomizerEnabled || !m_IsRandomizerAllowedForScene || !m_RandomizeProps || !m_RandomizeWorldProps) {
        return HookResult<void>(HookAction::Continue());
    }

    if (th->m_rMainItemKey) {
        const ZRepositoryID s_RepositoryId = GetRandomRepositoryId(m_PropsToSpawnInWorld);

        if (!s_RepositoryId.IsEmpty()) {
            th->m_rMainItemKey.m_pInterfaceRef->m_RepositoryId = s_RepositoryId;
        }
    }

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(
    Randomizer,
    ZCharacterSubcontrollerInventory::SCreateItem*,
    ZCharacterSubcontrollerInventory_CreateItem,
    ZCharacterSubcontrollerInventory* th,
    ZRepositoryID& repId,
    const ZString& sOnlineInstanceId,
    const TArray<ZRepositoryID>& instanceModifiersToApply,
    ZCharacterSubcontrollerInventory::ECreateItemType createItemType
) {
    if (!m_IsRandomizerEnabled || !m_IsRandomizerAllowedForScene || !m_RandomizeProps || !m_RandomizePlayerInventory) {
        return HookResult<ZCharacterSubcontrollerInventory::SCreateItem*>(HookAction::Continue());
    }

    const ZRepositoryID s_RepositoryId = GetRandomRepositoryId(m_PropsToSpawnInPlayerInventory);

    if (!s_RepositoryId.IsEmpty()) {
        repId = s_RepositoryId;
    }

    return HookResult<ZCharacterSubcontrollerInventory::SCreateItem*>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Randomizer, bool, ZActorInventoryHandler_RequestItem, ZActorInventoryHandler* th, ZRepositoryID& id) {
    if (!m_IsRandomizerEnabled || !m_IsRandomizerAllowedForScene || !m_RandomizeProps || !m_RandomizeActorInventory) {
        return HookResult<bool>(HookAction::Continue());
    }

    ZRepositoryID s_RepositoryId;
    bool s_IsWeapon = false;

    if (m_RepositoryWeapons.contains(id)) {
        s_RepositoryId = GetRandomRepositoryId(m_WeaponsToSpawnInActorInventory);
        s_IsWeapon = true;
    }
    else {
        s_RepositoryId = GetRandomRepositoryId(m_PropsToSpawnInActorInventory);
    }

    if (!s_RepositoryId.IsEmpty()) {
        id = s_RepositoryId;

        if (s_IsWeapon) {
            th->m_rWeaponID = id;
        }
    }

    return HookResult<bool>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(
    Randomizer,
    void,
    ZHitman5_SetOutfit,
    ZHitman5* th,
    TEntityRef<ZGlobalOutfitKit> rOutfitKit,
    int32_t nCharset,
    int32_t nVariation,
    bool bEnableOutfitModifiers,
    bool bIgnoreOutifChange
) {
    if (!m_IsRandomizerEnabled ||
        !m_IsRandomizerAllowedForScene ||
        !m_RandomizeOutfits ||
        !m_RandomizePlayerOutfit ||
        rOutfitKit.m_pInterfaceRef->m_sId != Globals::ContractsManager->m_contractContext.m_StartupDisguiseId) {
        return HookResult<void>(HookAction::Continue());
    }

    if (!m_AreOutfitsFiltered) {
        FilterRepositoryOutfits();

        m_AreOutfitsFiltered = true;
    }

    ZRepositoryID s_RepositoryId = GetRandomRepositoryId(m_OutfitsToSpawnForPlayer);
    ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;
    auto s_Iterator = s_ContentKitManager->m_repositoryGlobalOutfitKits.find(s_RepositoryId);

    if (s_Iterator == s_ContentKitManager->m_repositoryGlobalOutfitKits.end()) {
        return HookResult<void>(HookAction::Continue());
    }

    TEntityRef<ZGlobalOutfitKit> s_GlobalOutfitKit = s_Iterator->second;
    std::string s_CharSetCharacterType = "HeroA";
    int32_t s_CharacterSetIndex = nCharset;
    int32_t s_VariationIndex = nVariation;

    std::vector<ZRuntimeResourceID> s_OriginalHeroVariations;

    if (s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets.size() > 0) {
        if (m_RandomizeCharacterSetIndex) {
            s_CharacterSetIndex = GetRandomCharacterSetIndex(s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets);
        }

        if (m_RandomizeOutfitVariation) {
            ZOutfitVariationCollection* s_Collection =
                s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[s_CharacterSetIndex].m_pInterfaceRef;

            if (s_Collection->m_aCharacters.size() > 0) {
                s_VariationIndex = GetRandomOutfitVariationIndex(
                    s_Collection->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_HeroA)].
                    m_pInterfaceRef->m_aVariations
                );
            }
        }
    }

    p_Hook->CallOriginal(
        th,
        s_GlobalOutfitKit,
        s_CharacterSetIndex,
        s_VariationIndex,
        bEnableOutfitModifiers,
        bIgnoreOutifChange
    );

    return HookResult<void>(HookAction::Return());
}

DEFINE_PLUGIN_DETOUR(
    Randomizer,
    void,
    ZActor_SetOutfit,
    ZActor* th,
    TEntityRef<ZGlobalOutfitKit> rOutfit,
    int32_t charset,
    int32_t variation,
    bool bNude
) {
    if (!m_IsRandomizerEnabled ||
        !m_IsRandomizerAllowedForScene ||
        !m_RandomizeOutfits ||
        !m_RandomizeActorOutfit) {
        return HookResult<void>(HookAction::Continue());
    }

    if (!m_AreOutfitsFiltered) {
        FilterRepositoryOutfits();

        m_AreOutfitsFiltered = true;
    }

    ZRepositoryID s_RepositoryId;

    if (rOutfit.m_pInterfaceRef->m_bIsFemale) {
        if (rOutfit.m_pInterfaceRef->m_eActorType == EActorType::eAT_Civilian) {
            if (m_RandomizeCivilianOutfit) {
                s_RepositoryId = GetRandomRepositoryId(m_FemaleCivilianOutfitsToSpawnForActor);
            }
        }
        else {
            if (m_RandomizeGuardOutfit) {
                s_RepositoryId = GetRandomRepositoryId(m_FemaleGuardOutfitsToSpawnForActor);
            }
        }
    }
    else {
        if (rOutfit.m_pInterfaceRef->m_eActorType == EActorType::eAT_Civilian) {
            if (m_RandomizeCivilianOutfit) {
                s_RepositoryId = GetRandomRepositoryId(m_MaleCivilianOutfitsToSpawnForActor);
            }
        }
        else {
            if (m_RandomizeGuardOutfit) {
                s_RepositoryId = GetRandomRepositoryId(m_MaleGuardOutfitsToSpawnForActor);
            }
        }
    }

    ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;
    auto s_Iterator = s_ContentKitManager->m_repositoryGlobalOutfitKits.find(s_RepositoryId);

    if (s_Iterator == s_ContentKitManager->m_repositoryGlobalOutfitKits.end()) {
        return HookResult<void>(HookAction::Continue());
    }

    TEntityRef<ZGlobalOutfitKit> s_GlobalOutfitKit = s_Iterator->second;
    std::string s_CharSetCharacterType = "Actor";
    int32_t s_CharacterSetIndex = charset;
    int32_t s_VariationIndex = variation;

    std::vector<ZRuntimeResourceID> s_OriginalHeroVariations;

    if (s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets.size() > 0) {
        if (m_RandomizeCharacterSetIndex) {
            TryFindOutfitWithBonesAndCollisionResourceProperty(
                s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets,
                s_CharacterSetIndex,
                s_VariationIndex
            );
        }

        if (m_RandomizeOutfitVariation && !m_RandomizeCharacterSetIndex) {
            ZOutfitVariationCollection* s_Collection =
                s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[s_CharacterSetIndex].m_pInterfaceRef;

            if (s_Collection->m_aCharacters.size() > 0) {
                s_VariationIndex = GetRandomOutfitVariationIndex(
                    s_Collection->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_HeroA)].
                    m_pInterfaceRef->m_aVariations
                );
            }
        }
    }

    p_Hook->CallOriginal(
        th,
        s_GlobalOutfitKit,
        s_CharacterSetIndex,
        s_VariationIndex,
        bNude
    );

    return HookResult<void>(HookAction::Return());
}

DEFINE_PLUGIN_DETOUR(
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
) {
    if (!m_IsRandomizerEnabled ||
        !m_IsRandomizerAllowedForScene ||
        !m_RandomizeOutfits ||
        !m_RandomizeClothBundleOutfit) {
        return HookResult<TEntityRef<ZClothBundleEntity>*>(HookAction::Continue());
    }

    if (!m_AreOutfitsFiltered) {
        FilterRepositoryOutfits();

        m_AreOutfitsFiltered = true;
    }

    id = GetRandomRepositoryId(m_OutfitsToSpawnForClothBundle);

    ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;
    auto s_Iterator = s_ContentKitManager->m_repositoryGlobalOutfitKits.find(id);

    if (s_Iterator == s_ContentKitManager->m_repositoryGlobalOutfitKits.end()) {
        p_Hook->CallOriginal(result, mat, id, nOutfitVariation, nOutfitCharset, bSpawnedByHitman, bEnableOutfitModifiers);

        return HookResult<TEntityRef<ZClothBundleEntity>*>(HookAction::Return(), &result);
    }

    TEntityRef<ZGlobalOutfitKit> s_GlobalOutfitKit = s_Iterator->second;

    int32_t s_CharacterSetIndex = nOutfitCharset;
    int32_t s_VariationIndex = nOutfitVariation;

    if (s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets.size() > 0) {
        if (m_RandomizeCharacterSetIndex) {
            s_CharacterSetIndex = GetRandomCharacterSetIndex(s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets);
        }

        if (m_RandomizeOutfitVariation) {
            ZOutfitVariationCollection* s_Collection =
                s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[s_CharacterSetIndex].m_pInterfaceRef;

            s_VariationIndex = GetRandomOutfitVariationIndex(
                s_Collection->m_aCharacters[2].m_pInterfaceRef->m_aVariations
            );
        }
    }

    p_Hook->CallOriginal(result, mat, id, nOutfitVariation, nOutfitCharset, bSpawnedByHitman, bEnableOutfitModifiers);

    return HookResult<TEntityRef<ZClothBundleEntity>*>(HookAction::Return(), &result);
}

DEFINE_ZHM_PLUGIN(Randomizer);