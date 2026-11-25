#include "Player.h"

#include <imgui_internal.h>

#include <IconsMaterialDesign.h>

#include <Glacier/ZContentKitManager.h>
#include <Glacier/ZActor.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZAction.h>
#include <Glacier/ZHM5CrippleBox.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZItem.h>
#include <Glacier/SExternalReferences.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZAIGameState.h>

#include <Util/ImGuiUtils.h>

#undef min

Player::~Player() {
    const ZMemberDelegate<Player, void(const SGameUpdateEvent&)> s_Delegate(this, &Player::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);
}

void Player::OnEngineInitialized() {
    const ZMemberDelegate<Player, void(const SGameUpdateEvent&)> s_Delegate(this, &Player::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);
}

void Player::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    auto s_LocalHitman = SDK()->GetLocalPlayer();
    ZHitman5* s_Hitman = s_LocalHitman.m_pInterfaceRef;

    // The game clears m_bIsHiding on its own (leaving a hiding spot, cutscenes...).
    if (s_Hitman && m_IsInvisible && ++m_EnforceTimer >= 20) {
        m_EnforceTimer = 0;

        if (!s_Hitman->m_bIsHiding) {
            s_Hitman->m_bIsHiding = true;
        }
    }

    const int s_Pending = m_PendingHeroHidden.exchange(-1);

    if (s_Pending < 0) {
        return;
    }

    if (!m_AICrippleEntity && !CreateAICrippleEntity()) {
        return;
    }

    const std::string s_PinName = s_Pending == 1 ? "SetHeroHidden" : "SetHeroVisible";

    m_AICrippleEntity.SignalInputPin(s_PinName);
}

bool Player::CreateAICrippleEntity() {
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Error("[Player] Scene not loaded!");
        return false;
    }

    constexpr auto s_AICrippleEntityFactoryId = ResId<"[modules:/zaicrippleentity.class].pc_entitytype">;

    TResourcePtr<ZTemplateEntityFactory> s_AICrippleEntityFactory;
    Globals::ResourceManager->LoadResource(s_AICrippleEntityFactory, s_AICrippleEntityFactoryId);

    if (!s_AICrippleEntityFactory) {
        Logger::Error("[Player] AI cripple entity resource is not loaded!");
        return false;
    }

    SExternalReferences s_ExternalRefs;

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager,
        m_AICrippleEntity,
        "",
        s_AICrippleEntityFactory,
        s_Scene.m_entityRef,
        s_ExternalRefs,
        -1
    );

    if (!m_AICrippleEntity) {
        Logger::Error("[Player] Failed to spawn AI cripple entity!");
        return false;
    }

    return true;
}

void Player::Init() {
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &Player::OnClearScene);

    Hooks::ZSecuritySystemCameraManager_OnFrameUpdate->AddDetour(
        this,
        &Player::ZSecuritySystemCameraManager_OnFrameUpdate
    );
    Hooks::ZSecuritySystemCamera_FrameUpdate->AddDetour(this, &Player::ZSecuritySystemCamera_FrameUpdate);

    Hooks::ZHM5ItemWeapon_SetBulletsInMagazine->AddDetour(this, &Player::ZHM5ItemWeapon_SetBulletsInMagazine);
    Hooks::ZHitmanMorphemePostProcessor_UpdateWeaponRecoil->AddDetour(
        this,
        &Player::ZHitmanMorphemePostProcessor_UpdateWeaponRecoil
    );
    Hooks::ZHM5WeaponRecoilController_RecoilWeapon->AddDetour(this, &Player::ZHM5WeaponRecoilController_RecoilWeapon);
    Hooks::ZHM5ItemWeapon_FireProjectiles->AddDetour(this, &Player::ZHM5ItemWeapon_FireProjectiles);
    Hooks::ZHM5ItemWeapon_IsFiring->AddDetour(this, &Player::ZHM5ItemWeapon_IsFiring);
    Hooks::ZActor_YouGotHit->AddDetour(this, &Player::ZActor_YouGotHit);
    Hooks::ZGameStatsManager_SendAISignals01->AddDetour(this, &Player::ZGameStatsManager_SendAISignals);
    Hooks::ZGameStatsManager_SendAISignals02->AddDetour(this, &Player::ZGameStatsManager_SendAISignals);
}

