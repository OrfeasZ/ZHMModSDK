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

Noclip::Noclip()
{
}

Noclip::~Noclip()
{
    const ZMemberDelegate<Noclip, void(const SGameUpdateEvent&)> s_Delegate(this, &Noclip::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void Noclip::OnEngineInitialized()
{
    const ZMemberDelegate<Noclip, void(const SGameUpdateEvent&)> s_Delegate(this, &Noclip::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void Noclip::Init()
{
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &Noclip::OnClearScene);
}

void Noclip::OnDrawMenu()
{
    if (ImGui::Checkbox(ICON_MD_SELF_IMPROVEMENT " Noclip", &m_NoclipEnabled))
    {
        if (m_NoclipEnabled)
        {
            TEntityRef<ZHitman5> s_LocalHitman;
            Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

            if (s_LocalHitman)
            {
                if (const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>())
                    m_PlayerPosition = s_HitmanSpatial->GetWorldMatrix();
            }
        }
    }
}

void Noclip::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
    TEntityRef<ZHitman5> s_LocalHitman;
    Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

    if (!s_LocalHitman)
        return;

    const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

    if (!s_HitmanSpatial)
        return;

    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

    if (!s_CurrentCamera)
        return;

    // TODO: Use proper way to get inputs.
    // TODO: This triggers on every frame when the key is held down.
    if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState('N'))
    {
        m_NoclipEnabled = !m_NoclipEnabled;

        if (m_NoclipEnabled)
        {
            m_PlayerPosition = s_HitmanSpatial->GetWorldMatrix();
        }
    }

    if (!m_NoclipEnabled)
        return;

    auto s_CameraTrans = s_CurrentCamera->GetWorldMatrix();

    // Meters per second.
    float s_MoveSpeed = 5.f;

    if (GetAsyncKeyState(VK_SHIFT))
        s_MoveSpeed = 20.f;

    if (GetAsyncKeyState('W'))
        m_PlayerPosition.Trans += s_CameraTrans.Up * -s_MoveSpeed * p_UpdateEvent.m_GameTimeDelta.ToSeconds();

    if (GetAsyncKeyState('S'))
        m_PlayerPosition.Trans += s_CameraTrans.Up * s_MoveSpeed * p_UpdateEvent.m_GameTimeDelta.ToSeconds();

    if (GetAsyncKeyState('A'))
        m_PlayerPosition.Trans += s_CameraTrans.Left * -s_MoveSpeed * p_UpdateEvent.m_GameTimeDelta.ToSeconds();

    if (GetAsyncKeyState('D'))
        m_PlayerPosition.Trans += s_CameraTrans.Left * s_MoveSpeed * p_UpdateEvent.m_GameTimeDelta.ToSeconds();

    s_HitmanSpatial->SetWorldMatrix(m_PlayerPosition);
}

DECLARE_PLUGIN_DETOUR(Noclip, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear)
{
    m_NoclipEnabled = false;
    return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(Noclip);
