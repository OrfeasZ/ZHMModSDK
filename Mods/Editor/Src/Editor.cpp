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

#include "backends/imgui_impl_dx12.h"
#include "Glacier/SGameUpdateEvent.h"
#include "Glacier/ZCollision.h"
#include "Glacier/ZActor.h"
#include "Glacier/ZGameLoopManager.h"
#include "Glacier/ZKnowledge.h"

#include "../qnc/bindings.hpp"

#include <ResourceLib_HM3.h>

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
        this, &Editor::ZTemplateEntityBlueprintFactory_ctor
    );
    Hooks::SignalInputPin->AddDetour(this, &Editor::OnInputPin);
    Hooks::SignalOutputPin->AddDetour(this, &Editor::OnOutputPin);

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
        m_MenuVisible = !m_MenuVisible;
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

    if (ImGui::Button(ICON_MD_TUNE " ASSETS MENU")) {
        m_AssetsMenuActive = !m_AssetsMenuActive;
    }

    if (ImGui::Button(ICON_MD_TUNE " ITEMS MENU")) {
        m_ItemsMenuActive = !m_ItemsMenuActive;
    }

    if (ImGui::Button(ICON_MD_PEOPLE " ACTORS MENU")) {
        m_ActorsMenuActive = !m_ActorsMenuActive;
    }
}

