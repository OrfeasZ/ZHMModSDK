#include "Randomizer.h"

#include "IconsMaterialDesign.h"

#include <random>

#include "Glacier/ZItem.h"
#include "Glacier/ZActor.h"
#include "Glacier/ZUIMapTracker.h"
#include "Glacier/ZStash.h"
#include "Glacier/ZContract.h"
#include "Glacier/ZScene.h"
#include "Glacier/IEnumType.h"

#include "Util/ImGuiUtils.h"

void Randomizer::Init() {
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &Randomizer::OnLoadScene);
    Hooks::ZLevelManager_StartGame->AddDetour(this, &Randomizer::ZLevelManager_StartGame);

    Hooks::ZItemSpawner_RequestContentLoad->AddDetour(this, &Randomizer::ZItemSpawner_RequestContentLoad);
    Hooks::ZCharacterSubcontrollerInventory_CreateItem->AddDetour(
        this,
        &Randomizer::ZCharacterSubcontrollerInventory_CreateItem
    );
    Hooks::ZActorInventoryHandler_RequestItem->AddDetour(this, &Randomizer::ZActorInventoryHandler_RequestItem);

    if (!HasSetting("general", "enable_randomizer")) {
        SetSettingBool("general", "enable_randomizer", true);
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

    m_IsRandomizerEnabled = GetSettingBool("general", "enable_randomizer", false);

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
        if (m_AllRepositoryProps.empty()) {
            LoadRepositoryProps();
            LoadCategoriesFromSettings();
            LoadPropsToSpawnFromSettings();
            LoadPropsToExcludeFromSettings();
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

    ImGui::Text("Inventories");
    ImGui::Spacing();

    if (ImGui::Checkbox("Randomize World Props", &m_RandomizeWorldProps)) {
        SetSettingBool("general", "randomize_world_props", m_RandomizeWorldProps);
    }

    if (ImGui::Checkbox("Randomize Stash Props", &m_RandomizeStashProps)) {
        SetSettingBool("general", "randomize_stash_props", m_RandomizeStashProps);
    }

    if (ImGui::Checkbox("Randomize Player Inventory", &m_RandomizePlayerInventory)) {
        SetSettingBool("general", "randomize_player_inventory", m_RandomizePlayerInventory);
    }

    if (ImGui::Checkbox("Randomize Actor Inventory", &m_RandomizeActorInventory)) {
        SetSettingBool("general", "randomize_actor_inventory", m_RandomizeActorInventory);
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "At least one weapon category must be selected,\n"
            "or at least one weapon must be added to the props to spawn list."
        );
    }

    ImGui::Separator();

    if (ImGui::Checkbox("Randomize Items", &m_RandomizeItems)) {
        SetSettingBool("general", "randomize_items", m_RandomizeItems);
    }

    if (ImGui::Checkbox("Randomize Weapons", &m_RandomizeWeapons)) {
        SetSettingBool("general", "randomize_weapons", m_RandomizeWeapons);
    }

    ImGui::Separator();

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Number Of Props To Spawn In Stashes");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetFrameHeight() * 5.f);

    if (ImGui::InputInt("##RepositoryPropSpawnCount", &m_RepositoryPropSpawnCount)) {
        SetSettingInt("general", "number_of_props_to_spawn_in_stashes", m_RepositoryPropSpawnCount);
    }
}

void Randomizer::DrawCategoriesTab() {
    for (auto& [s_InventoryCategory, s_IsEnabled] : m_InventoryCategoryToState) {
        if (ImGui::Checkbox(s_InventoryCategory.c_str(), &s_IsEnabled)) {
            SetSettingBool("categories", Util::StringUtils::ToLowerCase(s_InventoryCategory), s_IsEnabled);
        }
    }
}

