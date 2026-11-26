#include "Player.h"

#include <format>
#include <string>

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
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZInputActionManager.h>

#include <cfloat>
#include <cmath>

#include <Util/ImGuiUtils.h>

#undef min

Player::~Player() {
    const ZMemberDelegate<Player, void(const SGameUpdateEvent&)> s_Delegate(this, &Player::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);

    // Mod unloaded: do not leave the NPCs frozen with no checkbox to unfreeze them.
    SetNpcsFrozen(false);
}

// Freeze all NPCs. This is the game's own debug switch: while it is set, ZActorManager skips
// the AI update (and the situations update), NPCs stay where they are and react to nothing.
// No position is ever written. Tested in Paris: 45 s frozen, then everything resumes.
bool Player::AreNpcsFrozen() {
    return Globals::ActorManager && Globals::ActorManager->m_bDisableAIBehavior;
}

void Player::SetNpcsFrozen(bool p_Frozen) {
    if (!Globals::ActorManager || Globals::ActorManager->m_bDisableAIBehavior == p_Frozen) {
        return;
    }

    Globals::ActorManager->m_bDisableAIBehavior = p_Frozen;
    Logger::Info("[Player] NPCs frozen: {}", p_Frozen);
}

void Player::OnEngineInitialized() {
    const ZMemberDelegate<Player, void(const SGameUpdateEvent&)> s_Delegate(this, &Player::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);
}

void Player::LoadOptions() {
    m_RememberOptions = GetSettingBool("options", "remember", true);

    if (!m_RememberOptions) {
        return;
    }

    m_IsInvincible = GetSettingBool("options", "invincible", false);
    m_IsInvisible = GetSettingBool("options", "invisible", false);
    m_IsInfiniteAmmoEnabled = GetSettingBool("options", "infinite_ammo", false);
    m_IsNoReloadEnabled = GetSettingBool("options", "no_reload", false);
    m_IsNoRecoilEnabled = GetSettingBool("options", "no_recoil", false);
    m_IsSuperAccuracyEnabled = GetSettingBool("options", "super_accuracy", false);
    m_IsRapidFireEnabled = GetSettingBool("options", "rapid_fire", false);
    m_IsOneHitKillEnabled = GetSettingBool("options", "one_hit_kill", false);
}

void Player::SaveOptions() {
    SetSettingBool("options", "remember", m_RememberOptions);
    SetSettingBool("options", "invincible", m_IsInvincible);
    SetSettingBool("options", "invisible", m_IsInvisible);
    SetSettingBool("options", "infinite_ammo", m_IsInfiniteAmmoEnabled);
    SetSettingBool("options", "no_reload", m_IsNoReloadEnabled);
    SetSettingBool("options", "no_recoil", m_IsNoRecoilEnabled);
    SetSettingBool("options", "super_accuracy", m_IsSuperAccuracyEnabled);
    SetSettingBool("options", "rapid_fire", m_IsRapidFireEnabled);
    SetSettingBool("options", "one_hit_kill", m_IsOneHitKillEnabled);
}

// Layout check: the fields the game describes itself (reflected properties) must be where the
// SDK expects them. After a game update that moves ZHitman5, invincibility and invisibility
// (which write to undescribed fields between these markers) are disabled instead of crashing.
bool Player::CheckLayout(const TEntityRef<ZHitman5>& p_Hitman) {
    ZHitman5* s_Hitman = p_Hitman.m_pInterfaceRef;
    auto* s_Entity = p_Hitman.m_entityRef.GetEntity();

    if (!s_Hitman || !s_Entity || !s_Entity->GetType()) {
        return true;
    }

    struct SField {
        const char* Name;
        uintptr_t Address;
    };

    const SField s_Fields[] = {
        { "m_InitialOutfitId", reinterpret_cast<uintptr_t>(&s_Hitman->m_InitialOutfitId) },
        { "m_rOutfitKit", reinterpret_cast<uintptr_t>(&s_Hitman->m_rOutfitKit) },
        { "m_nOutfitCharset", reinterpret_cast<uintptr_t>(&s_Hitman->m_nOutfitCharset) },
        { "m_nOutfitVariation", reinterpret_cast<uintptr_t>(&s_Hitman->m_nOutfitVariation) },
    };

    const auto s_Base = reinterpret_cast<uintptr_t>(p_Hitman.m_entityRef.m_pObj);
    int s_Checked = 0;
    std::string s_Wrong;

    for (const auto& s_Field : s_Fields) {
        const auto* s_Property = s_Entity->GetType()->FindProperty(
            Hash::Crc32(s_Field.Name, strlen(s_Field.Name))
        );

        if (!s_Property) {
            continue;
        }

        ++s_Checked;

        if (s_Base + s_Property->m_nPropertyOffset != s_Field.Address) {
            s_Wrong += std::format(
                " {} (game +0x{:X}, SDK +0x{:X})", s_Field.Name, s_Property->m_nPropertyOffset,
                s_Field.Address - s_Base
            );
        }
    }

    if (s_Checked == 0) {
        Logger::Warn("[Player] Could not verify the ZHitman5 layout (no known property found).");
        return true;
    }

    if (!s_Wrong.empty()) {
        m_LayoutError = "The game was updated: ZHitman5 moved." + s_Wrong;
        Logger::Error("[Player] {} Invincibility and invisibility are disabled.", m_LayoutError);
        return false;
    }

    Logger::Info("[Player] ZHitman5 layout verified ({} fields).", s_Checked);
    return true;
}

