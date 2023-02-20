#include "DebugMod.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZScene.h>
#include <Glacier/ZActor.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/EntityFactory.h>
#include <Glacier/ZCollision.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/ZHttp.h>
#include <Glacier/ZPhysics.h>
#include <Glacier/ZSetpieceEntity.h>
#include <Glacier/ZContentKitManager.h>
#include <Glacier/ZAction.h>
#include <Glacier/ZItem.h>
#include <Glacier/ZInventory.h>
#include <Glacier/ZHM5CrippleBox.h>
#include <IO/ZBinaryReader.h>
#include <IO/ZBinaryDeserializer.h>
#include <Crypto.h>

#include <Functions.h>
#include <Globals.h>

#include <ImGuizmo.h>
#include <lz4.h>

#include <winhttp.h>
#include <numbers>
#include <filesystem>
#include <imgui_internal.h>

#pragma comment(lib, "urlmon.lib")

DebugMod::~DebugMod()
{
    const ZMemberDelegate<DebugMod, void(const SGameUpdateEvent&)> s_Delegate(this, &DebugMod::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void DebugMod::Init()
{
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &DebugMod::OnClearScene);
}

void DebugMod::OnEngineInitialized()
{
    const ZMemberDelegate<DebugMod, void(const SGameUpdateEvent&)> s_Delegate(this, &DebugMod::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    std::thread thread(LoadHashMap);

    thread.detach();
}

void DebugMod::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{

}

void DebugMod::OnDrawMenu()
{
    if (ImGui::Button("DEBUG MENU"))
    {
        m_DebugMenuActive = !m_DebugMenuActive;
    }

    if (ImGui::Button("POSITIONS MENU"))
    {
        m_PositionsMenuActive = !m_PositionsMenuActive;
    }

    if (ImGui::Button("ENTITY MENU"))
    {
        m_EntityMenuActive = !m_EntityMenuActive;
    }

    if (ImGui::Button("PLAYER MENU"))
    {
        m_PlayerMenuActive = !m_PlayerMenuActive;
    }

    if (ImGui::Button("ITEMS MENU"))
    {
        m_ItemsMenuActive = !m_ItemsMenuActive;
    }

    if (ImGui::Button("ASSETS MENU"))
    {
        m_AssetsMenuActive = !m_AssetsMenuActive;
    }

    if (ImGui::Button("NPCs MENU"))
    {
        m_NPCsMenuActive = !m_NPCsMenuActive;
    }

    if (ImGui::Button("SCENE MENU"))
    {
        m_SceneMenuActive = !m_SceneMenuActive;
    }
}

void DebugMod::OnDrawUI(bool p_HasFocus)
{
    ImGuizmo::BeginFrame();

    DrawOptions(p_HasFocus);
    DrawEntityBox(p_HasFocus);
    DrawAssetsBox(p_HasFocus);
    DrawEntityBox(p_HasFocus);
    DrawItemsBox(p_HasFocus);
    DrawNPCsBox(p_HasFocus);
    DrawPlayerBox(p_HasFocus);
    DrawPositionBox(p_HasFocus);
    DrawSceneBox(p_HasFocus);

    auto s_ImgGuiIO = ImGui::GetIO();

    if (p_HasFocus)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !s_ImgGuiIO.WantCaptureMouse)
        {
            const auto s_MousePos = ImGui::GetMousePos();

            OnMouseDown(SVector2(s_MousePos.x, s_MousePos.y), !m_HoldingMouse);

            m_HoldingMouse = true;
        }
        else
        {
            m_HoldingMouse = false;
        }

        if (ImGui::IsKeyPressed(s_ImgGuiIO.KeyMap[ImGuiKey_Tab]))
        {
            if (m_GizmoMode == ImGuizmo::TRANSLATE)
                m_GizmoMode = ImGuizmo::ROTATE;
            else if (m_GizmoMode == ImGuizmo::ROTATE)
                m_GizmoMode = ImGuizmo::SCALE;
            else if (m_GizmoMode == ImGuizmo::SCALE)
                m_GizmoMode = ImGuizmo::TRANSLATE;
        }

        if (ImGui::IsKeyPressed(s_ImgGuiIO.KeyMap[ImGuiKey_Space]))
        {
            m_GizmoSpace = m_GizmoSpace == ImGuizmo::WORLD ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
        }
    }

    ImGuizmo::Enable(p_HasFocus);

    m_EntityMutex.lock_shared();

    if (m_SelectedEntity)
    {
        if (const auto s_CurrentCamera = Functions::GetCurrentCamera->Call())
        {
            if (const auto s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>())
            {
                auto s_ModelMatrix = s_SpatialEntity->GetWorldMatrix();
                auto s_ViewMatrix = s_CurrentCamera->GetViewMatrix();
                const SMatrix s_ProjectionMatrix = *s_CurrentCamera->GetProjectionMatrix();

                ImGuizmo::SetRect(0, 0, s_ImgGuiIO.DisplaySize.x, s_ImgGuiIO.DisplaySize.y);

                if (ImGuizmo::Manipulate(&s_ViewMatrix.XAxis.x, &s_ProjectionMatrix.XAxis.x, m_GizmoMode, m_GizmoSpace, &s_ModelMatrix.XAxis.x, NULL, m_UseSnap ? &m_SnapValue[0] : NULL))
                {
                    s_SpatialEntity->SetWorldMatrix(s_ModelMatrix);

                    if (const auto s_PhysicsAspect = m_SelectedEntity.QueryInterface<ZStaticPhysicsAspect>())
                        s_PhysicsAspect->m_pPhysicsObject->SetTransform(s_SpatialEntity->GetWorldMatrix());
                }
            }
        }
    }

    m_EntityMutex.unlock_shared();
}

