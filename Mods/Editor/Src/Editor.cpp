#include "Editor.h"

#include <numbers>

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZScene.h>

#include "IconsMaterialDesign.h"
#include "Glacier/ZGeomEntity.h"
#include "Glacier/ZModule.h"
#include "Glacier/ZSpatialEntity.h"
#include "Glacier/ZCameraEntity.h"
#include "Glacier/ZRender.h"
#include "Glacier/EntityFactory.h"
#include "Glacier/ZApplicationEngineWin32.h"
#include <ranges>

#include <imgui_impl_dx12.h>
#include <imgui_internal.h>

#include "Glacier/SGameUpdateEvent.h"
#include "Glacier/ZCollision.h"
#include "Glacier/ZActor.h"
#include "Glacier/ZGameLoopManager.h"
#include "Glacier/ZKnowledge.h"
#include "Glacier/SExternalReferences.h"

#include <ResourceLib_HM3.h>

#include "Util/StringUtils.h"

Editor::Editor() {
    // Disable ZTemplateEntityBlueprintFactory freeing its associated data.
    uint8_t s_Nop[0x45] = {};
    memset(s_Nop, 0x90, sizeof(s_Nop));

    if (!SDK()->PatchCode(
        "\x48\x85\xDB\x74\x00\xE8\x00\x00\x00\x00\x48\x8B\x08\x48\x85\xC9\x75\x00\xE8\x00\x00\x00\x00\x48\x8B\x48\x10\x48\x8B\x01\xEB\x00\x48\x8B\x01\x48\x8B\xD3\xFF\x50\x48\x48\x8B\xC8\x48\x85\xC0\x74\x00\x48\x8B\x00\x48\x8B\xD3\xFF\x50\x48\x48\xC7\x86\xA0\x01\x00\x00",
        "xxxx?x????xxxxxxx?x????xxxxxxxx?xxxxxxxxxxxxxxxx?xxxxxxxxxxxxxxxx",
        s_Nop,
        sizeof(s_Nop),
        0
    )) {
        Logger::Error("Could not patch ZTemplateEntityBlueprintFactory data freeing.");
    }

    if (!SDK()->PatchCode(
        "\x48\x85\xFF\x74\x00\xE8\x00\x00\x00\x00\x48\x8B\x08\x48\x85\xC9\x75\x00\xE8\x00\x00\x00\x00\x48\x8B\x48\x10\x48\x8B\x01\xEB\x00\x48\x8B\x01\x48\x8B\xD7\xFF\x50\x48\x48\x8B\xC8\x48\x85\xC0\x74\x00\x48\x8B\x00\x48\x8B\xD7\xFF\x50\x48\x48\xC7\x83\xA0\x01\x00\x00",
        "xxxx?x????xxxxxxx?x????xxxxxxxx?xxxxxxxxxxxxxxxx?xxxxxxxxxxxxxxxx",
        s_Nop,
        sizeof(s_Nop),
        0
    )) {
        Logger::Error("Could not patch ZTemplateEntityBlueprintFactory brick data freeing.");
    }

    // Initialize Winsock and create the Qne socket and relevant things.
    WSADATA s_Ws;
    if (WSAStartup(MAKEWORD(2, 2), &s_Ws) != 0) {
        Logger::Error("WSAStartup failed: %d", WSAGetLastError());
        return;
    }

    if ((m_QneSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        Logger::Error("Could not create socket: %d", WSAGetLastError());
        return;
    }

    // Make socket non-blocking.
    u_long s_NonBlocking = 1;
    if (ioctlsocket(m_QneSocket, FIONBIO, &s_NonBlocking) != 0) {
        Logger::Error("Could not make socket non-blocking: %d", WSAGetLastError());
        return;
    }

    m_QneAddress.sin_family = AF_INET;
    m_QneAddress.sin_port = htons(49494);
    m_QneAddress.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    m_raycastLogging = false;
}

Editor::~Editor() {
    const ZMemberDelegate<Editor, void(const SGameUpdateEvent&)> s_Delegate(this, &Editor::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);

    if (m_TrackCamActive) {
        DisableTrackCam();
    }

    if (m_SelectionForFreeCameraEditorStyleEntity) {
        Globals::Selections->clear();
        free(m_SelectionForFreeCameraEditorStyleEntity);
        m_SelectionForFreeCameraEditorStyleEntity = nullptr;
    }
}

void Editor::Init() {
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &Editor::OnLoadScene);
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &Editor::OnClearScene);
    Hooks::ZTemplateEntityBlueprintFactory_ZTemplateEntityBlueprintFactory->AddDetour(
        this,
        &Editor::ZTemplateEntityBlueprintFactory_ctor
    );
    Hooks::SignalInputPin->AddDetour(this, &Editor::OnInputPin);
    Hooks::SignalOutputPin->AddDetour(this, &Editor::OnOutputPin);

    Hooks::ZEntityManager_NewUninitializedEntity->AddDetour(this, &Editor::ZEntityManager_NewUninitializedEntity);
    Hooks::ZEntityManager_DeleteEntity->AddDetour(this, &Editor::ZEntityManager_DeleteEntity);

    Hooks::ZTemplateEntityFactory_ConfigureEntity->AddDetour(this, &Editor::ZTemplateEntityFactory_ConfigureEntity);

    Hooks::ZExtendedCppEntityTypeInstaller_Install->AddDetour(this, &Editor::ZExtendedCppEntityTypeInstaller_Install);

    Hooks::ZResourceManager_UninstallResource->AddDetour(this, &Editor::ZResourceManager_UninstallResource);

    m_UseSnap = GetSettingBool("general", "snap", true);
    m_SnapValue = GetSettingDouble("general", "snap_value", 1.0);
    m_UseAngleSnap = GetSettingBool("general", "angle_snap", true);
    m_AngleSnapValue = GetSettingDouble("general", "angle_snap_value", 90.0);
    m_UseScaleSnap = GetSettingBool("general", "scale_snap", true);
    m_ScaleSnapValue = GetSettingDouble("general", "scale_snap_value", 1.0);
    m_UseQneTransforms = GetSettingBool("general", "qne_transforms", false);
    m_EditorWindowsVisible = GetSettingBool("general", "editor_windows_visible", true);
}

