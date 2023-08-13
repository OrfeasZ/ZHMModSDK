#include "Clumsy.h"

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/SGameUpdateEvent.h>

#include "IconsMaterialDesign.h"
#include "Glacier/ZGeomEntity.h"
#include "Glacier/ZKnowledge.h"
#include "Glacier/ZModule.h"

Clumsy::Clumsy()
{
}

Clumsy::~Clumsy()
{
    const ZMemberDelegate<Clumsy, void(const SGameUpdateEvent&)> s_Delegate(this, &Clumsy::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    if (m_AudioEngine)
        m_AudioEngine->Suspend();

    m_MusicLoop.reset();
}

void Clumsy::OnEngineInitialized()
{
    m_AudioEngine = std::make_unique<DirectX::AudioEngine>(DirectX::AudioEngine_Default);
    m_Music = std::make_unique<DirectX::SoundEffect>(m_AudioEngine.get(), L"dab.wav");
    m_MusicLoop = m_Music->CreateInstance();

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
    m_AudioEngine->Update();

    if (m_Ragdolling)
        m_RagdollTimer += p_UpdateEvent.m_GameTimeDelta.ToSeconds();

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

        // TODO: Move hitman to floor using raycast.
    }
    else if (m_Ragdolling)
    {
        /*TEntityRef<ZHitman5> s_LocalHitman;
        Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

        if (!s_LocalHitman)
            return;

        // Disable player movement after 1.5s.
        /*if (m_RagdollTimer > 1.5f)
            Functions::ZHM5BaseCharacter_ActivateRagdoll->Call(s_LocalHitman.m_pInterfaceRef, true);#1#

        const auto s_NewPos = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>()->GetWorldMatrix().Trans;
        const auto s_InstantVelocity = s_NewPos - m_LastPos;

        m_SampledVelocitySum += s_InstantVelocity;

        if (m_VelocitySamples < VelocitiesToAverage)
        {
            m_SampledVelocities[m_VelocitySamples++] = s_InstantVelocity;
        }
        else
        {
            auto& s_OldestSample = m_SampledVelocities[m_VelocitySamples++ % VelocitiesToAverage];
            m_SampledVelocitySum -= s_OldestSample;
            s_OldestSample = s_InstantVelocity;
        }

        if (m_VelocitySamples > 100)
        {
            const auto s_MovingAverage = m_SampledVelocitySum / min(VelocitiesToAverage, m_VelocitySamples);

            // Disable ragdoll once 47 has stopped moving.
            if (fabs(s_MovingAverage.x) < 0.0001f && fabs(s_MovingAverage.y) <= 0.0001f && fabs(s_MovingAverage.z) <= 0.0001f)
            {
                m_Ragdolling = false;
                m_RagdollTimer = 0.f;

                //if (m_GetUpAnimation)
                //    m_GetUpAnimation.SignalInputPin("Start");

                // We queue the ragdoll de-activation for the next frame update because if we do it
                // immediately, the animation wouldn't have started playing yet, and we would get a
                // frame of 47 standing up before the animation
                m_DeactivateRagdollQueued = true;
            }
        }

        m_LastPos = s_NewPos;*/
    }

    /*ZInputAction s_RagdollAction("eIAKBMCover");
    if (Functions::ZInputAction_Digital->Call(&s_RagdollAction, -1))
    {
        TEntityRef<ZHitman5> s_LocalHitman;
        Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

        if (!s_LocalHitman)
            return;

        if (!m_Ragdolling && !m_DeactivateRagdollQueued)
        {
            m_GetUpAnimation = GetGetUpEntity();

            if (!m_GetUpAnimation)
            {
                m_ShowBrickWarning = true;
                return;
            }

            Logger::Debug("Activating ragdoll.");
            Functions::ZHM5BaseCharacter_ActivatePoweredRagdoll->Call(s_LocalHitman.m_pInterfaceRef, 0.3f, true, false, 0.15f, false);

            // Fly!
            for (int i = 0; i < 117; ++i)
            {
                if (i == 1)
                    Functions::ZRagdollHandler_ApplyImpulseOnRagdoll->Call(s_LocalHitman.m_pInterfaceRef->m_pRagdollHandler, float4(0, 0, 0, 0), float4(0, 0, 300, 1), i, false);
            }

            m_DeactivateRagdollQueued = false;
            m_Ragdolling = true;
            m_RagdollTimer = 0.f;
            m_VelocitySamples = 0;
            m_SampledVelocities = {};
            m_SampledVelocitySum = {};

            m_LastPos = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>()->GetWorldMatrix().Trans;
        }
        else if (m_Ragdolling)
        {
            m_Ragdolling = false;
            m_RagdollTimer = 0.f;

            if (m_GetUpAnimation)
                m_GetUpAnimation.SignalInputPin("Start");

            // We queue the ragdoll de-activation for the next frame update because if we do it
            // immediately, the animation wouldn't have started playing yet, and we would get a
            // frame of 47 standing up before the animation
            m_DeactivateRagdollQueued = true;
        }
    }*/

    /*ZInputAction s_RagdollAction("eIAKBMCover");
    if (Functions::ZInputAction_Digital->Call(&s_RagdollAction, -1))
    {
        TEntityRef<ZHitman5> s_LocalHitman;
        Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

        if (!s_LocalHitman)
            return;

        if (!m_Ragdolling)
        {
            Functions::ZHM5BaseCharacter_ActivatePoweredRagdoll->Call(s_LocalHitman.m_pInterfaceRef, 0.3f, true, true, 0.0f, false);
        }
        else
        {
            Functions::ZHM5BaseCharacter_DeactivateRagdoll->Call(s_LocalHitman.m_pInterfaceRef);
        }

        m_Ragdolling = !m_Ragdolling;
    }*/

    ZInputAction s_RagdollAction("eIAKBMCover");
    if (Functions::ZInputAction_Digital->Call(&s_RagdollAction, -1))
    {
        TEntityRef<ZHitman5> s_LocalHitman;
        Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

        if (!s_LocalHitman)
            return;

        if (!GetEntities())
            return;

        m_Ragdolling = !m_Ragdolling;
        Logger::Debug("Ragdolling: {}", m_Ragdolling);

        if (m_Ragdolling)
        {
            Functions::ZHM5BaseCharacter_ActivatePoweredRagdoll->Call(s_LocalHitman.m_pInterfaceRef, 0, false, true, 0, true);

            const auto s_EmitterSpatial = m_MusicEmitter.QueryInterface<ZSpatialEntity>();

            if (s_EmitterSpatial)
                s_EmitterSpatial->SetWorldMatrix(s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>()->GetWorldMatrix());

            m_ShakeEntity.SignalInputPin("Activate");
            m_MusicLoop->Play(true);
        }
        else
        {
            Functions::ZHM5BaseCharacter_DeactivateRagdoll->Call(s_LocalHitman.m_pInterfaceRef);
            m_ShakeEntity.SignalInputPin("Deactivate");
            m_MusicLoop->Stop(true);
        }
    }

    if (m_Ragdolling)
    {
        TEntityRef<ZHitman5> s_LocalHitman;
        Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

        if (!s_LocalHitman)
            return;

        const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
        const auto s_HitmanTransform = s_HitmanSpatial->GetWorldMatrix();

        for (int i = 0; i < *Globals::NextActorId; ++i)
        {
            const auto& s_Actor = Globals::ActorManager->m_aActiveActors[i];

            const auto s_ActorSpatial = s_Actor.m_ref.QueryInterface<ZSpatialEntity>();

            if (!s_ActorSpatial)
                continue;

            const auto s_ActorTrans = s_ActorSpatial->GetWorldMatrix();

            if (float4::Distance(s_ActorTrans.Trans, s_HitmanTransform.Trans) < 4.f)
            {
                Functions::ZHM5BaseCharacter_ActivateRagdoll->Call(s_Actor.m_pInterfaceRef, true);

                // pelvis = 1
                Functions::ZRagdollHandler_ApplyImpulseOnRagdoll->Call(s_Actor.m_pInterfaceRef->m_pRagdollHandler, {}, s_HitmanTransform.XAxis * 22.f + float4 { 0, 0, 10.f, 0 }, 1, false);
            }
        }

        const auto s_EmitterSpatial = m_MusicEmitter.QueryInterface<ZSpatialEntity>();

        if (s_EmitterSpatial)
            s_EmitterSpatial->SetWorldMatrix(s_HitmanTransform);

        // head = 11
        // l_hand = 128
        // r_hand = 157
        //Functions::ZHM5BaseCharacter_ActivatePoweredRagdoll->Call(s_LocalHitman.m_pInterfaceRef, 0, false, true, 0.6f, false);
        Functions::ZRagdollHandler_ApplyImpulseOnRagdoll->Call(s_LocalHitman.m_pInterfaceRef->m_pRagdollHandler, {}, s_HitmanTransform.Up + s_HitmanTransform.Left + s_HitmanTransform.Backward * -1, 128, false);
        Functions::ZRagdollHandler_ApplyImpulseOnRagdoll->Call(s_LocalHitman.m_pInterfaceRef->m_pRagdollHandler, {}, s_HitmanTransform.Up * 0.5 + s_HitmanTransform.Left * 0.4 + s_HitmanTransform.Backward * -1, 157, false);
        Functions::ZRagdollHandler_ApplyImpulseOnRagdoll->Call(s_LocalHitman.m_pInterfaceRef->m_pRagdollHandler, {}, s_HitmanTransform.Up + s_HitmanTransform.Left * -0.8 + s_HitmanTransform.Backward * -1.5, 11, false);
    }
}

