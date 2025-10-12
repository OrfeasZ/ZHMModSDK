#include "Noclip.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/SGameUpdateEvent.h>

#include "IconsMaterialDesign.h"
#include "Glacier/ZCameraEntity.h"
#include "Glacier/ZGeomEntity.h"
#include "Glacier/ZKnowledge.h"
#include "Glacier/ZModule.h"
#include "Glacier/ZInputActionManager.h"

Noclip::Noclip() :
    m_ToggleNoclipAction("ToggleNoclip"),
    m_ForwardAction("Forward"),
    m_BackwardAction("Backward"),
    m_LeftAction("Left"),
    m_RightAction("Right"),
    m_FastAction("Fast")
{
}

Noclip::~Noclip() {
    const ZMemberDelegate<Noclip, void(const SGameUpdateEvent&)> s_Delegate(this, &Noclip::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void Noclip::OnEngineInitialized() {
    const ZMemberDelegate<Noclip, void(const SGameUpdateEvent&)> s_Delegate(this, &Noclip::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    const char* binds = "NoclipInput={"
        "ToggleNoclip=& | hold(kb,lctrl) hold(kb,rctrl) tap(kb,n);"
        "Forward=hold(kb,w);"
        "Backward=hold(kb,s);"
        "Left=hold(kb,a);"
        "Right=hold(kb,d);"
        "Fast=hold(kb,lshift) | hold(kb,rshift);};";

    if (ZInputActionManager::AddBindings(binds)) {
        Logger::Debug("Successfully added bindings.");
    }
    else {
        Logger::Debug("Failed to add bindings.");
    }
}

void Noclip::Init() {
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &Noclip::OnClearScene);
}

void Noclip::OnDrawMenu() {
    if (ImGui::Checkbox(ICON_MD_SELF_IMPROVEMENT " Noclip", &m_NoclipEnabled)) {
        if (m_NoclipEnabled) {
            if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
                if (const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>())
                    m_PlayerPosition = s_HitmanSpatial->GetWorldMatrix();
            }
        }
    }
}

void Noclip::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman)
        return;

    const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

    if (!s_HitmanSpatial)
        return;

    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

    if (!s_CurrentCamera)
        return;

    if (Functions::ZInputAction_Digital->Call(&m_ToggleNoclipAction, -1)) {
        m_NoclipEnabled = !m_NoclipEnabled;

        if (m_NoclipEnabled) {
            m_PlayerPosition = s_HitmanSpatial->GetWorldMatrix();
        }
    }

    if (!m_NoclipEnabled)
        return;

    auto s_CameraTrans = s_CurrentCamera->GetWorldMatrix();

    // Meters per second.
    float s_MoveSpeed = 5.f;

    if (Functions::ZInputAction_Digital->Call(&m_FastAction, -1))
        s_MoveSpeed = 20.f;

    if (Functions::ZInputAction_Digital->Call(&m_ForwardAction, -1))
        m_PlayerPosition.Trans += s_CameraTrans.Up * -s_MoveSpeed * p_UpdateEvent.m_GameTimeDelta.ToSeconds();

    if (Functions::ZInputAction_Digital->Call(&m_BackwardAction, -1))
        m_PlayerPosition.Trans += s_CameraTrans.Up * s_MoveSpeed * p_UpdateEvent.m_GameTimeDelta.ToSeconds();

    if (Functions::ZInputAction_Digital->Call(&m_LeftAction, -1))
        m_PlayerPosition.Trans += s_CameraTrans.Right * -s_MoveSpeed * p_UpdateEvent.m_GameTimeDelta.ToSeconds();

    if (Functions::ZInputAction_Digital->Call(&m_RightAction, -1))
        m_PlayerPosition.Trans += s_CameraTrans.Right * s_MoveSpeed * p_UpdateEvent.m_GameTimeDelta.ToSeconds();

    s_HitmanSpatial->SetWorldMatrix(m_PlayerPosition);
}

DEFINE_PLUGIN_DETOUR(Noclip, void, OnClearScene, ZEntitySceneContext* th, bool forReload) {
    m_NoclipEnabled = false;
    return HookResult<void>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(Noclip);