void Editor::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_VIDEO_SETTINGS "  EDITOR")) {
        m_SettingsVisible = !m_SettingsVisible;
    }

    /*if (ImGui::Button(ICON_MD_VIDEO_SETTINGS "  EDITOR"))
    {
        const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

        if (s_Scene)
        {
            for (auto& s_Brick : Globals::Hitman5Module->m_pEntitySceneContext->m_aLoadedBricks)
            {
                if (s_Brick.runtimeResourceID != ResId<"[assembly:/_sdk/editor/editor_data.brick].pc_entitytype">)
                    continue;

                Logger::Debug("Found editor_data brick.");

                const auto s_BpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_Brick.entityRef.GetBlueprintFactory());

                const auto s_Index = s_BpFactory->GetSubEntityIndex(0xfeedbf5a41eb9c48);

                if (s_Index != -1)
                {
                    Logger::Debug("Found RT at index {}.", s_Index);
                    m_CameraRT = s_BpFactory->GetSubEntity(s_Brick.entityRef.m_pEntity, s_Index);

                    const auto s_CameraRTEntity = m_CameraRT.QueryInterface<ZRenderDestinationTextureEntity>();
                    const auto s_RT = reinterpret_cast<ZRenderDestination*>(s_CameraRTEntity->GetRenderDestination());

                    Logger::Debug("RTEntity = {} RT = {}", fmt::ptr(s_CameraRTEntity), fmt::ptr(s_RT));

                    const auto s_Camera = Functions::GetCurrentCamera->Call();

                    ZEntityRef s_CameraRef;
                    s_Camera->GetID(&s_CameraRef);

                    s_CameraRTEntity->AddClient(s_CameraRef);

                    for (auto& s_Client : *s_CameraRTEntity->GetClients())
                    {
                        Logger::Debug("RT client = {} {:x} {}", fmt::ptr(s_Client.GetEntity()), s_Client->GetType()->m_nEntityId, (*s_Client->GetType()->m_pInterfaces)[0].m_pTypeId->typeInfo()->m_pTypeName);
                    }
                }

                const auto s_CameraIndex = s_BpFactory->GetSubEntityIndex(0xfeedb6fc4f5626ea);

                if (s_CameraIndex != -1)
                {
                    Logger::Debug("Found Cam at index {}.", s_CameraIndex);
                    m_Camera = s_BpFactory->GetSubEntity(s_Brick.entityRef.m_pEntity, s_CameraIndex);

                    Logger::Debug("CamEntity = {}", fmt::ptr(m_Camera.GetEntity()));
                }

                break;
            }
        }
    }*/

    if (ImGui::Button(ICON_MD_TUNE " ITEMS")) {
        m_ItemsMenuActive = !m_ItemsMenuActive;
    }

    if (ImGui::Button(ICON_MD_PEOPLE " ACTORS")) {
        m_ActorsMenuActive = !m_ActorsMenuActive;
    }

    if (ImGui::Button(ICON_MD_CATEGORY " DEBUG CHANNELS")) {
        m_DebugChannelsMenuActive = !m_DebugChannelsMenuActive;
    }

    if (ImGui::Button(ICON_MD_MEETING_ROOM " ROOMS")) {
        m_RoomsMenuActive = !m_RoomsMenuActive;
    }
}

void Editor::ToggleEditorServerEnabled() {
    EditorServer::SetEnabled(!EditorServer::GetEnabled());
}

void Editor::CopyToClipboard(const std::string& p_String) const {
    if (!OpenClipboard(nullptr))
        return;

    EmptyClipboard();

    const auto s_GlobalData = GlobalAlloc(GMEM_MOVEABLE, p_String.size() + 1);

    if (!s_GlobalData) {
        CloseClipboard();
        return;
    }

    const auto s_GlobalDataPtr = GlobalLock(s_GlobalData);

    if (!s_GlobalDataPtr) {
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

void Editor::OnDraw3D(IRenderer* p_Renderer) {
    DrawEntityAABB(p_Renderer);

    /*const auto s_Color = SVector4(0.88, 0.88, 0.08, 0.4);
    const auto s_LineColor = SVector4(0.94, 0.12, 0.05, 1.0);

    p_Renderer->DrawQuad3D(
        { -26.179094, -25.697458, 0.5 },
        s_Color,
        { -25.915297, -27.365824, 0.5 },
        s_Color,
        { -27.750357, -27.950037, 0.5 },
        s_Color,
        { -27.304773, -25.154234, 0.5 },
        s_Color
    );*/

    //p_Renderer->DrawLine3D({ -35.352013, -23.58427, 0.4925564 }, { -27.71298, -24.866821, 0.4925564 }, s_LineColor, s_LineColor);
    //p_Renderer->DrawLine3D({ -27.71298, -24.866821, 0.4925564 }, { -26.691515, -38.064953, 0.4925564 }, s_LineColor, s_LineColor);
    //p_Renderer->DrawLine3D({ -26.691515, -38.064953, 0.4925564 }, { -41.43283, -33.25945, 0.49255627 }, s_LineColor, s_LineColor);
    //p_Renderer->DrawLine3D({ -41.43283, -33.25945, 0.49255627 }, { -35.352013, -23.58427, 0.4925564 }, s_LineColor, s_LineColor);
}

void Editor::OnDepthDraw3D(IRenderer* p_Renderer) {
    DrawDebugEntities(p_Renderer);
}

void Editor::OnEngineInitialized() {
    const ZMemberDelegate<Editor, void(const SGameUpdateEvent&)> s_Delegate(this, &Editor::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);
}

bool Editor::ImGuiCopyWidget(const std::string& p_Id) {
    ImGui::SameLine(0, 10.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.5, 0.5 });
    ImGui::SetWindowFontScale(0.6);

    const auto s_Result = ImGui::ButtonEx(
        (std::string(ICON_MD_CONTENT_COPY) + "##" + p_Id).c_str(),
        { m_CopyWidgetButtonSize, m_CopyWidgetButtonSize },
        ImGuiButtonFlags_AlignTextBaseLine
    );

    ImGui::SetWindowFontScale(1.0);
    ImGui::PopStyleVar(2);

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Copy to clipboard");
    }

    return s_Result;
}