void DebugMod::OnMouseDown(SVector2 p_Pos, bool p_FirstClick)
{
    SVector3 s_World;
    SVector3 s_Direction;
    SDK()->ScreenToWorld(p_Pos, s_World, s_Direction);

    float4 s_DirectionVec(s_Direction.x, s_Direction.y, s_Direction.z, 1.f);

    float4 s_From = float4(s_World.x, s_World.y, s_World.z, 1.f);
    float4 s_To = s_From + (s_DirectionVec * 200.f);

    if (!*Globals::CollisionManager)
    {
        Logger::Error("Collision manager not found.");
        return;
    }

    ZRayQueryInput s_RayInput {
        .m_vFrom = s_From,
        .m_vTo = s_To,
    };

    ZRayQueryOutput s_RayOutput {};

    Logger::Debug("RayCasting from {} to {}.", s_From, s_To);

    if (!(*Globals::CollisionManager)->RayCastClosestHit(s_RayInput, &s_RayOutput))
    {
        Logger::Error("Raycast failed.");
        return;
    }

    Logger::Debug("Raycast result: {} {}", fmt::ptr(&s_RayOutput), s_RayOutput.m_vPosition);

    m_From = s_From;
    m_To = s_To;
    m_Hit = s_RayOutput.m_vPosition;
    m_Normal = s_RayOutput.m_vNormal;

    if (p_FirstClick)
    {
        m_EntityMutex.lock();

        if (s_RayOutput.m_BlockingEntity)
        {
            const auto& s_Interfaces = *s_RayOutput.m_BlockingEntity->GetType()->m_pInterfaces;
            Logger::Trace("Hit entity of type '{}' with id '{:x}'.", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName, s_RayOutput.m_BlockingEntity->GetType()->m_nEntityId);
        }

        m_SelectedEntity = s_RayOutput.m_BlockingEntity;
        m_SelectedEntityName.clear();

        m_EntityMutex.unlock();
    }
}

