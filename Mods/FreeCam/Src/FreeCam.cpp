#include "FreeCam.h"

#include <random>

#include "Events.h"
#include "Functions.h"
#include "Logging.h"

#include <Glacier/ZActor.h>
#include <Glacier/SGameUpdateEvent.h>
#include <Glacier/ZObject.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZApplicationEngineWin32.h>
#include <Glacier/ZEngineAppCommon.h>
#include <Glacier/ZFreeCamera.h>
#include <Glacier/ZRender.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/ZHM5InputManager.h>
#include <Glacier/ZInputActionManager.h>

#include "IconsMaterialDesign.h"
#include <imgui_internal.h>

FreeCam::FreeCam() :
    m_FreeCamActive(false),
    m_ShouldToggle(false),
    m_FreeCamFrozen(false),
    m_GamePaused(false),
    m_MoveInFreecam(false),
    m_ToggleFreeCamAction("ToggleFreeCamera"),
    m_FreezeFreeCamActionGc("ActivateGameControl0"),
    m_FreezeFreeCamActionKb("KBMInspectNode"),
    m_InstantlyKillNpcAction("InstantKill"),
    m_TeleportMainCharacterAction("Teleport"),
    m_TogglePauseGame("TogglePauseGame"),
    m_MenuVisible(false),
    m_ControlsVisible(false),
    m_HasToggledFreecamBefore(false),
    m_EditorStyleFreecam(false) {
    m_PcControls = {
        {"K", "Toggle freecam"},
        {"F3", "Lock camera and enable 47 input"},
        {"Ctrl + W/S", "Change FOV"},
        {"Ctrl + A/D", "Roll camera"},
        {"Ctrl + X", "Reset roll"},
        {"Alt + W/S", "Change camera speed"},
        {"Space + Q/E", "Change camera height"},
        {"Space + W/S", "Move camera on axis"},
        {"Shift", "Increase camera speed"},
        {"F9", "Kill NPC"},
        {"Ctrl + F6", "Teleport Hitman"},
        {"F8", "Pause/Resume game"},
    };

    m_PcControlsEditorStyle = {
        {"P", "Toggle freecam"},
        {"F3", "Lock camera and enable 47 input"},
        {"MMB", "Drag camera"},
        {"Scroll Wheel", "Zoom camera"},
        {"RMB", "Activate rotate"},
        {"Alt + MMB or RMB", "Orbit camera"},
        {"Z + Alt + MMB or RMB", "Orbit camera around selected entity"},
        {"Z", "Zoom to selected entity (press twice to focus the gizmo)"},
        {"Alt + Scroll Wheel", "Zoom camera with precision"},
        {"Shift", "Speed modifier"},
        {"RMB + Scroll wheel", "Adjust speed"},
    };

    m_ControllerControls = {
        {"Y + L", "Change FOV"},
        {"A + L", "Roll camera"},
        {"A + L press", "Reset rotation"},
        {"B + R", "Change camera speed"},
        {"RT", "Increase camera speed"},
        {"LB", "Lock camera and enable 47 input"},
        {"LT + R", "Change camera height"},
    };
}

FreeCam::~FreeCam() {
    const ZMemberDelegate<FreeCam, void(const SGameUpdateEvent&)> s_Delegate(this, &FreeCam::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    // Reset the camera to default when unloading with freecam active.
    if (m_FreeCamActive) {
        TEntityRef<IRenderDestinationEntity> s_RenderDest;
        Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

        s_RenderDest.m_pInterfaceRef->SetSource(&m_OriginalCam);

        // Enable Hitman input.
        if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
            if (auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(
                Globals::InputManager
            )) {
                Logger::Debug(
                    "Got local hitman entity and input control! Enabling input. {} {}", fmt::ptr(s_InputControl),
                    fmt::ptr(s_LocalHitman.m_pInterfaceRef)
                );
                s_InputControl->m_bActive = true;
            }
        }
    }
}

