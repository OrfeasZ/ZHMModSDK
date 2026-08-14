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
    m_MoveInFreeCam(false),
    m_ToggleFreeCamAction("ToggleFreeCamera"),
    m_FreezeFreeCamActionGc("ActivateGameControl0"),
    m_FreezeFreeCamActionKb("KBMInspectNode"),
    m_InstantlyKillNpcAction("InstantKill"),
    m_TeleportMainCharacterAction("Teleport"),
    m_TogglePauseGame("TogglePauseGame"),
    m_MousePosX("MousePosX"),
    m_MousePosY("MousePosY"),
    m_ZoomToSelection("ZoomToSelection"),
    m_ActivateRotate("ActivateRotate"),
    m_ActivateObjectHook("ActivateObjectHook"),
    m_OrbitCamera("OrbitCamera"),
    m_Zoom("Zoom"),
    m_ShowFreeCamWindow(false),
    m_ShowControlsWindow(false),
    m_HasToggledFreeCamBefore(false),
    m_EditorStyleFreeCam(false) {
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
        {"Ctrl + F6", "Teleport hitman"},
        {"F8", "Pause/resume game"},
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
                    "[FreeCam] Got local hitman entity and input control! Enabling input. {} {}", fmt::ptr(s_InputControl),
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

    Hooks::ZFreeCameraControlEditorStyleEntity_UpdateMovementFromInput->AddDetour(
        this, &FreeCam::ZFreeCameraControlEditorStyleEntity_UpdateMovementFromInput
    );
    Hooks::ZFreeCameraControlEditorStyleEntity_GenerateActionBindingString->AddDetour(
        this, &FreeCam::ZFreeCameraControlEditorStyleEntity_GenerateActionBindingString
    );

    m_EditorStyleFreeCam = GetSettingBool("general", "use_editor_style_freecam", false);
    m_GamePaused = GetSettingBool("general", "toggle_pause", false);
    m_MoveInFreeCam = GetSettingBool("general", "move_in_freecam", false);
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
        Logger::Debug("[FreeCam] Successfully added bindings.");
    }
    else {
        Logger::Debug("[FreeCam] Failed to add bindings.");
    }
}

void FreeCam::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    if (!*Globals::ApplicationEngineWin32)
        return;

    if (!(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01.m_pInterfaceRef) {
        Logger::Debug("[FreeCam] Creating free camera.");
        Functions::ZEngineAppCommon_CreateFreeCameraAndControl->Call(&(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon);

        // If freecam was active we need to toggle.
        // This can happen after level restarts / changes.
        if (m_FreeCamActive)
            m_ShouldToggle = true;
    }

    if (m_EditorStyleFreeCam) {
        (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControlEditorStyle01.m_pInterfaceRef->
            SetActive(m_FreeCamActive);
    }
    else {
        (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControl01.m_pInterfaceRef->SetActive(
            m_FreeCamActive
        );
    }

    if (Functions::ZInputAction_Digital->Call(&m_ToggleFreeCamAction, -1)) {
        ToggleFreeCam();
    }

    if (m_ShouldToggle) {
        m_ShouldToggle = false;

        if (m_FreeCamActive)
            EnableFreeCam();
        else
            DisableFreeCam();
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

        if (m_EditorStyleFreeCam) {
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
                s_InputControl->m_bActive = s_FreezeFreeCam || m_MoveInFreeCam;
            }
        }
    }
}

void FreeCam::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_PHOTO_CAMERA " FREECAM")) {
        m_ShowFreeCamWindow = !m_ShowFreeCamWindow;
    }
}

void FreeCam::ToggleFreeCam() {
    m_FreeCamActive = !m_FreeCamActive;
    m_ShouldToggle = true;
    m_HasToggledFreeCamBefore = true;
}