void Editor::OnDrawUI(bool p_HasFocus) {
    auto s_ImgGuiIO = ImGui::GetIO();
    DrawSettings(p_HasFocus);

    if (m_EditorWindowsVisible) {
        DrawEntityTree();
        DrawEntityProperties();
        DrawEntityManipulator(p_HasFocus);
        //DrawPinTracer();
        //DrawLibrary();
    }

    DrawItems(p_HasFocus);
    DrawActors(p_HasFocus);
    DrawDebugChannels(p_HasFocus);
    DrawRooms(p_HasFocus);

    if (m_EditorCameraRT && m_EditorCamera) {
        ImGui::Begin("RT Texture");

        const auto s_RT = reinterpret_cast<ZRenderDestination*>(m_EditorCameraRT.m_pInterfaceRef->
            GetRenderDestination());

        m_EditorCameraRT.m_ref.SetProperty("m_bVisible", true);
        m_EditorCamera.m_ref.SetProperty("m_bVisible", true);

        if (s_RT)
            SDK()->ImGuiGameRenderTarget(s_RT);

        ImGui::End();
    }
}

void Editor::DrawSettings(const bool p_HasFocus) {
    if (!m_SettingsVisible || !p_HasFocus) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    ImGui::PushFont(SDK()->GetImGuiRegularFont());
    if (ImGui::Begin(ICON_MD_VIDEO_SETTINGS " EDITOR", &m_SettingsVisible)) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y), ImGuiCond_FirstUseEver);

        if (ImGui::Checkbox(ICON_MD_VIDEO_SETTINGS "  SHOW EDITOR WINDOWS", &m_EditorWindowsVisible)) {
            SetSettingBool("general", "editor_windows_visible", m_EditorWindowsVisible);
        }
        bool s_ServerEnabled = m_Server.GetEnabled();
        if (ImGui::Checkbox(ICON_MD_TERMINAL "  ENABLE EDITOR SERVER", &s_ServerEnabled)) {
            ToggleEditorServerEnabled();
        }

        ImGui::Spacing();
        ImGui::Text("Entity Highlight Mode");

        const int s_EntityHighlightMode = static_cast<int>(m_EntityHighlightMode);

        if (ImGui::RadioButton("Lines", s_EntityHighlightMode == 0)) {
            m_EntityHighlightMode = EntityHighlightMode::Lines;
        }

        ImGui::SameLine();

        if (ImGui::RadioButton("Lines and Rectangles", s_EntityHighlightMode == 1)) {
            m_EntityHighlightMode = EntityHighlightMode::LinesAndTriangles;
        }

        ImGui::Checkbox("Raycast logging", &m_raycastLogging);
    }
    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

