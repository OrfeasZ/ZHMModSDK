#include "Assets.h"

#include <IconsMaterialDesign.h>

#include "imgui_internal.h"

#include <Glacier/ZContentKitManager.h>
#include <Glacier/ZInventory.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/ZItem.h>
#include <Glacier/SExternalReferences.h>

#include <Util/ImGuiUtils.h>

void Assets::Init() {
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &Assets::OnClearScene);
}

void Assets::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_TUNE " ASSETS MENU")) {
        m_AssetsMenuActive = !m_AssetsMenuActive;
    }
}

void Assets::OnDrawUI(bool p_HasFocus) {
    if (!p_HasFocus || !m_AssetsMenuActive) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("ASSETS", &m_AssetsMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing) {
        if (m_RepositoryProps.size() == 0) {
            LoadRepositoryProps();
        }

        ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;

        static char s_PropTitle[2048]{ "" };
        static char s_PropAssemblyPath[2048]{ "" };

        static int s_RepositoryPropSpawnCount = 1;
        static int s_NonRepositoryPropSpawnCount = 1;
        static int s_ActorSpawnCount = 1;

        static int s_WorldInventoryButton = 1;
        static char s_ActorName[2048]{};

        ImGui::Text("Repository Props");
        ImGui::Text("");
        ImGui::Text("Prop Title");
        ImGui::SameLine();

        Util::ImGuiUtils::InputWithAutocomplete(
            "##RepositoryProps",
            s_PropTitle,
            sizeof(s_PropTitle),
            m_RepositoryProps,
            [](auto& p_Pair) -> const ZRepositoryID& { return p_Pair.first; },
            [](auto& p_Pair) -> const std::string& { return p_Pair.second; },
            [&](const ZRepositoryID& p_Id, const std::string& p_Name, const auto&) {
                for (size_t i = 0; i < s_RepositoryPropSpawnCount; ++i) {
                    SpawnRepositoryProp(p_Id, s_WorldInventoryButton == 1);
                }
            }
        );

        if (ImGui::RadioButton("Add To World", s_WorldInventoryButton == 1)) {
            s_WorldInventoryButton = 1;
        }

        ImGui::SameLine();

        if (ImGui::RadioButton("Add To Inventory", s_WorldInventoryButton == 2)) {
            s_WorldInventoryButton = 2;
        }

        ImGui::Text("Number Of Props To Spawn");
        ImGui::SameLine();

        ImGui::InputInt("##RepositoryPropSpawnCount", &s_RepositoryPropSpawnCount);

        ImGui::Separator();
        ImGui::Text("Non Repository Props");
        ImGui::Text("");
        ImGui::Text("Prop Assembly Path");
        ImGui::SameLine();

        ImGui::InputText("##PropAssemblyPath", s_PropAssemblyPath, sizeof(s_PropAssemblyPath));
        ImGui::SameLine();

        if (ImGui::Button("Spawn Prop")) {
            for (size_t i = 0; i < s_NonRepositoryPropSpawnCount; ++i) {
                SpawnNonRepositoryProp(s_PropAssemblyPath);
            }
        }

        ImGui::Text("Number Of Props To Spawn");
        ImGui::SameLine();

        ImGui::InputInt("##NonRepositoryPropSpawnCount", &s_NonRepositoryPropSpawnCount);
        ImGui::Separator();

        ImGui::Text("Actors");
        ImGui::Text("");
        ImGui::Text("Actor Name");
        ImGui::SameLine();

        ImGui::InputText("##ActorName", s_ActorName, sizeof(s_ActorName));

        static char s_OutfitName[2048]{ "" };

        ImGui::Text("Outfit");
        ImGui::SameLine();

        static ZRepositoryID s_RepositoryId = ZRepositoryID("");
        static TEntityRef<ZGlobalOutfitKit> s_GlobalOutfitKit = {};
        static uint8_t s_CurrentCharacterSetIndex = 0;
        static std::string s_CurrentCharSetCharacterType = "HeroA";
        static uint8_t s_CurrentOutfitVariationIndex = 0;

        Util::ImGuiUtils::InputWithAutocomplete(
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
            [&](const ZRepositoryID& p_RepoId, const std::string& p_Name, const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit) {
                s_RepositoryId = p_RepoId;
                s_GlobalOutfitKit = p_GlobalOutfitKit;
            },
            [](auto& p_Pair) -> const TEntityRef<ZGlobalOutfitKit>&{ return p_Pair.second; }
        );

        ImGui::Text("Character Set Index");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharacterSetIndex", std::to_string(s_CurrentCharacterSetIndex).data())) {
            if (s_GlobalOutfitKit) {
                for (size_t i = 0; i < s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets.size(); ++i) {
                    const bool s_IsSelected = s_CurrentCharacterSetIndex == i;

                    if (ImGui::Selectable(std::to_string(s_CurrentCharacterSetIndex).data(), s_IsSelected)) {
                        s_CurrentCharacterSetIndex = i;
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentCharSetCharacterType.data())) {
            if (s_GlobalOutfitKit) {
                for (const auto& m_CharSetCharacterType : m_CharSetCharacterTypes) {
                    const bool s_IsSelected = s_CurrentCharSetCharacterType == m_CharSetCharacterType;

                    if (ImGui::Selectable(m_CharSetCharacterType.data(), s_IsSelected)) {
                        s_CurrentCharSetCharacterType = m_CharSetCharacterType;
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("Outfit Variation");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##OutfitVariation", std::to_string(s_CurrentOutfitVariationIndex).data())) {
            if (s_GlobalOutfitKit)
            {
                const uint8_t s_CurrentCharacterSetIndex2 = s_CurrentCharacterSetIndex;
                const TEntityRef<ZOutfitVariationCollection>& s_OutfitVariationCollection =
                    s_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[s_CurrentCharacterSetIndex2];
                const ZCharsetCharacterType* s_CharsetCharacterType = s_OutfitVariationCollection.m_pInterfaceRef->m_aCharacters[0].m_pInterfaceRef;
                const size_t s_VariationCount = s_CharsetCharacterType->m_aVariations.size();

                for (size_t i = 0; i < s_VariationCount; ++i) {
                    const bool s_IsSelected = s_CurrentOutfitVariationIndex == i;

                    if (ImGui::Selectable(std::to_string(i).data(), s_IsSelected)) {
                        s_CurrentOutfitVariationIndex = i;
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("Number Of Props To Spawn");
        ImGui::SameLine();

        ImGui::InputInt("##ActorSpawnCount", &s_ActorSpawnCount);

        if (ImGui::Button("Spawn Actor")) {
            for (size_t i = 0; i < s_ActorSpawnCount; ++i) {
                SpawnActor(
                    s_ActorName,
                    s_RepositoryId,
                    s_GlobalOutfitKit,
                    s_CurrentCharacterSetIndex,
                    s_CurrentCharSetCharacterType,
                    s_CurrentOutfitVariationIndex
                );
            }
        }
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

void Assets::SpawnRepositoryProp(const ZRepositoryID& p_RepositoryId, const bool p_AddToWorld) {
    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        Logger::Debug("No local hitman");
        return;
    }

    if (!p_AddToWorld) {
        const auto s_Character = s_LocalHitman.m_pInterfaceRef->m_pCharacter.m_pInterfaceRef;
        const auto s_Controllers = &s_Character->m_rSubcontrollerContainer.m_pInterfaceRef->m_aReferencedControllers;
        const auto s_Inventory = static_cast<ZCharacterSubcontrollerInventory*>((*s_Controllers)[6].m_pInterfaceRef);

        Functions::ZCharacterSubcontrollerInventory_CreateItem->Call(
            s_Inventory,
            p_RepositoryId,
            "",
            {},
            ZCharacterSubcontrollerInventory::ECreateItemType::ECIT_ContractItem
        );

        return;
    }

    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Debug("Scene not loaded.");
        return;
    }

    const auto s_ID = ResId<"[modules:/zitemspawner.class].pc_entitytype">;
    const auto s_ID2 = ResId<"[modules:/zitemrepositorykeyentity.class].pc_entitytype">;

    TResourcePtr<ZTemplateEntityFactory> s_Resource, s_Resource2;

    Globals::ResourceManager->GetResourcePtr(s_Resource, s_ID, 0);
    Globals::ResourceManager->GetResourcePtr(s_Resource2, s_ID2, 0);

    Logger::Debug("Resource: {} {}", s_Resource.m_nResourceIndex.val, fmt::ptr(s_Resource.GetResource()));

    if (!s_Resource) {
        Logger::Debug("Resource is not loaded.");
        return;
    }

    ZEntityRef s_NewEntity, s_NewEntity2;
    SExternalReferences s_ExternalRefs;

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager, s_NewEntity, "", s_Resource, s_Scene.m_ref, s_ExternalRefs, -1
    );

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager, s_NewEntity2, "", s_Resource2, s_Scene.m_ref, s_ExternalRefs, -1
    );

    if (!s_NewEntity) {
        Logger::Debug("Failed to spawn entity.");
        return;
    }

    if (!s_NewEntity2) {
        Logger::Debug("Failed to spawn entity2.");
        return;
    }

    const auto s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
    const auto s_ItemSpawner = s_NewEntity.QueryInterface<ZItemSpawner>();

    s_ItemSpawner->m_ePhysicsMode = ZItemSpawner::EPhysicsMode::EPM_KINEMATIC;
    s_ItemSpawner->m_rMainItemKey.m_ref = s_NewEntity2;
    s_ItemSpawner->m_rMainItemKey.m_pInterfaceRef = s_NewEntity2.QueryInterface<ZItemRepositoryKeyEntity>();
    s_ItemSpawner->m_rMainItemKey.m_pInterfaceRef->m_RepositoryId = p_RepositoryId;
    s_ItemSpawner->m_bUsePlacementAttach = false;
    s_ItemSpawner->SetWorldMatrix(s_HitmanSpatialEntity->GetWorldMatrix());

    Functions::ZItemSpawner_RequestContentLoad->Call(s_ItemSpawner);
}

void Assets::SpawnNonRepositoryProp(const std::string& p_PropAssemblyPath) {
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Debug("Scene not loaded.");
        return;
    }

    const Hash::MD5Hash s_Hash = Hash::MD5(std::string_view(p_PropAssemblyPath));

    const uint32_t s_IdHigh = ((s_Hash.A >> 24) & 0x000000FF)
        | ((s_Hash.A >> 8) & 0x0000FF00)
        | ((s_Hash.A << 8) & 0x00FF0000);

    const uint32_t s_IdLow = ((s_Hash.B >> 24) & 0x000000FF)
        | ((s_Hash.B >> 8) & 0x0000FF00)
        | ((s_Hash.B << 8) & 0x00FF0000)
        | ((s_Hash.B << 24) & 0xFF000000);

    const auto s_RuntimeResourceID = ZRuntimeResourceID(s_IdHigh, s_IdLow);

    TResourcePtr<ZTemplateEntityFactory> s_Resource;
    Globals::ResourceManager->GetResourcePtr(s_Resource, s_RuntimeResourceID, 0);

    if (!s_Resource) {
        Logger::Debug("Resource is not loaded.");
        return;
    }

    ZEntityRef s_NewEntity;
    SExternalReferences s_ExternalRefs;

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager, s_NewEntity, "", s_Resource, s_Scene.m_ref, s_ExternalRefs, -1
    );

    if (!s_NewEntity) {
        Logger::Debug("Failed to spawn entity.");
        return;
    }

    s_NewEntity.SetProperty("m_eRoomBehaviour", ZSpatialEntity::ERoomBehaviour::ROOM_DYNAMIC);

    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        Logger::Debug("No local hitman.");
        return;
    }

    const auto s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
    const auto s_PropSpatialEntity = s_NewEntity.QueryInterface<ZSpatialEntity>();

    s_PropSpatialEntity->SetWorldMatrix(s_HitmanSpatialEntity->GetWorldMatrix());
}

void Assets::SpawnActor(
    const std::string& p_ActorName,
    const ZRepositoryID& p_RepositoryID,
    const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit,
    uint8_t p_CharacterSetIndex,
    const std::string& p_CharSetCharacterType,
    uint8_t p_OutfitVariationIndex
) {
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Debug("Scene not loaded.");
        return;
    }

    const auto s_RuntimeResourceId = ResId<
        "[assembly:/templates/gameplay/ai2/actors.template?/npcactor.entitytemplate].pc_entitytype">;

    TResourcePtr<ZTemplateEntityFactory> s_Resource;
    Globals::ResourceManager->GetResourcePtr(s_Resource, s_RuntimeResourceId, 0);

    if (!s_Resource) {
        Logger::Debug("Resource is not loaded.");
        return;
    }

    ZEntityRef s_NewEntity;
    SExternalReferences s_ExternalRefs;

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager, s_NewEntity, "", s_Resource, s_Scene.m_ref, s_ExternalRefs, -1
    );

    if (!s_NewEntity) {
        Logger::Debug("Could not spawn entity.");
        return;
    }

    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        Logger::Debug("No local hitman.");
        return;
    }

    ZActor* s_Actor = s_NewEntity.QueryInterface<ZActor>();

    s_Actor->m_sActorName = p_ActorName;
    s_Actor->m_bStartEnabled = true;
    s_Actor->m_nOutfitCharset = p_CharacterSetIndex;
    s_Actor->m_nOutfitVariation = p_OutfitVariationIndex;
    s_Actor->m_OutfitRepositoryID = p_RepositoryID;
    s_Actor->m_eRequiredVoiceVariation = EActorVoiceVariation::eAVV_Undefined;

    s_Actor->Activate(0);

    ZSpatialEntity* s_ActorSpatialEntity = s_NewEntity.QueryInterface<ZSpatialEntity>();
    ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

    s_ActorSpatialEntity->SetWorldMatrix(s_HitmanSpatialEntity->GetWorldMatrix());

    if (p_GlobalOutfitKit) {
        EquipOutfit(
            p_GlobalOutfitKit,
            p_CharacterSetIndex,
            p_CharSetCharacterType,
            p_OutfitVariationIndex,
            s_Actor
        );
    }
}