void Player::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_MAN " PLAYER")) {
        m_ShowPlayerWindow = !m_ShowPlayerWindow;
    }
}

void Player::OnDrawUI(const bool p_HasFocus) {
    if (!p_HasFocus || !m_ShowPlayerWindow) {
        return;
    }

    ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;

    auto s_LocalHitman = SDK()->GetLocalPlayer();

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_IsWindowExpanded = ImGui::Begin(ICON_MD_MAN " Player", &m_ShowPlayerWindow);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_IsWindowExpanded) {
        ImGui::BeginDisabled(!s_LocalHitman);

        ZHitman5* s_Hitman = s_LocalHitman.m_pInterfaceRef;
        ZHM5Health* s_Health = s_Hitman ? s_Hitman->m_pHealth : nullptr;

        bool s_IsInvincible = s_Health ? s_Health->m_bInvincible.Get() : false;

        ImGui::BeginDisabled(!s_Health);

        if (ImGui::Checkbox("Is invincible", &s_IsInvincible)) {
            s_Health->m_bInvincible.Set(s_IsInvincible);
        }

        ImGui::EndDisabled();

        if (ImGui::Checkbox("Is invisible", &m_IsInvisible)) {
            if (s_Hitman) {
                s_Hitman->m_bIsHiding = m_IsInvisible;
            }

            m_PendingHeroHidden = m_IsInvisible ? 1 : 0;
        }

        if (ImGui::Checkbox("Infinite ammo", &m_IsInfiniteAmmoEnabled)) {
            ToggleInfiniteAmmo();
        }

        ImGui::Checkbox("No reload", &m_IsNoReloadEnabled);

        ImGui::Checkbox("No recoil", &m_IsNoRecoilEnabled);

        ImGui::Checkbox("Super accuracy", &m_IsSuperAccuracyEnabled);

        ImGui::Checkbox("Rapid fire", &m_IsRapidFireEnabled);

        ImGui::Checkbox("One hit kill", &m_IsOneHitKillEnabled);

        static char s_OutfitName[2048] { "" };
        static uint8_t s_CurrentCharacterSetIndex = 0;
        static std::string s_CurrentCharSetCharacterType = "HeroA";
        static std::string s_CurrentCharSetCharacterType2 = "HeroA";
        static std::string s_CurrentCharSetCharacterType3 = "HeroA";
        static uint8_t s_CurrentOutfitVariationIndex = 1;

        if (s_LocalHitman) {
            if (!m_GlobalOutfitKit) {
                m_GlobalOutfitKit = s_LocalHitman.m_pInterfaceRef->m_rOutfitKit;
                s_CurrentCharacterSetIndex = s_LocalHitman.m_pInterfaceRef->m_nOutfitCharset;
                s_CurrentOutfitVariationIndex = s_LocalHitman.m_pInterfaceRef->m_nOutfitVariation;
            }
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Outfit");
        ImGui::SameLine();

        const bool s_IsPopupOpen = Util::ImGuiUtils::InputWithAutocomplete(
            "##OutfitsPopup",
            s_OutfitName,
            sizeof(s_OutfitName),
            s_ContentKitManager->m_repositoryGlobalOutfitKits,
            [](auto& p_Pair) -> uint32_t { return p_Pair.first.GetHashCode(); },
            [](auto& p_Pair) -> std::string {
                return std::string(
                    p_Pair.second.m_pInterfaceRef->m_sCommonName.c_str(),
                    p_Pair.second.m_pInterfaceRef->m_sCommonName.size()
                );
            },
            [&](uint32_t,
                const std::string& p_Name,
                const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit) {
                    s_CurrentCharacterSetIndex = 0;
                    s_CurrentOutfitVariationIndex = 0;

                    EquipOutfit(
                        p_GlobalOutfitKit,
                        s_CurrentCharacterSetIndex,
                        s_CurrentCharSetCharacterType.data(),
                        s_CurrentOutfitVariationIndex,
                        s_LocalHitman.m_pInterfaceRef
                    );

                    m_GlobalOutfitKit = p_GlobalOutfitKit;
            },
            [](auto& p_Pair) -> const TEntityRef<ZGlobalOutfitKit>&{ return p_Pair.second; }
        );

        if (!s_IsPopupOpen && s_LocalHitman && s_OutfitName[0] == '\0') {
            const char* s_OutfitName2 = s_LocalHitman.m_pInterfaceRef->m_rOutfitKit.m_pInterfaceRef->m_sCommonName.
                c_str();

            strncpy(s_OutfitName, s_OutfitName2, sizeof(s_OutfitName) - 1);

            s_OutfitName[sizeof(s_OutfitName) - 1] = '\0';
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Character set index");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharacterSetIndex", std::to_string(s_CurrentCharacterSetIndex).data())) {
            if (m_GlobalOutfitKit) {
                for (size_t i = 0; i < m_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets.size(); ++i) {
                    const bool s_IsSelected = s_CurrentCharacterSetIndex == i;

                    if (ImGui::Selectable(std::to_string(i).data(), s_IsSelected)) {
                        s_CurrentCharacterSetIndex = i;

                        EquipOutfit(
                            m_GlobalOutfitKit,
                            s_CurrentCharacterSetIndex,
                            s_CurrentCharSetCharacterType.data(),
                            s_CurrentOutfitVariationIndex,
                            s_LocalHitman.m_pInterfaceRef
                        );
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Charset character type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentCharSetCharacterType.data())) {
            if (m_GlobalOutfitKit) {
                for (size_t i = 0; i < 3; ++i) {
                    const bool s_IsSelected = s_CurrentCharSetCharacterType == m_CharSetCharacterTypes[i];

                    if (ImGui::Selectable(m_CharSetCharacterTypes[i].data(), s_IsSelected)) {
                        s_CurrentCharSetCharacterType = m_CharSetCharacterTypes[i].data();

                        EquipOutfit(
                            m_GlobalOutfitKit,
                            s_CurrentCharacterSetIndex,
                            s_CurrentCharSetCharacterType.data(),
                            s_CurrentOutfitVariationIndex,
                            s_LocalHitman.m_pInterfaceRef
                        );
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Outfit variation");
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
                            s_CurrentCharSetCharacterType.data(),
                            s_CurrentOutfitVariationIndex,
                            s_LocalHitman.m_pInterfaceRef
                        );
                    }
                }
            }

            ImGui::EndCombo();
        }

        if (m_GlobalOutfitKit) {
            ImGui::Checkbox("Weapons allowed", &m_GlobalOutfitKit.m_pInterfaceRef->m_bWeaponsAllowed);
            ImGui::Checkbox("Authority figure", &m_GlobalOutfitKit.m_pInterfaceRef->m_bAuthorityFigure);
        }

        ImGui::Separator();

        ImGui::Text("Get actor's outfit");

        ImGui::Spacing();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Charset character type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType2", s_CurrentCharSetCharacterType2.data())) {
            for (size_t i = 0; i < 3; ++i) {
                const bool s_IsSelected = s_CurrentCharSetCharacterType2 == m_CharSetCharacterTypes[i];

                if (ImGui::Selectable(m_CharSetCharacterTypes[i].data(), s_IsSelected)) {
                    s_CurrentCharSetCharacterType2 = m_CharSetCharacterTypes[i];
                }
            }

            ImGui::EndCombo();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Actor name");
        ImGui::SameLine();

        static char s_ActorName[2048]{ "" };

        Util::ImGuiUtils::InputWithAutocomplete(
            "##ActorName",
            s_ActorName,
            sizeof(s_ActorName),
            Globals::ActorManager->m_activatedActors,
            [](auto& p_EntityRef) -> uintptr_t { return reinterpret_cast<uintptr_t>(p_EntityRef.m_entityRef.GetEntity()); },
            [](auto& p_EntityRef) -> std::string {
                return std::string(
                    p_EntityRef.m_pInterfaceRef->m_sActorName.c_str(),
                    p_EntityRef.m_pInterfaceRef->m_sActorName.size()
                );
            },
            [&](uintptr_t, const std::string& actorName, const TEntityRef<ZActor>& p_EntityRef) {
                const ZActor* s_Actor = p_EntityRef.m_pInterfaceRef;

                if (s_Actor) {
                    EquipOutfit(
                        s_Actor->m_rOutfit,
                        s_Actor->m_nOutfitCharset,
                        s_CurrentCharSetCharacterType2.data(),
                        s_Actor->m_nOutfitVariation,
                        s_LocalHitman.m_pInterfaceRef
                    );
                }
            }
        );

        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Charset character type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType3", s_CurrentCharSetCharacterType3.data())) {
            for (size_t i = 0; i < 3; ++i) {
                const bool s_IsSelected = s_CurrentCharSetCharacterType3 == m_CharSetCharacterTypes[i];

                if (ImGui::Selectable(m_CharSetCharacterTypes[i].data(), s_IsSelected)) {
                    s_CurrentCharSetCharacterType3 = m_CharSetCharacterTypes[i];
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::Button("Get nearest actor's outfit")) {
            const ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();

            for (int i = 0; i < *Globals::NextActorId; ++i) {
                ZActor* actor = Globals::ActorManager->m_activatedActors[i].m_pInterfaceRef;
                ZEntityRef s_Ref;

                actor->GetID(s_Ref);

                ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

                const SVector3 s_Offset = s_ActorSpatialEntity->m_mTransform.Trans - s_HitmanSpatialEntity->m_mTransform.
                    Trans;
                const float s_Distance = sqrt(s_Offset.x * s_Offset.x + s_Offset.y * s_Offset.y + s_Offset.z * s_Offset.z);

                if (s_Distance <= 3.0f) {
                    EquipOutfit(
                        actor->m_rOutfit,
                        actor->m_nOutfitCharset,
                        s_CurrentCharSetCharacterType3.data(),
                        actor->m_nOutfitVariation,
                        s_LocalHitman.m_pInterfaceRef
                    );

                    break;
                }
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Teleport all items to player")) {
            auto s_HitmanSpatial = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();
            const ZHM5ActionManager* s_Hm5ActionManager = Globals::HM5ActionManager;

            for (size_t i = 0; i < s_Hm5ActionManager->m_Actions.size(); ++i) {
                const ZHM5Action* s_Action = s_Hm5ActionManager->m_Actions[i];

                if (!s_Action) {
                    continue;
                }

                if (s_Action->m_eActionType == EActionType::AT_PICKUP) {
                    const ZHM5Item* s_Item = s_Action->m_Object.QueryInterface<ZHM5Item>();

                    s_Item->m_rGeomentity.m_pInterfaceRef->SetObjectToWorldMatrixFromEditor(
                        s_HitmanSpatial->GetObjectToWorldMatrix()
                    );
                }
            }
        }

        if (ImGui::Button("Teleport all actors to player")) {
            const auto s_HitmanSpatialEntity = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();

            for (size_t i = 0; i < *Globals::NextActorId; ++i) {
                ZActor* s_Actor = Globals::ActorManager->m_activatedActors[i].m_pInterfaceRef;
                ZEntityRef s_Ref;

                s_Actor->GetID(s_Ref);

                ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

                s_ActorSpatialEntity->SetObjectToWorldMatrixFromEditor(
                    s_HitmanSpatialEntity->GetObjectToWorldMatrix()
                );
            }
        }

        ImGui::EndDisabled();
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

// True if the outfit has an actual resource for this character type and variation.
bool Player::HasOutfit(
    ZOutfitVariationCollection* p_Collection, const std::string& p_CharSetCharacterType, uint8_t p_VariationIndex
) {
    ECharSetCharacterType s_Type = ECharSetCharacterType::ECSCT_HeroA;

    if (p_CharSetCharacterType == "Actor") {
        s_Type = ECharSetCharacterType::ECSCT_Actor;
    }
    else if (p_CharSetCharacterType == "Nude") {
        s_Type = ECharSetCharacterType::ECSCT_Nude;
    }

    const auto& s_Character = p_Collection->m_aCharacters[static_cast<size_t>(s_Type)];

    if (!s_Character.m_pInterfaceRef) {
        return false;
    }

    const auto& s_Variations = s_Character.m_pInterfaceRef->m_aVariations;
    const size_t s_Index = p_VariationIndex < s_Variations.size() ? p_VariationIndex : 0;

    if (s_Variations.size() == 0 || !s_Variations[s_Index].m_pInterfaceRef) {
        return false;
    }

    return s_Variations[s_Index].m_pInterfaceRef->m_Outfit.GetID() != 0xFFFFFFFFFFFFFFFFull;
}

void Player::EquipOutfit(
    const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit,
    uint8_t p_CharSetIndex,
    const std::string& p_CharSetCharacterType,
    uint8_t p_OutfitVariationIndex,
    ZHitman5* p_Hitman
) {
    ZGlobalOutfitKit* s_GlobalOutfitKit = p_GlobalOutfitKit.m_pInterfaceRef;

    if (!s_GlobalOutfitKit) {
        Logger::Error("[Player] Couldn't equip outfit - global outfit kit is null!");
        return;
    }

    if (p_CharSetIndex >= s_GlobalOutfitKit->m_aCharSets.size()) {
        Logger::Error("[Player] Couldn't equip outfit - charset index isn't valid!");
        return;
    }

    ZOutfitVariationCollection* s_Collection = s_GlobalOutfitKit->m_aCharSets[p_CharSetIndex].m_pInterfaceRef;

    if (!s_Collection) {
        Logger::Error("[Player] Couldn't equip outfit - outvit variation collection is null!");
        return;
    }

    // Many NPC outfits (Dalia Margolis, the guests...) only have an "Actor" version: their
    // "HeroA" and "Nude" versions exist but are empty (resource FFFFFFFFFFFFFFFF), and the game
    // then silently applied nothing, so "Get actor's outfit" seemed to do nothing. Fall back to
    // a version that actually exists.
    std::string s_CharSetCharacterType = p_CharSetCharacterType;

    if (!HasOutfit(s_Collection, s_CharSetCharacterType, p_OutfitVariationIndex)) {
        for (const char* s_Fallback : { "HeroA", "Actor" }) {
            if (s_Fallback != s_CharSetCharacterType && HasOutfit(s_Collection, s_Fallback, p_OutfitVariationIndex)) {
                Logger::Info(
                    "[Player] Outfit has no '{}' version, using '{}' instead.",
                    s_CharSetCharacterType, s_Fallback
                );
                s_CharSetCharacterType = s_Fallback;
                break;
            }
        }
    }

    std::vector<ZRuntimeResourceID> s_OriginalHeroVariations;

    if (s_CharSetCharacterType != "HeroA") {
        auto* s_HeroType = &s_Collection->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_HeroA)];

        if (!s_HeroType->m_pInterfaceRef) {
            Logger::Error("[Player] Couldn't equip outfit - hero character type is null!");
            return;
        }

        TEntityRef<ZCharsetCharacterType>* s_TargetType = nullptr;

        if (s_CharSetCharacterType == "Actor") {
            s_TargetType = &s_Collection->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_Actor)];
        }
        else if (s_CharSetCharacterType == "Nude") {
            s_TargetType = &s_Collection->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_Nude)];
        }

        const auto& s_HeroVariations = s_HeroType->m_pInterfaceRef->m_aVariations;

        s_OriginalHeroVariations.reserve(s_HeroVariations.size());

        for (const auto& s_HeroVariation : s_HeroVariations) {
            s_OriginalHeroVariations.push_back(s_HeroVariation.m_pInterfaceRef->m_Outfit);
        }

        if (s_TargetType && s_TargetType->m_pInterfaceRef) {
            const auto& s_TargetVariations = s_TargetType->m_pInterfaceRef->m_aVariations;
            const size_t s_Count = std::min(s_HeroVariations.size(), s_TargetVariations.size());

            for (size_t i = 0; i < s_Count; ++i) {
                s_HeroVariations[i].m_pInterfaceRef->m_Outfit = s_TargetVariations[i].m_pInterfaceRef->m_Outfit;
            }
        }
    }

    Functions::ZHitman5_SetOutfit->Call(
        p_Hitman, p_GlobalOutfitKit, p_CharSetIndex, p_OutfitVariationIndex, false, false
    );

    if (s_CharSetCharacterType != "HeroA" && !s_OriginalHeroVariations.empty()) {
        auto* s_HeroType = &s_GlobalOutfitKit->m_aCharSets[p_CharSetIndex].
            m_pInterfaceRef->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_HeroA)];

        if (s_HeroType->m_pInterfaceRef) {
            auto& s_HeroVariations = s_HeroType->m_pInterfaceRef->m_aVariations;
            const size_t s_Count = std::min(s_HeroVariations.size(), s_OriginalHeroVariations.size());

            for (size_t i = 0; i < s_Count; ++i) {
                s_HeroVariations[i].m_pInterfaceRef->m_Outfit = s_OriginalHeroVariations[i];
            }
        }
    }
}

