#include "Editor.h"

#include <IconsMaterialDesign.h>

#include <Glacier/ZActor.h>
#include <Glacier/ZContentKitManager.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZApplicationEngineWin32.h>
#include <Glacier/ZRender.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZHM5InputManager.h>
#include <Glacier/ZFreeCamera.h>
#include <Glacier/ZTargetManager.h>

#include "imgui_internal.h"

#include <Util/ImGuiUtils.h>

#undef min

void Editor::DrawActors(const bool p_HasFocus) {
    if (!p_HasFocus || !m_ActorsMenuActive) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("Actors", &m_ActorsMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing && p_HasFocus) {
        if (!Globals::ActorManager) {
            return;
        }

        if (m_RepositoryWeapons.size() == 0) {
            LoadRepositoryWeapons();
        }

        static uint8_t s_CurrentCharacterSetIndex = 0;
        static std::string s_CurrentCharSetCharacterType = "Actor";
        static std::string s_CurrentCharSetCharacterType2 = "Actor";
        static uint8_t s_CurrentOutfitVariationIndex = 0;

        static char s_ActorName[2048]{ "" };

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Actor Name");
        ImGui::SameLine();

        ImGui::InputText("##ActorName", s_ActorName, sizeof(s_ActorName));

        if (ImGui::CollapsingHeader("Filters")) {
            ImGui::Checkbox("Show alive actors", &m_ShowAliveActors);

            ImGui::Separator();

            ImGui::Checkbox("Show civilians", &m_ShowCivilians);
            ImGui::Checkbox("Show guards", &m_ShowGuards);

            ImGui::Spacing();

            ImGui::Checkbox("Show male actors", &m_ShowMaleActors);
            ImGui::Checkbox("Show female actors", &m_ShowFemaleActors);

            ImGui::Separator();

            ImGui::Checkbox("Show targets", &m_ShowTargets);
            ImGui::Checkbox("Show active enforcers", &m_ShowActiveEnforcers);
            ImGui::Checkbox("Show potential enforcers", &m_ShowPotentialEnforcers);
            ImGui::Checkbox("Show dynamic enforcers", &m_ShowDynamicEnforcers);
            ImGui::Checkbox("Show crowd characters", &m_ShowCrowdCharacters);
            ImGui::Checkbox("Show active sentries", &m_ShowActiveSentries);
            ImGui::Checkbox("Show actors with cloth outfit", &m_ShowActorsWithClothOutfit);
        }

        ImGui::Checkbox("Select actor in list when clicked in game", &m_SelectActorOnMouseClick);

        ImGui::Separator();

        ImGui::BeginChild("left pane", ImVec2(300, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        TEntityRef<ZActor>* s_ActorArray;
        size_t s_ActorCount;

        if (m_ShowAliveActors) {
            s_ActorArray = Globals::ActorManager->m_aliveActors.data();
            s_ActorCount = Globals::ActorManager->m_aliveActors.size();
        }
        else {
            s_ActorArray = Globals::ActorManager->m_activatedActors.data();
            s_ActorCount = Globals::ActorManager->m_activatedActors.size();
        }

        for (int i = 0; i < s_ActorCount; ++i) {
            ZActor* s_Actor = s_ActorArray[i].m_pInterfaceRef;

            if (!s_Actor) {
                continue;
            }

            const ZGlobalOutfitKit* s_Outfit = s_Actor->m_rOutfit.m_pInterfaceRef;

            bool s_MatchesActorType = true;

            if (m_ShowCivilians || m_ShowGuards) {
                s_MatchesActorType = false;

                if (m_ShowCivilians && s_Outfit && s_Outfit->m_eActorType == EActorType::eAT_Civilian) {
                    s_MatchesActorType = true;
                }

                if (m_ShowGuards && s_Outfit && s_Outfit->m_eActorType == EActorType::eAT_Guard) {
                    s_MatchesActorType = true;
                }
            }

            bool s_MatchesGender = true;

            if (m_ShowMaleActors || m_ShowFemaleActors) {
                s_MatchesGender = false;

                if (m_ShowMaleActors && s_Outfit && !s_Outfit->m_bIsFemale) {
                    s_MatchesGender = true;
                }

                if (m_ShowFemaleActors && s_Outfit && s_Outfit->m_bIsFemale) {
                    s_MatchesGender = true;
                }
            }

            bool m_MatchesFlags = true;

            if (m_ShowTargets ||
                m_ShowActiveEnforcers ||
                m_ShowPotentialEnforcers ||
                m_ShowDynamicEnforcers ||
                m_ShowCrowdCharacters ||
                m_ShowActiveSentries ||
                m_ShowActorsWithClothOutfit)
            {
                m_MatchesFlags = false;

                if (m_ShowTargets && s_Actor->m_bContractTarget) {
                    m_MatchesFlags = true;
                }

                if (m_ShowActiveEnforcers && s_Actor->m_bIsActiveEnforcer) {
                    m_MatchesFlags = true;
                }

                if (m_ShowPotentialEnforcers && s_Actor->m_bIsPotentialEnforcer) {
                    m_MatchesFlags = true;
                }

                if (m_ShowDynamicEnforcers && s_Actor->m_bIsDynamicEnforcer) {
                    m_MatchesFlags = true;
                }

                if (m_ShowCrowdCharacters && s_Actor->m_bCrowdCharacter) {
                    m_MatchesFlags = true;
                }

                if (m_ShowActiveSentries && s_Actor->m_bActiveSentry) {
                    m_MatchesFlags = true;
                }

                if (m_ShowActorsWithClothOutfit && s_Actor->m_bHasClothOutfit) {
                    m_MatchesFlags = true;
                }
            }

            if (!s_MatchesActorType || !s_MatchesGender || !m_MatchesFlags) {
                continue;
            }

            std::string s_ActorName2 = s_Actor->GetActorName().c_str();

            if (!Util::StringUtils::FindSubstringUTF8(s_ActorName2, s_ActorName)) {
                continue;
            }

            const bool s_IsSelected = m_SelectedActor == s_Actor;

            if (m_ScrollToActor && s_IsSelected) {
                ImGui::SetScrollHereY(0.25f);

                m_ScrollToActor = false;
            }

            std::string s_ActorId = std::format("{}###{}", s_ActorName2, i);

            if (ImGui::Selectable(s_ActorId.c_str(), s_IsSelected)) {
                if (!s_IsSelected) {
                    m_SelectedActor = s_Actor;
                    m_GlobalOutfitKit = {};

                    Logger::Info("Selected actor (by list): {}", s_Actor->GetActorName());
                }
            }
        }

        ImGui::EndChild();

        if (!m_SelectedActor) {
            ImGui::PopFont();
            ImGui::End();
            ImGui::PopFont();

            return;
        }

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

        static char s_OutfitName[2048] {""};

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Outfit");
        ImGui::SameLine();

        if (!m_GlobalOutfitKit) {
            m_GlobalOutfitKit = m_SelectedActor->m_rOutfit;
            s_CurrentCharacterSetIndex = m_SelectedActor->m_nOutfitCharset;
            s_CurrentOutfitVariationIndex = m_SelectedActor->m_nOutfitVariation;
            s_OutfitName[0] = '\0';
        }

        const bool s_IsPopupOpen = Util::ImGuiUtils::InputWithAutocomplete(
            "##OutfitsPopup",
            s_OutfitName,
            sizeof(s_OutfitName),
            Globals::ContentKitManager->m_repositoryGlobalOutfitKits,
            [](auto& p_Pair) -> const ZRepositoryID& { return p_Pair.first; },
            [](auto& p_Pair) -> std::string {
                return std::string(
                    p_Pair.second.m_pInterfaceRef->m_sCommonName.c_str(),
                    p_Pair.second.m_pInterfaceRef->m_sCommonName.size()
                );
            },
            [&](const ZRepositoryID&,
                const std::string& p_Name,
                const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit) {
                    s_CurrentCharacterSetIndex = 0;
                    s_CurrentOutfitVariationIndex = 0;

                    EquipOutfit(
                        p_GlobalOutfitKit,
                        s_CurrentCharacterSetIndex,
                        s_CurrentCharSetCharacterType,
                        s_CurrentOutfitVariationIndex,
                        m_SelectedActor
                    );

                    m_GlobalOutfitKit = p_GlobalOutfitKit;
            },
            [](auto& p_Pair) -> const TEntityRef<ZGlobalOutfitKit>& { return p_Pair.second; },
            [](auto& p_Pair) -> bool {
                return p_Pair.second && !p_Pair.second.m_pInterfaceRef->m_bIsHitmanSuit;
            }
        );

        if (!s_IsPopupOpen && s_OutfitName[0] == '\0') {
            const char* s_OutfitName2 = m_SelectedActor->m_rOutfit.m_pInterfaceRef->m_sCommonName.c_str();

            strncpy(s_OutfitName, s_OutfitName2, sizeof(s_OutfitName) - 1);

            s_OutfitName[sizeof(s_OutfitName) - 1] = '\0';
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Character Set Index");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharacterSetIndex", std::to_string(s_CurrentCharacterSetIndex).data())) {
            if (m_GlobalOutfitKit) {
                for (size_t i = 0; i < m_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets.size(); ++i) {
                    std::string s_CharacterSetIndex = std::to_string(i);
                    const bool s_IsSelected = s_CurrentCharacterSetIndex == i;

                    if (ImGui::Selectable(s_CharacterSetIndex.c_str(), s_IsSelected)) {
                        s_CurrentCharacterSetIndex = i;

                        EquipOutfit(
                            m_GlobalOutfitKit,
                            s_CurrentCharacterSetIndex,
                            s_CurrentCharSetCharacterType,
                            s_CurrentOutfitVariationIndex,
                            m_SelectedActor
                        );
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentCharSetCharacterType.data())) {
            if (m_GlobalOutfitKit) {
                for (auto& m_CharSetCharacterType : m_CharSetCharacterTypes) {
                    const bool s_IsSelected = s_CurrentCharSetCharacterType == m_CharSetCharacterType;

                    if (ImGui::Selectable(m_CharSetCharacterType.data(), s_IsSelected)) {
                        s_CurrentCharSetCharacterType = m_CharSetCharacterType;

                        EquipOutfit(
                            m_GlobalOutfitKit,
                            s_CurrentCharacterSetIndex,
                            s_CurrentCharSetCharacterType,
                            s_CurrentOutfitVariationIndex,
                            m_SelectedActor
                        );
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Outfit Variation");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##OutfitVariation", std::to_string(s_CurrentOutfitVariationIndex).data())) {
            if (m_GlobalOutfitKit) {
                ECharSetCharacterType s_CharSetCharacterType;

                if (s_CurrentCharSetCharacterType == "Actor") {
                    s_CharSetCharacterType = ECharSetCharacterType::ECSCT_Actor;
                }
                else if (s_CurrentCharSetCharacterType == "Nude") {
                    s_CharSetCharacterType = ECharSetCharacterType::ECSCT_Nude;
                }
                else {
                    s_CharSetCharacterType = ECharSetCharacterType::ECSCT_HeroA;
                }

                const size_t s_VariationCount =
                    m_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[s_CurrentCharacterSetIndex].
                    m_pInterfaceRef->m_aCharacters[static_cast<size_t>(s_CharSetCharacterType)].
                    m_pInterfaceRef->m_aVariations.size();

                for (size_t i = 0; i < s_VariationCount; ++i) {
                    const bool s_IsSelected = s_CurrentOutfitVariationIndex == i;

                    if (ImGui::Selectable(std::to_string(i).data(), s_IsSelected)) {
                        s_CurrentOutfitVariationIndex = i;

                        EquipOutfit(
                            m_GlobalOutfitKit,
                            s_CurrentCharacterSetIndex,
                            s_CurrentCharSetCharacterType,
                            s_CurrentOutfitVariationIndex,
                            m_SelectedActor
                        );
                    }
                }
            }

            ImGui::EndCombo();
        }

        if (m_GlobalOutfitKit) {
            ImGui::Checkbox("Weapons Allowed", &m_GlobalOutfitKit.m_pInterfaceRef->m_bWeaponsAllowed);
            ImGui::Checkbox("Authority Figure", &m_GlobalOutfitKit.m_pInterfaceRef->m_bAuthorityFigure);
        }

        ImGui::Separator();

        static std::string s_ActorName2;

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Actor Name");
        ImGui::SameLine();

        ImGui::InputText("##ActorName", s_ActorName2.data(), s_ActorName2.size());
        ImGui::SameLine();

        if (ImGui::Button("Get Actor Outfit")) {
            if (const ZActor* s_Actor2 = Globals::ActorManager->GetActorByName(s_ActorName2)) {
                EquipOutfit(
                    s_Actor2->m_rOutfit,
                    s_Actor2->m_nOutfitCharset,
                    s_CurrentCharSetCharacterType2,
                    s_Actor2->m_nOutfitVariation,
                    m_SelectedActor
                );
            }
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType2", s_CurrentCharSetCharacterType2.data())) {
            if (m_GlobalOutfitKit) {
                for (const auto& m_CharSetCharacterType : m_CharSetCharacterTypes) {
                    const bool s_IsSelected = s_CurrentCharSetCharacterType2 == m_CharSetCharacterType;

                    if (ImGui::Selectable(m_CharSetCharacterType.data(), s_IsSelected)) {
                        s_CurrentCharSetCharacterType2 = m_CharSetCharacterType;
                    }
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::Button("Get Nearest Actor's Outfit")) {
            ZEntityRef s_Ref;

            m_SelectedActor->GetID(s_Ref);

            ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

            for (int i = 0; i < *Globals::NextActorId; ++i) {
                ZActor* s_Actor2 = Globals::ActorManager->m_activatedActors[i].m_pInterfaceRef;

                s_Actor2->GetID(s_Ref);

                const ZSpatialEntity* s_ActorSpatialEntity2 = s_Ref.QueryInterface<ZSpatialEntity>();

                const SVector3 s_Temp = s_ActorSpatialEntity->m_mTransform.Trans - s_ActorSpatialEntity2->m_mTransform.
                        Trans;
                const float s_Distance = sqrt(s_Temp.x * s_Temp.x + s_Temp.y * s_Temp.y + s_Temp.z * s_Temp.z);

                if (s_Distance <= 3.0f) {
                    EquipOutfit(
                        s_Actor2->m_rOutfit,
                        s_Actor2->m_nOutfitCharset,
                        s_CurrentCharSetCharacterType2,
                        s_Actor2->m_nOutfitVariation,
                        m_SelectedActor
                    );

                    break;
                }
            }
        }

        if (ImGui::Button("Select In Entity Tree")) {
            ZEntityRef s_Ref;

            m_SelectedActor->GetID(s_Ref);

            if (!m_CachedEntityTree || !m_CachedEntityTree->Entity) {
                UpdateEntities();
            }

            OnSelectEntity(s_Ref, true, std::nullopt);
        }

        if (ImGui::Button("Teleport Actor To Player")) {
            if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
                ZEntityRef s_Ref;
                m_SelectedActor->GetID(s_Ref);

                ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();
                ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

                s_ActorSpatialEntity->SetObjectToWorldMatrixFromEditor(
                    s_HitmanSpatialEntity->GetObjectToWorldMatrix()
                );
            }
        }

        if (ImGui::Button("Teleport Player To Actor")) {
            if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
                ZEntityRef s_Ref;
                m_SelectedActor->GetID(s_Ref);

                ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();
                ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

                s_HitmanSpatialEntity->SetObjectToWorldMatrixFromEditor(
                    s_ActorSpatialEntity->GetObjectToWorldMatrix()
                );
            }
        }

        if (ImGui::Button("Revive Actor")) {
            Functions::ZActor_ReviveActor->Call(m_SelectedActor);
        }

        if (ImGui::Button("Kill Actor")) {
            TEntityRef<IItem> s_Item;
            TEntityRef<ZSetpieceEntity> s_SetPieceEntity;

            Functions::ZActor_KillActor->Call(
                m_SelectedActor, s_Item, s_SetPieceEntity, EDamageEvent::eDE_UNDEFINED,
                EDeathBehavior::eDB_IMPACT_ANIM
            );
        }

        ImGui::Separator();

        ImGui::Text("Add Weapon To Inventory");
        ImGui::Spacing();

        static char s_WeaponTitle[2048]{ "" };

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Weapon Title");
        ImGui::SameLine();

        Util::ImGuiUtils::InputWithAutocomplete(
            "##RepositoryWeapons",
            s_WeaponTitle,
            sizeof(s_WeaponTitle),
            m_RepositoryWeapons,
            [](auto& p_Pair) -> const ZRepositoryID& { return p_Pair.first; },
            [](auto& p_Pair) -> const std::string& { return p_Pair.second; },
            [&](const ZRepositoryID& p_Id, const std::string& p_Name, const auto&) {
                ZEntityRef s_ActorEntityRef = m_SelectedActor->m_pInventoryHandler->m_rActor.m_entityRef;
                const uint64_t s_NewEntityID = Functions::ZEntityManager_GenerateDynamicObjectID->Call(
                    Globals::EntityManager,
                    s_ActorEntityRef,
                    EDynamicEntityType::eDET_CharacterInventoryItem,
                    0
                );

                const ZEntityRef s_ActorEntityRef2 = m_SelectedActor->m_pInventoryHandler->m_rActor.m_entityRef;
                const ZMemberDelegate<Editor, void(uint32 nTicket, TEntityRef<IItemBase> rNewItem)> s_Delegate(
                    this, &Editor::ItemCreatedHandler
                );

                uint32_t s_Ticket = Functions::ZWorldInventory_RequestNewItem->Call(
                    Globals::WorldInventory,
                    p_Id,
                    s_Delegate,
                    s_NewEntityID,
                    false,
                    {},
                    s_ActorEntityRef2
                );

                if (s_Ticket != *Globals::WorldInventory_InvalidTicket) {
                    ZActorInventoryHandler::SPendingItemInfo s_PendingItemInfo;
                    s_PendingItemInfo.m_nTicket = s_Ticket;
                    s_PendingItemInfo.m_eAttachLocation = EAttachLocation::eALUndefined;
                    s_PendingItemInfo.m_eMaxTension = EGameTension::EGT_Undefined;
                    s_PendingItemInfo.m_bLeftHand = false;
                    s_PendingItemInfo.m_bWeapon = true;

                    m_SelectedActor->m_pInventoryHandler->m_aPendingItems.push_back(s_PendingItemInfo);
                }
            }
        );

        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Main Weapon");
        ImGui::SameLine();

        ZHM5ItemWeapon* s_MainWeapon = m_SelectedActor->m_pInventoryHandler->m_rMainWeapon.m_pInterfaceRef;
        const char* s_MainWeaponName = m_SelectedActor->m_pInventoryHandler->m_rMainWeapon
            ? s_MainWeapon->m_pItemConfigDescriptor->m_sTitle.c_str()
            : "None";

        if (ImGui::BeginCombo("##MainWeapon", s_MainWeaponName)) {
            for (const auto& s_Item : m_SelectedActor->m_pInventoryHandler->m_aInventory) {
                if (!s_Item) {
                    continue;
                }

                ZHM5Item* s_HM5Item = static_cast<ZHM5Item*>(s_Item.m_pInterfaceRef);
                auto* s_ItemConfigDescriptor = s_HM5Item->m_pItemConfigDescriptor;

                if (!s_ItemConfigDescriptor) {
                    continue;
                }

                const bool s_IsSelected = s_HM5Item == s_MainWeapon;

                if (ImGui::Selectable(s_ItemConfigDescriptor->m_sTitle.c_str(), s_IsSelected)) {
                    ZEntityRef entityRef;
                    s_HM5Item->GetID(entityRef);
                    m_SelectedActor->m_pInventoryHandler->m_rMainWeapon = TEntityRef<ZHM5ItemWeapon>(entityRef);
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Separator();

        ImGui::Text("Inventory");
        ImGui::Spacing();

        for (const auto& s_Item : m_SelectedActor->m_pInventoryHandler->m_aInventory) {
            if (!s_Item) {
                continue;
            }

            ZHM5Item* s_HM5Item = static_cast<ZHM5Item*>(s_Item.m_pInterfaceRef);
            auto* s_ItemConfigDescriptor = s_HM5Item->m_pItemConfigDescriptor;

            if (!s_ItemConfigDescriptor) {
                continue;
            }

            std::string s_DisplayName = std::format(
                "{} [{}]",
                s_ItemConfigDescriptor->m_sTitle.c_str(),
                s_ItemConfigDescriptor->m_ItemID.ToString().c_str()
            );

            char s_Buffer[2048];

            strncpy(s_Buffer, s_DisplayName.c_str(), sizeof(s_Buffer));

            s_Buffer[sizeof(s_Buffer) - 1] = '\0';

            std::string s_InputId = std::format(
                "###{}",
                s_ItemConfigDescriptor->m_ItemID.ToString().c_str()
            );

            ImGui::InputText(s_InputId.c_str(), s_Buffer, sizeof(s_Buffer));

            ImGui::SameLine();

            std::string s_RemoveItemButtonId = std::format("###RemoveItem_{}", s_ItemConfigDescriptor->m_ItemID.ToString().c_str());

            if (ImGui::SmallButton((ICON_MD_CLOSE + s_RemoveItemButtonId).c_str())) {
                m_ItemToRemove = s_Item;
                m_RemoveItemFromInventory = true;
            }
        }

        ImGui::Separator();

        if (ImGui::Button(std::format("Track This Actor##{}", s_ActorName).c_str())) {

            if (m_RenderDest.m_entityRef == nullptr) {
                GetRenderDest();
            }

            if (m_TrackCam.m_entityRef == nullptr) {
                GetTrackCam();
            }

            if (m_PlayerCam == nullptr) {
                GetPlayerCam();
            }

            m_ActorTracked = m_SelectedActor;
            m_TrackCamActive = true;

            EnableTrackCam();
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop Tracking")) {
            m_TrackCamActive = false;
            m_ActorTracked = nullptr;

            DisableTrackCam();
        }

        ImGui::EndChild();
        ImGui::EndGroup();
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

void Editor::EquipOutfit(
    const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit,
    uint8_t p_CharSetIndex,
    const std::string& p_CharSetCharacterType,
    uint8_t p_OutfitVariationIndex,
    ZActor* p_Actor
) {
    if (!p_Actor) {
        Logger::Error("Couldn't equip outfit - actor is null!");
        return;
    }

    ZGlobalOutfitKit* s_GlobalOutfitKit = p_GlobalOutfitKit.m_pInterfaceRef;

    if (!s_GlobalOutfitKit) {
        Logger::Error("Couldn't equip outfit - global outfit kit is null!");
        return;
    }

    if (p_CharSetIndex >= s_GlobalOutfitKit->m_aCharSets.size()) {
        Logger::Error("Couldn't equip outfit - charset index isn't valid!");
        return;
    }

    ZOutfitVariationCollection* s_Collection = s_GlobalOutfitKit->m_aCharSets[p_CharSetIndex].m_pInterfaceRef;

    if (!s_Collection) {
        Logger::Error("Couldn't equip outfit - outvit variation collection is null!");
        return;
    }

    std::vector<ZRuntimeResourceID> s_OriginalActorVariations;

    if (p_CharSetCharacterType != "Actor") {
        auto* s_ActorType = &s_Collection->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_Actor)];

        if (!s_ActorType->m_pInterfaceRef) {
            Logger::Error("Couldn't equip outfit - actor character type is null!");
            return;
        }

        TEntityRef<ZCharsetCharacterType>* s_TargetType = nullptr;

        if (p_CharSetCharacterType == "HeroA") {
            s_TargetType = &s_Collection->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_HeroA)];
        }
        else if (p_CharSetCharacterType == "Nude") {
            s_TargetType = &s_Collection->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_Nude)];
        }

        const auto& s_ActorVariations = s_ActorType->m_pInterfaceRef->m_aVariations;

        s_OriginalActorVariations.reserve(s_ActorVariations.size());

        for (const auto& s_ActorVariation : s_ActorVariations) {
            s_OriginalActorVariations.push_back(s_ActorVariation.m_pInterfaceRef->m_Outfit);
        }

        if (s_TargetType && s_TargetType->m_pInterfaceRef) {
            const auto& s_TargetVariations = s_TargetType->m_pInterfaceRef->m_aVariations;
            const size_t s_Count = std::min(s_ActorVariations.size(), s_TargetVariations.size());

            for (size_t i = 0; i < s_Count; ++i) {
                s_ActorVariations[i].m_pInterfaceRef->m_Outfit = s_TargetVariations[i].m_pInterfaceRef->m_Outfit;
            }
        }
    }

    Functions::ZActor_SetOutfit->Call(
        p_Actor, p_GlobalOutfitKit, p_CharSetIndex, p_OutfitVariationIndex, false
    );

    if (p_CharSetCharacterType != "Actor" && !s_OriginalActorVariations.empty()) {
        auto* s_ActorType = &s_GlobalOutfitKit->m_aCharSets[p_CharSetIndex].
            m_pInterfaceRef->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_Actor)];

        if (s_ActorType->m_pInterfaceRef) {
            auto& s_ActorVariations = s_ActorType->m_pInterfaceRef->m_aVariations;
            const size_t s_Count = std::min(s_ActorVariations.size(), s_OriginalActorVariations.size());

            for (size_t i = 0; i < s_Count; ++i) {
                s_ActorVariations[i].m_pInterfaceRef->m_Outfit = s_OriginalActorVariations[i];
            }
        }
    }
}

void Editor::ItemCreatedHandler(uint32 p_Ticket, TEntityRef<IItemBase> p_NewItem) {
    for (auto& s_PendingItem : m_SelectedActor->m_pInventoryHandler->m_aPendingItems) {
        if (s_PendingItem.m_nTicket == p_Ticket) {
            ZHM5Item* s_Item = static_cast<ZHM5Item*>(p_NewItem.m_pInterfaceRef);
            SItemConfig& s_ItemConfig = s_Item->m_pItemConfigDescriptor->m_ItemConfig;

            s_PendingItem.m_rItem = TEntityRef<IItem>(p_NewItem.m_entityRef);
            s_PendingItem.m_eAttachLocation = s_ItemConfig.m_ItemHandsIdle == eItemHands::IH_TWOHANDED ?
                EAttachLocation::eALRifle : EAttachLocation::eALUndefined;

            break;
        }
    }

    Functions::ZActorInventoryHandler_FinalizePendingItems->Call(m_SelectedActor->m_pInventoryHandler);
}

void Editor::LoadRepositoryWeapons() {
    m_RepositoryWeapons.clear();

    static TResourcePtr<ZTemplateEntityFactory> m_RepositoryResource;

    if (m_RepositoryResource.m_nResourceIndex.val == -1) {
        const auto s_ID = ResId<"[assembly:/repository/pro.repo].pc_repo">;

        Globals::ResourceManager->GetResourcePtr(m_RepositoryResource, s_ID, 0);
    }

    if (m_RepositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID) {
        const auto s_RepositoryData = static_cast<THashMap<
            ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(m_RepositoryResource.
                GetResourceData());

        for (const auto& [s_RepositoryID, s_DynamicObject] : *s_RepositoryData) {
            TArray<SDynamicObjectKeyValuePair>* s_Entries = s_DynamicObject.As<TArray<
                SDynamicObjectKeyValuePair>>();

            ZString s_Id, s_Title, s_CommonName, s_Name;
            std::string s_FinalName;
            bool s_HasItemType = false;
            bool s_HasPrimaryConfiguration = false;

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
                else if (s_Entry.sKey == "ItemType") {
                    s_HasItemType = true;
                }
                else if (s_Entry.sKey == "PrimaryConfiguration") {
                    s_HasPrimaryConfiguration = true;
                }
            }

            if (s_Id.IsEmpty() || !s_HasItemType || !s_HasPrimaryConfiguration) {
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

            m_RepositoryWeapons.push_back(std::make_pair(s_Id, s_FinalName));
        }
    }

    std::ranges::sort(
        m_RepositoryWeapons,
        [](const auto& a, const auto& b) {
            auto [s_RepositoryIdA, s_NameA] = a;
            auto [s_RepositoryIdB, s_NameB] = b;

            std::ranges::transform(s_NameA, s_NameA.begin(), [](unsigned char c) { return std::tolower(c); });
            std::ranges::transform(s_NameB, s_NameB.begin(), [](unsigned char c) { return std::tolower(c); });

            return s_NameA < s_NameB;
        }
    );
}

void Editor::EnableTrackCam() {
    m_RenderDest.m_pInterfaceRef->SetSource(&m_TrackCam.m_entityRef);
    SetPlayerControlActive(false);
}

void Editor::UpdateTrackCam() const {
    ZEntityRef s_Ref;

    m_ActorTracked->GetID(s_Ref);

    SMatrix s_ActorWorldMatrix = s_Ref.QueryInterface<ZSpatialEntity>()->GetObjectToWorldMatrix();
    SMatrix s_TrackCamWorldMatrix = m_TrackCam.m_pInterfaceRef->GetObjectToWorldMatrix();
    s_TrackCamWorldMatrix.Trans = s_ActorWorldMatrix.Trans + float4(0.f, 0.f, 2.f, 0.f);

    m_TrackCam.m_pInterfaceRef->SetObjectToWorldMatrixFromEditor(s_TrackCamWorldMatrix);
}

void Editor::DisableTrackCam() {
    m_RenderDest.m_pInterfaceRef->SetSource(&m_PlayerCam);
    SetPlayerControlActive(true);
}

void Editor::GetPlayerCam() {
    m_PlayerCam = *m_RenderDest.m_pInterfaceRef->GetSource();
}

void Editor::GetTrackCam() {
    m_TrackCam = (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01;
}

void Editor::GetRenderDest() {
    Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &m_RenderDest);
}

void Editor::SetPlayerControlActive(bool s_Active) {
    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (s_LocalHitman) {
        auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(Globals::InputManager);

        if (s_InputControl) {
            s_InputControl->m_bActive = s_Active;
        }
    }
}

bool Editor::IsActorTarget(ZActor* p_Actor) {
    if (!Globals::TargetManager) {
        return false;
    }

    for (const auto& targetInfo : Globals::TargetManager->m_aTargets) {
        if (targetInfo.m_rActor.m_pInterfaceRef == p_Actor) {
            return true;
        }
    }

    return false;
}