#include "DebugMod.h"

#include <Glacier/ZActor.h>
#include <Glacier/ZContentKitManager.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZApplicationEngineWin32.h>
#include <Glacier/ZRender.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZHM5InputManager.h>
#include <Glacier/ZFreeCamera.h>

#include "imgui_internal.h"

void DebugMod::DrawNPCsBox(bool p_HasFocus)
{
    if (!p_HasFocus || !m_NPCsMenuActive)
    {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("NPCs", &m_NPCsMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing && p_HasFocus)
    {
        ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;
        static size_t s_Selected = 0;

        ImGui::BeginChild("left pane", ImVec2(300, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        static char s_NpcName[256] { "" };

        ImGui::Text("NPC Name");
        ImGui::SameLine();

        ImGui::InputText("##NPCName", s_NpcName, sizeof(s_NpcName));

        for (int i = 0; i < *Globals::NextActorId; ++i)
        {
            const ZActor* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
            std::string s_NpcName2 = s_Actor->m_sActorName.c_str();

            if (!strstr(s_NpcName2.c_str(), s_NpcName))
            {
                continue;
            }

            if (ImGui::Selectable(s_NpcName2.c_str(), s_Selected == i))
            {
                s_Selected = i;
            }
        }

        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

        ZActor* s_Actor = Globals::ActorManager->m_aActiveActors[s_Selected].m_pInterfaceRef;
        static char s_OutfitName[256] { "" };

        ImGui::Text("Outfit");
        ImGui::SameLine();

        const bool s_IsInputTextEnterPressed = ImGui::InputText("##OutfitName", s_OutfitName, sizeof(s_OutfitName), ImGuiInputTextFlags_EnterReturnsTrue);
        const bool s_IsInputTextActive = ImGui::IsItemActive();

        if (ImGui::IsItemActivated())
        {
            ImGui::OpenPopup("##popup");
        }

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

        static TEntityRef<ZGlobalOutfitKit>* s_GlobalOutfitKit = nullptr;
        static char s_CurrentCharacterSetIndex[3] { "0" };
        static const char* s_CurrentcharSetCharacterType = "Actor";
        static const char* s_CurrentcharSetCharacterType2 = "Actor";
        static char s_CurrentOutfitVariationIndex[3] { "0" };

        if (ImGui::BeginPopup("##popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
        {
            for (auto it = s_ContentKitManager->m_repositoryGlobalOutfitKits.begin(); it != s_ContentKitManager->m_repositoryGlobalOutfitKits.end(); ++it)
            {
                TEntityRef<ZGlobalOutfitKit>* s_GlobalOutfitKit2 = &it->second;
                const char* s_OutfitName2 = s_GlobalOutfitKit2->m_pInterfaceRef->m_sCommonName.c_str();

                if (!strstr(s_OutfitName2, s_OutfitName))
                {
                    continue;
                }

                if (ImGui::Selectable(s_OutfitName2))
                {
                    ImGui::ClearActiveID();
                    strcpy_s(s_OutfitName, s_OutfitName2);

                    EquipOutfit(it->second, std::stoi(s_CurrentCharacterSetIndex), s_CurrentcharSetCharacterType, std::stoi(s_CurrentOutfitVariationIndex), s_Actor);

                    s_GlobalOutfitKit = s_GlobalOutfitKit2;
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

        if (ImGui::BeginCombo("##CharacterSetIndex", s_CurrentCharacterSetIndex))
        {
            if (s_GlobalOutfitKit)
            {
                for (size_t i = 0; i < s_GlobalOutfitKit->m_pInterfaceRef->m_aCharSets.size(); ++i)
                {
                    std::string s_CharacterSetIndex = std::to_string(i);
                    const bool s_IsSelected = s_CurrentCharacterSetIndex == s_CharacterSetIndex.c_str();

                    if (ImGui::Selectable(s_CharacterSetIndex.c_str(), s_IsSelected))
                    {
                        strcpy_s(s_CurrentCharacterSetIndex, s_CharacterSetIndex.c_str());

                        if (s_GlobalOutfitKit)
                        {
                            EquipOutfit(*s_GlobalOutfitKit, std::stoi(s_CurrentCharacterSetIndex), s_CurrentcharSetCharacterType, std::stoi(s_CurrentOutfitVariationIndex), s_Actor);
                        }
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentcharSetCharacterType))
        {
            if (s_GlobalOutfitKit)
            {
                for (size_t i = 0; i < 3; ++i)
                {
                    const bool s_IsSelected = s_CurrentcharSetCharacterType == m_CharSetCharacterTypes[i];

                    if (ImGui::Selectable(m_CharSetCharacterTypes[i], s_IsSelected))
                    {
                        s_CurrentcharSetCharacterType = m_CharSetCharacterTypes[i];

                        if (s_GlobalOutfitKit)
                        {
                            EquipOutfit(*s_GlobalOutfitKit, std::stoi(s_CurrentCharacterSetIndex), s_CurrentcharSetCharacterType, std::stoi(s_CurrentOutfitVariationIndex), s_Actor);
                        }
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("Outfit Variation");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##OutfitVariation", s_CurrentOutfitVariationIndex))
        {
            if (s_GlobalOutfitKit)
            {
                const unsigned int s_CurrentCharacterSetIndex2 = std::stoi(s_CurrentCharacterSetIndex);
                const size_t s_VariationCount = s_GlobalOutfitKit->m_pInterfaceRef->m_aCharSets[s_CurrentCharacterSetIndex2].m_pInterfaceRef->m_aCharacters[0].m_pInterfaceRef->m_aVariations.size();

                for (size_t i = 0; i < s_VariationCount; ++i)
                {
                    std::string s_OutfitVariationIndex = std::to_string(i);
                    const bool s_IsSelected = s_CurrentOutfitVariationIndex == s_OutfitVariationIndex.c_str();

                    if (ImGui::Selectable(s_OutfitVariationIndex.c_str(), s_IsSelected))
                    {
                        strcpy_s(s_CurrentOutfitVariationIndex, s_OutfitVariationIndex.c_str());

                        if (s_GlobalOutfitKit)
                        {
                            EquipOutfit(*s_GlobalOutfitKit, std::stoi(s_CurrentCharacterSetIndex), s_CurrentcharSetCharacterType, std::stoi(s_CurrentOutfitVariationIndex), s_Actor);
                        }
                    }
                }
            }

            ImGui::EndCombo();
        }

        if (s_GlobalOutfitKit)
        {
            ImGui::Checkbox("Weapons Allowed", &s_GlobalOutfitKit->m_pInterfaceRef->m_bWeaponsAllowed);
            ImGui::Checkbox("Authority Figure", &s_GlobalOutfitKit->m_pInterfaceRef->m_bAuthorityFigure);
        }

        ImGui::Separator();

        static char s_NpcName2[256] { "" };

        ImGui::Text("NPC Name");
        ImGui::SameLine();

        ImGui::InputText("##NPCName", s_NpcName2, sizeof(s_NpcName2));
        ImGui::SameLine();

        if (ImGui::Button("Get NPC Outfit"))
        {
            const ZActor* s_Actor2 = Globals::ActorManager->GetActorByName(s_NpcName2);

            if (s_Actor2)
            {
                EquipOutfit(s_Actor2->m_rOutfit, s_Actor2->m_nOutfitCharset, s_CurrentcharSetCharacterType2, s_Actor2->m_nOutfitVariation, s_Actor);
            }
        }

        if (ImGui::Button("Get Nearest NPC's Outfit"))
        {
            ZEntityRef s_Ref;

            s_Actor->GetID(&s_Ref);

            ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

            for (int i = 0; i < *Globals::NextActorId; ++i)
            {
                ZActor* s_Actor2 = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
                ZEntityRef s_Ref;

                s_Actor2->GetID(&s_Ref);

                const ZSpatialEntity* s_ActorSpatialEntity2 = s_Ref.QueryInterface<ZSpatialEntity>();

                const SVector3 s_Temp = s_ActorSpatialEntity->m_mTransform.Trans - s_ActorSpatialEntity2->m_mTransform.Trans;
                const float s_Distance = sqrt(s_Temp.x * s_Temp.x + s_Temp.y * s_Temp.y + s_Temp.z * s_Temp.z);

                if (s_Distance <= 3.0f)
                {
                    EquipOutfit(s_Actor2->m_rOutfit, s_Actor2->m_nOutfitCharset, s_CurrentcharSetCharacterType2, s_Actor2->m_nOutfitVariation, s_Actor);

                    break;
                }
            }
        }

        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentcharSetCharacterType2))
        {
            if (s_GlobalOutfitKit)
            {
                for (size_t i = 0; i < 3; ++i)
                {
                    const bool s_IsSelected = s_CurrentcharSetCharacterType2 == m_CharSetCharacterTypes[i];

                    if (ImGui::Selectable(m_CharSetCharacterTypes[i], s_IsSelected))
                    {
                        s_CurrentcharSetCharacterType2 = m_CharSetCharacterTypes[i];
                    }
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::Button("Teleport NPC To Player"))
        {
            TEntityRef<ZHitman5> s_LocalHitman;
            Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

            if (s_LocalHitman)
            {
                ZEntityRef s_Ref;
                s_Actor->GetID(&s_Ref);

                ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
                ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

                s_ActorSpatialEntity->SetWorldMatrix(s_HitmanSpatialEntity->GetWorldMatrix());
            }
        }

        ImGui::Separator();

        if (ImGui::Button(std::format("Track This NPC##{}", s_NpcName).c_str()))
        {
            if (m_RenderDest.m_ref == nullptr) GetRenderDest();
            if (m_TrackCam.m_ref == nullptr) GetTrackCam();
            if (m_PlayerCam == nullptr) GetPlayerCam();
            m_NPCTracked = s_Actor;
            m_TrackCamActive = true;
            EnableTrackCam();
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop Tracking"))
        {
            m_TrackCamActive = false;
            m_NPCTracked = nullptr;
            DisableTrackCam();
        }

        ImGui::EndChild();
        ImGui::EndGroup();
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

void DebugMod::EnableTrackCam()
{
    m_RenderDest.m_pInterfaceRef->SetSource(&m_TrackCam.m_ref);
    SetPlayerControlActive(false);
}

void DebugMod::UpdateTrackCam()
{
    ZEntityRef s_Ref;
    m_NPCTracked->GetID(&s_Ref);
    SMatrix s_ActorWorldMatrix = s_Ref.QueryInterface<ZSpatialEntity>()->GetWorldMatrix();
    SMatrix s_TrackCamWorldMatrix = m_TrackCam.m_pInterfaceRef->GetWorldMatrix();
    s_TrackCamWorldMatrix.Trans = s_ActorWorldMatrix.Trans + float4(0.f, 0.f, 2.f, 0.f);

    m_TrackCam.m_pInterfaceRef->SetWorldMatrix(s_TrackCamWorldMatrix);
}

void DebugMod::DisableTrackCam()
{
    m_RenderDest.m_pInterfaceRef->SetSource(&m_PlayerCam);
    SetPlayerControlActive(true);
}

void DebugMod::GetPlayerCam()
{
    m_PlayerCam = *m_RenderDest.m_pInterfaceRef->GetSource();
}

void DebugMod::GetTrackCam()
{
    m_TrackCam = (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01;
}

void DebugMod::GetRenderDest()
{
    Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &m_RenderDest);
}

void DebugMod::SetPlayerControlActive(bool s_Active)
{
    TEntityRef<ZHitman5> s_LocalHitman;
    Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);
    if (s_LocalHitman)
    {
        auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(Globals::InputManager);
        if (s_InputControl) s_InputControl->m_bActive = s_Active;
    }
}