void Player::ToggleInfiniteAmmo() {
    if (m_IsInfiniteAmmoEnabled) {
        auto s_LocalHitman = SDK()->GetLocalPlayer();

        if (!s_LocalHitman) {
            Logger::Debug("[Player] Local player is not alive.");

            return;
        }

        bool s_IsHM5CrippleBoxEntityCreated = m_HM5CrippleBoxEntity;

        if (!s_IsHM5CrippleBoxEntityCreated) {
            s_IsHM5CrippleBoxEntityCreated = CreateHM5CrippleBoxEntity();
        }

        if (s_IsHM5CrippleBoxEntityCreated) {
            ZHM5CrippleBox* s_HM5CrippleBox = m_HM5CrippleBoxEntity.QueryInterface<ZHM5CrippleBox>();

            s_HM5CrippleBox->m_bActivateOnStart = true;
            s_HM5CrippleBox->m_rHitmanCharacter = s_LocalHitman;
            s_HM5CrippleBox->m_bLimitedAmmo = false;

            s_HM5CrippleBox->Activate(0);
        }
    }
    else {
        if (m_HM5CrippleBoxEntity) {
            Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, m_HM5CrippleBoxEntity, {});

            m_HM5CrippleBoxEntity = {};
        }
    }
}

bool Player::CreateHM5CrippleBoxEntity() {
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Debug("[Player] Scene not loaded.");

        return false;
    }

    constexpr auto s_CrippleBoxFactoryId = ResId<"[modules:/zhm5cripplebox.class].pc_entitytype">;

    TResourcePtr<ZTemplateEntityFactory> s_CrippleBoxFactory;
    Globals::ResourceManager->LoadResource(s_CrippleBoxFactory, s_CrippleBoxFactoryId);

    if (!s_CrippleBoxFactory) {
        Logger::Debug("[Player] Resource is not loaded.");

        return false;
    }

    SExternalReferences s_ExternalRefs;

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager,
        m_HM5CrippleBoxEntity,
        "",
        s_CrippleBoxFactory,
        s_Scene.m_entityRef,
        s_ExternalRefs,
        -1
    );

    if (!m_HM5CrippleBoxEntity) {
        Logger::Debug("[Player] Failed to spawn entity.");

        return false;
    }

    return true;
}