void FreeCam::Init() {
    Hooks::ZInputAction_Digital->AddDetour(this, &FreeCam::ZInputAction_Digital);
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &FreeCam::OnLoadScene);
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &FreeCam::OnClearScene);

    m_GamePaused = GetSettingBool("general", "toggle_pause", false);
    m_MoveInFreecam = GetSettingBool("general", "move_in_freecam", false);
}

void FreeCam::OnEngineInitialized() {
    const ZMemberDelegate<FreeCam, void(const SGameUpdateEvent&)> s_Delegate(this, &FreeCam::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    const char* binds = "FreeCameraInput={"
            "ToggleFreeCamera=tap(kb,k);"
            "Teleport=& | hold(kb,lctrl) hold(kb,rctrl) tap(kb,f6);"
            "InstantKill=tap(kb,f9);"
            "TogglePauseGame=tap(kb,f8);};";

    if (ZInputActionManager::AddBindings(binds)) {
        Logger::Debug("Successfully added bindings.");
    }
    else {
        Logger::Debug("Failed to add bindings.");
    }
}

void FreeCam::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    if (!*Globals::ApplicationEngineWin32)
        return;

    if (!(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01.m_pInterfaceRef) {
        Logger::Debug("Creating free camera.");
        Functions::ZEngineAppCommon_CreateFreeCamera->Call(&(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon);

        // If freecam was active we need to toggle.
        // This can happen after level restarts / changes.
        if (m_FreeCamActive)
            m_ShouldToggle = true;
    }

    if (m_EditorStyleFreecam) {
        (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControlEditorStyle01.m_pInterfaceRef->
                                            SetActive(m_FreeCamActive);
    }
    else {
        (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControl01.m_pInterfaceRef->SetActive(
            m_FreeCamActive
        );
    }

    if (Functions::ZInputAction_Digital->Call(&m_ToggleFreeCamAction, -1)) {
        ToggleFreecam();
    }

    if (m_ShouldToggle) {
        m_ShouldToggle = false;

        if (m_FreeCamActive)
            EnableFreecam();
        else
            DisableFreecam();
    }

    // While freecam is active, only enable hitman input when the "freeze camera" button is pressed.
    if (m_FreeCamActive) {
        if (Functions::ZInputAction_Digital->Call(&m_FreezeFreeCamActionKb, -1))
            m_FreeCamFrozen = !m_FreeCamFrozen;

        if (Functions::ZInputAction_Digital->Call(&m_TogglePauseGame, -1)) {
            m_GamePaused = !m_GamePaused;
            SetSettingBool("general", "toggle_pause", m_GamePaused);
            Globals::GameTimeManager->m_bPaused = m_GamePaused;
        }

        if (Functions::ZInputAction_Digital->Call(&m_InstantlyKillNpcAction, -1)) {
            InstantlyKillNpc();
        }

        if (Functions::ZInputAction_Digital->Call(&m_TeleportMainCharacterAction, -1)) {
            TeleportMainCharacter();
        }

        const bool s_FreezeFreeCam = Functions::ZInputAction_Digital->Call(&m_FreezeFreeCamActionGc, -1) ||
                m_FreeCamFrozen;

        if (m_EditorStyleFreecam) {
            (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControlEditorStyle01.m_pInterfaceRef->
                                                m_bActive = !s_FreezeFreeCam;
        }
        else {
            (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControl01.m_pInterfaceRef->
                                                m_bFreezeCamera = s_FreezeFreeCam;
        }

        if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
            if (auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(
                Globals::InputManager
            )) {
                s_InputControl->m_bActive = s_FreezeFreeCam || m_MoveInFreecam;
            }
        }
    }
}

void FreeCam::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_PHOTO_CAMERA " FREECAM")) {
        m_MenuVisible = !m_MenuVisible;
    }
}

void FreeCam::ToggleFreecam() {
    m_FreeCamActive = !m_FreeCamActive;
    m_ShouldToggle = true;
    m_HasToggledFreecamBefore = true;
}

void FreeCam::EnableFreecam() {
    auto s_Camera = (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01;

    TEntityRef<IRenderDestinationEntity> s_RenderDest;
    Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

    m_OriginalCam = *s_RenderDest.m_pInterfaceRef->GetSource();

    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();
    s_Camera.m_pInterfaceRef->SetWorldMatrix(s_CurrentCamera->GetWorldMatrix());

    Logger::Debug("Camera trans: {}", fmt::ptr(&s_Camera.m_pInterfaceRef->m_mTransform.Trans));

    s_RenderDest.m_pInterfaceRef->SetSource(&s_Camera.m_ref);

    if (m_GamePaused)
        Globals::GameTimeManager->m_bPaused = true;
}

void FreeCam::DisableFreecam() {
    TEntityRef<IRenderDestinationEntity> s_RenderDest;
    Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

    s_RenderDest.m_pInterfaceRef->SetSource(&m_OriginalCam);

    // Enable Hitman input.
    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (s_LocalHitman) {
        auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(Globals::InputManager);

        if (s_InputControl) {
            Logger::Debug(
                "Got local hitman entity and input control! Enabling input. {} {}", fmt::ptr(s_InputControl),
                fmt::ptr(s_LocalHitman.m_pInterfaceRef)
            );
            s_InputControl->m_bActive = true;
        }
    }

    Globals::GameTimeManager->m_bPaused = false;
}

void FreeCam::InstantlyKillNpc() {
    ZRayQueryOutput s_RayOutput {};

    if (GetFreeCameraRayCastClosestHitQueryOutput(s_RayOutput) && s_RayOutput.m_pBlockingSpatialEntity.m_pInterfaceRef) {
        ZEntityRef s_LogicalParent = s_RayOutput.m_pBlockingSpatialEntity.m_ref.GetLogicalParent();
        ZActor* s_Actor = s_LogicalParent.QueryInterface<ZActor>();

        if (s_Actor) {
            TEntityRef<IItem> s_Item;
            TEntityRef<ZSetpieceEntity> s_SetPieceEntity;

            Functions::ZActor_KillActor->Call(
                s_Actor, s_Item, s_SetPieceEntity, EDamageEvent::eDE_UNDEFINED, EDeathBehavior::eDB_IMPACT_ANIM
            );
        }
    }
}

void FreeCam::TeleportMainCharacter() {
    ZRayQueryOutput s_RayOutput {};

    if (GetFreeCameraRayCastClosestHitQueryOutput(s_RayOutput) && s_RayOutput.m_pBlockingSpatialEntity.m_pInterfaceRef) {
        if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
            ZSpatialEntity* s_SpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
            SMatrix s_WorldMatrix = s_SpatialEntity->GetWorldMatrix();

            s_WorldMatrix.Trans = s_RayOutput.m_vPosition;

            s_SpatialEntity->SetWorldMatrix(s_WorldMatrix);
        }
    }
}

bool FreeCam::GetFreeCameraRayCastClosestHitQueryOutput(ZRayQueryOutput& p_RayOutput) {
    auto s_Camera = (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01;
    SMatrix s_WorldMatrix = s_Camera.m_pInterfaceRef->GetWorldMatrix();
    float4 s_InvertedDirection = float4(
        -s_WorldMatrix.ZAxis.x, -s_WorldMatrix.ZAxis.y, -s_WorldMatrix.ZAxis.z, -s_WorldMatrix.ZAxis.w
    );
    float4 s_From = s_WorldMatrix.Trans;
    float4 s_To = s_WorldMatrix.Trans + s_InvertedDirection * 500.f;

    if (!*Globals::CollisionManager) {
        Logger::Error("Collision manager not found.");

        return false;
    }

    ZRayQueryInput s_RayInput {
        .m_vFrom = s_From,
        .m_vTo = s_To,
    };

    if (!(*Globals::CollisionManager)->RayCastClosestHit(s_RayInput, &p_RayOutput)) {
        Logger::Error("Raycast failed.");

        return false;
    }

    return true;
}

void FreeCam::OnDrawUI(bool p_HasFocus) {
    if (m_MenuVisible) {
        const auto s_Center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(s_Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushFont(SDK()->GetImGuiBlackFont());
        const auto s_MenuExpanded = ImGui::Begin(ICON_MD_PHOTO_CAMERA " FreeCam", &m_MenuVisible);
        ImGui::PushFont(SDK()->GetImGuiRegularFont());

        if (s_MenuExpanded) {
            bool s_FreeCamActive = m_FreeCamActive;
            if (ImGui::Checkbox(ICON_MD_PHOTO_CAMERA " Enable freecam", &s_FreeCamActive)) {
                ToggleFreecam();
            }

            ImGui::BeginDisabled(s_FreeCamActive);
            ImGui::Checkbox("Use editor style freecam", &m_EditorStyleFreecam);
            ImGui::EndDisabled();

            if (ImGui::Checkbox("Move in freecam", &m_MoveInFreecam)) {
                SetSettingBool("general", "move_in_freecam", m_MoveInFreecam);
            }

            if (ImGui::Checkbox("Pause game in freecam", &m_GamePaused)) {
                SetSettingBool("general", "toggle_pause", m_GamePaused);

                if (m_FreeCamActive) {
                    Globals::GameTimeManager->m_bPaused = m_GamePaused;
                }
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Whether the Hitman should move along with the camera when in freecam.");
            }

            if (ImGui::Button(ICON_MD_SPORTS_ESPORTS " Show freecam controls")) {
                m_ControlsVisible = !m_ControlsVisible;
            }
        }

        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
    }

    if (m_ControlsVisible) {
        ImGui::PushFont(SDK()->GetImGuiBlackFont());
        const auto s_ControlsExpanded = ImGui::Begin(ICON_MD_PHOTO_CAMERA " FreeCam Controls", &m_ControlsVisible);
        ImGui::PushFont(SDK()->GetImGuiRegularFont());

        if (s_ControlsExpanded) {
            ImGui::TextUnformatted("PC Controls");

            ImGui::BeginTable("FreeCamControlsPc", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit);

            if (m_EditorStyleFreecam) {
                for (auto& [s_Key, s_Description] : m_PcControlsEditorStyle) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(s_Key.c_str());
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(s_Description.c_str());
                }
            }
            else {
                for (auto& [s_Key, s_Description] : m_PcControls) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(s_Key.c_str());
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(s_Description.c_str());
                }
            }

            ImGui::EndTable();

            if (!m_EditorStyleFreecam) {
                ImGui::TextUnformatted("Controller Controls");

                ImGui::BeginTable(
                    "FreeCamControlsController", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit
                );

                for (auto& [s_Key, s_Description] : m_ControllerControls) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(s_Key.c_str());
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(s_Description.c_str());
                }

                ImGui::EndTable();
            }
        }

        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
    }
}

DEFINE_PLUGIN_DETOUR(FreeCam, bool, ZInputAction_Digital, ZInputAction* th, int a2) {
    if (!m_FreeCamActive)
        return HookResult<bool>(HookAction::Continue());

    if (strcmp(th->m_szName, "ActivateGameControl0") == 0 && m_FreeCamFrozen)
        return HookResult(HookAction::Return(), true);

    return HookResult<bool>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(FreeCam, void, OnLoadScene, ZEntitySceneContext* th, SSceneInitParameters&) {
    if (m_FreeCamActive)
        DisableFreecam();

    m_FreeCamActive = false;
    m_ShouldToggle = false;

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(FreeCam, void, OnClearScene, ZEntitySceneContext* th, bool) {
    if (m_FreeCamActive)
        DisableFreecam();

    m_FreeCamActive = false;
    m_ShouldToggle = false;

    return HookResult<void>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(FreeCam);
