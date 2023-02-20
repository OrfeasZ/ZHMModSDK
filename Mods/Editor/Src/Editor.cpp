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
#include <ranges>

#include "backends/imgui_impl_dx12.h"
#include "Glacier/ZCollision.h"

Editor::Editor()
{
    // Disable ZTemplateEntityBlueprintFactory freeing its associated data.
    uint8_t s_Nop[21] = {};
    memset(s_Nop, 0x90, sizeof(s_Nop));

    if (!SDK()->PatchCode("\x48\x85\xC9\x74\x00\xE8\x00\x00\x00\x00\x49\xC7\x86\xA0\x01\x00\x00", "xxxx?x????xxxxxxx", s_Nop, sizeof(s_Nop)))
    {
        Logger::Error("Could not patch ZTemplateEntityBlueprintFactory data freeing.");
    }

    if (!SDK()->PatchCode("\x48\x85\xC9\x74\x00\xE8\x00\x00\x00\x00\x48\xC7\x83\xA0\x01\x00\x00\x00\x00\x00\x00\x8B\x43\x10", "xxxx?x????xxxxxxx????xxx", s_Nop, sizeof(s_Nop)))
    {
        Logger::Error("Could not patch ZTemplateEntityBlueprintFactory brick data freeing.");
    }
}

void Editor::Init()
{
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &Editor::OnLoadScene);
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &Editor::OnClearScene);
    Hooks::ZTemplateEntityBlueprintFactory_ZTemplateEntityBlueprintFactory->AddDetour(this, &Editor::ZTemplateEntityBlueprintFactory_ctor);
    Hooks::SignalInputPin->AddDetour(this, &Editor::OnInputPin);
    Hooks::SignalOutputPin->AddDetour(this, &Editor::OnOutputPin);
}

void Editor::OnDrawMenu()
{
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
}


void Editor::CopyToClipboard(const std::string& p_String) const
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

void Editor::OnDraw3D(IRenderer* p_Renderer)
{
    DrawEntityAABB(p_Renderer);
}

bool Editor::ImGuiCopyWidget(const std::string& p_Id)
{
    ImGui::SameLine(0, 10.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.5, 0.5 });
    ImGui::SetWindowFontScale(0.6);

    const auto s_Result = ImGui::Button((std::string(ICON_MD_CONTENT_COPY) + "##" + p_Id).c_str(), {20, 20});

    ImGui::SetWindowFontScale(1.0);
    ImGui::PopStyleVar(2);

    return s_Result;
}

void Editor::OnDrawUI(bool p_HasFocus)
{
    auto s_ImgGuiIO = ImGui::GetIO();

    DrawEntityTree();
    DrawEntityProperties();
    DrawEntityManipulator(p_HasFocus);
    DrawPinTracer();

    if (m_CameraRT && m_Camera)
    {
        ImGui::Begin("RT Texture");

        const auto s_CameraRTEntity = m_CameraRT.QueryInterface<ZRenderDestinationTextureEntity>();
        const auto s_RT = reinterpret_cast<ZRenderDestination*>(s_CameraRTEntity->GetRenderDestination());

        m_CameraRT.SetProperty("m_bVisible", true);
        m_Camera.SetProperty("m_bVisible", true);

        if (s_RT)
            SDK()->ImGuiGameRenderTarget(s_RT);

        ImGui::End();
    }
}

void Editor::OnMouseDown(SVector2 p_Pos, bool p_FirstClick)
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
        if (s_RayOutput.m_BlockingEntity)
        {
            const auto& s_Interfaces = *s_RayOutput.m_BlockingEntity->GetType()->m_pInterfaces;
            Logger::Trace("Hit entity of type '{}' with id '{:x}'.", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName, s_RayOutput.m_BlockingEntity->GetType()->m_nEntityId);

            m_SelectedEntity = s_RayOutput.m_BlockingEntity;
            m_ShouldScrollToEntity = true;

            const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;

            for (int i = 0; i < s_SceneCtx->m_aLoadedBricks.size(); ++i)
            {
                const auto& s_Brick = s_SceneCtx->m_aLoadedBricks[i];

                if (m_SelectedEntity.IsAnyParent(s_Brick.entityRef))
                {
                    m_SelectedBrickIndex = i;
                    break;
                }
            }
        }
    }
}