DEFINE_PLUGIN_DETOUR(Player, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene) {
    if (m_HM5CrippleBoxEntity) {
        Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, m_HM5CrippleBoxEntity, {});

        m_HM5CrippleBoxEntity = {};
    }

    m_IsInfiniteAmmoEnabled = false;
    m_GlobalOutfitKit = {};

    // The AI cripple entity belongs to the scene being unloaded; the new 47 starts visible.
    m_AICrippleEntity = {};
    m_PendingHeroHidden = -1;
    m_IsInvisible = false;

    return { HookAction::Continue() };
}

DEFINE_PLUGIN_DETOUR(
    Player,
    void,
    ZSecuritySystemCameraManager_OnFrameUpdate,
    ZSecuritySystemCameraManager* th,
    const SGameUpdateEvent& updateEvent
) {
    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (s_LocalHitman && s_LocalHitman.m_pInterfaceRef->m_bIsHiding) {
        return { HookAction::Return() };
    }

    return { HookAction::Continue() };
}

DEFINE_PLUGIN_DETOUR(
    Player,
    void,
    ZSecuritySystemCamera_FrameUpdate,
    ZSecuritySystemCamera* th,
    const SGameUpdateEvent& updateEvent
) {
    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (s_LocalHitman && s_LocalHitman.m_pInterfaceRef->m_bIsHiding) {
        return { HookAction::Return() };
    }

    return { HookAction::Continue() };
}

