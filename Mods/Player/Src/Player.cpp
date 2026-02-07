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

#include <Util/ImGuiUtils.h>

#undef min

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
}

void Player::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_MAN " PLAYER")) {
        m_PlayerMenuActive = !m_PlayerMenuActive;
    }
}

void Player::OnDrawUI(const bool p_HasFocus) {
    if (!p_HasFocus || !m_PlayerMenuActive) {
        return;
    }

    ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;

    auto s_LocalHitman = SDK()->GetLocalPlayer();

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("PLAYER", &m_PlayerMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing) {
        if (ImGui::Checkbox("Is Invincible", &m_IsInvincible)) {
            ToggleInvincibility();
        }

        if (ImGui::Checkbox("Is Invisible", &m_IsInvisible)) {
            ToggleInvisibility();
        }

        if (ImGui::Checkbox("Infinite Ammo", &m_IsInfiniteAmmoEnabled)) {
            ToggleInfiniteAmmo();
        }

        ImGui::Checkbox("No Reload", &m_IsNoReloadEnabled);

        ImGui::Checkbox("No Recoil", &m_IsNoRecoilEnabled);

        ImGui::Checkbox("Super Accuracy", &m_IsSuperAccuracyEnabled);

        ImGui::Checkbox("RapidFire", &m_IsRapidFireEnabled);

        ImGui::Checkbox("One Hit Kill", &m_IsOneHitKillEnabled);

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
                        s_CurrentCharSetCharacterType.data(),
                        s_CurrentOutfitVariationIndex,
                        s_LocalHitman.m_pInterfaceRef
                    );

                    m_GlobalOutfitKit = p_GlobalOutfitKit;
            },
            [](auto& p_Pair) -> const TEntityRef<ZGlobalOutfitKit>& { return p_Pair.second; }
        );

        if (!s_IsPopupOpen && s_LocalHitman && s_OutfitName[0] == '\0') {
            const char* s_OutfitName2 = s_LocalHitman.m_pInterfaceRef->m_rOutfitKit.m_pInterfaceRef->m_sCommonName.
                c_str();

            strncpy(s_OutfitName, s_OutfitName2, sizeof(s_OutfitName) - 1);

            s_OutfitName[sizeof(s_OutfitName) - 1] = '\0';
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Character Set Index");
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
        ImGui::Text("CharSet Character Type");
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
            ImGui::Checkbox("Weapons Allowed", &m_GlobalOutfitKit.m_pInterfaceRef->m_bWeaponsAllowed);
            ImGui::Checkbox("Authority Figure", &m_GlobalOutfitKit.m_pInterfaceRef->m_bAuthorityFigure);
        }

        ImGui::Separator();

        ImGui::Text("Get Actor's Outfit");

        ImGui::Spacing();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("CharSet Character Type");
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
        ImGui::Text("Actor Name");
        ImGui::SameLine();

        static char s_ActorName[2048]{ "" };

        Util::ImGuiUtils::InputWithAutocomplete(
            "##ActorName",
            s_ActorName,
            sizeof(s_ActorName),
            Globals::ActorManager->m_activatedActors,
            [](auto& p_EntityRef) -> const ZEntityRef& { return p_EntityRef.m_entityRef; },
            [](auto& p_EntityRef) -> std::string {
                return std::string(
                    p_EntityRef.m_pInterfaceRef->m_sActorName.c_str(),
                    p_EntityRef.m_pInterfaceRef->m_sActorName.size()
                );
            },
            [&](const ZEntityRef&, const std::string& actorName, const TEntityRef<ZActor>& p_EntityRef) {
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
        ImGui::Text("CharSet Character Type");
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

        if (ImGui::Button("Get Nearest Actor's Outfit")) {
            const ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();

            for (int i = 0; i < *Globals::NextActorId; ++i) {
                ZActor* actor = Globals::ActorManager->m_activatedActors[i].m_pInterfaceRef;
                ZEntityRef s_Ref;

                actor->GetID(s_Ref);

                ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

                const SVector3 s_Temp = s_ActorSpatialEntity->m_mTransform.Trans - s_HitmanSpatialEntity->m_mTransform.
                    Trans;
                const float s_Distance = sqrt(s_Temp.x * s_Temp.x + s_Temp.y * s_Temp.y + s_Temp.z * s_Temp.z);

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

        if (ImGui::Button("Teleport All Items To Player")) {
            auto s_HitmanSpatial = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();
            const ZHM5ActionManager* s_Hm5ActionManager = Globals::HM5ActionManager;

            for (size_t i = 0; i < s_Hm5ActionManager->m_Actions.size(); ++i) {
                const ZHM5Action* s_Action = s_Hm5ActionManager->m_Actions[i];

                if (s_Action->m_eActionType == EActionType::AT_PICKUP) {
                    const ZHM5Item* s_Item = s_Action->m_Object.QueryInterface<ZHM5Item>();

                    s_Item->m_rGeomentity.m_pInterfaceRef->SetObjectToWorldMatrixFromEditor(
                        s_HitmanSpatial->GetObjectToWorldMatrix()
                    );
                }
            }
        }

        if (ImGui::Button("Teleport All Actors To Player")) {
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
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
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

    std::vector<ZRuntimeResourceID> s_OriginalHeroVariations;

    if (p_CharSetCharacterType != "HeroA") {
        auto* s_HeroType = &s_Collection->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_HeroA)];

        if (!s_HeroType->m_pInterfaceRef) {
            Logger::Error("Couldn't equip outfit - hero character type is null!");
            return;
        }

        TEntityRef<ZCharsetCharacterType>* s_TargetType = nullptr;

        if (p_CharSetCharacterType == "Actor") {
            s_TargetType = &s_Collection->m_aCharacters[static_cast<size_t>(ECharSetCharacterType::ECSCT_Actor)];
        }
        else if (p_CharSetCharacterType == "Nude") {
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

    if (p_CharSetCharacterType != "HeroA" && !s_OriginalHeroVariations.empty()) {
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

void Player::ToggleInvincibility() {
    bool s_IsAICrippleEntityCreated = m_AICrippleEntity;

    if (!s_IsAICrippleEntityCreated) {
        s_IsAICrippleEntityCreated = CreateAICrippleEntity();
    }

    if (s_IsAICrippleEntityCreated) {
        const std::string s_PinName = m_IsInvincible ? "SetHeroInvincible" : "SetHeroVulnerable";

        m_AICrippleEntity.SignalInputPin(s_PinName);
    }
}

void Player::ToggleInvisibility() {
    bool s_IsAICrippleEntityCreated = m_AICrippleEntity;

    if (!s_IsAICrippleEntityCreated) {
        s_IsAICrippleEntityCreated = CreateAICrippleEntity();
    }

    if (s_IsAICrippleEntityCreated) {
        const std::string s_PinName = m_IsInvisible ? "SetHeroHidden" : "SetHeroVisible";

        m_AICrippleEntity.SignalInputPin(s_PinName);
    }
}

void Player::ToggleInfiniteAmmo() {
    if (m_IsInfiniteAmmoEnabled) {
        auto s_LocalHitman = SDK()->GetLocalPlayer();

        if (!s_LocalHitman) {
            Logger::Debug("Local player is not alive.");

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
    } else {
        if (m_HM5CrippleBoxEntity) {
            Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, m_HM5CrippleBoxEntity, {});

            m_HM5CrippleBoxEntity = {};
        }
    }
}

bool Player::CreateAICrippleEntity() {
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Error("Scene not loaded!");

        return false;
    }

    constexpr auto s_AICrippleEntityFactoryId = ResId<"[modules:/zaicrippleentity.class].pc_entitytype">;

    TResourcePtr<ZTemplateEntityFactory> s_AICrippleEntityFactory;
    Globals::ResourceManager->GetResourcePtr(s_AICrippleEntityFactory, s_AICrippleEntityFactoryId, 0);

    if (!s_AICrippleEntityFactory) {
        Logger::Error("Resource is not loaded!");

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
        Logger::Error("Failed to spawn AI Cripple Entity entity!");

        return false;
    }

    return true;
}

bool Player::CreateHM5CrippleBoxEntity() {
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Debug("Scene not loaded.");

        return false;
    }

    constexpr auto s_CrippleBoxFactoryId = ResId<"[modules:/zhm5cripplebox.class].pc_entitytype">;

    TResourcePtr<ZTemplateEntityFactory> s_CrippleBoxFactory;
    Globals::ResourceManager->GetResourcePtr(s_CrippleBoxFactory, s_CrippleBoxFactoryId, 0);

    if (!s_CrippleBoxFactory) {
        Logger::Debug("Resource is not loaded.");

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
        Logger::Debug("Failed to spawn entity.");

        return false;
    }

    return true;
}

DEFINE_PLUGIN_DETOUR(Player, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene) {
    if (m_AICrippleEntity) {
        Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, m_AICrippleEntity, {});

        m_AICrippleEntity = {};
    }

    if (m_HM5CrippleBoxEntity) {
        Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, m_HM5CrippleBoxEntity, {});

        m_HM5CrippleBoxEntity = {};
    }

    m_IsInvincible = false;
    m_IsInvisible = false;
    m_IsInfiniteAmmoEnabled = false;
    m_GlobalOutfitKit = {};

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(
    Player,
    void,
    ZSecuritySystemCameraManager_OnFrameUpdate,
    ZSecuritySystemCameraManager* th,
    const SGameUpdateEvent& updateEvent
) {
    if (m_IsInvisible) {
        return HookResult<void>(HookAction::Return());
    }

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(
    Player,
    void,
    ZSecuritySystemCamera_FrameUpdate,
    ZSecuritySystemCamera* th,
    const SGameUpdateEvent& updateEvent
) {
    if (m_IsInvisible) {
        return HookResult<void>(HookAction::Return());
    }

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Player, void, ZHM5ItemWeapon_SetBulletsInMagazine, IFirearm* th, int32_t nBullets) {
    if (!m_IsNoReloadEnabled) {
        return HookResult<void>(HookAction::Continue());
    }

    const auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        return HookResult<void>(HookAction::Continue());
    }

    ZHM5ItemWeapon* s_HM5ItemWeapon = static_cast<ZHM5ItemWeapon*>(th);

    if (s_HM5ItemWeapon->m_pOwner != s_LocalHitman.m_entityRef) {
        return HookResult<void>(HookAction::Continue());
    }

    if (s_HM5ItemWeapon->m_nBulletsFired == s_HM5ItemWeapon->m_nBulletsToFire) {
        s_HM5ItemWeapon->m_nBulletsFired = 0;
    }

    if (nBullets != 0) {
        return HookResult<void>(HookAction::Continue());
    }

    const bool s_IsInfiniteAmmoEnabled = !s_LocalHitman.m_pInterfaceRef->IsCustomFlagEnabled(
        static_cast<ECustomFlags>(0x2000000)
    );

    if (!s_IsInfiniteAmmoEnabled) {
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

    return HookResult<void>(HookAction::Return());
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
        return HookResult<void>(HookAction::Return());
    }

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(
    Player,
    void,
    ZHM5WeaponRecoilController_RecoilWeapon,
    ZHM5WeaponRecoilController* th,
    const TEntityRef<ZHM5ItemWeapon>& rWeapon
) {
    if (!m_IsNoRecoilEnabled) {
        return HookResult<void>(HookAction::Continue());
    }

    p_Hook->CallOriginal(th, rWeapon);

    th->m_vRecoil = SVector2(0.f, 0.f);

    return HookResult<void>(HookAction::Return());
}

DEFINE_PLUGIN_DETOUR(Player, bool, ZHM5ItemWeapon_FireProjectiles, ZHM5ItemWeapon* th, bool bMayStartSound) {
    if (!m_IsSuperAccuracyEnabled) {
        return HookResult<bool>(HookAction::Continue());
    }

    const auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        return HookResult<bool>(HookAction::Continue());
    }

    if (th->m_pOwner != s_LocalHitman.m_entityRef) {
        return HookResult<bool>(HookAction::Continue());
    }

    bool s_Result = p_Hook->CallOriginal(th, bMayStartSound);

    th->m_fPrecisionFactor = 0.f;

    return HookResult<bool>(HookAction::Return(), s_Result);
}

DEFINE_PLUGIN_DETOUR(Player, bool, ZHM5ItemWeapon_IsFiring, IFirearm* th) {
    if (!m_IsRapidFireEnabled) {
        return HookResult<bool>(HookAction::Continue());
    }

    const auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        return HookResult<bool>(HookAction::Continue());
    }

    ZHM5ItemWeapon* s_HM5ItemWeapon = static_cast<ZHM5ItemWeapon*>(th);

    if (s_HM5ItemWeapon->m_pOwner != s_LocalHitman.m_entityRef) {
        return HookResult<bool>(HookAction::Continue());
    }

    s_HM5ItemWeapon->m_tLastShootTime = ZGameTime {};

    return HookResult<bool>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Player, bool, ZActor_YouGotHit, IBaseCharacter* th, const SHitInfo& hitInfo) {
    if (m_IsOneHitKillEnabled) {
        ZActor* s_Actor = static_cast<ZActor*>(th);

        s_Actor->m_fCurrentHitPoints = 0.f;
    }

    return HookResult<bool>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(Player);