void Randomizer::DrawPropsToSpawnTab() {
    ImGui::BeginDisabled(!m_RandomizeWorldProps);

    if (ImGui::Checkbox("Spawn In World", &m_SpawnInWorld)) {
        SetSettingBool("props_to_spawn", "spawn_in_world", m_SpawnInWorld);
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeStashProps);

    if (ImGui::Checkbox("Spawn In Stash", &m_SpawnInStash)) {
        SetSettingBool("props_to_spawn", "spawn_in_stash", m_SpawnInStash);
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizePlayerInventory);

    if (ImGui::Checkbox("Spawn In PlayerInventory", &m_SpawnInPlayerInventory)) {
        SetSettingBool("props_to_spawn", "spawn_in_player_inventory", m_SpawnInPlayerInventory);
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!m_RandomizeActorInventory);

    if (ImGui::Checkbox("Spawn In ActorInventory", &m_SpawnInActorInventory)) {
        SetSettingBool("props_to_spawn", "spawn_in_actor_inventory", m_SpawnInActorInventory);
    }

    ImGui::EndDisabled();

    static char s_PropTitle[2048]{ "" };

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Prop Title");
    ImGui::SameLine();

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

            m_PropsToSpawn.push_back(std::make_tuple(
                p_Id,
                p_Name,
                m_SpawnInWorld,
                m_SpawnInStash,
                m_SpawnInPlayerInventory,
                m_SpawnInActorInventory
            ));

            const std::string s_SettingValue = std::format(
                "{},{},{},{}",
                m_SpawnInWorld ? "true" : "false",
                m_SpawnInStash ? "true" : "false",
                m_SpawnInPlayerInventory ? "true" : "false",
                m_SpawnInActorInventory ? "true" : "false"
            );

            SetSetting("props_to_spawn", p_Name, s_SettingValue);
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

            return false;
        }
    );

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

        ImGui::BeginDisabled(!m_RandomizeWorldProps);

        if (ImGui::Checkbox("Spawn In World", &s_SpawnInWorld)) {
            const std::string s_SettingValue = std::format(
                "{},{},{},{}",
                s_SpawnInWorld ? "true" : "false",
                s_SpawnInStash ? "true" : "false",
                s_SpawnInPlayerInventory ? "true" : "false",
                s_SpawnInActorInventory ? "true" : "false"
            );

            SetSetting("props_to_spawn", s_Name, s_SettingValue);
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizeStashProps);

        if (ImGui::Checkbox("Spawn In Stash", &s_SpawnInStash)) {
            const std::string s_SettingValue = std::format(
                "{},{},{},{}",
                s_SpawnInWorld ? "true" : "false",
                s_SpawnInStash ? "true" : "false",
                s_SpawnInPlayerInventory ? "true" : "false",
                s_SpawnInActorInventory ? "true" : "false"
            );

            SetSetting("props_to_spawn", s_Name, s_SettingValue);
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizePlayerInventory);

        if (ImGui::Checkbox("Spawn In PlayerInventory", &s_SpawnInPlayerInventory)) {
            const std::string s_SettingValue = std::format(
                "{},{},{},{}",
                s_SpawnInWorld ? "true" : "false",
                s_SpawnInStash ? "true" : "false",
                s_SpawnInPlayerInventory ? "true" : "false",
                s_SpawnInActorInventory ? "true" : "false"
            );

            SetSetting("props_to_spawn", s_Name, s_SettingValue);
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!m_RandomizeActorInventory);

        if (ImGui::Checkbox("Spawn In ActorInventory", &s_SpawnInActorInventory)) {
            const std::string s_SettingValue = std::format(
                "{},{},{},{}",
                s_SpawnInWorld ? "true" : "false",
                s_SpawnInStash ? "true" : "false",
                s_SpawnInPlayerInventory ? "true" : "false",
                s_SpawnInActorInventory ? "true" : "false"
            );

            SetSetting("props_to_spawn", s_Name, s_SettingValue);
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::SmallButton(ICON_MD_DELETE)) {
            RemoveSetting("props_to_spawn", s_Name);

            m_PropsToSpawn.erase(m_PropsToSpawn.begin() + i);

            ImGui::PopID();

            break;
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Remove");
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    if (!m_PropsToSpawn.empty()) {
        if (ImGui::Button("Clear All")) {
            for (const auto& s_PropToSpawn : m_PropsToSpawn) {
                RemoveSetting("props_to_spawn", std::get<1>(s_PropToSpawn));
            }

            m_PropsToSpawn.clear();
        }
    }
}