void Editor::SpawnCameras()
{
    auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene)
    {
        Logger::Error("Scene is not yet loaded. Cannot spawn editor cameras.");
        return;
    }

    {
        TResourcePtr<ZTemplateEntityFactory> s_CameraResource;
        Globals::ResourceManager->GetResourcePtr(s_CameraResource, ResId<"[assembly:/_sdk/editor/editor_camera.brick].pc_entitytype">, 0);

        if (!s_CameraResource)
        {
            Logger::Error("Could not get editor camera resource. Is the editor brick loaded?");
            return;
        }

        Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, m_Camera, "SDKCam", s_CameraResource, s_Scene.m_ref, nullptr, -1);

        if (!m_Camera)
        {
            Logger::Error("Could not spawn editor camera entity.");
            return;
        }
    }

    {
        TResourcePtr<ZTemplateEntityFactory> s_RTResource;
        Globals::ResourceManager->GetResourcePtr(s_RTResource, ResId<"[assembly:/_sdk/editor/camera_texture.brick].pc_entitytype">, 0);

        if (!s_RTResource)
        {
            Logger::Error("Could not get editor camera texture resource. Is the editor brick loaded?");
            return;
        }

        Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, m_CameraRT, "SDKCamRT", s_RTResource, s_Scene.m_ref, nullptr, -1);

        if (!m_CameraRT)
        {
            Logger::Error("Could not spawn editor camera texture entity.");
            return;
        }
    }

    const auto s_Camera = m_Camera.QueryInterface<ZCameraEntity>();
    Logger::Debug("Spawned camera = {}", fmt::ptr(s_Camera));

    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();
    s_Camera->SetWorldMatrix(s_CurrentCamera->GetWorldMatrix());

    const auto s_CameraRT = m_CameraRT.QueryInterface<ZRenderDestinationTextureEntity>();
    Logger::Debug("Spawned rt = {} sources = {} source = {}", fmt::ptr(s_CameraRT), s_CameraRT->m_aMultiSource.size(), s_CameraRT->m_nSelectedSource);

    s_CameraRT->SetSource(&m_Camera);

    Logger::Debug("Added source to rt = {} sources = {} source = {}", fmt::ptr(s_CameraRT), s_CameraRT->m_aMultiSource.size(), s_CameraRT->m_nSelectedSource);
}

DECLARE_PLUGIN_DETOUR(Editor, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData)
{
    //if (p_SceneData.m_sceneName == "assembly:/_PRO/Scenes/Frontend/MainMenu.entity")
    //	p_SceneData.m_sceneName = "assembly:/_pro/scenes/users/notex/test.entity";
    //    p_SceneData.m_sceneName = "assembly:/_PRO/Scenes/Missions/TheFacility/_Scene_Mission_Polarbear_Module_002_B.entity";

    return HookResult<void>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(Editor, ZTemplateEntityBlueprintFactory*, ZTemplateEntityBlueprintFactory_ctor, ZTemplateEntityBlueprintFactory* th, STemplateEntityBlueprint* pTemplateEntityBlueprint, ZResourcePending& ResourcePending)
{
    //Logger::Debug("Creating Blueprint Factory {} with template {}", fmt::ptr(th), fmt::ptr(pTemplateEntityBlueprint));
    return HookResult<ZTemplateEntityBlueprintFactory*>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(Editor, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear)
{
    m_SelectedBrickIndex = 0;
    m_SelectedEntity = {};
    m_Camera = {};
    m_CameraRT = {};
    m_ShouldScrollToEntity = false;

    return HookResult<void>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(Editor, bool, OnInputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data)
{
    //if (entity == m_SelectedEntity)
    {
        m_FiredInputPins[pinId] = PinFireInfo {
            .m_FireTime = std::chrono::system_clock::now(),
        };
    }

    return { HookAction::Continue() };
}

DECLARE_PLUGIN_DETOUR(Editor, bool, OnOutputPin, ZEntityRef entity, uint32_t pinId, const ZObjectRef& data)
{
    //if (entity == m_SelectedEntity)
    {
        m_FiredOutputPins[pinId] = PinFireInfo {
            .m_FireTime = std::chrono::system_clock::now(),
        };
    }

    return { HookAction::Continue() };
}

DECLARE_ZHM_PLUGIN(Editor);
