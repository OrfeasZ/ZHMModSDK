#include "Clumsy.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/SGameUpdateEvent.h>

#include "IconsMaterialDesign.h"
#include "Glacier/ZGeomEntity.h"
#include "Glacier/ZModule.h"

Clumsy::Clumsy()
{

}

Clumsy::~Clumsy()
{
    const ZMemberDelegate<Clumsy, void(const SGameUpdateEvent&)> s_Delegate(this, &Clumsy::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void Clumsy::OnEngineInitialized()
{
    const ZMemberDelegate<Clumsy, void(const SGameUpdateEvent&)> s_Delegate(this, &Clumsy::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void Clumsy::Init()
{
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &Clumsy::OnClearScene);
}

void Clumsy::OnDrawUI(bool p_HasFocus)
{
    if (m_ShowBrickWarning)
    {
        ImGui::PushFont(SDK()->GetImGuiBlackFont());
        const auto s_Expanded = ImGui::Begin(ICON_MD_WARNING " Clumsy Warning", &m_ShowBrickWarning);
        ImGui::PushFont(SDK()->GetImGuiRegularFont());

        if (s_Expanded)
        {
            ImGui::Text("Could not find Clumsy brick!");
            ImGui::Text("Did you install the Clumsy SMF mod?");
        }

        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
    }
}


void Clumsy::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
    if (m_DeactivateRagdollQueued)
    {
        m_DeactivateRagdollQueued = false;

        TEntityRef<ZHitman5> s_LocalHitman;
        Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

        if (!s_LocalHitman)
            return;

        Logger::Debug("Deactivating ragdoll.");
        Functions::ZHM5BaseCharacter_DeactivateRagdoll->Call(s_LocalHitman.m_pInterfaceRef);

        const auto s_Animator = s_LocalHitman.m_pInterfaceRef->m_Animator.QueryInterface<ZHM5Animator>();
        auto s_Time = 1.f;

        Functions::ZHM5Animator_ActivateRagdollToAnimationBlend->Call(s_Animator, &s_Time);
    }
    else if (m_SlipStartTimer > 0.0)
    {
        m_SlipStartTimer -= p_UpdateEvent.m_GameTimeDelta.ToSeconds();

        if (m_SlipStartTimer < 0.0)
        {
            TEntityRef<ZHitman5> s_LocalHitman;
            Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

            if (!s_LocalHitman)
                return;

            Logger::Debug("Activating ragdoll.");
            Functions::ZHM5BaseCharacter_ActivateRagdoll->Call(s_LocalHitman.m_pInterfaceRef, true);

            // Fly!
            //s_LocalHitman.m_pInterfaceRef->m_rRagdollEntity.m_pInterfaceRef->ApplyForce();

            m_DeactivateRagdollQueued = false;
            m_SlipStopTimer = 3.0;
        }
    }
    else if (m_SlipStopTimer > 0.0)
    {
        m_SlipStopTimer -= p_UpdateEvent.m_GameTimeDelta.ToSeconds();

        // TODO: Only trigger if 47 has stopped moving.
        if (m_SlipStopTimer < 0.0)
        {
            if (const auto s_GetUp = GetGetUpEntity())
            {
                s_GetUp.SignalInputPin("Start");
            }
            else
            {
                m_ShowBrickWarning = true;
            }

            // We queue the ragdoll de-activation for the next frame update because if we do it
            // immediately, the animation wouldn't have started playing yet, and we would get a
            // frame of 47 standing up before the animation
            m_DeactivateRagdollQueued = true;
        }
    }

    ZInputAction s_RagdollAction("eIAKBMCover");
    if (Functions::ZInputAction_Digital->Call(&s_RagdollAction, -1))
    {
        TEntityRef<ZHitman5> s_LocalHitman;
        Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

        if (!s_LocalHitman)
            return;

        if (m_SlipStartTimer < 0.0 && m_SlipStopTimer < 0.0)
            m_SlipStartTimer = 0.1;
        else if (m_SlipStartTimer < 0.0 && m_SlipStopTimer > 0.0)
            m_SlipStopTimer = 3.0;
    }
}

ZEntityRef Clumsy::GetGetUpEntity()
{
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene)
        return {};

    for (auto& s_Brick : Globals::Hitman5Module->m_pEntitySceneContext->m_aLoadedBricks)
    {
        if (s_Brick.runtimeResourceID != ResId<"[assembly:/_sdk/get_up.brick].pc_entitytype">)
            continue;

        Logger::Debug("Found get_up brick.");

        const auto s_BpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_Brick.entityRef.GetBlueprintFactory());

        const auto s_Index = s_BpFactory->GetSubEntityIndex(0xfeedf42ba555b602);

        if (s_Index != -1)
            return s_BpFactory->GetSubEntity(s_Brick.entityRef.m_pEntity, s_Index);

        return {};
    }

    return {};
}

DECLARE_PLUGIN_DETOUR(Clumsy, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear)
{
    m_SlipStartTimer = -1.f;
    m_SlipStopTimer = -1.f;
    m_DeactivateRagdollQueued = false;

    return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(Clumsy);