void Editor::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    ProcessTasks();
    if (m_TrackCamActive) {
        if (!*Globals::ApplicationEngineWin32)
            return;

        if (!(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01.m_pInterfaceRef) {
            Logger::Debug("Creating free camera.");
            Functions::ZEngineAppCommon_CreateFreeCameraAndControl->Call(
                &(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon
            );
        }

        (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControl01.m_pInterfaceRef->SetActive(
            m_TrackCamActive
        );

        UpdateTrackCam();
    }

    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!m_EditorData && (*Globals::ApplicationEngineWin32)->m_bSceneLoaded && s_Scene) {
        //SpawnCameras();
    }

    std::lock_guard lock(m_EntityDestructionMutex);

    for (const auto& [s_Entity, s_ClientId] : m_EntitiesToDestroy) {
        DestroyEntityInternal(s_Entity, s_ClientId);
    }

    m_EntitiesToDestroy.clear();

    if (m_CachedEntityTree && !m_IsBuildingEntityTree.load()) {
        std::vector<ZEntityRef> s_EntitiesToAdd;
        
        {
            std::scoped_lock s_ScopedLock(m_PendingDynamicEntitiesMutex);

            if (!m_PendingDynamicEntities.empty()) {
                s_EntitiesToAdd.reserve(m_PendingDynamicEntities.size());

                for (const auto& ref : m_PendingDynamicEntities) {
                    s_EntitiesToAdd.push_back(ref);
                }

                m_PendingDynamicEntities.clear();
            }
        }

        if (!s_EntitiesToAdd.empty()) {
            std::scoped_lock s_ScopedLock(m_CachedEntityTreeMutex);

            UpdateEntityTree(m_CachedEntityTreeMap, s_EntitiesToAdd, true);

            if (m_ReparentDynamicOutfitEntities) {
                ReparentDynamicOutfitEntities(m_CachedEntityTreeMap);
            }

            {
                std::scoped_lock s_ScopedLock(m_DebugEntitiesMutex);

                if (!m_EntityRefToDebugEntities.empty()) {
                    for (const auto& s_EntityToAdd : s_EntitiesToAdd) {
                        auto s_Iterator = m_CachedEntityTreeMap.find(s_EntityToAdd);

                        if (s_Iterator != m_CachedEntityTreeMap.end()) {
                            GetDebugEntities(s_Iterator->second);
                        }
                    }
                }
            }
        }
    }

    if (m_RemoveItemFromInventory) {
        if (!m_ItemToRemove) {
            m_RemoveItemFromInventory = false;
            return;
        }

        const bool s_IsMainWeapon = m_SelectedActor->m_pInventoryHandler->m_rMainWeapon == m_ItemToRemove;

        m_SelectedActor->ReleaseItem(m_ItemToRemove, false);

        Functions::ZWorldInventory_DestroyItem->Call(
            Globals::WorldInventory,
            TEntityRef<IItemBase>(m_ItemToRemove.m_ref)
        );

        if (s_IsMainWeapon && m_SelectedActor->m_pInventoryHandler->m_aInventory.size() > 0) {
            m_SelectedActor->m_pInventoryHandler->m_rMainWeapon = TEntityRef<ZHM5ItemWeapon>(
                m_SelectedActor->m_pInventoryHandler->m_aInventory[0].m_ref
            );
        }

        m_ItemToRemove = {};
        m_RemoveItemFromInventory = false;
    }
}

void Editor::ProcessTasks() {
    std::vector<std::function<void()>> tasksToRun;

    {
        std::lock_guard lock(m_TaskMutex);
        if (m_TaskQueue.empty()) {
            return;
        }
        tasksToRun.swap(m_TaskQueue);
    }

    for (const auto& task : tasksToRun) {
        task();
    }
}

void Editor::QueueTask(std::function<void()> p_Task) {
    std::lock_guard lock(m_TaskMutex);
    m_TaskQueue.push_back(std::move(p_Task));
}

void Editor::OnMouseDown(SVector2 p_Pos, bool p_FirstClick) {
    SVector3 s_World;
    SVector3 s_Direction;
    SDK()->ScreenToWorld(p_Pos, s_World, s_Direction);

    if (m_DrawGizmos && RayCastGizmos(s_World, s_Direction)) {
        return;
    }

    float4 s_DirectionVec(s_Direction.x, s_Direction.y, s_Direction.z, 1.f);

    float4 s_From = float4(s_World.x, s_World.y, s_World.z, 1.f);
    float4 s_To = s_From + (s_DirectionVec * 200.f);

    if (!*Globals::CollisionManager) {
        Logger::Error("Collision manager not found.");
        return;
    }

    ZRayQueryInput s_RayInput {
        .m_vFrom = s_From,
        .m_vTo = s_To,
    };

    ZRayQueryOutput s_RayOutput {};

    if (m_raycastLogging) {
        Logger::Debug("RayCasting from {} to {}.", s_From, s_To);
    }

    if (!(*Globals::CollisionManager)->RayCastClosestHit(s_RayInput, &s_RayOutput)) {
        if (m_raycastLogging) {
            Logger::Error("Raycast failed.");
        }
        return;
    }

    if (m_raycastLogging) {
        Logger::Debug("Raycast result: {} {}", fmt::ptr(&s_RayOutput), s_RayOutput.m_vPosition);
    }

    m_From = s_From;
    m_To = s_To;
    m_Hit = s_RayOutput.m_vPosition;
    m_Normal = s_RayOutput.m_vNormal;

    if (p_FirstClick) {
        if (s_RayOutput.m_pBlockingSpatialEntity.m_pInterfaceRef) {
            const auto& s_Interfaces = *s_RayOutput.m_pBlockingSpatialEntity.m_pInterfaceRef->GetType()->m_pInterfaces;
            Logger::Trace(
                "Hit entity of type '{}' with id '{:x}'.",
                s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName,
                s_RayOutput.m_pBlockingSpatialEntity.m_ref->GetType()->m_nEntityId
            );

            const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;
            ZEntityRef s_SelectedEntity = s_RayOutput.m_pBlockingSpatialEntity.m_ref;

            for (int i = 0; i < s_SceneCtx->m_aLoadedBricks.size(); ++i) {
                const auto& s_Brick = s_SceneCtx->m_aLoadedBricks[i];

                if (s_SelectedEntity.IsAnyParent(s_Brick.m_EntityRef)) {
                    Logger::Debug("Found entity in brick {} (idx = {}).", s_Brick.m_RuntimeResourceID, i);
                    m_SelectedBrickIndex = i;
                    break;
                }
            }

            OnSelectEntity(s_SelectedEntity, true, std::nullopt);
        }
    }
}

void Editor::SpawnCameras() {
    static const std::string s_EditorDataJson =
            R"(
{
    "tempHash": "00644fe9eb9feff5",
    "tbluHash": "005474211f99b411",
    "rootEntity": "fffffffffffffffe",
    "entities": {
        "fffffffffffffffe": {
            "parent": null,
            "name": "editor_data",
            "factory": "[modules:/zspatialentity.class].pc_entitytype",
            "blueprint": "[modules:/zspatialentity.class].pc_entityblueprint"
        },
        "feed678791f1b3e1": {
            "parent": "fffffffffffffffe",
            "name": "Tablet_A",
            "factory": "[assembly:/_pro/environment/templates/props/accessories/tablet_a.template?/tablet_a.entitytemplate].pc_entitytype",
            "blueprint": "[assembly:/_pro/environment/templates/props/accessories/tablet_a.template?/tablet_a.entitytemplate].pc_entityblueprint",
            "properties": {
                "m_mTransform": {
                    "type": "SMatrix43",
                    "value": {
                        "rotation": {
                            "x": -87.4014365441793,
                            "y": 0.0000017075472925031877,
                            "z": 91.0032070293913
                        },
                        "position": {
                            "x": -40.105434,
                            "y": -29.001667,
                            "z": -999.3575625
                        }
                    }
                },
                "Texture2D_04_dest": {
                    "type": "SEntityTemplateReference",
                    "value": "feedbf5a41eb9c48"
                },
                "m_eRoomBehaviour": {
                    "type": "ZSpatialEntity.ERoomBehaviour",
                    "value": "ROOM_DYNAMIC"
                },
                "m_eidParent": {
                    "type": "SEntityTemplateReference",
                    "value": "fffffffffffffffe",
                    "postInit": true
                }
            }
        },
        "feedbf5a41eb9c48": {
            "parent": "fffffffffffffffe",
            "name": "RenderDestinationTexture",
            "factory": "[modules:/zrenderdestinationtextureentity.class].pc_entitytype",
            "blueprint": "[modules:/zrenderdestinationtextureentity.class].pc_entityblueprint",
            "properties": {
                "m_aMultiSource": {
                    "type": "TArray<SEntityTemplateReference>",
                    "value": [
                        "feedb6fc4f5626ea"
                    ]
                },
                "m_nWidth": {
                    "type": "uint32",
                    "value": 1280
                },
                "m_nHeight": {
                    "type": "uint32",
                    "value": 720
                },
                "m_bUseBGRA": {
                    "type": "bool",
                    "value": true
                },
                "m_bIsPIP": {
                    "type": "bool",
                    "value": false
                },
                "m_bDrawGates": {
                    "type": "bool",
                    "value": true
                },
                "m_nGateTraversalDepth": {
                    "type": "int32",
                    "value": 10000
                },
                "m_bForceVisible": {
                    "type": "bool",
                    "value": true
                },
                "m_eRoomBehaviour": {
                    "type": "ZSpatialEntity.ERoomBehaviour",
                    "value": "ROOM_DYNAMIC"
                }
            }
        },
        "feedb6fc4f5626ea": {
            "parent": "fffffffffffffffe",
            "name": "Camera",
            "factory": "[modules:/zcameraentity.class].pc_entitytype",
            "blueprint": "[modules:/zcameraentity.class].pc_entityblueprint",
            "properties": {
                "m_bAllowAutoCameraCuts": {
                    "type": "bool",
                    "value": false
                },
                "m_fNearZ": {
                    "type": "float32",
                    "value": 0.5
                },
                "m_fAspectWByH": {
                    "type": "float32",
                    "value": 1
                },
                "m_fFovYDeg": {
                    "type": "float32",
                    "value": 35
                },
                "m_fFarZ": {
                    "type": "float32",
                    "value": 250
                },
                "m_bIsUICamera": {
                    "type": "bool",
                    "value": true
                },
                "m_mTransform": {
                    "type": "SMatrix43",
                    "value": {
                        "rotation": {
                            "x": -73.77378164046733,
                            "y": 1.7075472925031877e-06,
                            "z": 90.61792971868923
                        },
                        "position": {
                            "x": -43.491676,
                            "y": -28.87086,
                            "z": 3.2203503
                        }
                    }
                },
                "m_bForceVisible": {
                    "type": "bool",
                    "value": true
                },
                "m_nPIPPriority": {
                    "type": "uint32",
                    "value": 0
                },
                "m_eRoomBehaviour": {
                    "type": "ZSpatialEntity.ERoomBehaviour",
                    "value": "ROOM_DYNAMIC"
                }
            }
        }
    },
    "propertyOverrides": [],
    "overrideDeletes": [],
    "pinConnectionOverrides": [],
    "pinConnectionOverrideDeletes": [],
    "externalScenes": [],
    "subType": "brick",
    "quickEntityVersion": 3.1,
    "extraFactoryDependencies": [],
    "extraBlueprintDependencies": [],
    "comments": []
}
)";

    Logger::Debug("Spawning editor data entity.");

    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    TResourcePtr<ZTemplateEntityFactory> s_CameraRTFactory;
    TResourcePtr<ZTemplateEntityBlueprintFactory> s_CameraRTBpFactory;

    if (!SDK()->LoadQnEntity(s_EditorDataJson, s_CameraRTBpFactory, s_CameraRTFactory)) {
        Logger::Error("Failed to load editor data entity.");
        return;
    }

    SExternalReferences s_ExternalRefs;
    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager,
        m_EditorData,
        "SDKCam",
        s_CameraRTFactory,
        s_Scene.m_ref,
        s_ExternalRefs,
        -1
    );

    Logger::Debug("Spawned editor data entity!");

    if (const auto idx = s_CameraRTBpFactory.GetResource()->GetSubEntityIndex(0xfeedb6fc4f5626ea); idx != -1) {
        if (const auto s_Ent = s_CameraRTBpFactory.GetResource()->GetSubEntity(m_EditorData.m_pEntity, idx)) {
            m_EditorCamera = TEntityRef<ZCameraEntity>(s_Ent);
        }
    }

    if (const auto idx = s_CameraRTBpFactory.GetResource()->GetSubEntityIndex(0xfeedbf5a41eb9c48); idx != -1) {
        if (const auto s_Ent = s_CameraRTBpFactory.GetResource()->GetSubEntity(m_EditorData.m_pEntity, idx)) {
            m_EditorCameraRT = TEntityRef<ZRenderDestinationTextureEntity>(s_Ent);
        }
    }

    if (!m_EditorCamera || !m_EditorCameraRT) {
        Logger::Error("Failed to get editor camera or render destination texture entity.");
        return;
    }

    // If we have a current camera, move the editor camera to its position.
    if (const auto s_CurrentCamera = Functions::GetCurrentCamera->Call()) {
        m_EditorCamera.m_pInterfaceRef->SetWorldMatrix(s_CurrentCamera->GetWorldMatrix());
    }
}