DEFINE_PLUGIN_DETOUR(Player, void, ZHM5ItemWeapon_SetBulletsInMagazine, IFirearm* th, int32_t nBullets) {
    if (!m_IsNoReloadEnabled) {
        return { HookAction::Continue() };
    }

    const auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        return { HookAction::Continue() };
    }

    ZHM5ItemWeapon* s_HM5ItemWeapon = static_cast<ZHM5ItemWeapon*>(th);

    if (s_HM5ItemWeapon->m_pOwner != s_LocalHitman.m_entityRef) {
        return { HookAction::Continue() };
    }

    if (s_HM5ItemWeapon->m_nBulletsFired == s_HM5ItemWeapon->m_nBulletsToFire) {
        s_HM5ItemWeapon->m_nBulletsFired = 0;
    }

    if (nBullets != 0) {
        return { HookAction::Continue() };
    }

    if (!s_LocalHitman.m_pInterfaceRef->IsInfiniteAmmoEnabled()) {
        auto s_Character = s_LocalHitman.m_pInterfaceRef->m_pCharacter.m_pInterfaceRef;
        auto s_Controllers = &s_Character->m_rSubcontrollerContainer.m_pInterfaceRef->m_aReferencedControllers;
        auto s_Inventory = static_cast<ZCharacterSubcontrollerInventory*>((*s_Controllers)[6].m_pInterfaceRef);

        const eAmmoType s_AmmoType = th->GetAmmoType();

        uint32 s_AmmoInPocket = Functions::ZCharacterSubcontrollerInventory_GetAmmoInPocketForType->Call(
            s_Inventory,
            s_AmmoType
        );

        if (s_AmmoInPocket > 0) {
            s_AmmoInPocket -= s_HM5ItemWeapon->GetMagazineCapacity();

            s_Inventory->m_nAmmoInPocket[static_cast<size_t>(s_AmmoType)] = s_AmmoInPocket;

            nBullets = s_HM5ItemWeapon->GetMagazineCapacity();
        }
    }
    else {
        nBullets = s_HM5ItemWeapon->GetMagazineCapacity();
    }

    p_Hook->CallOriginal(th, nBullets);

    return { HookAction::Return() };
}