void DebugMod::DrawOptions(bool p_HasFocus)
{
    if (!p_HasFocus || !m_DebugMenuActive)
    {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    auto s_Showing = ImGui::Begin("DEBUG MENU", &m_DebugMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing)
    {
        ImGui::Checkbox("Render NPC position boxes", &m_RenderNpcBoxes);
        ImGui::Checkbox("Render NPC names", &m_RenderNpcNames);
        ImGui::Checkbox("Render NPC repository IDs", &m_RenderNpcRepoIds);
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

void DebugMod::EquipOutfit(
    const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit,
    unsigned int p_CurrentCharSetIndex,
    const char* p_CurrentCharSetCharacterType,
    unsigned int p_CurrentOutfitVariationIndex,
    ZHitman5* p_LocalHitman
)
{
    std::vector<ZRuntimeResourceID> s_HeroOutfitVariations;

    if (strcmp(p_CurrentCharSetCharacterType, "HeroA") != 0)
    {
        const ZOutfitVariationCollection* s_OutfitVariationCollection = p_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[p_CurrentCharSetIndex].m_pInterfaceRef;

        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType2 = &s_OutfitVariationCollection->m_aCharacters[2];
        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType = nullptr;

        if (strcmp(p_CurrentCharSetCharacterType, "Actor") == 0)
        {
            s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[0];
        }
        else if (strcmp(p_CurrentCharSetCharacterType, "Nude") == 0)
        {
            s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[1];
        }

        for (size_t i = 0; i < s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
        {
            s_HeroOutfitVariations.push_back(s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit);
        }

        if (s_CharsetCharacterType)
        {
            for (size_t i = 0; i < s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
            {
                s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = s_CharsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit;
            }
        }
    }

    Functions::ZHitman5_SetOutfit->Call(p_LocalHitman, p_GlobalOutfitKit, p_CurrentCharSetIndex, p_CurrentOutfitVariationIndex, false, false);

    if (strcmp(p_CurrentCharSetCharacterType, "HeroA") != 0)
    {
        ZOutfitVariationCollection* outfitVariationCollection = p_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[p_CurrentCharSetIndex].m_pInterfaceRef;
        TEntityRef<ZCharsetCharacterType>* charsetCharacterType = &outfitVariationCollection->m_aCharacters[2];

        for (size_t i = 0; i < s_HeroOutfitVariations.size(); ++i)
        {
            charsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = s_HeroOutfitVariations[i];
        }
    }
}

void DebugMod::EquipOutfit(
    const TEntityRef<ZGlobalOutfitKit>& p_GlobalOutfitKit,
    unsigned int p_CurrentCharSetIndex,
    const char* p_CurrentCharSetCharacterType,
    unsigned int p_CurrentOutfitVariationIndex,
    ZActor* p_Actor
)
{
    std::vector<ZRuntimeResourceID> s_ActorOutfitVariations;

    if (strcmp(p_CurrentCharSetCharacterType, "Actor") != 0)
    {
        const ZOutfitVariationCollection* s_OutfitVariationCollection = p_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[p_CurrentCharSetIndex].m_pInterfaceRef;

        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType2 = &s_OutfitVariationCollection->m_aCharacters[0];
        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType = nullptr;

        if (strcmp(p_CurrentCharSetCharacterType, "Nude") == 0)
        {
            s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[1];
        }
        else if (strcmp(p_CurrentCharSetCharacterType, "HeroA") == 0)
        {
            s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[2];
        }

        for (size_t i = 0; i < s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
        {
            s_ActorOutfitVariations.push_back(s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit);
        }

        if (s_CharsetCharacterType)
        {
            for (size_t i = 0; i < s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations.size(); ++i)
            {
                s_CharsetCharacterType2->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = s_CharsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit;
            }
        }
    }

    Functions::ZActor_SetOutfit->Call(p_Actor, p_GlobalOutfitKit, p_CurrentCharSetIndex, p_CurrentOutfitVariationIndex, false);

    if (strcmp(p_CurrentCharSetCharacterType, "Actor") != 0)
    {
        const ZOutfitVariationCollection* s_OutfitVariationCollection = p_GlobalOutfitKit.m_pInterfaceRef->m_aCharSets[p_CurrentCharSetIndex].m_pInterfaceRef;
        const TEntityRef<ZCharsetCharacterType>* s_CharsetCharacterType = &s_OutfitVariationCollection->m_aCharacters[0];

        for (size_t i = 0; i < s_ActorOutfitVariations.size(); ++i)
        {
            s_CharsetCharacterType->m_pInterfaceRef->m_aVariations[i].m_pInterfaceRef->m_Outfit = s_ActorOutfitVariations[i];
        }
    }
}

void DebugMod::SpawnRepositoryProp(const ZRepositoryID& p_RepositoryId, const bool addToWorld)
{
    TEntityRef<ZHitman5> s_LocalHitman;
    Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

    if (!s_LocalHitman)
    {
        Logger::Debug("No local hitman");
        return;
    }

    if (!addToWorld)
    {
        const TArray<TEntityRef<ZCharacterSubcontroller>>* s_Controllers = &s_LocalHitman.m_pInterfaceRef->m_pCharacter.m_pInterfaceRef->m_rSubcontrollerContainer.m_pInterfaceRef->m_aReferencedControllers;
        auto* s_Inventory = static_cast<ZCharacterSubcontrollerInventory*>(s_Controllers->operator[](6).m_pInterfaceRef);

        TArray<ZRepositoryID> s_ModifierIds;
        Functions::ZCharacterSubcontrollerInventory_AddDynamicItemToInventory->Call(s_Inventory, p_RepositoryId, "", &s_ModifierIds, 2);

        return;
    }

    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene)
    {
        Logger::Debug("Scene not loaded.");
        return;
    }

    const auto s_ID = ResId<"[modules:/zitemspawner.class].pc_entitytype">;
    const auto s_ID2 = ResId<"[modules:/zitemrepositorykeyentity.class].pc_entitytype">;

    TResourcePtr<ZTemplateEntityFactory> s_Resource, s_Resource2;

    Globals::ResourceManager->GetResourcePtr(s_Resource, s_ID, 0);
    Globals::ResourceManager->GetResourcePtr(s_Resource2, s_ID2, 0);

    Logger::Debug("Resource: {} {}", s_Resource.m_nResourceIndex, fmt::ptr(s_Resource.GetResource()));

    if (!s_Resource)
    {
        Logger::Debug("Resource is not loaded.");
        return;
    }

    ZEntityRef s_NewEntity, s_NewEntity2;

    Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_NewEntity, "", s_Resource, s_Scene.m_ref, nullptr, -1);
    Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_NewEntity2, "", s_Resource2, s_Scene.m_ref, nullptr, -1);

    if (!s_NewEntity)
    {
        Logger::Debug("Failed to spawn entity.");
        return;
    }

    if (!s_NewEntity2)
    {
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

void DebugMod::SpawnNonRepositoryProp(const char* p_PropAssemblyPath)
{
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene)
    {
        Logger::Debug("Scene not loaded.");
        return;
    }

    const Hash::MD5Hash s_Hash = Hash::MD5(std::string_view(p_PropAssemblyPath, strlen(p_PropAssemblyPath)));

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

    if (!s_Resource)
    {
        Logger::Debug("Resource is not loaded.");
        return;
    }

    ZEntityRef s_NewEntity;
    Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_NewEntity, "", s_Resource, s_Scene.m_ref, nullptr, -1);

    if (!s_NewEntity)
    {
        Logger::Debug("Failed to spawn entity.");
        return;
    }

    s_NewEntity.SetProperty("m_eRoomBehaviour", ZSpatialEntity::ERoomBehaviour::ROOM_DYNAMIC);

    TEntityRef<ZHitman5> s_LocalHitman;
    Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

    if (!s_LocalHitman)
    {
        Logger::Debug("No local hitman.");
        return;
    }

    const auto s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
    const auto s_PropSpatialEntity = s_NewEntity.QueryInterface<ZSpatialEntity>();

    s_PropSpatialEntity->SetWorldMatrix(s_HitmanSpatialEntity->GetWorldMatrix());
}

auto DebugMod::SpawnNPC(
    const char* p_NpcName,
    const ZRepositoryID& repositoryID,
    const TEntityRef<ZGlobalOutfitKit>* p_GlobalOutfitKit,
    const char* p_CurrentCharacterSetIndex,
    const char* p_CurrentcharSetCharacterType,
    const char* p_CurrentOutfitVariationIndex
) -> void
{
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene)
    {
        Logger::Debug("Scene not loaded.");
        return;
    }

    const auto s_RuntimeResourceId = ResId<"[assembly:/templates/gameplay/ai2/actors.template?/npcactor.entitytemplate].pc_entitytype">;

    TResourcePtr<ZTemplateEntityFactory> s_Resource;
    Globals::ResourceManager->GetResourcePtr(s_Resource, s_RuntimeResourceId, 0);

    if (!s_Resource)
    {
        Logger::Debug("Resource is not loaded.");
        return;
    }

    ZEntityRef s_NewEntity;
    Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_NewEntity, "", s_Resource, s_Scene.m_ref, nullptr, -1);

    if (!s_NewEntity)
    {
        Logger::Debug("Could not spawn entity.");
        return;
    }

    TEntityRef<ZHitman5> s_LocalHitman;
    Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

    if (!s_LocalHitman)
    {
        Logger::Debug("No local hitman.");
        return;
    }

    ZActor* actor = s_NewEntity.QueryInterface<ZActor>();

    actor->m_sActorName = p_NpcName;
    actor->m_bStartEnabled = true;
    actor->m_nOutfitCharset = std::stoi(p_CurrentCharacterSetIndex);
    actor->m_nOutfitVariation = std::stoi(p_CurrentOutfitVariationIndex);
    actor->m_OutfitRepositoryID = repositoryID;
    actor->m_eRequiredVoiceVariation = EActorVoiceVariation::eAVV_Undefined;

    actor->Activate(0);

    ZSpatialEntity* s_ActorSpatialEntity = s_NewEntity.QueryInterface<ZSpatialEntity>();
    ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

    s_ActorSpatialEntity->SetWorldMatrix(s_HitmanSpatialEntity->GetWorldMatrix());

    if (p_GlobalOutfitKit)
    {
        EquipOutfit(*p_GlobalOutfitKit, std::stoi(p_CurrentCharacterSetIndex), p_CurrentcharSetCharacterType, std::stoi(p_CurrentOutfitVariationIndex), actor);
    }
}

void DebugMod::LoadRepositoryProps()
{
    if (m_RepositoryResource.m_nResourceIndex == -1)
    {
        const auto s_ID = ResId<"[assembly:/repository/pro.repo].pc_repo">;

        Globals::ResourceManager->GetResourcePtr(m_RepositoryResource, s_ID, 0);
    }

    if (m_RepositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID)
    {
        const auto s_RepositoryData = static_cast<THashMap<ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(m_RepositoryResource.GetResourceData());

        for (auto it = s_RepositoryData->begin(); it != s_RepositoryData->end(); ++it)
        {
            const ZDynamicObject* s_DynamicObject = &it->second;
            const TArray<SDynamicObjectKeyValuePair>* s_Entries = s_DynamicObject->As<TArray<SDynamicObjectKeyValuePair>>();

            std::string s_Id;

            for (size_t i = 0; i < s_Entries->size(); ++i)
            {
                std::string s_Key = s_Entries->operator[](i).sKey.c_str();

                if (s_Key == "ID_")
                {
                    s_Id = ConvertDynamicObjectValueTString(&s_Entries->operator[](i).value);
                }

                if (s_Key == "Title")
                {
                    std::string s_Title = ConvertDynamicObjectValueTString(&s_Entries->operator[](i).value);

                    m_RepositoryProps.insert(std::make_pair(s_Title, ZRepositoryID(s_Id.c_str())));

                    break;
                }
            }
        }
    }
}

void DebugMod::LoadHashMap()
{
    // TODO: Re-do.

    /*if (!std::filesystem::exists("hash_list.txt"))
    {
        DownloadHashMap();

        //LZMA::Extract();
    }

    auto s_InputFile = std::ifstream("hash_list.txt", std::ios::binary | std::ios::ate);

    if (s_InputFile.fail())
    {
        return;
    }

    const auto s_FileSize = static_cast<uint64_t>(s_InputFile.tellg());

    s_InputFile.seekg(0, s_InputFile.beg);

    std::vector<char> s_Data(s_FileSize, 0);

    s_InputFile.read(s_Data.data(), s_FileSize);

    uint64_t s_Position = 0;
    uint64_t s_LastPosition = 0;
    uint64_t s_LineCount = 0;

    mutex.lock();

    while (s_Position < s_Data.size())
    {
        if (s_Data[s_Position] == 0x0A)
        {
            s_LineCount++;

            s_Data[s_Position] = 0x0;

            if (s_LineCount > 3)
            {
                const auto s_StringView = std::string_view(&s_Data[s_LastPosition]);
                const size_t s_Index = s_StringView.find_first_of(',');

                auto s_Hash = std::string(s_StringView.substr(0, (s_Index - 5)));
                const auto s_ResourceId = std::string(s_StringView.substr(s_Index + 1, s_StringView.length() - (s_Index + 1)));

                runtimeResourceIDsToResourceIDs[std::stoull(s_Hash, nullptr, 16)] = s_ResourceId;

            }

            s_LastPosition = s_Position + 1;
        }

        s_Position++;
    }

    mutex.unlock();*/
}

void DebugMod::DownloadHashMap()
{
    const std::string s_FolderPath = std::format("{}\\latest-hashes.7z", std::filesystem::current_path().string());
    URLDownloadToFileA(nullptr, "https://hitmandb.glaciermodding.org/latest-hashes.7z", s_FolderPath.c_str(), 0, nullptr);
}

std::string DebugMod::GetEntityName(
    unsigned long long p_TempBrickHash,
    unsigned long long p_EntityId,
    unsigned long long& p_ResourceHash
)
{
    std::string s_EntityName;
    TResourcePtr<ZTemplateEntityFactory> s_TempBrickResource;

    Globals::ResourceManager->GetResourcePtr(s_TempBrickResource, ZRuntimeResourceID(p_TempBrickHash), 0);

    ZTemplateEntityFactory* s_Resource = s_TempBrickResource.GetResource();

    if (!s_Resource)
    {
        return s_EntityName;
    }

    ZResourceContainer::SResourceInfo s_TbluBrickResourceInfo = (*Globals::ResourceContainer)->m_resources[s_Resource->m_blueprintResource.m_nResourceIndex];

    unsigned int s_EntityIndex = -1;
    static unsigned long long s_DataSectionOffset = 0x10;
    std::vector<char> s_TempBrickResourceData;
    std::vector<char> s_TbluBrickResourceData;

    TArray<ZString>* s_MountedPackages = &(*Globals::ResourceContainer)->m_MountedPackages;
    std::string s_RpkgFilePath = (*s_MountedPackages)[s_MountedPackages->size() - 1].c_str();

    LoadResourceData(p_TempBrickHash, s_TempBrickResourceData, s_RpkgFilePath);
    LoadResourceData(s_TbluBrickResourceInfo.rid.GetID(), s_TbluBrickResourceData, s_RpkgFilePath);

    ZBinaryReader s_BinaryReader(&s_TbluBrickResourceData);

    s_BinaryReader.Seek(s_DataSectionOffset + 0x8);

    unsigned long long s_SubEntitiesStartOffset = s_BinaryReader.Read<unsigned long long>();
    unsigned long long s_SubEntitiesEndOffset = s_BinaryReader.Read<unsigned long long>();
    auto s_SubEntityCount = static_cast<unsigned int>((s_SubEntitiesEndOffset - s_SubEntitiesStartOffset) / 0xA8); //0xA8 is size of STemplateBlueprintSubEntity

    for (unsigned int i = 0; i < s_SubEntityCount; ++i)
    {
        s_BinaryReader.Seek(s_DataSectionOffset + s_SubEntitiesStartOffset + i * 0xA8 + 0x28);

        unsigned long long s_EntityId2 = s_BinaryReader.Read<unsigned long long>();

        if (p_EntityId == s_EntityId2)
        {
            s_BinaryReader.Skip(0x10);

            unsigned long long entityNameOffset = s_BinaryReader.Read<unsigned long long>();

            s_BinaryReader.Seek(entityNameOffset + s_DataSectionOffset - 4);

            int stringLength = s_BinaryReader.Read<unsigned int>();
            s_EntityName = s_BinaryReader.ReadString(stringLength - 1);
            s_EntityIndex = i;

            break;
        }
    }

    if (s_EntityIndex != -1)
    {
        ZBinaryReader s_BinaryReader2(&s_TempBrickResourceData);

        s_BinaryReader2.Seek(s_DataSectionOffset + 0x10);

        unsigned long long s_SubEntitiesStartOffset2 = s_BinaryReader2.Read<unsigned long long>();
        unsigned long long s_SubEntityOffset = s_DataSectionOffset + s_SubEntitiesStartOffset2 + s_EntityIndex * 0x70; //0x70 is size of STemplateFactorySubEntity

        s_BinaryReader2.Seek(s_SubEntityOffset + 0x20);

        int s_EntityTypeResourceIndex = s_BinaryReader2.Read<unsigned int>();

        TArray<ZResourceIndex> s_ReferenceIndices;
        TArray<unsigned char> s_ReferenceFlags;

        Functions::ZResourceContainer_GetResourceReferences->Call(*Globals::ResourceContainer, ZResourceIndex(s_TempBrickResource.m_nResourceIndex), s_ReferenceIndices, s_ReferenceFlags);

        ZResourceContainer::SResourceInfo s_ReferenceInfo = (*Globals::ResourceContainer)->m_resources[s_ReferenceIndices[s_EntityTypeResourceIndex].val];

        p_ResourceHash = s_ReferenceInfo.rid.GetID();
    }

    return s_EntityName;
}


std::string DebugMod::FindNPCEntityNameInBrickBackReferences(
    unsigned long long p_TempBrickHash,
    unsigned long long p_EntityId,
    unsigned long long& p_ResourceHash
)
{
    std::string s_EntityName;
    ZResourceContainer* s_ResourceContainer = *Globals::ResourceContainer;

    for (unsigned int i = 0; i < s_ResourceContainer->m_resourcesSize; ++i)
    {
        const ZResourceContainer::SResourceInfo* s_ResourceInfo = &s_ResourceContainer->m_resources[i];
        unsigned long long s_ResourceHash2 = s_ResourceInfo->rid.GetID();

        if (s_ResourceInfo->resourceType == 'TEMP' &&
            s_ResourceInfo->numReferences > 0 &&
            m_RuntimeResourceIDsToResourceIDs.contains(s_ResourceHash2) &&
            m_RuntimeResourceIDsToResourceIDs[s_ResourceHash2].ends_with(".brick].pc_entitytype") &&
            m_RuntimeResourceIDsToResourceIDs[s_ResourceHash2].contains("/npc_"))
        {
            const ZResourceIndex s_ResourceIndex = s_ResourceContainer->m_indices.find(s_ResourceInfo->rid)->second;
            TArray<ZResourceIndex> s_ReferenceIndices;
            TArray<unsigned char> s_ReferenceFlags;

            Functions::ZResourceContainer_GetResourceReferences->Call(s_ResourceContainer, ZResourceIndex(s_ResourceIndex), s_ReferenceIndices, s_ReferenceFlags);

            for (size_t j = 0; j < s_ReferenceIndices.size(); ++j)
            {
                const size_t s_ReferenceIndex = s_ReferenceIndices[j].val;

                if (s_ReferenceIndex == -1)
                {
                    continue;
                }

                ZResourceContainer::SResourceInfo s_ReferenceInfo = s_ResourceContainer->m_resources[s_ReferenceIndex];
                const unsigned long long s_ReferenceHash = s_ReferenceInfo.rid.GetID();

                if (s_ReferenceHash == p_TempBrickHash)
                {
                    s_EntityName = GetEntityName(s_ResourceHash2, p_EntityId, p_ResourceHash);

                    if (!s_EntityName.empty())
                    {
                        break;
                    }
                }
            }
        }

        if (!s_EntityName.empty())
        {
            break;
        }
    }

    return s_EntityName;
}

std::string DebugMod::ConvertDynamicObjectValueTString(ZDynamicObject* p_DynamicObject)
{
    std::string s_Result;
    const IType* s_Type = p_DynamicObject->m_pTypeID->typeInfo();

    if (strcmp(s_Type->m_pTypeName, "ZString") == 0)
    {
        const auto s_Value = p_DynamicObject->As<ZString>();
        s_Result = s_Value->c_str();
    }
    else if (strcmp(s_Type->m_pTypeName, "bool") == 0)
    {
        if (*p_DynamicObject->As<bool>())
        {
            s_Result = "true";
        }
        else
        {
            s_Result = "false";
        }
    }
    else if (strcmp(s_Type->m_pTypeName, "float64") == 0)
    {
        double value = *p_DynamicObject->As<double>();

        s_Result = std::to_string(value).c_str();
    }
    else
    {
        s_Result = s_Type->m_pTypeName;
    }

    return s_Result;
}

void DebugMod::LoadResourceData(unsigned long long p_Hash, std::vector<char>& p_ResourceData)
{
    static std::string s_RpkgFilePath = GetPatchRPKGFilePath();
    LoadResourceData(p_Hash, p_ResourceData, s_RpkgFilePath);
}

void DebugMod::LoadResourceData(unsigned long long p_Hash, std::vector<char>& p_ResourceData, const std::string& p_RpkgFilePath)
{
    ZBinaryReader s_BinaryReader(p_RpkgFilePath);

    s_BinaryReader.Seek(0xD);

    unsigned int s_ResourceCount = s_BinaryReader.Read<unsigned int>();
    unsigned int s_ResourceHeadersChunkSize = s_BinaryReader.Read<unsigned int>();
    unsigned int s_ResourcesChunkSize = s_BinaryReader.Read<unsigned int>();
    unsigned int s_PatchDeletionEntryCount = s_BinaryReader.Read<unsigned int>();
    bool s_IsPatchFile = false;

    if (s_PatchDeletionEntryCount * 8 + 0x2D >= s_BinaryReader.GetSize())
    {
        s_IsPatchFile = false;
    }
    else
    {
        s_BinaryReader.Seek(s_PatchDeletionEntryCount * 8 + 0x24);

        unsigned char s_TestZeroValue = s_BinaryReader.Read<unsigned char>();
        unsigned long long s_TestHeaderOffset = s_BinaryReader.Read<unsigned long long>();

        if (s_TestHeaderOffset == (s_ResourceHeadersChunkSize + s_ResourcesChunkSize + s_PatchDeletionEntryCount * 8 + 0x1D) && s_TestZeroValue == 0)
        {
            s_IsPatchFile = true;
        }
    }

    if (s_IsPatchFile)
    {
        s_BinaryReader.Seek(0x1D);
        s_BinaryReader.Skip(8 * s_PatchDeletionEntryCount);
    }
    else
    {
        s_BinaryReader.Seek(0x19);
    }

    unsigned int i = 0;

    while (i < s_ResourceCount)
    {
        unsigned long long s_Hash2 = s_BinaryReader.Read<unsigned long long>();

        if (s_Hash2 == p_Hash)
        {
            unsigned long long s_ResourceDataOffset = s_BinaryReader.Read<unsigned long long>();
            unsigned int s_DataSize = s_BinaryReader.Read<unsigned int>();

            bool s_IsResourceEncrypted = (s_DataSize & 0x80000000) == 0x80000000;
            bool s_IsResourceCompressed = (s_DataSize & 0x3FFFFFFF) != 0;

            s_BinaryReader.Seek(s_ResourceDataOffset);

            TResourcePtr<ZTemplateEntityFactory> s_Resource;

            Globals::ResourceManager->GetResourcePtr(s_Resource, ZRuntimeResourceID(p_Hash), 0);

            ZResourceContainer::SResourceInfo s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[s_Resource.m_nResourceIndex];

            if (s_IsResourceEncrypted)
            {
                s_DataSize &= 0x3FFFFFFF;
            }
            else
            {
                s_DataSize = s_ResourceInfo.finalDataSize;
            }

            std::vector<char> s_InputResourceData;

            s_InputResourceData.reserve(s_DataSize);
            s_BinaryReader.ReadBytes(s_InputResourceData.data(), s_DataSize);

            if (s_IsResourceEncrypted)
            {
                Crypto::XORData(s_InputResourceData.data(), s_DataSize);
            }

            std::vector<char> s_OutputResourceData(s_ResourceInfo.finalDataSize, 0);

            if (s_IsResourceCompressed)
            {
                LZ4_decompress_safe(s_InputResourceData.data(), s_OutputResourceData.data(), s_DataSize, s_ResourceInfo.finalDataSize);

                p_ResourceData = s_OutputResourceData;
            }
            else
            {
                p_ResourceData = s_InputResourceData;
            }

            break;
        }

        size_t s_CurrentPosition = s_BinaryReader.GetPosition();

        s_BinaryReader.Seek(s_CurrentPosition + 12);

        i++;
    }
}

std::string DebugMod::GetPatchRPKGFilePath()
{
    for (const auto& s_Entry : std::filesystem::directory_iterator("../Runtime"))
    {
        if (s_Entry.path().string().starts_with("../Runtime\\chunk0"))
        {
            return s_Entry.path().string();
        }
    }

    return "";
}

unsigned long long DebugMod::GetDDSTextureHash(const std::string p_Image)
{
    static std::unordered_map<std::string, unsigned long long> g_OresEntries;

    if (g_OresEntries.empty())
    {
        const auto s_ID = ResId<"[assembly:/_pro/online/default/offlineconfig/config.blobs].pc_blobs">;
        TResourcePtr<ZTemplateEntityFactory> s_Resource;

        Globals::ResourceManager->GetResourcePtr(s_Resource, s_ID, 0);

        ZResourceContainer::SResourceInfo s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[s_Resource.m_nResourceIndex];

        unsigned long long s_OresHash = s_ResourceInfo.rid.GetID();
        std::vector<char> s_OresResourceData;

        LoadResourceData(s_OresHash, s_OresResourceData);

        ZBinaryReader s_BinaryReader(&s_OresResourceData);

        s_BinaryReader.Seek(0x10);
        s_BinaryReader.Seek(s_BinaryReader.Read<unsigned int>() + 0xC);

        unsigned s_ResourceCount = s_BinaryReader.Read<unsigned int>();

        for (unsigned int i = 0; i < s_ResourceCount; ++i)
        {
            auto s_StringLength = s_BinaryReader.Read<unsigned int>();

            s_BinaryReader.Seek(0x4, ZBinaryReader::ESeekOrigin::current);

            auto s_StringOffset = s_BinaryReader.Read<unsigned long long>();
            auto s_RuntimeResourceId = s_BinaryReader.Read<ZRuntimeResourceID>();

            size_t s_CurrentPosition = s_BinaryReader.GetPosition();

            s_BinaryReader.Seek(s_StringOffset + 0x10 - 0x4);

            auto s_StringLength2 = s_BinaryReader.Read<unsigned int>();
            auto s_Image2 = s_BinaryReader.ReadString(s_StringLength2 - 1);

            s_BinaryReader.Seek(s_CurrentPosition);

            g_OresEntries[s_Image2.c_str()] = s_RuntimeResourceId.GetID();
        }
    }

    return g_OresEntries[p_Image];
}

void DebugMod::EnableInfiniteAmmo()
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

    Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_NewCrippleBox, "", s_CrippleBoxFactory, s_Scene.m_ref, nullptr, -1);

    if (!s_NewCrippleBox)
    {
        Logger::Debug("Failed to spawn entity.");
        return;
    }

    TEntityRef<ZHitman5> s_LocalHitman;
    Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

    if (!s_LocalHitman)
    {
        Logger::Debug("Local player is not alive.");
        return;
    }

    ZHM5CrippleBox* hm5CrippleBox = s_NewCrippleBox.QueryInterface<ZHM5CrippleBox>();

    hm5CrippleBox->m_bActivateOnStart = true;
    hm5CrippleBox->m_rHitmanCharacter = s_LocalHitman;
    hm5CrippleBox->m_bLimitedAmmo = false;

    hm5CrippleBox->Activate(0);
}

