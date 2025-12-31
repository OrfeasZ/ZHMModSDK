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
        }

        ImGui::Checkbox("Enable Randomizer", &m_IsRandomizerEnabled);

        ImGui::Separator();

        ImGui::Text("Inventories");
        ImGui::Spacing();

        ImGui::Checkbox("Randomize World Props", &m_RandomizeWorldProps);
        ImGui::Checkbox("Randomize Stash Props", &m_RandomizeStashProps);
        ImGui::Checkbox("Randomize Player Inventory", &m_RandomizePlayerInventory);
        ImGui::Checkbox("Randomize Actor Inventory", &m_RandomizeActorInventory);

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "At least one weapon category must be selected,\n"
                "or at least one weapon must be added to the props to spawn list."
            );
        }

        ImGui::Separator();

        ImGui::Checkbox("Randomize Items", &m_RandomizeItems);
        ImGui::Checkbox("Randomize Weapons", &m_RandomizeWeapons);

        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Number Of Props To Spawn In Stashes");
        ImGui::SameLine();

        ImGui::InputInt("##RepositoryPropSpawnCount", &m_RepositoryPropSpawnCount);

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Categories")) {
            for (auto& [s_InventoryCategory, s_IsEnabled] : m_InventoryCategoryToState) {
                ImGui::Checkbox(s_InventoryCategory.c_str(), &s_IsEnabled);
            }
        }

        ImGui::Separator();

        ImGui::Text("Props To Spawn");
        ImGui::Spacing();

        ImGui::BeginDisabled(!m_RandomizeWorldProps);
        ImGui::Checkbox("Spawn In World", &m_SpawnInWorld);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!m_RandomizeStashProps);
        ImGui::Checkbox("Spawn In Stash", &m_SpawnInStash);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!m_RandomizePlayerInventory);
        ImGui::Checkbox("Spawn In PlayerInventory", &m_SpawnInPlayerInventory);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!m_RandomizeActorInventory);
        ImGui::Checkbox("Spawn In ActorInventory", &m_SpawnInActorInventory);
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
            ImGui::Checkbox("Spawn In World", &s_SpawnInWorld);
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(!m_RandomizeStashProps);
            ImGui::Checkbox("Spawn In Stash", &s_SpawnInStash);
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(!m_RandomizePlayerInventory);
            ImGui::Checkbox("Spawn In PlayerInventory", &s_SpawnInPlayerInventory);
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(!m_RandomizeActorInventory);
            ImGui::Checkbox("Spawn In ActorInventory", &s_SpawnInActorInventory);
            ImGui::EndDisabled();

            ImGui::SameLine();

            if (ImGui::SmallButton(ICON_MD_DELETE)) {
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
                m_PropsToSpawn.clear();
            }
        }
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
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

DEFINE_PLUGIN_DETOUR(Randomizer, bool, OnLoadScene, ZEntitySceneContext* th, SSceneInitParameters& parameters) {
    m_IsRandomizerAllowedForScene = parameters.m_Type != "evergreen";

    if (m_AllRepositoryProps.empty()) {
        LoadRepositoryProps();
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