void Player::ApplyHitmanState(ZHitman5* p_Hitman, bool p_OnLoad) {
    if (!p_Hitman || !m_LayoutOk) {
        return;
    }

    ZHM5Health* s_Health = p_Hitman->m_pHealth;

    if (p_OnLoad) {
        if (m_IsInvincible && s_Health) {
            s_Health->m_bInvincible.Set(true);
        }

        if (m_IsInvisible) {
            p_Hitman->m_bIsHiding = true;
            m_PendingHeroHidden = 1;
        }

        if (m_IsInfiniteAmmoEnabled && !m_HM5CrippleBoxEntity) {
            ToggleInfiniteAmmo();
        }

        return;
    }

    // Change requested (menu or shortcut).
    if (s_Health) {
        s_Health->m_bInvincible.Set(m_IsInvincible);
    }

    p_Hitman->m_bIsHiding = m_IsInvisible;
    m_PendingHeroHidden = m_IsInvisible ? 1 : 0;
}

// Bring the actor closest to the center of the screen (8 degrees at most) to 47.
void Player::TeleportAimedActor() {
    auto s_LocalHitman = SDK()->GetLocalPlayer();
    auto* s_Camera = Functions::GetCurrentCamera->Call();

    if (!s_LocalHitman || !s_Camera || !Globals::ActorManager) {
        return;
    }

    const SMatrix s_CameraMatrix = s_Camera->GetObjectToWorldMatrix();
    const float4 s_From = s_CameraMatrix.Trans;
    const float4 s_Forward = -s_CameraMatrix.ZAxis;

    ZActor* s_Best = nullptr;
    ZEntityRef s_BestRef;
    float s_BestCos = std::cos(8.f * 3.14159265f / 180.f);

    for (int i = 0; i < *Globals::NextActorId; ++i) {
        ZActor* s_Actor = Globals::ActorManager->m_activatedActors[i].m_pInterfaceRef;

        if (!s_Actor || !s_Actor->IsAlive()) {
            continue;
        }

        ZEntityRef s_Ref;
        s_Actor->GetID(s_Ref);

        const auto* s_Spatial = s_Ref.QueryInterface<ZSpatialEntity>();

        if (!s_Spatial) {
            continue;
        }

        const float4 s_Pos = s_Spatial->GetObjectToWorldMatrix().Trans;
        const float s_Dx = s_Pos.x - s_From.x;
        const float s_Dy = s_Pos.y - s_From.y;
        const float s_Dz = s_Pos.z + 1.2f - s_From.z;
        const float s_Dist = std::sqrt(s_Dx * s_Dx + s_Dy * s_Dy + s_Dz * s_Dz);

        if (s_Dist < 1.f || s_Dist > 120.f) {
            continue;
        }

        const float s_Cos = (s_Dx * s_Forward.x + s_Dy * s_Forward.y + s_Dz * s_Forward.z) / s_Dist;

        if (s_Cos > s_BestCos) {
            s_BestCos = s_Cos;
            s_Best = s_Actor;
            s_BestRef = s_Ref;
        }
    }

    if (!s_Best) {
        Logger::Info("[Player] No actor under the crosshair.");
        return;
    }

    // 1.5 m in front of 47, along the camera direction.
    SMatrix s_Target = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>()->GetObjectToWorldMatrix();
    const float s_Len = std::sqrt(s_Forward.x * s_Forward.x + s_Forward.y * s_Forward.y);

    if (s_Len > 0.01f) {
        s_Target.Trans.x += s_Forward.x / s_Len * 1.5f;
        s_Target.Trans.y += s_Forward.y / s_Len * 1.5f;
    }

    Logger::Info("[Player] Bringing {} to the player.", s_Best->m_sActorName.c_str());
    m_ActorTeleporter.Queue(s_BestRef, s_Best, s_Target);
}