void Randomizer::DrawPropsToExcludeTab() {
    static char s_PropTitle[2048]{ "" };

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Prop Title");
    ImGui::SameLine();

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

            SetSettingBool("props_to_exclude", p_Name, true);
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

            return false;
        }
    );

    ImGui::TextUnformatted("Props:");
    ImGui::BeginChild("PropList", ImVec2(0, 250), true, ImGuiWindowFlags_HorizontalScrollbar);

    for (size_t i = 0; i < m_PropsToExclude.size(); ++i) {
        auto& [s_RepositoryId, s_Name] = m_PropsToExclude[i];

        ImGui::PushID(static_cast<int>(i));

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(s_Name.c_str());

        ImGui::SameLine();

        if (ImGui::SmallButton(ICON_MD_DELETE)) {
            RemoveSetting("props_to_exclude", s_Name);

            m_PropsToExclude.erase(m_PropsToExclude.begin() + i);
            m_ExcludedPropRepositoryIds.erase(s_RepositoryId);

            ImGui::PopID();

            break;
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Remove");
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    if (!m_PropsToExclude.empty()) {
        if (ImGui::Button("Clear All")) {
            for (const auto& [s_RepositoryId, s_Name] : m_PropsToExclude) {
                RemoveSetting("props_to_exclude", s_Name);
            }

            m_PropsToExclude.clear();
            m_ExcludedPropRepositoryIds.clear();
        }
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

void Randomizer::FilterRepositoryProps() {
    m_PropsToSpawnInWorld.clear();
    m_PropsToSpawnInStash.clear();
    m_PropsToSpawnInPlayerInventory.clear();
    m_PropsToSpawnInActorInventory.clear();
    m_WeaponsToSpawnInActorInventory.clear();

    if (m_PropsToSpawn.empty()) {
        for (const auto& [s_RepositoryId, s_Name, s_IsWeapon, s_InventoryCategoryName] : m_AllRepositoryProps) {
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

const ZRepositoryID& Randomizer::GetRandomRepositoryId(const std::vector<ZRepositoryID>& p_Props) {
    static const ZRepositoryID s_Empty {};

    if (p_Props.empty()) {
        return s_Empty;
    }

    static std::mt19937 s_RandomEngine { std::random_device{}() };

    std::uniform_int_distribution<size_t> s_Distribution(
        0,
        p_Props.size() - 1
    );

    return p_Props[s_Distribution(s_RandomEngine)];
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
    if (!HasSetting("props_to_spawn", "spawn_in_world")) {
        SetSettingBool("props_to_spawn", "spawn_in_world", true);
    }

    if (!HasSetting("props_to_spawn", "spawn_in_stash")) {
        SetSettingBool("props_to_spawn", "spawn_in_stash", true);
    }

    if (!HasSetting("props_to_spawn", "spawn_in_player_inventory")) {
        SetSettingBool("props_to_spawn", "spawn_in_player_inventory", true);
    }

    if (!HasSetting("props_to_spawn", "spawn_in_actor_inventory")) {
        SetSettingBool("props_to_spawn", "spawn_in_actor_inventory", true);
    }

    m_SpawnInWorld = GetSettingBool("props_to_spawn", "spawn_in_world", true);
    m_SpawnInStash = GetSettingBool("props_to_spawn", "spawn_in_stash", true);
    m_SpawnInPlayerInventory = GetSettingBool("props_to_spawn", "spawn_in_player_inventory", true);
    m_SpawnInActorInventory = GetSettingBool("props_to_spawn", "spawn_in_actor_inventory", true);

    for (const auto& [s_RepositoryId, s_Name, s_IsWeapon, s_InventoryCategoryIcon] : m_AllRepositoryProps) {
        const ZString s_SettingName = Util::StringUtils::ToLowerCase(s_Name);

        if (!HasSetting("props_to_spawn", s_SettingName)) {
            continue;
        }

        const std::string s_SettingValue = GetSetting("props_to_spawn", s_SettingName, "").c_str();

        if (s_SettingValue.empty()) {
            continue;
        }

        bool s_SpawnInWorld = false;
        bool s_SpawnInStash = false;
        bool s_SpawnInPlayerInventory = false;
        bool s_SpawnInActorInventory = false;

        size_t s_TokenBegin = 0;
        size_t s_Index = 0;

        while (s_Index < 4) {
            const size_t s_TokenEnd = s_SettingValue.find(',', s_TokenBegin);

            const std::string s_Token =
                (s_TokenEnd == std::string::npos)
                ? s_SettingValue.substr(s_TokenBegin)
                : s_SettingValue.substr(s_TokenBegin, s_TokenEnd - s_TokenBegin);

            const bool s_Value = s_Token == "true";

            switch (s_Index) {
                case 0: s_SpawnInWorld = s_Value; break;
                case 1: s_SpawnInStash = s_Value; break;
                case 2: s_SpawnInPlayerInventory = s_Value; break;
                case 3: s_SpawnInActorInventory = s_Value; break;
            }

            if (s_TokenEnd == std::string::npos) {
                break;
            }

            s_TokenBegin = s_TokenEnd + 1;
            ++s_Index;
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
        const ZString s_SettingName = Util::StringUtils::ToLowerCase(s_Name);

        if (!HasSetting("props_to_exclude", s_SettingName)) {
            continue;
        }

        m_PropsToExclude.push_back(std::make_pair(s_RepositoryId, s_Name));
        m_ExcludedPropRepositoryIds.insert(s_RepositoryId);
    }
}

DEFINE_PLUGIN_DETOUR(Randomizer, bool, OnLoadScene, ZEntitySceneContext* th, SSceneInitParameters& parameters) {
    m_IsRandomizerAllowedForScene = parameters.m_Type != "evergreen";

    if (m_AllRepositoryProps.empty()) {
        LoadRepositoryProps();
        LoadCategoriesFromSettings();
        LoadPropsToSpawnFromSettings();
        LoadPropsToExcludeFromSettings();
    }
    
    FilterRepositoryProps();

    return HookResult<bool>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Randomizer, void, ZLevelManager_StartGame, ZLevelManager* th) {
    if (!m_IsRandomizerEnabled || !m_IsRandomizerAllowedForScene || !m_RandomizeStashProps) {
        return HookResult<void>(HookAction::Continue());
    }

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

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Randomizer, void, ZItemSpawner_RequestContentLoad, ZItemSpawner* th) {
    if (!m_IsRandomizerEnabled || !m_IsRandomizerAllowedForScene || !m_RandomizeWorldProps) {
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
    if (!m_IsRandomizerEnabled || !m_IsRandomizerAllowedForScene || !m_RandomizePlayerInventory) {
        return HookResult<ZCharacterSubcontrollerInventory::SCreateItem*>(HookAction::Continue());
    }

    const ZRepositoryID s_RepositoryId = GetRandomRepositoryId(m_PropsToSpawnInPlayerInventory);

    if (!s_RepositoryId.IsEmpty()) {
        repId = s_RepositoryId;
    }

    return HookResult<ZCharacterSubcontrollerInventory::SCreateItem*>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Randomizer, bool, ZActorInventoryHandler_RequestItem, ZActorInventoryHandler* th, ZRepositoryID& id) {
    if (!m_IsRandomizerEnabled || !m_IsRandomizerAllowedForScene || !m_RandomizeActorInventory) {
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

DEFINE_ZHM_PLUGIN(Randomizer);