void DebugMod::CopyToClipboard(const std::string& p_String) const
{
    if (!OpenClipboard(nullptr))
        return;

    EmptyClipboard();

    const auto s_GlobalData = GlobalAlloc(GMEM_MOVEABLE, p_String.size() + 1);

    if (!s_GlobalData)
    {
        CloseClipboard();
        return;
    }

    const auto s_GlobalDataPtr = GlobalLock(s_GlobalData);

    if (!s_GlobalDataPtr)
    {
        CloseClipboard();
        GlobalFree(s_GlobalData);
        return;
    }

    memset(s_GlobalDataPtr, 0, p_String.size() + 1);
    memcpy(s_GlobalDataPtr, p_String.c_str(), p_String.size());

    GlobalUnlock(s_GlobalData);

    SetClipboardData(CF_TEXT, s_GlobalData);
    CloseClipboard();
}

void DebugMod::OnDraw3D(IRenderer* p_Renderer)
{
    if (m_RenderNpcBoxes || m_RenderNpcNames || m_RenderNpcRepoIds)
    {
        for (int i = 0; i < *Globals::NextActorId; ++i)
        {
            auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

            ZEntityRef s_Ref;
            s_Actor->GetID(&s_Ref);

            auto* s_SpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

            SMatrix s_Transform;
            Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

            if (m_RenderNpcBoxes)
            {
                float4 s_Min, s_Max;

                s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

                p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(1.f, 0.f, 0.f, 1.f));
            }

            if (m_RenderNpcNames)
            {
                SVector2 s_ScreenPos;
                if (p_Renderer->WorldToScreen(SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.05f), s_ScreenPos))
                    p_Renderer->DrawText2D(s_Actor->m_sActorName, s_ScreenPos, SVector4(1.f, 0.f, 0.f, 1.f), 0.f, 0.5f);
            }

            if (m_RenderNpcRepoIds)
            {
                auto* s_RepoEntity = s_Ref.QueryInterface<ZRepositoryItemEntity>();
                SVector2 s_ScreenPos;
                bool s_Success;

                if (m_RenderNpcNames)
                    s_Success = p_Renderer->WorldToScreen(SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.1f), s_ScreenPos);
                else
                    s_Success = p_Renderer->WorldToScreen(SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.05f), s_ScreenPos);

                if (s_Success)
                    p_Renderer->DrawText2D(s_RepoEntity->m_sId.ToString(), s_ScreenPos, SVector4(1.f, 0.f, 0.f, 1.f), 0.f, 0.5f);
            }
        }
    }

    m_EntityMutex.lock_shared();

    if (m_SelectedEntity)
    {
        auto* s_SpatialEntity = m_SelectedEntity.QueryInterface<ZSpatialEntity>();

        SMatrix s_Transform;
        Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

        float4 s_Min, s_Max;

        s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

        p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(0.f, 0.f, 1.f, 1.f));
    }

    m_EntityMutex.unlock_shared();

    /*SVector2 s_StartPos;
    if (p_Renderer->WorldToScreen(SVector3(m_From.x, m_From.y, m_From.z), s_StartPos))
        p_Renderer->DrawText2D("0", s_StartPos, SVector4(1, 1, 1, 1), 0, 0.5f);

    SVector2 s_EndPos;
    if (p_Renderer->WorldToScreen(SVector3(m_To.x, m_To.y, m_To.z), s_EndPos))
        p_Renderer->DrawText2D("o", s_EndPos, SVector4(1, 1, 1, 1), 0, 0.25f);

    SVector2 s_HitPos;
    if (p_Renderer->WorldToScreen(SVector3(m_Hit.x, m_Hit.y, m_Hit.z), s_HitPos))
        p_Renderer->DrawText2D("x", s_HitPos, SVector4(1, 1, 1, 1), 0, 0.25f);

    p_Renderer->DrawBox3D(
        SVector3(m_From.x - 0.05f, m_From.y - 0.05f, m_From.z - 0.05f),
        SVector3(m_From.x + 0.05f, m_From.y + 0.05f, m_From.z + 0.05f),
        SVector4(0.f, 0.f, 1.f, 1.0f)
    );

    p_Renderer->DrawBox3D(
        SVector3(m_To.x - 0.05f, m_To.y - 0.05f, m_To.z - 0.05f),
        SVector3(m_To.x + 0.05f, m_To.y + 0.05f, m_To.z + 0.05f),
        SVector4(0.f, 1.f, 0.f, 1.0f)
    );

    p_Renderer->DrawBox3D(
        SVector3(m_Hit.x - 0.01f, m_Hit.y - 0.01f, m_Hit.z - 0.01f),
        SVector3(m_Hit.x + 0.01f, m_Hit.y + 0.01f, m_Hit.z + 0.01f),
        SVector4(0.f, 1.f, 1.f, 1.0f)
    );

    p_Renderer->DrawLine3D(
        SVector3(m_From.x, m_From.y, m_From.z),
        SVector3(m_To.x, m_To.y, m_To.z),
        SVector4(1.f, 1.f, 1.f, 1.f),
        SVector4(1.f, 1.f, 1.f, 1.f)
    );

    p_Renderer->DrawLine3D(
        SVector3(m_Hit.x + (m_Normal.x * 0.15f), m_From.y + (m_Normal.y * 0.15f), m_From.z + (m_Normal.z * 0.15f)),
        SVector3(m_Hit.x, m_Hit.y, m_Hit.z),
        SVector4(0.63f, 0.13f, 0.94f, 1.f),
        SVector4(0.63f, 0.13f, 0.94f, 1.f)
    );*/
}

DECLARE_PLUGIN_DETOUR(DebugMod, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear)
{
    m_EntityMutex.lock();
    m_SelectedEntity = ZEntityRef();

    m_SelectedEntityName = "";
    m_SelectedResourceHash = 0;
    m_EntityId = 0;
    m_BrickEntityId = 0;
    m_BrickHashes.clear();

    m_TextureSrvGpuHandle = {};
    m_Width = 0;
    m_Height = 0;
    m_RepositoryResource = {};
    m_TextureResourceData.clear();
    m_RepositoryProps.clear();
    m_Hm5CrippleBox = nullptr;

    m_EntityMutex.unlock();

    return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(DebugMod);