void Assets::EquipOutfit(
    const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit,
    uint8_t p_CharSetIndex,
    const std::string& p_CharSetCharacterType,
    uint8_t p_OutfitVariationIndex,
    ZActor* p_Actor
) {
    if (!p_Actor) {
        Logger::Error("Could not equip outfit - no actor selected");
        return;
    }

    std::vector<ZRuntimeResourceID> s_ActorOutfitVariations;

    if (p_CharSetCharacterType != "HeroA") {
        const ZOutfitVariationCollection* s_OutfitVariationCollection = p_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[
            p_CharSetIndex].m_pInterfaceRef;

        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType2 = &s_OutfitVariationCollection->m_aCharacters[
            0];
        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType = nullptr;

        if (p_CharSetCharacterType == "Nude") {
            s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[1];
        }
        else if (p_CharSetCharacterType == "HeroA") {
            s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[2];
        }

        for (size_t i = 0; i < s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i) {
            s_ActorOutfitVariations.push_back(
                s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit
            );
        }

        if (s_CharsetCharacterType) {
            for (size_t i = 0; i < s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i) {
                s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit =
                    s_CharsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit;
            }
        }
    }

    Functions::ZActor_SetOutfit->Call(
        p_Actor, p_GlobalOutfitKit, p_CharSetIndex, p_OutfitVariationIndex, false
    );

    if (p_CharSetCharacterType != "Actor") {
        const ZOutfitVariationCollection* s_OutfitVariationCollection = p_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[
            p_CharSetIndex].m_pInterfaceRef;
        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[
            0];

        for (size_t i = 0; i < s_ActorOutfitVariations.size(); ++i) {
            s_CharsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit =
                s_ActorOutfitVariations[i];
        }
    }
}

