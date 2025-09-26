#include "Editor.h"

#include <Glacier/ZActor.h>
#include <Glacier/ZContentKitManager.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZApplicationEngineWin32.h>
#include <Glacier/ZRender.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZHM5InputManager.h>
#include <Glacier/ZFreeCamera.h>

#include "imgui_internal.h"

#include <Util/StringUtils.h>

void Editor::DrawActors(const bool p_HasFocus) {
    static size_t s_SelectedID = -1;

    if (!p_HasFocus || !m_ActorsMenuActive)
    {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("Actors", &m_ActorsMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing && p_HasFocus)
    {
        if (!Globals::ActorManager)
        {
            return;
        }

        ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;
        ZEntityRef ref;

        ImGui::BeginChild("left pane", ImVec2(300, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        static char s_ActorName_Substring[2048]{ "" };

        ImGui::Text("Actor Name");
        ImGui::SameLine();

        ImGui::InputText("##ActorName", s_ActorName_Substring, sizeof(s_ActorName_Substring));

        for (int i = 0; i < *Globals::NextActorId; ++i)
        {
            ZActor* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

            if (!s_Actor)
            {
                continue;
            }

            std::string s_ActorName = s_Actor->m_sActorName.c_str();

            if (!Util::StringUtils::FindSubstring(s_ActorName, s_ActorName_Substring))
            {
                continue;
            }

            std::string s_ButtonId = std::format("{}###{}", s_ActorName, i);

            if (!s_CurrentlySelectedActor)
            {
                s_SelectedID = -1;
            }

            if (ImGui::Selectable(s_ButtonId.c_str(), s_SelectedID == i) || s_CurrentlySelectedActor == s_Actor)
            {
                if (s_SelectedID != i)
                {
                    s_CurrentlySelectedActor = s_Actor;
                    s_SelectedID = i;

                    Logger::Info("Selected actor (by list): {}", s_CurrentlySelectedActor->m_sActorName);
                }
            }
        }

        ImGui::EndChild();

        if (!s_CurrentlySelectedActor)
        {
            ImGui::PopFont();
            ImGui::End();
            ImGui::PopFont();

            return;
        }

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

        static char s_OutfitName[2048]{ "" };

        if (s_OutfitName[0] == '\0')
        {
            const char* s_OutfitName2 = s_CurrentlySelectedActor->m_rOutfit.m_pInterfaceRef->m_sCommonName.c_str();

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

        static TEntityRef<ZGlobalOutfitKit>* s_GlobalOutfitKit = nullptr;
        static uint8_t s_CurrentCharacterSetIndex = 0;
        static std::string s_CurrentcharSetCharacterType = "Actor";
        static std::string s_CurrentcharSetCharacterType2 = "Actor";
        static uint8_t s_CurrentOutfitVariationIndex = 0;

        if (!s_GlobalOutfitKit)
        {
            s_GlobalOutfitKit = &s_CurrentlySelectedActor->m_rOutfit;
            s_CurrentCharacterSetIndex = s_CurrentlySelectedActor->m_nOutfitCharset;
            s_CurrentOutfitVariationIndex = s_CurrentlySelectedActor->m_nOutfitVariation;
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
                TEntityRef<ZGlobalOutfitKit>* s_GlobalOutfitKit2 = &it->second;
                const std::string s_OutfitName2 = s_GlobalOutfitKit2->m_pInterfaceRef->m_sCommonName.c_str();

                if (!Util::StringUtils::FindSubstring(s_OutfitName2, s_OutfitName))
                {
                    continue;
                }

                if (ImGui::Selectable(s_OutfitName2.data()))
                {
                    ImGui::ClearActiveID();
                    strcpy_s(s_OutfitName, s_OutfitName2.c_str());

                    s_CurrentCharacterSetIndex = 0;
                    s_CurrentOutfitVariationIndex = 0;

                    EquipOutfit(
                        it->second, s_CurrentCharacterSetIndex, s_CurrentcharSetCharacterType,
                        s_CurrentOutfitVariationIndex, s_CurrentlySelectedActor
                    );

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

        if (ImGui::BeginCombo("##CharacterSetIndex", std::to_string(s_CurrentCharacterSetIndex).data()))
        {
            if (s_GlobalOutfitKit)
            {
                for (size_t i = 0; i < s_GlobalOutfitKit->m_pInterfaceRef->m_aCharSets.size(); ++i)
                {
                    std::string s_CharacterSetIndex = std::to_string(i);
                    const bool s_IsSelected = s_CurrentCharacterSetIndex == i;

                    if (ImGui::Selectable(s_CharacterSetIndex.c_str(), s_IsSelected))
                    {
                        s_CurrentCharacterSetIndex = i;

                        if (s_GlobalOutfitKit)
                        {
                            EquipOutfit(
                                *s_GlobalOutfitKit, s_CurrentCharacterSetIndex, s_CurrentcharSetCharacterType,
                                s_CurrentOutfitVariationIndex, s_CurrentlySelectedActor
                            );
                        }
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentcharSetCharacterType.data()))
        {
            if (s_GlobalOutfitKit)
            {
                for (auto& m_CharSetCharacterType : m_CharSetCharacterTypes)
                {
                    const bool s_IsSelected = s_CurrentcharSetCharacterType == m_CharSetCharacterType;

                    if (ImGui::Selectable(m_CharSetCharacterType.data(), s_IsSelected))
                    {
                        s_CurrentcharSetCharacterType = m_CharSetCharacterType;

                        if (s_GlobalOutfitKit)
                        {
                            EquipOutfit(
                                *s_GlobalOutfitKit, s_CurrentCharacterSetIndex, s_CurrentcharSetCharacterType,
                                s_CurrentOutfitVariationIndex, s_CurrentlySelectedActor
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
            if (s_GlobalOutfitKit)
            {
                const uint8_t s_CurrentCharacterSetIndex2 = s_CurrentCharacterSetIndex;
                const size_t s_VariationCount = s_GlobalOutfitKit->m_pInterfaceRef->m_aCharSets[
                    s_CurrentCharacterSetIndex2].m_pInterfaceRef->m_aCharacters[0].m_pInterfaceRef->
                        m_aVariations.
                        size();

                    for (size_t i = 0; i < s_VariationCount; ++i)
                    {
                        const bool s_IsSelected = s_CurrentOutfitVariationIndex == i;

                        if (ImGui::Selectable(std::to_string(i).data(), s_IsSelected))
                        {
                            s_CurrentOutfitVariationIndex = i;

                            if (s_GlobalOutfitKit)
                            {
                                EquipOutfit(
                                    *s_GlobalOutfitKit, s_CurrentCharacterSetIndex, s_CurrentcharSetCharacterType,
                                    s_CurrentOutfitVariationIndex, s_CurrentlySelectedActor
                                );
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

        static std::string s_ActorName2;

        ImGui::Text("Actor Name");
        ImGui::SameLine();

        ImGui::InputText("##ActorName", s_ActorName2.data(), s_ActorName2.size());
        ImGui::SameLine();

        if (ImGui::Button("Get Actor Outfit"))
        {
            if (const ZActor* s_Actor2 = Globals::ActorManager->GetActorByName(s_ActorName2))
            {
                EquipOutfit(
                    s_Actor2->m_rOutfit, s_Actor2->m_nOutfitCharset, s_CurrentcharSetCharacterType2,
                    s_Actor2->m_nOutfitVariation, s_CurrentlySelectedActor
                );
            }
        }

        if (ImGui::Button("Get Nearest Actor's Outfit"))
        {
            ZEntityRef s_Ref;

            s_CurrentlySelectedActor->GetID(&s_Ref);

            ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

            for (int i = 0; i < *Globals::NextActorId; ++i)
            {
                ZActor* s_Actor2 = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

                s_Actor2->GetID(&s_Ref);

                const ZSpatialEntity* s_ActorSpatialEntity2 = s_Ref.QueryInterface<ZSpatialEntity>();

                const SVector3 s_Temp = s_ActorSpatialEntity->m_mTransform.Trans - s_ActorSpatialEntity2->m_mTransform.
                    Trans;
                const float s_Distance = sqrt(s_Temp.x * s_Temp.x + s_Temp.y * s_Temp.y + s_Temp.z * s_Temp.z);

                if (s_Distance <= 3.0f)
                {
                    EquipOutfit(
                        s_Actor2->m_rOutfit, s_Actor2->m_nOutfitCharset, s_CurrentcharSetCharacterType2,
                        s_Actor2->m_nOutfitVariation, s_CurrentlySelectedActor
                    );

                    break;
                }
            }
        }

        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentcharSetCharacterType2.data()))
        {
            if (s_GlobalOutfitKit)
            {
                for (const auto& m_CharSetCharacterType : m_CharSetCharacterTypes)
                {
                    const bool s_IsSelected = s_CurrentcharSetCharacterType2 == m_CharSetCharacterType;

                    if (ImGui::Selectable(m_CharSetCharacterType.data(), s_IsSelected))
                    {
                        s_CurrentcharSetCharacterType2 = m_CharSetCharacterType;
                    }
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::Button("Select In Entity Tree"))
        {
            ZEntityRef s_Ref;

            s_CurrentlySelectedActor->GetID(&s_Ref);

            OnSelectEntity(s_Ref, std::nullopt);
        }

        if (ImGui::Button("Teleport Actor To Player"))
        {
            if (auto s_LocalHitman = SDK()->GetLocalPlayer())
            {
                ZEntityRef s_Ref;
                s_CurrentlySelectedActor->GetID(&s_Ref);

                ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
                ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

                s_ActorSpatialEntity->SetWorldMatrix(s_HitmanSpatialEntity->GetWorldMatrix());
            }
        }

        if (ImGui::Button("Teleport Player To Actor"))
        {
            if (auto s_LocalHitman = SDK()->GetLocalPlayer())
            {
                ZEntityRef s_Ref;
                s_CurrentlySelectedActor->GetID(&s_Ref);

                ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
                ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

                s_HitmanSpatialEntity->SetWorldMatrix(s_ActorSpatialEntity->GetWorldMatrix());
            }
        }

        if (ImGui::Button("Kill Actor"))
        {
            TEntityRef<IItem> s_Item;
            TEntityRef<ZSetpieceEntity> s_SetPieceEntity;

            Functions::ZActor_KillActor->Call(
                s_CurrentlySelectedActor, s_Item, s_SetPieceEntity, EDamageEvent::eDE_UNDEFINED, EDeathBehavior::eDB_IMPACT_ANIM
            );
        }

        ImGui::Separator();

        if (ImGui::Button(std::format("Track This Actor##{}", s_ActorName_Substring).c_str()))
        {
            if (m_RenderDest.m_ref == nullptr) GetRenderDest();
            if (m_TrackCam.m_ref == nullptr) GetTrackCam();
            if (m_PlayerCam == nullptr) GetPlayerCam();
            m_ActorTracked = s_CurrentlySelectedActor;
            m_TrackCamActive = true;
            EnableTrackCam();
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop Tracking"))
        {
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
    uint8_t n_CurrentCharSetIndex,
    const std::string& s_CurrentCharSetCharacterType,
    uint8_t n_CurrentOutfitVariationIndex,
    ZActor* p_Actor
) {
    if (!p_Actor)
    {
        Logger::Error("Could not equip outfit - no actor selected");
        return;
    }

    std::vector<ZRuntimeResourceID> s_ActorOutfitVariations;
    if (s_CurrentCharSetCharacterType != "HeroA")
    {
        const ZOutfitVariationCollection* s_OutfitVariationCollection = p_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[
            n_CurrentCharSetIndex].m_pInterfaceRef;

        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType2 = &s_OutfitVariationCollection->m_aCharacters[
            0];
        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType = nullptr;

        if (s_CurrentCharSetCharacterType == "Nude")
        {
            s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[1];
        }
        else if (s_CurrentCharSetCharacterType == "HeroA")
        {
            s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[2];
        }

        for (size_t i = 0; i < s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
        {
            s_ActorOutfitVariations.push_back(
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

    Functions::ZActor_SetOutfit->Call(
        p_Actor, p_GlobalOutfitKit, n_CurrentCharSetIndex, n_CurrentOutfitVariationIndex, false
    );

    if (s_CurrentCharSetCharacterType != "Actor")
    {
        const ZOutfitVariationCollection* s_OutfitVariationCollection = p_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[
            n_CurrentCharSetIndex].m_pInterfaceRef;
        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[
            0];

        for (size_t i = 0; i < s_ActorOutfitVariations.size(); ++i)
        {
            s_CharsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit =
                s_ActorOutfitVariations[i];
        }
    }
}

void Editor::EnableTrackCam() {
    m_RenderDest.m_pInterfaceRef->SetSource(&m_TrackCam.m_ref);
    SetPlayerControlActive(false);
}

void Editor::UpdateTrackCam() const {
    ZEntityRef s_Ref;
    m_ActorTracked->GetID(&s_Ref);
    SMatrix s_ActorWorldMatrix = s_Ref.QueryInterface<ZSpatialEntity>()->GetWorldMatrix();
    SMatrix s_TrackCamWorldMatrix = m_TrackCam.m_pInterfaceRef->GetWorldMatrix();
    s_TrackCamWorldMatrix.Trans = s_ActorWorldMatrix.Trans + float4(0.f, 0.f, 2.f, 0.f);

    m_TrackCam.m_pInterfaceRef->SetWorldMatrix(s_TrackCamWorldMatrix);
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
    if (s_LocalHitman)
    {
        auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(Globals::InputManager);
        if (s_InputControl)
        {
            s_InputControl->m_bActive = s_Active;
        }
    }
}