void Editor::ActivateCamera(ZEntityRef* m_CameraEntity) {
    TEntityRef<IRenderDestinationEntity> s_RenderDest;
    Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

    m_OriginalCam = *s_RenderDest.m_pInterfaceRef->GetSource();

    s_RenderDest.m_pInterfaceRef->SetSource(m_CameraEntity);
}

void Editor::DeactivateCamera() {
    TEntityRef<IRenderDestinationEntity> s_RenderDest;
    Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

    s_RenderDest.m_pInterfaceRef->SetSource(&m_OriginalCam);
}

QneTransform Editor::MatrixToQneTransform(const SMatrix& p_Matrix) {
    // This is adapted from QN: https://github.com/atampy25/quickentity-rs/blob/240ffba9d23dedc864bd39f1f029646837d3916d/src/lib.rs#L2528
    auto s_Trans = p_Matrix;

    constexpr float c_RAD2DEG = 180.0f / std::numbers::pi;

    const auto n11 = s_Trans.XAxis.x;
    const auto n12 = s_Trans.XAxis.y;
    const auto n13 = s_Trans.XAxis.z;
    const auto n14 = 0.0f;
    const auto n21 = s_Trans.YAxis.x;
    const auto n22 = s_Trans.YAxis.y;
    const auto n23 = s_Trans.YAxis.z;
    const auto n24 = 0.0f;
    const auto n31 = s_Trans.ZAxis.x;
    const auto n32 = s_Trans.ZAxis.y;
    const auto n33 = s_Trans.ZAxis.z;
    const auto n34 = 0.0f;
    const auto n41 = s_Trans.Trans.x;
    const auto n42 = s_Trans.Trans.y;
    const auto n43 = s_Trans.Trans.z;
    const auto n44 = 1.0f;

    const auto det =
            n41 * (n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34
                   - n12 * n23 * n34) + n42
            * (n11 * n23 * n34 - n11 * n24 * n33 + n14 * n21 * n33 - n13 * n21 * n34 + n13 * n24 * n31
               - n14 * n23 * n31) + n43
            * (n11 * n24 * n32 - n11 * n22 * n34 - n14 * n21 * n32 + n12 * n21 * n34 + n14 * n22 * n31
               - n12 * n24 * n31) + n44
            * (-n13 * n22 * n31 - n11 * n23 * n32 + n11 * n22 * n33 + n13 * n21 * n32 - n12 * n21 * n33
               + n12 * n23 * n31);

    auto sx = n11 * n11 + n21 * n21 + n31 * n31;
    const auto sy = n12 * n12 + n22 * n22 + n32 * n32;
    const auto sz = n13 * n13 + n23 * n23 + n33 * n33;

    if (det < 0.0f) {
        sx = -sx;
    }

    const auto inv_sx = 1.0f / sx;
    const auto inv_sy = 1.0f / sy;
    const auto inv_sz = 1.0f / sz;

    s_Trans.XAxis.x *= inv_sx;
    s_Trans.YAxis.x *= inv_sx;
    s_Trans.ZAxis.x *= inv_sx;
    s_Trans.XAxis.y *= inv_sy;
    s_Trans.YAxis.y *= inv_sy;
    s_Trans.ZAxis.y *= inv_sy;
    s_Trans.XAxis.z *= inv_sz;
    s_Trans.YAxis.z *= inv_sz;
    s_Trans.ZAxis.z *= inv_sz;

    float s_RotationX = abs(s_Trans.XAxis.z) < 0.9999999f
                            ? atan2f(-s_Trans.YAxis.z, s_Trans.ZAxis.z) * c_RAD2DEG
                            : atan2f(s_Trans.ZAxis.y, s_Trans.YAxis.y) * c_RAD2DEG;

    float s_RotationY = asinf(min(max(-1.f, s_Trans.XAxis.z), 1.f)) * c_RAD2DEG;

    float s_RotationZ = abs(s_Trans.XAxis.z) < 0.9999999f
                            ? atan2f(-s_Trans.XAxis.y, s_Trans.XAxis.x) * c_RAD2DEG
                            : 0.f;

    return QneTransform {
        .Position = { n41, n42, n43 },
        .Rotation = { s_RotationX, s_RotationY, s_RotationZ },
        .Scale = { sx, sy, sz },
    };
}