void FreeCam::EnableFreeCam() {
    auto s_Camera = (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01;

    TEntityRef<IRenderDestinationEntity> s_RenderDest;
    Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

    m_OriginalCam = *s_RenderDest.m_pInterfaceRef->GetSource();

    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();
    s_Camera.m_pInterfaceRef->SetObjectToWorldMatrixFromEditor(s_CurrentCamera->GetObjectToWorldMatrix());

    Logger::Debug("[FreeCam] Camera trans: {}", fmt::ptr(&s_Camera.m_pInterfaceRef->m_mTransform.Trans));

    s_RenderDest.m_pInterfaceRef->SetSource(&s_Camera.m_entityRef);

    if (m_GamePaused)
        Globals::GameTimeManager->m_bPaused = true;
}

void FreeCam::DisableFreeCam() {
    TEntityRef<IRenderDestinationEntity> s_RenderDest;
    Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

    s_RenderDest.m_pInterfaceRef->SetSource(&m_OriginalCam);

    // Enable Hitman input.
    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (s_LocalHitman) {
        auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(Globals::InputManager);

        if (s_InputControl) {
            Logger::Debug(
                "[FreeCam] Got local hitman entity and input control! Enabling input. {} {}", fmt::ptr(s_InputControl),
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
        ZEntityRef s_LogicalParent = s_RayOutput.m_pBlockingSpatialEntity.m_entityRef.GetLogicalParent();
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
            ZSpatialEntity* s_SpatialEntity = s_LocalHitman.m_entityRef.QueryInterface<ZSpatialEntity>();
            SMatrix s_WorldMatrix = s_SpatialEntity->GetObjectToWorldMatrix();

            s_WorldMatrix.Trans = s_RayOutput.m_vPosition;

            s_SpatialEntity->SetObjectToWorldMatrixFromEditor(s_WorldMatrix);
        }
    }
}

bool FreeCam::GetFreeCameraRayCastClosestHitQueryOutput(ZRayQueryOutput& p_RayOutput) {
    auto s_Camera = (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01;
    SMatrix s_WorldMatrix = s_Camera.m_pInterfaceRef->GetObjectToWorldMatrix();
    float4 s_InvertedDirection = float4(
        -s_WorldMatrix.ZAxis.x, -s_WorldMatrix.ZAxis.y, -s_WorldMatrix.ZAxis.z, -s_WorldMatrix.ZAxis.w
    );
    float4 s_From = s_WorldMatrix.Trans;
    float4 s_To = s_WorldMatrix.Trans + s_InvertedDirection * 500.f;

    if (!*Globals::CollisionManager) {
        Logger::Error("[FreeCam] Collision manager not found.");

        return false;
    }

    ZRayQueryInput s_RayInput {
        .m_vFrom = s_From,
        .m_vTo = s_To,
    };

    if (!(*Globals::CollisionManager)->RayCastClosestHit(s_RayInput, &p_RayOutput)) {
        Logger::Error("[FreeCam] Raycast failed.");

        return false;
    }

    return true;
}

void FreeCam::OnDrawUI(bool p_HasFocus) {
    if (m_ShowFreeCamWindow) {
        const auto s_Center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(s_Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushFont(SDK()->GetImGuiBlackFont());
        const auto s_IsWindowExpanded = ImGui::Begin(ICON_MD_PHOTO_CAMERA " Freecam", &m_ShowFreeCamWindow);
        ImGui::PushFont(SDK()->GetImGuiRegularFont());

        if (s_IsWindowExpanded) {
            bool s_FreeCamActive = m_FreeCamActive;
            if (ImGui::Checkbox(ICON_MD_PHOTO_CAMERA " Enable freecam", &s_FreeCamActive)) {
                ToggleFreeCam();
            }

            ImGui::BeginDisabled(s_FreeCamActive);
            if (ImGui::Checkbox("Use editor style freecam", &m_EditorStyleFreeCam)) {
                SetSettingBool("general", "use_editor_style_freecam", m_EditorStyleFreeCam);
            }
            ImGui::EndDisabled();

            if (ImGui::Checkbox("Move in freecam", &m_MoveInFreeCam)) {
                SetSettingBool("general", "move_in_freecam", m_MoveInFreeCam);
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
                m_ShowControlsWindow = !m_ShowControlsWindow;
            }
        }

        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
    }

    if (m_ShowControlsWindow) {
        ImGui::PushFont(SDK()->GetImGuiBlackFont());
        const auto s_ControlsExpanded = ImGui::Begin(ICON_MD_PHOTO_CAMERA " FreeCam controls", &m_ShowControlsWindow);
        ImGui::PushFont(SDK()->GetImGuiRegularFont());

        if (s_ControlsExpanded) {
            ImGui::TextUnformatted("PC controls");

            if (ImGui::BeginTable("FreeCamControlsPc", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
                if (m_EditorStyleFreeCam) {
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
            }

            if (!m_EditorStyleFreeCam) {
                ImGui::TextUnformatted("Controller controls");

                if (ImGui::BeginTable("FreeCamControlsController", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
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
        }

        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
    }
}

void FreeCam::HandleDrag(ZFreeCameraControlEditorStyleEntity* p_FreeCamera, bool bRotationIsActive, bool bObjectHookIsActive, bool bIsOrbitActive) {
    if (!bRotationIsActive && !bObjectHookIsActive) {
        p_FreeCamera->m_bDraggingIsActive = false;
        return;
    }

    if (!p_FreeCamera->m_bDraggingIsActive) {
        p_FreeCamera->m_bDraggingIsActive = true;
    }

    /*
     * The original implementation uses anaraw(ms,posx/posy) and computes
     * dragDelta = m_vMousePos - m_vLastDragPoint.
     *
     * This implementation uses rel(ms,x/y), so m_vMousePos already contains
     * per-frame mouse deltas and m_vLastDragPoint is intentionally
     * unused.
     */
    const float s_DeltaX = p_FreeCamera->m_vMousePos.x;
    const float s_DeltaY = p_FreeCamera->m_vMousePos.y;

    if (s_DeltaX == 0.0f && s_DeltaY == 0.0f) {
        return;
    }

    constexpr float c_OrbitSensitivity = 0.003f;

    if (bObjectHookIsActive) {
        if (!bIsOrbitActive) {
            Functions::ZCameraUtil_PanCamera->Call(
                TEntityRef<ICameraEntity>(p_FreeCamera->m_pControlledCameraEntity.m_entityRef),
                TEntityRef<ZSpatialEntity>(p_FreeCamera->m_pControlledCameraEntity.m_entityRef),
                { s_DeltaX, s_DeltaY, 0.f, 0.f },
                p_FreeCamera->m_vHookPoint
            );

            return;
        }

        Functions::ZFreeCameraControlEditorStyleEntity_OrbitCamera->Call(
            p_FreeCamera,
            float4(s_DeltaX * c_OrbitSensitivity, s_DeltaY * c_OrbitSensitivity, 0.f, 0.f),
            p_FreeCamera->m_vHookPoint
        );

        return;
    }

    if (bIsOrbitActive) {
        Functions::ZFreeCameraControlEditorStyleEntity_OrbitCamera->Call(
            p_FreeCamera,
            float4(s_DeltaX * c_OrbitSensitivity, s_DeltaY * c_OrbitSensitivity, 0.f, 0.f),
            p_FreeCamera->m_vHookPoint
        );

        return;
    }

    if (!p_FreeCamera->m_pControlledCameraEntity) {
        return;
    }

    constexpr float c_MouseSensitivity = 0.16f;

    const float s_YawDelta = s_DeltaX * c_MouseSensitivity;
    const float s_PitchDelta = s_DeltaY * c_MouseSensitivity;

    Functions::ZFreeCameraControlEditorStyleEntity_Rotate->Call(p_FreeCamera, s_YawDelta, s_PitchDelta);
}

DEFINE_PLUGIN_DETOUR(FreeCam, bool, ZInputAction_Digital, ZInputAction* th, int32_t controllerId) {
    if (!m_FreeCamActive)
        return HookResult<bool>(HookAction::Continue());

    if (strcmp(th->m_szName, "ActivateGameControl0") == 0 && m_FreeCamFrozen)
        return HookResult(HookAction::Return(), true);

    return HookResult<bool>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(FreeCam, bool, OnLoadScene, ZEntitySceneContext* th, SSceneInitParameters&) {
    if (m_FreeCamActive)
        DisableFreeCam();

    m_FreeCamActive = false;
    m_ShouldToggle = false;

    return HookResult<bool>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(FreeCam, void, OnClearScene, ZEntitySceneContext* th, bool) {
    if (m_FreeCamActive)
        DisableFreeCam();

    m_FreeCamActive = false;
    m_ShouldToggle = false;

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(
    FreeCam, void, ZFreeCameraControlEditorStyleEntity_UpdateMovementFromInput, ZFreeCameraControlEditorStyleEntity* th
) {
    th->m_vMousePos.x = Functions::ZInputAction_Analog->Call(&m_MousePosX, -1);
    th->m_vMousePos.y = Functions::ZInputAction_Analog->Call(&m_MousePosY, -1);

    const bool s_ZoomToSelectionActive = Functions::ZInputAction_Digital->Call(&m_ZoomToSelection, -1);

    if (s_ZoomToSelectionActive && !th->m_bZoomToSelectionWasActive) {
        th->ZoomCameraToSelection();
    }

    th->m_bZoomToSelectionWasActive = s_ZoomToSelectionActive;

    const bool s_RotationActive = Functions::ZInputAction_Digital->Call(&m_ActivateRotate, -1);
    const bool s_ObjectHookActive = Functions::ZInputAction_Digital->Call(&m_ActivateObjectHook, -1);

    if ((s_RotationActive && !th->m_bRotationWasActive) ||
        (s_ObjectHookActive && !th->m_bObjectHookWasActive)) {
        if (th->m_pControlledCameraEntity) {
            auto* s_Camera = th->m_pControlledCameraEntity.m_pInterfaceRef;

            th->m_vHookPoint =
                s_Camera->GetObjectToWorldMatrix().Trans -
                s_Camera->GetObjectToWorldMatrix().ZAxis * 2.5f;
        }
        else {
            th->m_vHookPoint = {};
        }
    }

    th->m_bRotationWasActive = s_RotationActive;
    th->m_bObjectHookWasActive = s_ObjectHookActive;

    const bool s_OrbitCameraActive = Functions::ZInputAction_Digital->Call(&m_OrbitCamera, -1);

    HandleDrag(
        th,
        s_RotationActive,
        s_ObjectHookActive,
        s_OrbitCameraActive
    );

    const float s_Zoom = Functions::ZInputAction_Analog->Call(&m_Zoom, -1);

    if (s_Zoom == 0.f) {
        return { HookAction::Return() };
    }

    if (s_RotationActive) {
        constexpr float s_SpeedMultiplier = 1.8f;

        if (s_Zoom > 0.f) {
            th->m_fSpeed *= s_SpeedMultiplier;
        }
        else {
            th->m_fSpeed /= s_SpeedMultiplier;
        }

        th->m_fSpeed = std::clamp(th->m_fSpeed, 0.1f, 100.f);
    }
    else {
        Functions::ZFreeCameraControlEditorStyleEntity_ZoomCamera->Call(th, std::clamp(s_Zoom, -1.f, 1.f));
    }

    return { HookAction::Return() };
}

DEFINE_PLUGIN_DETOUR(
    FreeCam, ZString*, ZFreeCameraControlEditorStyleEntity_GenerateActionBindingString, ZFreeCameraControlEditorStyleEntity* th, ZString& result
) {
    result = "FreeCamControlEditorStyle0={"
        "MousePosX=rel(ms,x);"
        "MousePosY=rel(ms,y);"
        "MoveX=+ -hold(kb,right) hold(kb,left) -hold(kb,d) hold(kb,a);"
        "MoveY=+ -hold(kb,down) hold(kb,up) -hold(kb,s) hold(kb,w);"
        "MoveZ=+ -hold(kb,pgdn) hold(kb,pgup) -hold(kb,q) hold(kb,e);"
        "MoveFast=| hold(kb,lshift) hold(kb,rshift);"
        "Zoom=rel(ms,wheel);"
        "ZoomPrecision=| hold(kb,lalt) hold(kb,ralt);"
        "ZoomToSelection= hold(kb,z);"
        "OrbitCamera=| hold(kb,lalt) hold(kb,ralt);"
        "ActivateRotate=hold(ms,mb2);"
        "ActivateObjectHook=hold(ms,mb3);"
        "};";

    return { HookAction::Return(), &result };
}

DEFINE_ZHM_PLUGIN(FreeCam);