void Editor::ToggleEditorServerEnabled() {
    m_Server.SetEnabled(!m_Server.GetEnabled());
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

void Editor::OnEngineInitialized() {
    const ZMemberDelegate<Editor, void(const SGameUpdateEvent&)> s_Delegate(this, &Editor::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);
}

bool Editor::ImGuiCopyWidget(const std::string& p_Id) {
    ImGui::SameLine(0, 10.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, {0.5, 0.5});
    ImGui::SetWindowFontScale(0.6);

    const auto s_Result = ImGui::Button((std::string(ICON_MD_CONTENT_COPY) + "##" + p_Id).c_str(), {20, 20});

    ImGui::SetWindowFontScale(1.0);
    ImGui::PopStyleVar(2);

    return s_Result;
}

bool SpawnEntity2(const char* p_Json, ZEntityRef& p_Entity) {
    auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Error("Scene is not yet loaded. Cannot spawn editor cameras.");
        return false;
    }

    const std::string s_TestData =
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
                            "z": 2.3575625
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

    //p_Json = s_TestData.c_str();

    Logger::Debug("Converting QN entity JSON to RT JSON and meta...");

    const auto s_QnData = convert_qn_entity(
        reinterpret_cast<const uint8_t*>(p_Json), strlen(p_Json)
    );

    Logger::Debug("Converted!");

    const std::string_view s_FactoryJson {s_QnData.factory_json, s_QnData.factory_json_len};
    const std::string_view s_FactoryMetaJson {s_QnData.factory_meta_json, s_QnData.factory_meta_json_len};
    const std::string_view s_BlueprintJson {s_QnData.blueprint_json, s_QnData.blueprint_json_len};
    const std::string_view s_BlueprintMetaJson {s_QnData.blueprint_meta_json, s_QnData.blueprint_meta_json_len};

    //free_qn_converted_data(s_QnData);

    Logger::Debug("Generating BIN1 resources from RT JSON...");

    const auto s_ResourceTempMem = HM3_GetGeneratorForResource("TEMP")->FromJsonStringToResourceMem(
        s_FactoryJson.data(),
        s_FactoryJson.size(),
        false
    );

    const auto s_ResourceTbluMem = HM3_GetGeneratorForResource("TBLU")->FromJsonStringToResourceMem(
        s_BlueprintJson.data(),
        s_BlueprintJson.size(),
        false
    );

    Logger::Debug("Generated!");

    auto LoadResource = [](
        ResourceMem* p_ResourceMem, std::string_view p_MetaJson,
        std::function<void(ZResourcePending*)> p_Install
    ) {
        // Parse meta, create resource, and register references.
        simdjson::ondemand::parser s_Parser;
        const auto s_Json = simdjson::padded_string(p_MetaJson);
        simdjson::ondemand::document s_JsonMsg = s_Parser.iterate(s_Json);

        const std::string s_ResIdStr {std::string_view(s_JsonMsg["hash_value"])};
        const auto s_ResId = ZRuntimeResourceID::FromString(s_ResIdStr);

        Logger::Debug("Loading resource {}...", s_ResId);

        // Create a new resource index.
        ZResourceIndex s_Index;
        Functions::ZResourceContainer_AddResourceInternal->Call(*Globals::ResourceContainer, s_Index, s_ResId);

        Logger::Debug("Got resource index {} for rid {}.", s_Index.val, s_ResId);
        Logger::Debug("Collecting references from meta...");

        std::vector<std::pair<ZRuntimeResourceID, SResourceReferenceFlags>> s_References;
        for (auto s_Ref : s_JsonMsg["hash_reference_data"]) {
            const std::string s_Hash {std::string_view(s_Ref["hash"])};
            const std::string s_Flag {std::string_view(s_Ref["flag"])};

            const auto s_RefId = ZRuntimeResourceID::FromString(s_Hash);

            // Parse flag as hex byte.
            const SResourceReferenceFlags s_Flags {
                .flags = static_cast<uint8_t>(std::stoul(s_Flag, nullptr, 16))
            };

            s_References.emplace_back(s_RefId, s_Flags);
        }

        Logger::Debug("Found {} references! Adding...", s_References.size());

        auto& s_ResInfo = (*Globals::ResourceContainer)->m_resources[s_Index.val];
        s_ResInfo.refCount = 69;

        if (!s_References.empty()) {
            s_ResInfo.firstReferenceIndex = (*Globals::ResourceContainer)->m_references.size();
            s_ResInfo.numReferences = s_References.size();
            s_ResInfo.dataSize = p_ResourceMem->DataSize;
            s_ResInfo.compressedDataSize = p_ResourceMem->DataSize;
            s_ResInfo.dataOffset = 0;

            for (const auto& [s_RefId, s_RefFlags] : s_References) {
                Logger::Debug("Adding reference {} -> {} (flags = {:x}).", s_ResId, s_RefId, s_RefFlags.flags);
                Functions::ZResourceContainer_AddResourceReferenceInternal->Call(
                    *Globals::ResourceContainer, s_RefId, s_RefFlags
                );
            }

            Functions::ZResourceContainer_AcquireReferences->Call(*Globals::ResourceContainer, s_Index);

            Logger::Debug(
                "Acquired references!", s_ResInfo.firstReferenceIndex, s_ResInfo.numReferences
            );

            while (!Globals::ResourceManager->DoneLoading()) {
                Logger::Debug("Waiting for references to load (left: {})!", Globals::ResourceManager->m_nNumProcessing);

                for (const auto& s_RefId : s_References | std::views::keys) {
                    ZResourcePtr s_RefRes;
                    Globals::ResourceManager->GetResourcePtr(s_RefRes, s_RefId, 0);

                    if (!s_RefRes) {
                        Logger::Debug("Reference '{}' not loaded!", s_RefId);
                    }
                }

                Globals::ResourceManager->Update(true);
            }

            Logger::Debug("All references loaded!");

            for (const auto& s_RefId : s_References | std::views::keys) {
                ZResourcePtr s_RefRes;
                Globals::ResourceManager->GetResourcePtr(s_RefRes, s_RefId, 0);
                Logger::Debug("Reference '{}' loaded at: {}", s_RefId, fmt::ptr(s_RefRes.GetResourceData()));
            }
        }

        auto* s_Buffer = static_cast<ZResourceDataBuffer*>((*Globals::MemoryManager)->m_pNormalAllocator->
            AllocateAligned(
                sizeof(ZResourceDataBuffer),
                alignof(ZResourceDataBuffer)
            ));

        new(s_Buffer) ZResourceDataBuffer();

        s_Buffer->m_pData = const_cast<void*>(p_ResourceMem->ResourceData);
        s_Buffer->m_nSize = p_ResourceMem->DataSize;
        s_Buffer->m_iRefCount = 69;
        s_Buffer->m_nCapacity = p_ResourceMem->DataSize;
        s_Buffer->m_bOwnsDataPtr = false;

        ZResourceDataPtr s_DataPtr {.m_pObject = s_Buffer};

        auto* s_Reader = static_cast<ZResourceReader*>((*Globals::MemoryManager)->m_pNormalAllocator->
            AllocateAligned(
                sizeof(ZResourceReader),
                alignof(ZResourceReader)
            ));

        Functions::ZResourceReader_ZResourceReader->Call(
            s_Reader,
            &s_Index,
            &s_DataPtr,
            p_ResourceMem->DataSize
        );

        ZResourcePending s_Pending {};
        s_Pending.m_pResource.m_nResourceIndex = s_Index.val;
        s_Pending.m_pResourceReader.m_pObject = s_Reader;

        // Increment m_nNumProcessing by 1 because installing will set the
        // resource status to valid, which will decrement m_nNumProcessing.
        InterlockedIncrement(&Globals::ResourceManager->m_nNumProcessing);
        p_Install(&s_Pending);

        // TODO: Free s_Reader, s_Buffer, etc.

        return std::make_tuple(s_Index, s_ResId);
    };

    if (s_ResourceTbluMem && s_ResourceTempMem) {
        Logger::Info("Creating TBLU resource...");

        auto [s_TbluIndex, s_TbluId] = LoadResource(
            s_ResourceTbluMem, s_BlueprintMetaJson,
            [](ZResourcePending* r) { Functions::ZTemplateBlueprintInstaller_Install->Call(nullptr, r); }
        );

        Logger::Info("TBLU rid = {}, index = {}", s_TbluId, s_TbluIndex.val);

        Logger::Info("Creating TEMP resource...");

        auto [s_TempIndex, s_TempId] = LoadResource(
            s_ResourceTempMem, s_FactoryMetaJson,
            [](ZResourcePending* r) { Functions::ZTemplateInstaller_Install->Call(nullptr, r); }
        );

        Logger::Info("TEMP rid = {}, index = {}", s_TempId, s_TempIndex.val);

        TResourcePtr<ZTemplateEntityFactory> s_RTResource;
        Globals::ResourceManager->GetResourcePtr(s_RTResource, s_TempId, 0);

        if (!s_RTResource) {
            Logger::Error("Could not get editor camera resource.");
            return false;
        }

        Functions::ZEntityManager_NewEntity->Call(
            Globals::EntityManager, p_Entity, "SDKThing", s_RTResource, s_Scene.m_ref, nullptr, -1
        );

        if (!p_Entity) {
            Logger::Error("Could not spawn editor camera texture entity.");
            return false;
        }

        Logger::Info(
            "Spawned entity from rid {} with id {}!", s_RTResource.GetResourceInfo().rid,
            p_Entity->GetType()->m_nEntityId
        );

        return true;
    }

    Logger::Error("Failed to generate editor resources.");

    return false;
}

