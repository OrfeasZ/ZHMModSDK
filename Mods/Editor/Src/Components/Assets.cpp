#include "Editor.h"

#include "imgui_internal.h"

#include <Glacier/ZContentKitManager.h>
#include <Glacier/ZInventory.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/ZItem.h>
#include <Glacier/SExternalReferences.h>

#include <Util/StringUtils.h>

void Editor::DrawAssets(bool p_HasFocus) {
    if (!p_HasFocus || !m_AssetsMenuActive) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("ASSETS", &m_AssetsMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing && p_HasFocus) {
        if (m_RepositoryProps.size() == 0) {
            LoadRepositoryProps();
        }

        ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;

        static char s_PropTitle_SubString[2048] {""};
        static char s_PropAssemblyPath[2048] {""};

        static int s_NumberOfPropsToSpawn_Repo = 1;
        static int s_NumberOfPropsToSpawn_NonRepo = 1;
        static int s_NumberOfPropsToSpawn_NPCs = 1;

        static int s_WorldInventoryButton = 1;
        static char s_NpcName[2048] {};

        ImGui::Text("Repository Props");
        ImGui::Text("");
        ImGui::Text("Prop Title");
        ImGui::SameLine();

        const bool s_IsInputTextEnterPressed = ImGui::InputText(
            "##PropRepositoryID", s_PropTitle_SubString, sizeof(s_PropTitle_SubString),
            ImGuiInputTextFlags_EnterReturnsTrue
        );
        const bool s_IsInputTextActive = ImGui::IsItemActive();

        if (ImGui::IsItemActivated()) {
            ImGui::OpenPopup("##popup");
        }

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

        if (ImGui::BeginPopup(
            "##popup",
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_ChildWindow
        )) {
            for (auto& [s_Id, s_Name] : m_RepositoryProps) {
                if (!Util::StringUtils::FindSubstring(s_Name, s_PropTitle_SubString)) {
                    continue;
                }

                std::string s_ButtonId = std::format("{}###{}", s_Name, s_Id.ToString().c_str());

                if (ImGui::Selectable(s_ButtonId.c_str())) {
                    ImGui::ClearActiveID();
                    strcpy_s(s_PropTitle_SubString, s_Name.c_str());

                    for (size_t i = 0; i < s_NumberOfPropsToSpawn_Repo; ++i) {
                        SpawnRepositoryProp(s_Id, s_WorldInventoryButton == 1);
                    }
                }
            }

            if (s_IsInputTextEnterPressed || (!s_IsInputTextActive && !ImGui::IsWindowFocused())) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (ImGui::RadioButton("Add To World", s_WorldInventoryButton == 1)) {
            s_WorldInventoryButton = 1;
        }

        ImGui::SameLine();

        if (ImGui::RadioButton("Add To Inventory", s_WorldInventoryButton == 2)) {
            s_WorldInventoryButton = 2;
        }

        ImGui::Text("Number Of Props To Spawn");
        ImGui::SameLine();

        ImGui::InputInt("##NumberOfPropsToSpawn_Repo)", &s_NumberOfPropsToSpawn_Repo);

        ImGui::Separator();
        ImGui::Text("Non Repository Props");
        ImGui::Text("");
        ImGui::Text("Prop Assembly Path");
        ImGui::SameLine();

        ImGui::InputText("##Prop Assembly Path", s_PropAssemblyPath, sizeof(s_PropAssemblyPath));
        ImGui::SameLine();

        if (ImGui::Button("Spawn Prop")) {
            for (size_t i = 0; i < s_NumberOfPropsToSpawn_Repo; ++i) {
                SpawnNonRepositoryProp(s_PropAssemblyPath);
            }
        }

        ImGui::Text("Number Of Props To Spawn");
        ImGui::SameLine();

        ImGui::InputInt("##NumberOfPropsToSpawn_NonRepo", &s_NumberOfPropsToSpawn_NonRepo);
        ImGui::Separator();

        ImGui::Text("NPCs");
        ImGui::Text("");
        ImGui::Text("NPC Name");
        ImGui::SameLine();

        ImGui::InputText("##NPCName", s_NpcName, sizeof(s_NpcName));

        static char outfitName_SubString[2048] {""};

        ImGui::Text("Outfit");
        ImGui::SameLine();

        const bool s_IsInputTextEnterPressed2 = ImGui::InputText(
            "##OutfitName", outfitName_SubString, sizeof(outfitName_SubString), ImGuiInputTextFlags_EnterReturnsTrue
        );
        const bool s_IsInputTextActive2 = ImGui::IsItemActive();

        if (ImGui::IsItemActivated()) {
            ImGui::OpenPopup("##popup2");
        }

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetItemRectSize().x, 300));

        static ZRepositoryID s_RepositoryId = ZRepositoryID("");
        static TEntityRef<ZGlobalOutfitKit>* s_GlobalOutfitKit = nullptr;
        static uint8_t n_CurrentCharacterSetIndex = 0;
        static std::string s_CurrentcharSetCharacterType = "HeroA";
        static uint8_t n_CurrentOutfitVariationIndex = 0;

        if (ImGui::BeginPopup(
            "##popup2",
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_ChildWindow
        )) {
            for (auto it = s_ContentKitManager->m_repositoryGlobalOutfitKits.begin(); it != s_ContentKitManager->
                 m_repositoryGlobalOutfitKits.end(); ++it) {
                TEntityRef<ZGlobalOutfitKit>* s_GlobalOutfitKit2 = &it->second;
                const std::string outfitName = s_GlobalOutfitKit2->m_pInterfaceRef->m_sCommonName.c_str();

                if (outfitName.empty()) {
                    continue;
                }

                if (!Util::StringUtils::FindSubstring(outfitName, outfitName_SubString)) {
                    continue;
                }

                if (ImGui::Selectable(outfitName.c_str())) {
                    ImGui::ClearActiveID();
                    strcpy_s(outfitName_SubString, outfitName.c_str());

                    s_RepositoryId = it->first;
                    s_GlobalOutfitKit = s_GlobalOutfitKit2;
                }
            }

            if (s_IsInputTextEnterPressed2 || (!s_IsInputTextActive2 && !ImGui::IsWindowFocused())) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::Text("Character Set Index");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharacterSetIndex", std::to_string(n_CurrentCharacterSetIndex).data())) {
            if (s_GlobalOutfitKit) {
                for (size_t i = 0; i < s_GlobalOutfitKit->m_pInterfaceRef->m_aCharSets.size(); ++i) {
                    const bool s_IsSelected = n_CurrentCharacterSetIndex == i;

                    if (ImGui::Selectable(std::to_string(n_CurrentCharacterSetIndex).data(), s_IsSelected)) {
                        n_CurrentCharacterSetIndex = i;
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("CharSet Character Type");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##CharSetCharacterType", s_CurrentcharSetCharacterType.data())) {
            if (s_GlobalOutfitKit) {
                for (const auto& m_CharSetCharacterType : m_CharSetCharacterTypes) {
                    const bool s_IsSelected = s_CurrentcharSetCharacterType == m_CharSetCharacterType;

                    if (ImGui::Selectable(m_CharSetCharacterType.data(), s_IsSelected)) {
                        s_CurrentcharSetCharacterType = m_CharSetCharacterType;
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("Outfit Variation");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##OutfitVariation", std::to_string(n_CurrentOutfitVariationIndex).data())) {
            if (s_GlobalOutfitKit) {
                const uint8_t s_CurrentCharacterSetIndex2 = n_CurrentCharacterSetIndex;
                const size_t s_VariationCount = s_GlobalOutfitKit->m_pInterfaceRef->m_aCharSets[
                            s_CurrentCharacterSetIndex2].m_pInterfaceRef->m_aCharacters[0].m_pInterfaceRef->
                        m_aVariations.
                        size();

                for (size_t i = 0; i < s_VariationCount; ++i) {
                    const bool s_IsSelected = n_CurrentOutfitVariationIndex == i;

                    if (ImGui::Selectable(std::to_string(i).data(), s_IsSelected)) {
                        n_CurrentOutfitVariationIndex = i;
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("Number Of Props To Spawn");
        ImGui::SameLine();

        ImGui::InputInt("##NumberOfPropsToSpawn_NPCs", &s_NumberOfPropsToSpawn_NPCs);

        if (ImGui::Button("Spawn NPC")) {
            for (size_t i = 0; i < s_NumberOfPropsToSpawn_NPCs; ++i) {
                SpawnNPC(
                    s_NpcName,
                    s_RepositoryId,
                    s_GlobalOutfitKit,
                    n_CurrentCharacterSetIndex,
                    s_CurrentcharSetCharacterType,
                    n_CurrentOutfitVariationIndex
                );
            }
        }
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

void Editor::SpawnRepositoryProp(const ZRepositoryID& p_RepositoryId, const bool addToWorld) {
    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        Logger::Debug("No local hitman");
        return;
    }

    if (!addToWorld) {
        const TArray<TEntityRef<ZCharacterSubcontroller>>* s_Controllers = &s_LocalHitman.m_pInterfaceRef->m_pCharacter.
                m_pInterfaceRef->m_rSubcontrollerContainer.m_pInterfaceRef->m_aReferencedControllers;
        auto* s_Inventory = static_cast<ZCharacterSubcontrollerInventory*>(s_Controllers->operator[](6).
            m_pInterfaceRef);

        TArray<ZRepositoryID> s_ModifierIds;
        Functions::ZCharacterSubcontrollerInventory_AddDynamicItemToInventory->Call(
            s_Inventory, p_RepositoryId, "", &s_ModifierIds, 2
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

void Editor::SpawnNonRepositoryProp(const std::string& s_PropAssemblyPath) {
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Debug("Scene not loaded.");
        return;
    }

    const Hash::MD5Hash s_Hash = Hash::MD5(std::string_view(s_PropAssemblyPath));

    const uint32_t s_IdHigh = ((s_Hash.A >> 24) & 0x000000FF)
            | ((s_Hash.A >> 8) & 0x0000FF00)
            | ((s_Hash.A << 8) & 0x00FF0000);

    const uint32_t s_IdLow = ((s_Hash.B >> 24) & 0x000000FF)
            | ((s_Hash.B >> 8) & 0x0000FF00)
            | ((s_Hash.B << 8) & 0x00FF0000)
            | ((s_Hash.B << 24) & 0xFF000000);

    const auto s_RuntimeResourceId = ZRuntimeResourceID(s_IdHigh, s_IdLow);

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

auto Editor::SpawnNPC(
    const std::string& p_NpcName,
    const ZRepositoryID& repositoryID,
    const TEntityRef<ZGlobalOutfitKit>* p_GlobalOutfitKit,
    uint8_t n_CurrentCharacterSetIndex,
    const std::string& s_CurrentcharSetCharacterType,
    uint8_t n_CurrentOutfitVariationIndex
) -> void {
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

    ZActor* actor = s_NewEntity.QueryInterface<ZActor>();

    actor->m_sActorName = p_NpcName;
    actor->m_bStartEnabled = true;
    actor->m_nOutfitCharset = n_CurrentCharacterSetIndex;
    actor->m_nOutfitVariation = n_CurrentOutfitVariationIndex;
    actor->m_OutfitRepositoryID = repositoryID;
    actor->m_eRequiredVoiceVariation = EActorVoiceVariation::eAVV_Undefined;

    actor->Activate(0);

    ZSpatialEntity* s_ActorSpatialEntity = s_NewEntity.QueryInterface<ZSpatialEntity>();
    ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

    s_ActorSpatialEntity->SetWorldMatrix(s_HitmanSpatialEntity->GetWorldMatrix());

    if (p_GlobalOutfitKit) {
        EquipOutfit(
            *p_GlobalOutfitKit, n_CurrentCharacterSetIndex, s_CurrentcharSetCharacterType,
            n_CurrentOutfitVariationIndex, actor
        );
    }
}

void Editor::LoadRepositoryProps() {
    m_RepositoryProps.clear();

    if (m_RepositoryResource.m_nResourceIndex.val == -1) {
        const auto s_ID = ResId<"[assembly:/repository/pro.repo].pc_repo">;

        Globals::ResourceManager->GetResourcePtr(m_RepositoryResource, s_ID, 0);
    }

    if (m_RepositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID) {
        const auto s_RepositoryData = static_cast<THashMap<
            ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(m_RepositoryResource.
            GetResourceData());

        for (auto it = s_RepositoryData->begin(); it != s_RepositoryData->end(); ++it) {
            const ZDynamicObject* s_DynamicObject = &it->second;
            const TArray<SDynamicObjectKeyValuePair>* s_Entries = s_DynamicObject->As<TArray<
                SDynamicObjectKeyValuePair>>();

            std::string s_Id, s_Title, s_CommonName, s_Name, s_FinalName;
            bool s_IsItem = false;

            for (size_t i = 0; i < s_Entries->size(); ++i) {
                std::string s_Key = s_Entries->operator[](i).sKey.c_str();

                if (s_Key == "ID_") {
                    s_Id = ConvertDynamicObjectValueToString(s_Entries->at(i).value);
                }
                else if (s_Key == "Title") {
                    s_Title = ConvertDynamicObjectValueToString(s_Entries->at(i).value);
                }
                else if (s_Key == "CommonName") {
                    s_CommonName = ConvertDynamicObjectValueToString(s_Entries->at(i).value);
                }
                else if (s_Key == "Name") {
                    s_Name = ConvertDynamicObjectValueToString(s_Entries->at(i).value);
                }
                else if (!s_IsItem) {
                    s_IsItem = s_Key == "ItemType" ||
                            s_Key == "IsHitmanSuit" ||
                            s_Key == "IsWeapon" ||
                            s_Key == "Items";
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

    // Sort props based on lower-case name.
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

std::string Editor::ConvertDynamicObjectValueToString(const ZDynamicObject& p_DynamicObject) {
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