SMatrix Editor::QneTransformToMatrix(const QneTransform& p_Transform) {
    // This is adapted from QN: https://github.com/atampy25/quickentity-rs/blob/240ffba9d23dedc864bd39f1f029646837d3916d/src/lib.rs#L2782
    constexpr float c_DEG2RAD = std::numbers::pi / 180.0f;

    const auto x = p_Transform.Rotation.x * c_DEG2RAD;
    const auto y = p_Transform.Rotation.y * c_DEG2RAD;
    const auto z = p_Transform.Rotation.z * c_DEG2RAD;

    const auto c1 = cosf(x / 2.0f);
    const auto c2 = cosf(y / 2.0f);
    const auto c3 = cosf(z / 2.0f);

    const auto s1 = sinf(x / 2.0f);
    const auto s2 = sinf(y / 2.0f);
    const auto s3 = sinf(z / 2.0f);

    const auto quat_x = s1 * c2 * c3 + c1 * s2 * s3;
    const auto quat_y = c1 * s2 * c3 - s1 * c2 * s3;
    const auto quat_z = c1 * c2 * s3 + s1 * s2 * c3;
    const auto quat_w = c1 * c2 * c3 - s1 * s2 * s3;

    const auto x2 = quat_x + quat_x;
    const auto y2 = quat_y + quat_y;
    const auto z2 = quat_z + quat_z;
    const auto xx = quat_x * x2;
    const auto xy = quat_x * y2;
    const auto xz = quat_x * z2;
    const auto yy = quat_y * y2;
    const auto yz = quat_y * z2;
    const auto zz = quat_z * z2;
    const auto wx = quat_w * x2;
    const auto wy = quat_w * y2;
    const auto wz = quat_w * z2;

    SMatrix s_Matrix;

    s_Matrix.XAxis.x = (1.0f - (yy + zz)) * p_Transform.Scale.x;
    s_Matrix.XAxis.y = (xy - wz) * p_Transform.Scale.y;
    s_Matrix.XAxis.z = (xz + wy) * p_Transform.Scale.z;

    s_Matrix.YAxis.x = (xy + wz) * p_Transform.Scale.x;
    s_Matrix.YAxis.y = (1.0f - (xx + zz)) * p_Transform.Scale.y;
    s_Matrix.YAxis.z = (yz - wx) * p_Transform.Scale.z;

    s_Matrix.ZAxis.x = (xz - wy) * p_Transform.Scale.x;
    s_Matrix.ZAxis.y = (yz + wx) * p_Transform.Scale.y;
    s_Matrix.ZAxis.z = (1.0f - (xx + yy)) * p_Transform.Scale.z;

    s_Matrix.Trans.x = p_Transform.Position.x;
    s_Matrix.Trans.y = p_Transform.Position.y;
    s_Matrix.Trans.z = p_Transform.Position.z;

    return s_Matrix;
}