void Assets::LoadRepositoryProps() {
    m_RepositoryProps.clear();

    if (m_RepositoryResource.m_nResourceIndex.val == -1) {
        const auto s_ID = ResId<"[assembly:/repository/pro.repo].pc_repo">;

        Globals::ResourceManager->GetResourcePtr(m_RepositoryResource, s_ID, 0);
    }

    if (m_RepositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID) {
        const auto s_RepositoryData = static_cast<THashMap<
            ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(m_RepositoryResource.
                GetResourceData());

        for (const auto& [s_RepositoryID, s_DynamicObject] : *s_RepositoryData) {
            const TArray<SDynamicObjectKeyValuePair>* s_Entries = s_DynamicObject.As<TArray<
                SDynamicObjectKeyValuePair>>();

            std::string s_Id, s_Title, s_CommonName, s_Name, s_FinalName;
            bool s_IsItem = false;

            for (const auto& s_Entry : *s_Entries) {
                const std::string s_Key = std::string(s_Entry.sKey.c_str(), s_Entry.sKey.size());

                if (s_Key == "ID_") {
                    s_Id = ConvertDynamicObjectValueToString(s_Entry.value);
                }
                else if (s_Key == "Title") {
                    s_Title = ConvertDynamicObjectValueToString(s_Entry.value);
                }
                else if (s_Key == "CommonName") {
                    s_CommonName = ConvertDynamicObjectValueToString(s_Entry.value);
                }
                else if (s_Key == "Name") {
                    s_Name = ConvertDynamicObjectValueToString(s_Entry.value);
                }
                else if (!s_IsItem) {
                    s_IsItem = s_Key == "ItemType";
                }
            }

            if (s_Id.empty() || !s_IsItem) {
                continue;
            }

            if (s_Title.empty() && s_CommonName.empty() && s_Name.empty()) {
                s_FinalName = "<unnamed> [" + s_Id + "]";
            }
            else if (!s_Title.empty()) {
                s_FinalName = s_Title + " [" + s_Id + "]";
            }
            else if (!s_CommonName.empty()) {
                s_FinalName = s_CommonName + " [" + s_Id + "]";
            }
            else if (!s_Name.empty()) {
                s_FinalName = s_Name + " [" + s_Id + "]";
            }

            const auto s_RepoId = ZRepositoryID(s_Id);

            m_RepositoryProps.push_back(std::make_pair(s_RepoId, s_FinalName));
        }
    }

    std::ranges::sort(
        m_RepositoryProps, [](const auto& a, const auto& b) {
            auto [_1, s_LowerA] = a;
            auto [_2, s_LowerB] = b;

            std::ranges::transform(s_LowerA, s_LowerA.begin(), [](unsigned char c) { return std::tolower(c); });
            std::ranges::transform(s_LowerB, s_LowerB.begin(), [](unsigned char c) { return std::tolower(c); });

            return s_LowerA < s_LowerB;
        }
    );
}

std::string Assets::ConvertDynamicObjectValueToString(const ZDynamicObject& p_DynamicObject) {
    std::string s_Result;
    const IType* s_Type = p_DynamicObject.GetTypeID()->typeInfo();

    if (strcmp(s_Type->m_pTypeName, "ZString") == 0) {
        const auto s_Value = p_DynamicObject.As<ZString>();
        s_Result = s_Value->c_str();
    }
    else if (strcmp(s_Type->m_pTypeName, "bool") == 0) {
        if (*p_DynamicObject.As<bool>()) {
            s_Result = "true";
        }
        else {
            s_Result = "false";
        }
    }
    else if (strcmp(s_Type->m_pTypeName, "float64") == 0) {
        double value = *p_DynamicObject.As<double>();

        s_Result = std::to_string(value).c_str();
    }
    else {
        s_Result = s_Type->m_pTypeName;
    }

    return s_Result;
}

DEFINE_PLUGIN_DETOUR(Assets, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene) {
    m_RepositoryResource = {};
    m_RepositoryProps.clear();

    return HookResult<void>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(Assets);