// Every living actor around 47, in rings, one per frame (each teleport scans the actor's
// memory, which costs a few dozen milliseconds: all at once froze the game for seconds).
void Player::BuildTeleportAllQueue() {
    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman || !Globals::ActorManager) {
        return;
    }

    const SMatrix s_Center = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>()->GetObjectToWorldMatrix();
    m_TeleportAllQueue.clear();

    int s_Index = 0;

    for (int i = 0; i < *Globals::NextActorId; ++i) {
        ZActor* s_Actor = Globals::ActorManager->m_activatedActors[i].m_pInterfaceRef;

        if (!s_Actor || !s_Actor->IsAlive()) {
            continue;
        }

        ZEntityRef s_Ref;
        s_Actor->GetID(s_Ref);

        const int s_Ring = s_Index / 12;
        const float s_Radius = 1.5f + 1.2f * s_Ring;
        const float s_Angle = (s_Index % 12) * (2.f * 3.14159265f / 12.f) + s_Ring * 0.26f;

        SMatrix s_Target = s_Center;
        s_Target.Trans.x += std::cos(s_Angle) * s_Radius;
        s_Target.Trans.y += std::sin(s_Angle) * s_Radius;

        m_TeleportAllQueue.push_back(PendingTeleport { s_Ref, s_Actor, s_Target });
        ++s_Index;
    }

    Logger::Info("[Player] Teleporting {} actors to the player, one per frame.", m_TeleportAllQueue.size());
}