DEFINE_PLUGIN_DETOUR(Editor, bool, OnLoadScene, ZEntitySceneContext* th, SSceneInitParameters& p_Parameters) {
    if (m_SelectionForFreeCameraEditorStyleEntity) {
        m_SelectionForFreeCameraEditorStyleEntity->m_selection.clear();
    }

    m_EntityDestructionMutex.lock();
    m_EntitiesToDestroy.clear();
    m_EntityDestructionMutex.unlock();

    m_CachedEntityTreeMutex.lock();
    m_CachedEntityTree.reset();

    for (auto& s_Entity: m_SpawnedEntities | std::views::values) {
        Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, s_Entity, {});
    }

    m_SpawnedEntities.clear();
    m_EntityNames.clear();
    m_CachedEntityTreeMutex.unlock();

    m_FilteredEntityTreeNodes.clear();
    m_DirectEntityTreeNodeMatches.clear();

    if (m_EditorData) {
        m_EditorCamera = {};
        m_EditorCameraRT = {};
        Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, m_EditorData, {});
        m_EditorData = {};
    }

    std::vector<std::string> s_Bricks;

    for (auto& s_Brick: p_Parameters.m_aAdditionalBrickResources) {
        s_Bricks.push_back(s_Brick.c_str());
    }

    m_Server.OnSceneLoading(p_Parameters.m_SceneResource.c_str(), s_Bricks);

    if (m_TrackCamActive) {
        DisableTrackCam();

        m_TrackCamActive = false;
    }

    return { HookAction::Continue() };
}

DEFINE_PLUGIN_DETOUR(
    Editor,
    ZTemplateEntityBlueprintFactory*,
    ZTemplateEntityBlueprintFactory_ctor,
    ZTemplateEntityBlueprintFactory* th,
    STemplateEntityBlueprint* pTemplateEntityBlueprint,
    ZResourcePending& ResourcePending
) {
    //Logger::Debug("Creating Blueprint Factory {} with template {}", fmt::ptr(th), fmt::ptr(pTemplateEntityBlueprint));
    //auto s_Result = p_Hook->CallOriginal(th, pTemplateEntityBlueprint, ResourcePending);
    //return HookResult(HookAction::Return(), s_Result);
    return { HookAction::Continue() };
}

DEFINE_PLUGIN_DETOUR(Editor, void, OnClearScene, ZEntitySceneContext* th, bool p_FullyUnloadScene) {
    m_SelectedBrickIndex = 0;
    m_SelectedEntity = {};
    m_ScrollToEntity = false;

    m_EntityDestructionMutex.lock();
    m_EntitiesToDestroy.clear();
    m_EntityDestructionMutex.unlock();

    m_CachedEntityTreeMutex.lock();
    m_CachedEntityTree.reset();

    for (auto& s_Entity: m_SpawnedEntities | std::views::values) {
        Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, s_Entity, {});
    }

    m_SpawnedEntities.clear();
    m_EntityNames.clear();
    m_CachedEntityTreeMutex.unlock();

    m_PendingDynamicEntities.clear();
    m_DynamicEntities.clear();

    m_Server.OnSceneClearing(p_FullyUnloadScene);

    if (m_TrackCamActive) {
        DisableTrackCam();

        m_TrackCamActive = false;
    }

    m_SelectedActor = nullptr;
    m_GlobalOutfitKit = {};

    m_SelectedGizmoEntity = nullptr;

    m_EntityRefToDebugEntities.clear();

    if (m_EditorData) {
        m_EditorCamera = {};
        m_EditorCameraRT = {};
        Functions::ZEntityManager_DeleteEntity->Call(Globals::EntityManager, m_EditorData, {});
        m_EditorData = {};
    }

    m_EntityRefToFactoryRuntimeResourceIDs.clear();

    m_SortedRoomEntities.clear();

    return { HookAction::Continue() };
}

DEFINE_PLUGIN_DETOUR(Editor, bool, OnInputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data) {
    //if (entity == m_SelectedEntity)
    {
        m_FiredInputPins[pinId] = PinFireInfo {
            .m_FireTime = std::chrono::system_clock::now(),
        };
    }

    return { HookAction::Continue() };
}

DEFINE_PLUGIN_DETOUR(Editor, bool, OnOutputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data) {
    //if (entity == m_SelectedEntity)
    {
        m_FiredOutputPins[pinId] = PinFireInfo {
            .m_FireTime = std::chrono::system_clock::now(),
        };
    }

    return { HookAction::Continue() };
}

