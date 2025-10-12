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

void Player::Init()
{
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &Player::OnClearScene);
}

void Player::OnDrawMenu()
{
    if (ImGui::Button(ICON_MD_MAN " PLAYER MENU"))
    {
        m_PlayerMenuActive = !m_PlayerMenuActive;
    }
}

void Player::OnDrawUI(const bool p_HasFocus)
{
    if (!p_HasFocus || !m_PlayerMenuActive)
    {
        return;
    }

    ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;

    auto s_LocalHitman = SDK()->GetLocalPlayer();

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("PLAYER", &m_PlayerMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing)
    {
        if (s_LocalHitman)
        {
            static bool s_IsInvincible = s_LocalHitman.m_ref.GetProperty<bool>("m_bIsInvincible").Get();

            if (ImGui::Checkbox("Is Invincible", &s_IsInvincible))
            {
                s_LocalHitman.m_ref.SetProperty("m_bIsInvincible", s_IsInvincible);
            }

            if (ImGui::Button("Enable Infinite Ammo"))
            {
                EnableInfiniteAmmo();
            }
        }

        static char s_OutfitName[2048]{ "" };

        if (s_OutfitName[0] == '\0')
        {
            const char* s_OutfitName2 = s_LocalHitman.m_pInterfaceRef->m_rOutfitKit.m_pInterfaceRef->m_sCommonName.c_str();

            strncpy(s_OutfitName, s_OutfitName2, sizeof(s_OutfitName) - 1);

            s_OutfitName[sizeof(s_OutfitName) - 1] = '\0';
        }

        ImGui::Text("Outfit");
        ImGui::SameLine();

        const bool s_IsInputTextEnterPressed = ImGui::InputText(
            "##OutfitName", s_OutfitName, sizeof(s_OutfitName), ImGuiInputTextFlags_EnterReturnsTrue
        );
        const bool s_IsInputTextActive = ImGui::IsItemActive();

        if (ImGui::IsItemActivated())
        {
            ImGui::OpenPopup("##popup");
        }

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

        static uint8_t s_CurrentCharacterSetIndex = 0;
        static std::string s_CurrentCharSetCharacterType = "HeroA";
        static std::string s_CurrentCharSetCharacterType2 = "HeroA";
        static uint8_t s_CurrentOutfitVariationIndex = 1;

        if (!m_GlobalOutfitKit)
        {
            m_GlobalOutfitKit = &s_LocalHitman.m_pInterfaceRef->m_rOutfitKit;
            s_CurrentCharacterSetIndex = s_LocalHitman.m_pInterfaceRef->m_nOutfitCharset;
            s_CurrentOutfitVariationIndex = s_LocalHitman.m_pInterfaceRef->m_nOutfitVariation;
        }

        if (ImGui::BeginPopup(
            "##popup",
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_ChildWindow
        ))
        {
            for (auto it = s_ContentKitManager->m_repositoryGlobalOutfitKits.begin(); it != s_ContentKitManager->
                m_repositoryGlobalOutfitKits.end(); ++it)
            {
                TEntityRef<ZGlobalOutfitKit>* s_GlobalOutfitKit = &it->second;
                const char* s_OutfitName2 = s_GlobalOutfitKit->m_pInterfaceRef->m_sCommonName.c_str();

                if (!strstr(s_OutfitName2, s_OutfitName))
                {
                    continue;
                }

                if (ImGui::Selectable(s_OutfitName2))
                {
                    ImGui::ClearActiveID();
                    strcpy_s(s_OutfitName, s_OutfitName2);

                    s_CurrentCharacterSetIndex = 0;
                    s_CurrentOutfitVariationIndex = 0;

                    EquipOutfit(
                        it->second,
                        s_CurrentCharacterSetIndex,
                        s_CurrentCharSetCharacterType.data(),
                        s_CurrentOutfitVariationIndex,
                        s_LocalHitman.m_pInterfaceRef
                    );

                    m_GlobalOutfitKit = s_GlobalOutfitKit;
                }
            }

            if (s_IsInputTextEnterPressed || (!s_IsInputTextActive && !ImGui::IsWindowFocused()))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::Text("Character Set Index");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharacterSetIndex", std::to_string(s_CurrentCharacterSetIndex).data()))
        {
            if (m_GlobalOutfitKit)
            {
                for (size_t i = 0; i < m_GlobalOutfitKit->m_pInterfaceRef->m_aCharSets.size(); ++i)
                {
                    const bool s_IsSelected = s_CurrentCharacterSetIndex == i;

                    if (ImGui::Selectable(std::to_string(i).data(), s_IsSelected))
                    {
                        s_CurrentCharacterSetIndex = i;

                        if (m_GlobalOutfitKit)
                        {
                            EquipOutfit(
                                *m_GlobalOutfitKit, s_CurrentCharacterSetIndex, s_CurrentCharSetCharacterType.data(),
                                s_CurrentOutfitVariationIndex, s_LocalHitman.m_pInterfaceRef
                            );
                        }
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentCharSetCharacterType.data()))
        {
            if (m_GlobalOutfitKit)
            {
                for (size_t i = 0; i < 3; ++i)
                {
                    const bool s_IsSelected = s_CurrentCharSetCharacterType == m_CharSetCharacterTypes[i];

                    if (ImGui::Selectable(m_CharSetCharacterTypes[i].data(), s_IsSelected))
                    {
                        s_CurrentCharSetCharacterType = m_CharSetCharacterTypes[i].data();

                        if (m_GlobalOutfitKit)
                        {
                            EquipOutfit(
                                *m_GlobalOutfitKit, s_CurrentCharacterSetIndex, s_CurrentCharSetCharacterType.data(),
                                s_CurrentOutfitVariationIndex, s_LocalHitman.m_pInterfaceRef
                            );
                        }
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("Outfit Variation");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##OutfitVariation", std::to_string(s_CurrentOutfitVariationIndex).data()))
        {
            if (m_GlobalOutfitKit)
            {
                const auto s_CurrentCharacterSetIndex2 = s_CurrentCharacterSetIndex;
                const size_t s_VariationCount = m_GlobalOutfitKit->m_pInterfaceRef->m_aCharSets[
                    s_CurrentCharacterSetIndex2].m_pInterfaceRef->m_aCharacters[0].m_pInterfaceRef->
                        m_aVariations.
                        size();

                    for (size_t i = 0; i < s_VariationCount; ++i)
                    {
                        const bool s_IsSelected = s_CurrentOutfitVariationIndex == i;

                        if (ImGui::Selectable(std::to_string(i).data(), s_IsSelected))
                        {
                            s_CurrentOutfitVariationIndex = i;

                            if (m_GlobalOutfitKit)
                            {
                                EquipOutfit(
                                    *m_GlobalOutfitKit, s_CurrentCharacterSetIndex, s_CurrentCharSetCharacterType.data(),
                                    s_CurrentOutfitVariationIndex, s_LocalHitman.m_pInterfaceRef
                                );
                            }
                        }
                    }
            }

            ImGui::EndCombo();
        }

        if (m_GlobalOutfitKit)
        {
            ImGui::Checkbox("Weapons Allowed", &m_GlobalOutfitKit->m_pInterfaceRef->m_bWeaponsAllowed);
            ImGui::Checkbox("Authority Figure", &m_GlobalOutfitKit->m_pInterfaceRef->m_bAuthorityFigure);
        }

        ImGui::Separator();

        static char npcName[2048]{ "" };

        ImGui::Text("NPC Name");
        ImGui::SameLine();

        ImGui::InputText("##NPCName", npcName, sizeof(npcName));
        ImGui::SameLine();

        if (ImGui::Button("Get NPC Outfit"))
        {
            const ZActor* s_Actor = Globals::ActorManager->GetActorByName(npcName);

            if (s_Actor)
            {
                EquipOutfit(
                    s_Actor->m_rOutfit, s_Actor->m_nOutfitCharset, s_CurrentCharSetCharacterType2.data(),
                    s_Actor->m_nOutfitVariation, s_LocalHitman.m_pInterfaceRef
                );
            }
        }

        if (ImGui::Button("Get Nearest NPC's Outfit"))
        {
            const ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

            for (int i = 0; i < *Globals::NextActorId; ++i)
            {
                ZActor* actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
                ZEntityRef s_Ref;

                actor->GetID(&s_Ref);

                ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

                const SVector3 s_Temp = s_ActorSpatialEntity->m_mTransform.Trans - s_HitmanSpatialEntity->m_mTransform.
                    Trans;
                const float s_Distance = sqrt(s_Temp.x * s_Temp.x + s_Temp.y * s_Temp.y + s_Temp.z * s_Temp.z);

                if (s_Distance <= 3.0f)
                {
                    EquipOutfit(
                        actor->m_rOutfit, actor->m_nOutfitCharset, s_CurrentCharSetCharacterType2.data(),
                        actor->m_nOutfitVariation, s_LocalHitman.m_pInterfaceRef
                    );

                    break;
                }
            }
        }

        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType2", s_CurrentCharSetCharacterType2.data()))
        {
            if (m_GlobalOutfitKit)
            {
                for (size_t i = 0; i < 3; ++i)
                {
                    const bool s_IsSelected = s_CurrentCharSetCharacterType2 == m_CharSetCharacterTypes[i];

                    if (ImGui::Selectable(m_CharSetCharacterTypes[i].data(), s_IsSelected))
                    {
                        s_CurrentCharSetCharacterType2 = m_CharSetCharacterTypes[i];
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Separator();

        if (ImGui::Button("Teleport All Items To Player"))
        {
            auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
            const ZHM5ActionManager* s_Hm5ActionManager = Globals::HM5ActionManager;

            for (size_t i = 0; i < s_Hm5ActionManager->m_Actions.size(); ++i)
            {
                const ZHM5Action* s_Action = s_Hm5ActionManager->m_Actions[i];

                if (s_Action->m_eActionType == EActionType::AT_PICKUP)
                {
                    const ZHM5Item* s_Item = s_Action->m_Object.QueryInterface<ZHM5Item>();

                    s_Item->m_rGeomentity.m_pInterfaceRef->SetWorldMatrix(s_HitmanSpatial->GetWorldMatrix());
                }
            }
        }

        if (ImGui::Button("Teleport All NPCs To Player"))
        {
            const auto s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

            for (size_t i = 0; i < *Globals::NextActorId; ++i)
            {
                ZActor* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
                ZEntityRef s_Ref;

                s_Actor->GetID(&s_Ref);

                ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

                s_ActorSpatialEntity->SetWorldMatrix(s_HitmanSpatialEntity->GetWorldMatrix());
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
)
{
    std::vector<ZRuntimeResourceID> s_HeroOutfitVariations;

    if (p_CharSetCharacterType != "HeroA")
    {
        const ZOutfitVariationCollection* s_OutfitVariationCollection = p_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[
            p_CharSetIndex].m_pInterfaceRef;

        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType2 = &s_OutfitVariationCollection->m_aCharacters[
            2];
        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType = nullptr;

        if (p_CharSetCharacterType == "Actor")
        {
            s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[0];
        }
        else if (p_CharSetCharacterType == "Nude")
        {
            s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[1];
        }

        for (size_t i = 0; i < s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
        {
            s_HeroOutfitVariations.push_back(
                s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit
            );
        }

        if (s_CharsetCharacterType)
        {
            for (size_t i = 0; i < s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
            {
                s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit =
                    s_CharsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit;
            }
        }
    }

    Functions::ZHitman5_SetOutfit->Call(
        p_Hitman, p_GlobalOutfitKit, p_CharSetIndex, p_OutfitVariationIndex, false, false
    );

    if (p_CharSetCharacterType != "HeroA")
    {
        const auto* s_OutfitVariationCollection = p_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[p_CharSetIndex].
            m_pInterfaceRef;
        const auto* s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[2];

        for (size_t i = 0; i < s_HeroOutfitVariations.size(); ++i)
        {
            s_CharsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = s_HeroOutfitVariations[
                i];
        }
    }
}

void Player::EnableInfiniteAmmo()
{
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene)
    {
        Logger::Debug("Scene not loaded.");
        return;
    }

    constexpr auto s_CrippleBoxFactoryId = ResId<"[modules:/zhm5cripplebox.class].pc_entitytype">;

    TResourcePtr<ZTemplateEntityFactory> s_CrippleBoxFactory;
    Globals::ResourceManager->GetResourcePtr(s_CrippleBoxFactory, s_CrippleBoxFactoryId, 0);

    if (!s_CrippleBoxFactory)
    {
        Logger::Debug("Resource is not loaded.");
        return;
    }

    ZEntityRef s_NewCrippleBox;
    SExternalReferences s_ExternalRefs;

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager, s_NewCrippleBox, "", s_CrippleBoxFactory, s_Scene.m_ref, s_ExternalRefs, -1
    );

    if (!s_NewCrippleBox)
    {
        Logger::Debug("Failed to spawn entity.");
        return;
    }

    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman)
    {
        Logger::Debug("Local player is not alive.");
        return;
    }

    ZHM5CrippleBox* s_HM5CrippleBox = s_NewCrippleBox.QueryInterface<ZHM5CrippleBox>();

    s_HM5CrippleBox->m_bActivateOnStart = true;
    s_HM5CrippleBox->m_rHitmanCharacter = s_LocalHitman;
    s_HM5CrippleBox->m_bLimitedAmmo = false;

    s_HM5CrippleBox->Activate(0);
}

DEFINE_PLUGIN_DETOUR(Player, void, OnClearScene, ZEntitySceneContext* th, bool forReload)
{
    m_GlobalOutfitKit = nullptr;

    return HookResult<void>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(Player);