void Player::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    auto s_LocalHitman = SDK()->GetLocalPlayer();
    ZHitman5* s_Hitman = s_LocalHitman.m_pInterfaceRef;

    // New mission (or new 47): layout check, then options applied again.
    if (s_Hitman != m_LastHitman) {
        m_LastHitman = s_Hitman;
        m_ApplyDelay = s_Hitman ? 30 : 0;
    }

    if (s_Hitman && m_ApplyDelay > 0 && --m_ApplyDelay == 0) {
        m_LayoutOk = CheckLayout(s_LocalHitman);
        ApplyHitmanState(s_Hitman, true);
    }

    if (s_Hitman && m_ApplyDelay == 0) {
        if (IsHotkeyPressed("invincible")) {
            m_IsInvincible = !m_IsInvincible;
            m_StateDirty = true;
            SaveOptions();
            Logger::Info("[Player] Invincible: {}", m_IsInvincible);
        }

        if (IsHotkeyPressed("invisible")) {
            m_IsInvisible = !m_IsInvisible;
            m_StateDirty = true;
            SaveOptions();
            Logger::Info("[Player] Invisible: {}", m_IsInvisible);
        }

        if (IsHotkeyPressed("teleport_aimed_actor")) {
            TeleportAimedActor();
        }

        if (IsHotkeyPressed("freeze_npcs")) {
            m_FreezeNpcsRequest = 2;
        }

        if (const int s_Request = m_FreezeNpcsRequest.exchange(-1); s_Request >= 0) {
            SetNpcsFrozen(s_Request == 2 ? !AreNpcsFrozen() : s_Request == 1);
        }

        if (m_StateDirty.exchange(false)) {
            ApplyHitmanState(s_Hitman, false);
        }

        // The game sometimes resets these states (leaving a hiding spot, cutscene).
        if (++m_EnforceTimer >= 20 && m_LayoutOk) {
            m_EnforceTimer = 0;

            if (m_IsInvincible && s_Hitman->m_pHealth && !s_Hitman->m_pHealth->m_bInvincible.Get()) {
                s_Hitman->m_pHealth->m_bInvincible.Set(true);
            }

            if (m_IsInvisible && !s_Hitman->m_bIsHiding) {
                s_Hitman->m_bIsHiding = true;
            }
        }

        if (m_TeleportAllRequested.exchange(false)) {
            BuildTeleportAllQueue();
        }

        if (!m_TeleportAllQueue.empty()) {
            const PendingTeleport s_Next = m_TeleportAllQueue.back();
            m_TeleportAllQueue.pop_back();
            m_ActorTeleporter.Queue(s_Next.Entity, s_Next.Actor, s_Next.Target);
        }
    }

    m_ActorTeleporter.Update();

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
    LoadOptions();

    // Shortcuts, configurable from the SDK menu (SHORTCUTS). Alt rather than Ctrl by
    // default: Ctrl makes 47 crouch.
    RegisterHotkey("invincible", "Toggle invincibility", "Alt+I");
    RegisterHotkey("invisible", "Toggle invisibility", "Alt+U");
    RegisterHotkey("teleport_aimed_actor", "Bring the actor under the crosshair to the player", "Alt+T");
    RegisterHotkey("freeze_npcs", "Freeze all NPCs (AI paused)", "Alt+G");

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
    // The window used to open too small to be readable.
    ImGui::SetNextWindowSizeConstraints(ImVec2(560.f, 560.f), ImVec2(FLT_MAX, FLT_MAX));
    const auto s_IsWindowExpanded = ImGui::Begin(ICON_MD_MAN " Player", &m_ShowPlayerWindow);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_IsWindowExpanded) {
        ImGui::BeginDisabled(!s_LocalHitman);

        ZHitman5* s_Hitman = s_LocalHitman.m_pInterfaceRef;

        if (ImGui::Checkbox("Remember options between missions", &m_RememberOptions)) {
            SaveOptions();
        }

        if (!m_LayoutOk) {
            ImGui::TextColored(ImVec4(1.f, 0.35f, 0.35f, 1.f), "%s", m_LayoutError.c_str());
        }

        ImGui::BeginDisabled(!m_LayoutOk);

        if (ImGui::Checkbox(std::format("Is invincible ({})###invincible", GetHotkeyText("invincible")).c_str(), &m_IsInvincible)) {
            m_StateDirty = true;
            SaveOptions();
        }

        if (ImGui::Checkbox(std::format("Is invisible ({})###invisible", GetHotkeyText("invisible")).c_str(), &m_IsInvisible)) {
            m_StateDirty = true;
            SaveOptions();
        }

        ImGui::EndDisabled();

        {
            bool s_Frozen = AreNpcsFrozen();

            if (ImGui::Checkbox(
                std::format("Freeze all NPCs ({})###freeze_npcs", GetHotkeyText("freeze_npcs")).c_str(), &s_Frozen
            )) {
                m_FreezeNpcsRequest = s_Frozen ? 1 : 0;
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("NPCs stop where they are and react to nothing until unfrozen.\nTurned off when the mission ends.");
            }
        }

        if (ImGui::Checkbox("Infinite ammo", &m_IsInfiniteAmmoEnabled)) {
            ToggleInfiniteAmmo();
            SaveOptions();
        }

        if (ImGui::Checkbox("No reload", &m_IsNoReloadEnabled)) {
            SaveOptions();
        }

        if (ImGui::Checkbox("No recoil", &m_IsNoRecoilEnabled)) {
            SaveOptions();
        }

        if (ImGui::Checkbox("Super accuracy", &m_IsSuperAccuracyEnabled)) {
            SaveOptions();
        }

        if (ImGui::Checkbox("Rapid fire", &m_IsRapidFireEnabled)) {
            SaveOptions();
        }

        if (ImGui::Checkbox("One hit kill", &m_IsOneHitKillEnabled)) {
            SaveOptions();
        }

        ImGui::TextDisabled(
            "%s: bring the actor under the crosshair to the player (shortcuts: SDK menu > SHORTCUTS)",
            GetHotkeyText("teleport_aimed_actor").c_str()
        );

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
            // Done from the game update, one actor per frame.
            m_TeleportAllRequested = true;
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
    // Freezing never carries over to the next mission.
    m_FreezeNpcsRequest = -1;
    SetNpcsFrozen(false);

    if (m_HM5CrippleBoxEntity) {
        Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, m_HM5CrippleBoxEntity, {});

        m_HM5CrippleBoxEntity = {};
    }

    // Remembered options stay on and are applied again in the next mission.
    if (!m_RememberOptions) {
        m_IsInfiniteAmmoEnabled = false;
        m_IsInvincible = false;
        m_IsInvisible = false;
    }

    m_GlobalOutfitKit = {};

    // These entities belong to the scene being unloaded.
    m_AICrippleEntity = {};
    m_PendingHeroHidden = -1;
    m_LastHitman = nullptr;
    m_TeleportAllQueue.clear();
    m_ActorTeleporter.Clear();

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