DEFINE_PLUGIN_DETOUR(
    Editor,
    void,
    ZTemplateEntityFactory_ConfigureEntity,
    ZTemplateEntityFactory* th,
    ZEntityType** pEntity,
    const SExternalReferences& externalRefs,
    uint8_t* unk0
) {
    IEntityBlueprintFactory* s_EntityBlueprintFactory = static_cast<IEntityBlueprintFactory*>(th->m_blueprintResource.
        GetResourceData());

    for (size_t i = 0; i < s_EntityBlueprintFactory->GetSubEntitiesCount(); ++i) {
        const ZEntityRef s_SubEntity = s_EntityBlueprintFactory->GetSubEntity(pEntity, i);
        IEntityFactory* s_SubEntityFactory = th->m_pFactories[i];
        ZRuntimeResourceID s_SubEntityFactoryRuntimeResourceID;

        if (s_SubEntityFactory->IsTemplateEntityFactory()) {
            s_SubEntityFactoryRuntimeResourceID = static_cast<ZTemplateEntityFactory*>(s_SubEntityFactory)->
                    m_ridResource;
        } else if (s_SubEntityFactory->IsAspectEntityFactory()) {
            s_SubEntityFactoryRuntimeResourceID = static_cast<ZAspectEntityFactory*>(s_SubEntityFactory)->m_ridResource;
        } else if (s_SubEntityFactory->IsCppEntityFactory()) {
            s_SubEntityFactoryRuntimeResourceID = static_cast<ZCppEntityFactory*>(s_SubEntityFactory)->m_ridResource;
        } else if (s_SubEntityFactory->IsExtendedCppEntityFactory()) {
            ZExtendedCppEntityFactory* s_ExtendedCppEntityFactory = static_cast<ZExtendedCppEntityFactory*>(
                s_SubEntityFactory);

            std::scoped_lock s_Lock(m_ExtendedCppEntityFactoryResourceMapsMutex);
            auto s_Iterator = m_ExtendedCppEntityFactoryToRuntimeResourceID.find(s_ExtendedCppEntityFactory);

            if (s_Iterator != m_ExtendedCppEntityFactoryToRuntimeResourceID.end()) {
                s_SubEntityFactoryRuntimeResourceID = s_Iterator->second;
            }
        } else if (s_SubEntityFactory->IsUIControlEntityFactory()) {
            s_SubEntityFactoryRuntimeResourceID = static_cast<ZUIControlEntityFactory*>(s_SubEntityFactory)->
                    m_ridResource;
        } else if (s_SubEntityFactory->IsRenderMaterialEntityFactory()) {
            s_SubEntityFactoryRuntimeResourceID = static_cast<ZRenderMaterialEntityFactory*>(s_SubEntityFactory)->
                    m_ridResource;
        } else if (s_SubEntityFactory->IsBehaviorTreeEntityFactory()) {
            s_SubEntityFactoryRuntimeResourceID = static_cast<ZBehaviorTreeEntityFactory*>(s_SubEntityFactory)->
                    m_ridResource;
        } else if (s_SubEntityFactory->IsAudioSwitchEntityFactory()) {
            s_SubEntityFactoryRuntimeResourceID = static_cast<ZAudioSwitchEntityFactory*>(s_SubEntityFactory)->
                    m_ridResource;
        } else if (s_SubEntityFactory->IsAudioStateEntityFactory()) {
            s_SubEntityFactoryRuntimeResourceID = static_cast<ZAudioStateEntityFactory*>(s_SubEntityFactory)->
                    m_ridResource;
        } {
            std::unique_lock s_Lock(m_EntityRefToFactoryRuntimeResourceIDsMutex);

            m_EntityRefToFactoryRuntimeResourceIDs[s_SubEntity] = {
                s_SubEntityFactoryRuntimeResourceID, th->m_ridResource
            };
        }
    }

    p_Hook->CallOriginal(th, pEntity, externalRefs, unk0);

    return HookResult<void>(HookAction::Return());
}

DEFINE_PLUGIN_DETOUR(
    Editor,
    bool,
    ZExtendedCppEntityTypeInstaller_Install,
    ZExtendedCppEntityTypeInstaller* th,
    ZResourcePending& ResourcePending
) {
    bool s_Result = p_Hook->CallOriginal(th, ResourcePending);

    ZExtendedCppEntityFactory* s_ExtendedCppEntityFactory =
            static_cast<ZExtendedCppEntityFactory*>(ResourcePending.m_pResource.GetResourceData());
    const ZRuntimeResourceID s_RuntimeResourceID = ResourcePending.m_pResource.GetResourceInfo().rid; {
        std::scoped_lock s_Lock(m_ExtendedCppEntityFactoryResourceMapsMutex);

        m_ExtendedCppEntityFactoryToRuntimeResourceID[s_ExtendedCppEntityFactory] = s_RuntimeResourceID;
        m_RuntimeResourceIDToExtendedCppEntityFactory[s_RuntimeResourceID] = s_ExtendedCppEntityFactory;
    }

    return HookResult<bool>(HookAction::Return(), s_Result);
}

DEFINE_PLUGIN_DETOUR(
    Editor,
    void,
    ZResourceManager_UninstallResource,
    ZResourceManager* th,
    ZResourceIndex index
) {
    if (m_RuntimeResourceIDToExtendedCppEntityFactory.empty()) {
        return HookResult<void>(HookAction::Continue());
    }

    const auto& s_ResourceInfo = (*Globals::ResourceContainer)->m_resources[index.val];
    const ZRuntimeResourceID s_RuntimeResourceID = s_ResourceInfo.rid;
    
    {
        std::scoped_lock s_Lock(m_ExtendedCppEntityFactoryResourceMapsMutex);
        auto s_Iterator = m_RuntimeResourceIDToExtendedCppEntityFactory.find(s_RuntimeResourceID);

        if (s_Iterator != m_RuntimeResourceIDToExtendedCppEntityFactory.end()) {
            ZExtendedCppEntityFactory* s_ExtendedCppEntityFactory = s_Iterator->second;

            m_ExtendedCppEntityFactoryToRuntimeResourceID.erase(s_ExtendedCppEntityFactory);
            m_RuntimeResourceIDToExtendedCppEntityFactory.erase(s_Iterator);
        }
    }

    return HookResult<void>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(Editor);
