#include "Editor.h"

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

        static char s_ActorName[2048]{ "" };

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Actor Name");
        ImGui::SameLine();

        ImGui::InputText("##ActorName", s_ActorName, sizeof(s_ActorName));

        ImGui::Checkbox("Show only alive actors", &m_ShowOnlyAliveActors);
        ImGui::Checkbox("Show only targets", &m_ShowOnlyTargets);
        ImGui::Checkbox("Select actor in list when clicked in game", &m_SelectActorOnMouseClick);

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::BeginChild("left pane", ImVec2(300, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        TEntityRef<ZActor>* s_ActorArray;
        size_t s_ActorCount;

        if (m_ShowOnlyAliveActors) {
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

            if (m_ShowOnlyTargets && !IsActorTarget(s_Actor)) {
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

        if (s_OutfitName[0] == '\0') {
            const char* s_OutfitName2 = m_SelectedActor->m_rOutfit.m_pInterfaceRef->m_sCommonName.c_str();

            strncpy(s_OutfitName, s_OutfitName2, sizeof(s_OutfitName) - 1);

            s_OutfitName[sizeof(s_OutfitName) - 1] = '\0';
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Outfit");
        ImGui::SameLine();

        static TEntityRef<ZGlobalOutfitKit> s_GlobalOutfitKit = {};
        static uint8_t s_CurrentCharacterSetIndex = 0;
        static std::string s_CurrentcharSetCharacterType = "Actor";
        static std::string s_CurrentcharSetCharacterType2 = "Actor";
        static uint8_t s_CurrentOutfitVariationIndex = 0;

        if (!s_GlobalOutfitKit) {
            s_GlobalOutfitKit = m_SelectedActor->m_rOutfit;
            s_CurrentCharacterSetIndex = m_SelectedActor->m_nOutfitCharset;
            s_CurrentOutfitVariationIndex = m_SelectedActor->m_nOutfitVariation;
        }

        Util::ImGuiUtils::InputWithAutocomplete(
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
                        s_CurrentcharSetCharacterType,
                        s_CurrentOutfitVariationIndex,
                        m_SelectedActor
                    );

                    s_GlobalOutfitKit = p_GlobalOutfitKit;
            },
            [](auto& p_Pair) -> const TEntityRef<ZGlobalOutfitKit>& { return p_Pair.second; },
            [](auto& p_Pair) -> bool {
                return p_Pair.second && !p_Pair.second.m_pInterfaceRef->m_bIsHitmanSuit;
            }
        );

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Character Set Index");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharacterSetIndex", std::to_string(s_CurrentCharacterSetIndex).data())) {
            if (s_GlobalOutfitKit) {
                for (size_t i = 0; i < s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets.size(); ++i) {
                    std::string s_CharacterSetIndex = std::to_string(i);
                    const bool s_IsSelected = s_CurrentCharacterSetIndex == i;

                    if (ImGui::Selectable(s_CharacterSetIndex.c_str(), s_IsSelected)) {
                        s_CurrentCharacterSetIndex = i;

                        if (s_GlobalOutfitKit) {
                            EquipOutfit(
                                s_GlobalOutfitKit,
                                s_CurrentCharacterSetIndex,
                                s_CurrentcharSetCharacterType,
                                s_CurrentOutfitVariationIndex,
                                m_SelectedActor
                            );
                        }
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentcharSetCharacterType.data())) {
            if (s_GlobalOutfitKit) {
                for (auto& m_CharSetCharacterType : m_CharSetCharacterTypes) {
                    const bool s_IsSelected = s_CurrentcharSetCharacterType == m_CharSetCharacterType;

                    if (ImGui::Selectable(m_CharSetCharacterType.data(), s_IsSelected)) {
                        s_CurrentcharSetCharacterType = m_CharSetCharacterType;

                        if (s_GlobalOutfitKit) {
                            EquipOutfit(
                                s_GlobalOutfitKit,
                                s_CurrentCharacterSetIndex,
                                s_CurrentcharSetCharacterType,
                                s_CurrentOutfitVariationIndex,
                                m_SelectedActor
                            );
                        }
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Outfit Variation");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##OutfitVariation", std::to_string(s_CurrentOutfitVariationIndex).data())) {
            if (s_GlobalOutfitKit) {
                const uint8_t s_CurrentCharacterSetIndex2 = s_CurrentCharacterSetIndex;
                const size_t s_VariationCount = s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[
                            s_CurrentCharacterSetIndex2].m_pInterfaceRef->m_aCharacters[0].m_pInterfaceRef->
                        m_aVariations.
                        size();

                for (size_t i = 0; i < s_VariationCount; ++i) {
                    const bool s_IsSelected = s_CurrentOutfitVariationIndex == i;

                    if (ImGui::Selectable(std::to_string(i).data(), s_IsSelected)) {
                        s_CurrentOutfitVariationIndex = i;

                        if (s_GlobalOutfitKit) {
                            EquipOutfit(
                                s_GlobalOutfitKit,
                                s_CurrentCharacterSetIndex,
                                s_CurrentcharSetCharacterType,
                                s_CurrentOutfitVariationIndex,
                                m_SelectedActor
                            );
                        }
                    }
                }
            }

            ImGui::EndCombo();
        }

        if (s_GlobalOutfitKit) {
            ImGui::Checkbox("Weapons Allowed", &s_GlobalOutfitKit.m_pInterfaceRef->m_bWeaponsAllowed);
            ImGui::Checkbox("Authority Figure", &s_GlobalOutfitKit.m_pInterfaceRef->m_bAuthorityFigure);
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
                    s_CurrentcharSetCharacterType2,
                    s_Actor2->m_nOutfitVariation,
                    m_SelectedActor
                );
            }
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
                        s_CurrentcharSetCharacterType2,
                        s_Actor2->m_nOutfitVariation,
                        m_SelectedActor
                    );

                    break;
                }
            }
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentcharSetCharacterType2.data())) {
            if (s_GlobalOutfitKit) {
                for (const auto& m_CharSetCharacterType : m_CharSetCharacterTypes) {
                    const bool s_IsSelected = s_CurrentcharSetCharacterType2 == m_CharSetCharacterType;

                    if (ImGui::Selectable(m_CharSetCharacterType.data(), s_IsSelected)) {
                        s_CurrentcharSetCharacterType2 = m_CharSetCharacterType;
                    }
                }
            }

            ImGui::EndCombo();
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

                ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
                ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

                s_ActorSpatialEntity->SetWorldMatrix(s_HitmanSpatialEntity->GetWorldMatrix());
            }
        }

        if (ImGui::Button("Teleport Player To Actor")) {
            if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
                ZEntityRef s_Ref;
                m_SelectedActor->GetID(s_Ref);

                ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
                ZSpatialEntity* s_ActorSpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

                s_HitmanSpatialEntity->SetWorldMatrix(s_ActorSpatialEntity->GetWorldMatrix());
            }
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

        if (ImGui::Button(std::format("Track This Actor##{}", s_ActorName).c_str())) {

            if (m_RenderDest.m_ref == nullptr) {
                GetRenderDest();
            }

            if (m_TrackCam.m_ref == nullptr) {
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
        auto* s_ActorType = &s_Collection->m_aCharacters[0];

        if (!s_ActorType->m_pInterfaceRef) {
            Logger::Error("Couldn't equip outfit - actor character type is null!");
            return;
        }

        TEntityRef<ZCharsetCharacterType>* s_TargetType = nullptr;

        if (p_CharSetCharacterType == "HeroA") {
            s_TargetType = &s_Collection->m_aCharacters[2];
        }
        else if (p_CharSetCharacterType == "Nude") {
            s_TargetType = &s_Collection->m_aCharacters[1];
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
        auto* s_ActorType = &s_GlobalOutfitKit->m_aCharSets[p_CharSetIndex].m_pInterfaceRef->m_aCharacters[0];

        if (s_ActorType->m_pInterfaceRef) {
            auto& s_ActorVariations = s_ActorType->m_pInterfaceRef->m_aVariations;
            const size_t s_Count = std::min(s_ActorVariations.size(), s_OriginalActorVariations.size());

            for (size_t i = 0; i < s_Count; ++i) {
                s_ActorVariations[i].m_pInterfaceRef->m_Outfit = s_OriginalActorVariations[i];
            }
        }
    }
}

void Editor::EnableTrackCam() {
    m_RenderDest.m_pInterfaceRef->SetSource(&m_TrackCam.m_ref);
    SetPlayerControlActive(false);
}

void Editor::UpdateTrackCam() const {
    ZEntityRef s_Ref;

    m_ActorTracked->GetID(s_Ref);

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