DEFINE_PLUGIN_DETOUR(
    Player,
    void,
    ZHitmanMorphemePostProcessor_UpdateWeaponRecoil,
    ZHitmanMorphemePostProcessor* th,
    float fDeltaTime,
    const THashMap<int32_t, int32_t, TDefaultHashMapPolicy<int32_t>>& charboneMap,
    TArrayRef<int32_t> hierarchy
) {
    if (m_IsNoRecoilEnabled) {
        return { HookAction::Return() };
    }

    return { HookAction::Continue() };
}

DEFINE_PLUGIN_DETOUR(
    Player,
    void,
    ZHM5WeaponRecoilController_RecoilWeapon,
    ZHM5WeaponRecoilController* th,
    const TEntityRef<ZHM5ItemWeapon>& rWeapon
) {
    if (!m_IsNoRecoilEnabled) {
        return { HookAction::Continue() };
    }

    p_Hook->CallOriginal(th, rWeapon);

    th->m_vRecoil = SVector2(0.f, 0.f);

    return { HookAction::Return() };
}

DEFINE_PLUGIN_DETOUR(Player, bool, ZHM5ItemWeapon_FireProjectiles, ZHM5ItemWeapon* th, bool bMayStartSound) {
    if (!m_IsSuperAccuracyEnabled) {
        return { HookAction::Continue() };
    }

    const auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        return { HookAction::Continue() };
    }

    if (th->m_pOwner != s_LocalHitman.m_entityRef) {
        return { HookAction::Continue() };
    }

    bool s_Result = p_Hook->CallOriginal(th, bMayStartSound);

    th->m_fPrecisionFactor = 0.f;

    return { HookAction::Return(), s_Result };
}