void Editor::OnDrawUI(bool p_HasFocus) {
    auto s_ImgGuiIO = ImGui::GetIO();

    if (m_MenuVisible) {
        const auto s_Center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(s_Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushFont(SDK()->GetImGuiBlackFont());
        const auto s_MenuExpanded = ImGui::Begin(ICON_MD_VIDEO_SETTINGS "  Editor", &m_MenuVisible);
        ImGui::PushFont(SDK()->GetImGuiRegularFont());

        if (s_MenuExpanded) {
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
        }

        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
    }

    if (m_EditorWindowsVisible) {
        DrawEntityTree();
        DrawEntityProperties();
        DrawEntityManipulator(p_HasFocus);
        //DrawPinTracer();

        DrawAssets(p_HasFocus);
        DrawItems(p_HasFocus);
        DrawActors(p_HasFocus);
        DrawLibrary();
    }

    if (m_CameraRT && m_Camera) {
        ImGui::Begin("RT Texture");

        const auto s_CameraRTEntity = m_CameraRT.QueryInterface<ZRenderDestinationTextureEntity>();
        const auto s_RT = reinterpret_cast<ZRenderDestination*>(s_CameraRTEntity->GetRenderDestination());

        m_CameraRT.SetProperty("m_bVisible", true);
        m_Camera.SetProperty("m_bVisible", true);

        if (s_RT)
            SDK()->ImGuiGameRenderTarget(s_RT);

        ImGui::End();
    }

    ImGui::SetNextWindowPos({750, 110}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({800, 800}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Entity spawner")) {
        ImGui::Text("Paste your QN entity JSON here:");

        static char s_Buffer[10 * 1024 * 1024] = {};

        // Get available content region
        ImVec2 contentRegion = ImGui::GetContentRegionAvail();

        // Reserve space for the button (button height + spacing)
        float buttonHeight = ImGui::GetFrameHeight();
        float spacing = ImGui::GetStyle().ItemSpacing.y;
        float multilineHeight = contentRegion.y - buttonHeight - spacing;

        // Make the multiline input fill the available width and calculated height
        ImGui::InputTextMultiline(
            "##qn_json", s_Buffer, sizeof(s_Buffer),
            ImVec2(contentRegion.x, multilineHeight)
        );

        // Position the button at the bottom left
        if (ImGui::Button("Spawn")) {
            ZEntityRef s_Ent;
            SpawnEntity2(s_Buffer, s_Ent);

            if (s_Ent) {
                m_CachedEntityTreeMutex.lock();

                m_SpawnedEntities.push_back(s_Ent);

                if (m_CachedEntityTree && m_CachedEntityTreeMap.size() > 0) {
                    UpdateEntityTree(m_CachedEntityTreeMap, {s_Ent});
                }

                m_CachedEntityTreeMutex.unlock();
            }
        }
    }
    ImGui::End();

    /*ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Expanded = ImGui::Begin("Behaviors");
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Expanded)
    {
        for (int i = 0; i < *Globals::NextActorId; ++i)
        {
            const auto& s_Actor = Globals::ActorManager->m_aActiveActors[i];

            const auto s_ActorSpatial = s_Actor.m_ref.QueryInterface<ZSpatialEntity>();

            if (!s_ActorSpatial)
                continue;

            std::string s_BehaviorName = "<none>";

            if (s_Actor.m_pInterfaceRef->m_nCurrentBehaviorIndex >= 0)
            {
                auto& s_BehaviorData = Globals::BehaviorService->m_aKnowledgeData[s_Actor.m_pInterfaceRef->m_nCurrentBehaviorIndex];

                if (s_BehaviorData.m_pCurrentBehavior)
                    s_BehaviorName = BehaviorToString(static_cast<ECompiledBehaviorType>(s_BehaviorData.m_pCurrentBehavior->m_Type));
            }

            ImGui::Text(fmt::format("{} => {}", s_Actor.m_pInterfaceRef->m_sActorName, s_BehaviorName).c_str());
        }
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();*/
}

void Editor::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    if (m_TrackCamActive) {
        if (!*Globals::ApplicationEngineWin32)
            return;

        if (!(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01.m_pInterfaceRef) {
            Logger::Debug("Creating free camera.");
            Functions::ZEngineAppCommon_CreateFreeCamera->Call(
                &(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon
            );
        }

        (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControl01.m_pInterfaceRef->SetActive(
            m_TrackCamActive
        );

        UpdateTrackCam();
    }
}

void Editor::OnMouseDown(SVector2 p_Pos, bool p_FirstClick) {
    SVector3 s_World;
    SVector3 s_Direction;
    SDK()->ScreenToWorld(p_Pos, s_World, s_Direction);

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
        if (s_RayOutput.m_BlockingEntity) {
            const auto& s_Interfaces = *s_RayOutput.m_BlockingEntity->GetType()->m_pInterfaces;
            Logger::Trace(
                "Hit entity of type '{}' with id '{:x}'.", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName,
                s_RayOutput.m_BlockingEntity->GetType()->m_nEntityId
            );

            const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;
            ZEntityRef s_SelectedEntity = s_RayOutput.m_BlockingEntity;

            for (int i = 0; i < s_SceneCtx->m_aLoadedBricks.size(); ++i) {
                const auto& s_Brick = s_SceneCtx->m_aLoadedBricks[i];

                if (s_SelectedEntity.IsAnyParent(s_Brick.entityRef)) {
                    Logger::Debug("Found entity in brick {} (idx = {}).", s_Brick.runtimeResourceID, i);
                    m_SelectedBrickIndex = i;
                    break;
                }
            }

            OnSelectEntity(s_SelectedEntity, std::nullopt);
        }
    }
}

void Editor::SpawnCameras() {
    auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Error("Scene is not yet loaded. Cannot spawn editor cameras.");
        return;
    }

    {
        TResourcePtr<ZTemplateEntityFactory> s_CameraResource;
        Globals::ResourceManager->GetResourcePtr(
            s_CameraResource, ResId<"[assembly:/_sdk/editor/editor_camera.brick].pc_entitytype">, 0
        );

        if (!s_CameraResource) {
            Logger::Error("Could not get editor camera resource. Is the editor brick loaded?");
            return;
        }

        Functions::ZEntityManager_NewEntity->Call(
            Globals::EntityManager, m_Camera, "SDKCam", s_CameraResource, s_Scene.m_ref, nullptr, -1
        );

        if (!m_Camera) {
            Logger::Error("Could not spawn editor camera entity.");
            return;
        }
    }

    {
        TResourcePtr<ZTemplateEntityFactory> s_RTResource;
        Globals::ResourceManager->GetResourcePtr(
            s_RTResource, ResId<"[assembly:/_sdk/editor/camera_texture.brick].pc_entitytype">, 0
        );

        if (!s_RTResource) {
            Logger::Error("Could not get editor camera texture resource. Is the editor brick loaded?");
            return;
        }

        Functions::ZEntityManager_NewEntity->Call(
            Globals::EntityManager, m_CameraRT, "SDKCamRT", s_RTResource, s_Scene.m_ref, nullptr, -1
        );

        if (!m_CameraRT) {
            Logger::Error("Could not spawn editor camera texture entity.");
            return;
        }
    }

    const auto s_Camera = m_Camera.QueryInterface<ZCameraEntity>();
    Logger::Debug("Spawned camera = {}", fmt::ptr(s_Camera));

    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();
    s_Camera->SetWorldMatrix(s_CurrentCamera->GetWorldMatrix());

    const auto s_CameraRT = m_CameraRT.QueryInterface<ZRenderDestinationTextureEntity>();
    Logger::Debug(
        "Spawned rt = {} sources = {} source = {}", fmt::ptr(s_CameraRT), s_CameraRT->m_aMultiSource.size(),
        s_CameraRT->m_nSelectedSource
    );

    s_CameraRT->SetSource(&m_Camera);

    Logger::Debug(
        "Added source to rt = {} sources = {} source = {}", fmt::ptr(s_CameraRT), s_CameraRT->m_aMultiSource.size(),
        s_CameraRT->m_nSelectedSource
    );
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
        .Position = {n41, n42, n43},
        .Rotation = {s_RotationX, s_RotationY, s_RotationZ},
        .Scale = {sx, sy, sz},
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

DEFINE_PLUGIN_DETOUR(Editor, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData) {
    if (p_SceneData.m_sceneName == "assembly:/_PRO/Scenes/Frontend/MainMenu.entity" ||
        p_SceneData.m_sceneName == "assembly:/_PRO/Scenes/Frontend/Boot.entity")
        //    p_SceneData.m_sceneName = "assembly:/_pro/scenes/users/notex/test.entity";
        p_SceneData.m_sceneName =
                "assembly:/_PRO/Scenes/Missions/TheFacility/_Scene_Mission_Polarbear_Module_002_B.entity";
    //    p_SceneData.m_sceneName = "assembly:/_pro/scenes/missions/golden/mission_gecko/scene_gecko_basic.entity";


    if (m_SelectionForFreeCameraEditorStyleEntity) {
        m_SelectionForFreeCameraEditorStyleEntity->m_selection.clear();
    }

    m_CachedEntityTreeMutex.lock();
    m_CachedEntityTree.reset();
    m_CachedEntityTreeMutex.unlock();

    m_FilteredEntityTreeNodes.clear();
    m_DirectEntityTreeNodeMatches.clear();

    m_NavpAreas.clear();

    std::vector<std::string> s_Bricks;

    for (auto& s_Brick : p_SceneData.m_sceneBricks) {
        s_Bricks.push_back(s_Brick.c_str());
    }

    m_Server.OnSceneLoading(p_SceneData.m_sceneName.c_str(), s_Bricks);

    if (m_TrackCamActive) {
        DisableTrackCam();

        m_TrackCamActive = false;
    }

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(
    Editor, ZTemplateEntityBlueprintFactory*, ZTemplateEntityBlueprintFactory_ctor,
    ZTemplateEntityBlueprintFactory* th,
    STemplateEntityBlueprint* pTemplateEntityBlueprint, ZResourcePending& ResourcePending
) {
    //Logger::Debug("Creating Blueprint Factory {} with template {}", fmt::ptr(th), fmt::ptr(pTemplateEntityBlueprint));
    //auto s_Result = p_Hook->CallOriginal(th, pTemplateEntityBlueprint, ResourcePending);
    //return HookResult(HookAction::Return(), s_Result);
    return {HookAction::Continue()};
}

DEFINE_PLUGIN_DETOUR(Editor, void, OnClearScene, ZEntitySceneContext* th, bool forReload) {
    m_SelectedBrickIndex = 0;
    m_SelectedEntity = {};
    m_Camera = {};
    m_CameraRT = {};
    m_ShouldScrollToEntity = false;

    m_CachedEntityTreeMutex.lock();
    m_CachedEntityTree.reset();
    m_CachedEntityTreeMutex.unlock();

    m_NavpAreas.clear();

    m_Server.OnSceneClearing(forReload);

    m_RepositoryResource = {};
    m_RepositoryProps.clear();

    if (m_TrackCamActive) {
        DisableTrackCam();

        m_TrackCamActive = false;
    }

    s_CurrentlySelectedActor = nullptr;

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Editor, bool, OnInputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data) {
    //if (entity == m_SelectedEntity)
    {
        m_FiredInputPins[pinId] = PinFireInfo {
            .m_FireTime = std::chrono::system_clock::now(),
        };
    }

    return {HookAction::Continue()};
}

DEFINE_PLUGIN_DETOUR(Editor, bool, OnOutputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data) {
    //if (entity == m_SelectedEntity)
    {
        m_FiredOutputPins[pinId] = PinFireInfo {
            .m_FireTime = std::chrono::system_clock::now(),
        };
    }

    return {HookAction::Continue()};
}

DEFINE_ZHM_PLUGIN(Editor);