bool Clumsy::GetEntities()
{
    if (m_GetUpAnimation && m_ShakeEntity)
        return true;

    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene)
        return false;

    m_GetUpAnimation = {};
    m_ShakeEntity = {};
    m_MusicEntity = {};
    m_MusicEmitter = {};

    for (auto& s_Brick : Globals::Hitman5Module->m_pEntitySceneContext->m_aLoadedBricks)
    {
        if (s_Brick.runtimeResourceID != ResId<"[assembly:/_sdk/get_up.brick].pc_entitytype">)
            continue;

        Logger::Debug("Found get_up brick.");

        const auto s_BpFactory = reinterpret_cast<ZTemplateEntityBlueprintFactory*>(s_Brick.entityRef.GetBlueprintFactory());

        if (const auto s_Index = s_BpFactory->GetSubEntityIndex(0xfeedf42ba555b602); s_Index != -1)
            m_GetUpAnimation = s_BpFactory->GetSubEntity(s_Brick.entityRef.m_pEntity, s_Index);

        if (const auto s_Index = s_BpFactory->GetSubEntityIndex(0xfeed8cfffcae85dd); s_Index != -1)
            m_ShakeEntity = s_BpFactory->GetSubEntity(s_Brick.entityRef.m_pEntity, s_Index);

        if (const auto s_Index = s_BpFactory->GetSubEntityIndex(0xfeed6ea2fc060cbd); s_Index != -1)
            m_MusicEntity = s_BpFactory->GetSubEntity(s_Brick.entityRef.m_pEntity, s_Index);

        if (const auto s_Index = s_BpFactory->GetSubEntityIndex(0xfeed9c1aab9f3d1e); s_Index != -1)
            m_MusicEmitter = s_BpFactory->GetSubEntity(s_Brick.entityRef.m_pEntity, s_Index);

        break;
    }

    return m_GetUpAnimation && m_ShakeEntity;
}

DEFINE_PLUGIN_DETOUR(Clumsy, void, OnClearScene, ZEntitySceneContext* th, bool forReload)
{
    m_MusicLoop->Stop(true);

    m_DeactivateRagdollQueued = false;
    m_Ragdolling = false;
    m_RagdollTimer = 0.f;
    m_VelocitySamples = 0;
    m_SampledVelocities = {};
    m_SampledVelocitySum = {};

    m_GetUpAnimation = {};
    m_ShakeEntity = {};
    m_MusicEntity = {};
    m_MusicEmitter = {};

    return HookResult<void>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(Clumsy);