DEFINE_PLUGIN_DETOUR(Player, bool, ZHM5ItemWeapon_IsFiring, IFirearm* th) {
    if (!m_IsRapidFireEnabled) {
        return { HookAction::Continue() };
    }

    const auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        return { HookAction::Continue() };
    }

    ZHM5ItemWeapon* s_HM5ItemWeapon = static_cast<ZHM5ItemWeapon*>(th);

    if (s_HM5ItemWeapon->m_pOwner != s_LocalHitman.m_entityRef) {
        return { HookAction::Continue() };
    }

    s_HM5ItemWeapon->m_tLastShootTime = ZGameTime {};

    return { HookAction::Continue() };
}

DEFINE_PLUGIN_DETOUR(Player, bool, ZActor_YouGotHit, IBaseCharacter* th, const SHitInfo& hitInfo) {
    if (m_IsOneHitKillEnabled) {
        ZActor* s_Actor = static_cast<ZActor*>(th);

        s_Actor->m_fCurrentHitPoints = 0.f;
    }

    return { HookAction::Continue() };
}

// While invisible, 47 is never identified. Before the game broadcasts its AI state (the
// "Compromised" HUD, signals to NPCs), clear what points at 47: broken / suspicious /
// investigated disguise, 47 in vision, trespassing spotted, and the tension towards him.
// The general tension is left untouched: a gunshot is still heard and a body is still found
// and investigated, NPCs just don't know who did it.
// Note: capping ZKnowledge::SetGameTension instead makes the game crash ~60-75 s after a kill
// (guards keep requesting combat in a loop).
DEFINE_PLUGIN_DETOUR(Player, void, ZGameStatsManager_SendAISignals, ZGameStatsManager* th) {
    if (m_IsInvisible && th) {
        auto& s_State = th->m_gameState;

        s_State.m_bDisguiseBroken = false;
        s_State.m_bDisguiseSuspicious = false;
        s_State.m_bDisguiseBeingInvestigated = false;
        s_State.m_bHitmanInVision = false;
        s_State.m_bHitmanTrespassingSpotted = false;
        s_State.m_eGameTensionToHitmanGuard = EGameTension::EGT_Ambient;
        s_State.m_eGameTensionToHitmanCivilian = EGameTension::EGT_Ambient;
    }

    return { HookAction::Continue() };
}

DEFINE_ZHM_PLUGIN(Player);
