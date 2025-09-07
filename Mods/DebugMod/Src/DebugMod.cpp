#include "DebugMod.h"
#include "Hooks.h"
#include "Logging.h"

#include <winhttp.h>
#include <numbers>
#include <filesystem>

#include <imgui_internal.h>

#include <IconsMaterialDesign.h>

#include <Glacier/ZScene.h>
#include <Glacier/ZActor.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZKnowledge.h>

#include <Functions.h>
#include <Globals.h>

DebugMod::~DebugMod() {
    const ZMemberDelegate<DebugMod, void(const SGameUpdateEvent&)> s_Delegate(this, &DebugMod::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void DebugMod::Init() {
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &DebugMod::OnLoadScene);
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &DebugMod::OnClearScene);
}

void DebugMod::OnEngineInitialized() {
    const ZMemberDelegate<DebugMod, void(const SGameUpdateEvent&)> s_Delegate(this, &DebugMod::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void DebugMod::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
}

void DebugMod::OnDrawMenu() {
    if (ImGui::Button(ICON_MD_BUILD " DEBUG MENU")) {
        m_DebugMenuActive = !m_DebugMenuActive;
    }

    if (ImGui::Button(ICON_MD_PLACE " POSITIONS MENU")) {
        m_PositionsMenuActive = !m_PositionsMenuActive;
    }
}

void DebugMod::OnDrawUI(bool p_HasFocus) {
    DrawOptions(p_HasFocus);
    DrawPositionBox(p_HasFocus);
}

void DebugMod::DrawOptions(const bool p_HasFocus) {
    if (!p_HasFocus || !m_DebugMenuActive) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("DEBUG MENU", &m_DebugMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing) {

        if (ImGui::CollapsingHeader("Actors")) {
            ImGui::Checkbox("Render Actor position boxes", &m_RenderActorBoxes);
            ImGui::Checkbox("Render Actor names", &m_RenderActorNames);
            ImGui::Checkbox("Render Actor repository IDs", &m_RenderActorRepoIds);
            ImGui::Checkbox("Render Actor behaviors", &m_RenderActorBehaviors);
        }
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

void DebugMod::CopyToClipboard(const std::string& p_String) {
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

void DebugMod::OnDraw3D(IRenderer* p_Renderer) {
    if (m_RenderActorBoxes || m_RenderActorNames || m_RenderActorRepoIds || m_RenderActorBehaviors)
    {
        for (size_t i = 0; i < *Globals::NextActorId; ++i)
        {
            auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

            ZEntityRef s_Ref;
            s_Actor->GetID(&s_Ref);

            auto* s_SpatialEntity = s_Ref.QueryInterface<ZSpatialEntity>();

            auto s_Transform = s_SpatialEntity->GetWorldMatrix();

            if (m_RenderActorBoxes)
            {
                float4 s_Min, s_Max;

                s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

                p_Renderer->DrawOBB3D(
                    SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform,
                    SVector4(1.f, 0.f, 0.f, 1.f)
                );
            }

            if (m_RenderActorNames)
            {
                SVector2 s_ScreenPos;
                if (p_Renderer->WorldToScreen(
                    SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.05f), s_ScreenPos
                ))
                    p_Renderer->DrawText2D(s_Actor->m_sActorName, s_ScreenPos, SVector4(1.f, 0.f, 0.f, 1.f), 0.f, 0.5f);
            }

            if (m_RenderActorRepoIds)
            {
                auto* s_RepoEntity = s_Ref.QueryInterface<ZRepositoryItemEntity>();
                SVector2 s_ScreenPos;
                bool s_Success;

                if (m_RenderActorNames)
                    s_Success = p_Renderer->WorldToScreen(
                        SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.1f), s_ScreenPos
                    );
                else
                    s_Success = p_Renderer->WorldToScreen(
                        SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.05f), s_ScreenPos
                    );

                if (s_Success)
                    p_Renderer->DrawText2D(
                        s_RepoEntity->m_sId.ToString(), s_ScreenPos, SVector4(1.f, 0.f, 0.f, 1.f), 0.f, 0.5f
                    );
            }

            if (m_RenderActorBehaviors)
            {
                const SBehaviorBase* s_BehaviorBase = Globals::BehaviorService->m_aKnowledgeData[i].m_pCurrentBehavior;

                if (s_BehaviorBase)
                {
                    const ECompiledBehaviorType s_CompiledBehaviorType = static_cast<ECompiledBehaviorType>(s_BehaviorBase->m_Type);

                    SVector2 s_ScreenPos;
                    if (p_Renderer->WorldToScreen(
                        SVector3(s_Transform.mat[3].x, s_Transform.mat[3].y, s_Transform.mat[3].z + 2.05f), s_ScreenPos
                    ))
                        p_Renderer->DrawText2D(BehaviorToString(s_CompiledBehaviorType), s_ScreenPos, SVector4(1.f, 0.f, 0.f, 1.f), 0.f, 0.5f);
                }
            }
        }
    }
}

std::string DebugMod::BehaviorToString(ECompiledBehaviorType p_Type)
{
    switch (p_Type)
    {
    case ECompiledBehaviorType::BT_ConditionScope: return "BT_ConditionScope";
    case ECompiledBehaviorType::BT_Random: return "BT_Random";
    case ECompiledBehaviorType::BT_Match: return "BT_Match";
    case ECompiledBehaviorType::BT_Sequence: return "BT_Sequence";
    case ECompiledBehaviorType::BT_Dummy: return "BT_Dummy";
    case ECompiledBehaviorType::BT_Dummy2: return "BT_Dummy2";
    case ECompiledBehaviorType::BT_Error: return "BT_Error";
    case ECompiledBehaviorType::BT_Wait: return "BT_Wait";
    case ECompiledBehaviorType::BT_WaitForStanding: return "BT_WaitForStanding";
    case ECompiledBehaviorType::BT_WaitBasedOnDistanceToTarget: return "BT_WaitBasedOnDistanceToTarget";
    case ECompiledBehaviorType::BT_WaitForItemHandled: return "BT_WaitForItemHandled";
    case ECompiledBehaviorType::BT_AbandonOrder: return "BT_AbandonOrder";
    case ECompiledBehaviorType::BT_CompleteOrder: return "BT_CompleteOrder";
    case ECompiledBehaviorType::BT_PlayAct: return "BT_PlayAct";
    case ECompiledBehaviorType::BT_ConfiguredAct: return "BT_ConfiguredAct";
    case ECompiledBehaviorType::BT_PlayReaction: return "BT_PlayReaction";
    case ECompiledBehaviorType::BT_SimpleReaction: return "BT_SimpleReaction";
    case ECompiledBehaviorType::BT_SituationAct: return "BT_SituationAct";
    case ECompiledBehaviorType::BT_SituationApproach: return "BT_SituationApproach";
    case ECompiledBehaviorType::BT_SituationGetHelp: return "BT_SituationGetHelp";
    case ECompiledBehaviorType::BT_SituationFace: return "BT_SituationFace";
    case ECompiledBehaviorType::BT_SituationConversation: return "BT_SituationConversation";
    case ECompiledBehaviorType::BT_Holster: return "BT_Holster";
    case ECompiledBehaviorType::BT_SpeakWait: return "BT_SpeakWait";
    case ECompiledBehaviorType::BT_SpeakWaitWithFallbackIfAlone: return "BT_SpeakWaitWithFallbackIfAlone";
    case ECompiledBehaviorType::BT_ConfiguredSpeak: return "BT_ConfiguredSpeak";
    case ECompiledBehaviorType::BT_ConditionedConfiguredSpeak: return "BT_ConditionedConfiguredSpeak";
    case ECompiledBehaviorType::BT_ConditionedConfiguredAct: return "BT_ConditionedConfiguredAct";
    case ECompiledBehaviorType::BT_SpeakCustomOrDefaultDistractionAckSoundDef: return
        "BT_SpeakCustomOrDefaultDistractionAckSoundDef";
    case ECompiledBehaviorType::BT_SpeakCustomOrDefaultDistractionInvestigationSoundDef: return
        "BT_SpeakCustomOrDefaultDistractionInvestigationSoundDef";
    case ECompiledBehaviorType::BT_SpeakCustomOrDefaultDistractionStndSoundDef: return
        "BT_SpeakCustomOrDefaultDistractionStndSoundDef";
    case ECompiledBehaviorType::BT_Pickup: return "BT_Pickup";
    case ECompiledBehaviorType::BT_Drop: return "BT_Drop";
    case ECompiledBehaviorType::BT_PlayConversation: return "BT_PlayConversation";
    case ECompiledBehaviorType::BT_PlayAnimation: return "BT_PlayAnimation";
    case ECompiledBehaviorType::BT_MoveToLocation: return "BT_MoveToLocation";
    case ECompiledBehaviorType::BT_MoveToTargetKnownPosition: return "BT_MoveToTargetKnownPosition";
    case ECompiledBehaviorType::BT_MoveToTargetActualPosition: return "BT_MoveToTargetActualPosition";
    case ECompiledBehaviorType::BT_MoveToInteraction: return "BT_MoveToInteraction";
    case ECompiledBehaviorType::BT_MoveToNPC: return "BT_MoveToNPC";
    case ECompiledBehaviorType::BT_FollowTargetKnownPosition: return "BT_FollowTargetKnownPosition";
    case ECompiledBehaviorType::BT_FollowTargetActualPosition: return "BT_FollowTargetActualPosition";
    case ECompiledBehaviorType::BT_PickUpItem: return "BT_PickUpItem";
    case ECompiledBehaviorType::BT_GrabItem: return "BT_GrabItem";
    case ECompiledBehaviorType::BT_PutDownItem: return "BT_PutDownItem";
    case ECompiledBehaviorType::BT_Search: return "BT_Search";
    case ECompiledBehaviorType::BT_LimitedSearch: return "BT_LimitedSearch";
    case ECompiledBehaviorType::BT_MoveTo: return "BT_MoveTo";
    case ECompiledBehaviorType::BT_Reposition: return "BT_Reposition";
    case ECompiledBehaviorType::BT_SituationMoveTo: return "BT_SituationMoveTo";
    case ECompiledBehaviorType::BT_FormationMove: return "BT_FormationMove";
    case ECompiledBehaviorType::BT_SituationJumpTo: return "BT_SituationJumpTo";
    case ECompiledBehaviorType::BT_AmbientWalk: return "BT_AmbientWalk";
    case ECompiledBehaviorType::BT_AmbientStand: return "BT_AmbientStand";
    case ECompiledBehaviorType::BT_CrowdAmbientStand: return "BT_CrowdAmbientStand";
    case ECompiledBehaviorType::BT_AmbientItemUse: return "BT_AmbientItemUse";
    case ECompiledBehaviorType::BT_AmbientLook: return "BT_AmbientLook";
    case ECompiledBehaviorType::BT_Act: return "BT_Act";
    case ECompiledBehaviorType::BT_Patrol: return "BT_Patrol";
    case ECompiledBehaviorType::BT_MoveToPosition: return "BT_MoveToPosition";
    case ECompiledBehaviorType::BT_AlertedStand: return "BT_AlertedStand";
    case ECompiledBehaviorType::BT_AlertedDebug: return "BT_AlertedDebug";
    case ECompiledBehaviorType::BT_AttentionToPerson: return "BT_AttentionToPerson";
    case ECompiledBehaviorType::BT_StunnedByFlashGrenade: return "BT_StunnedByFlashGrenade";
    case ECompiledBehaviorType::BT_CuriousIdle: return "BT_CuriousIdle";
    case ECompiledBehaviorType::BT_InvestigateWeapon: return "BT_InvestigateWeapon";
    case ECompiledBehaviorType::BT_DeliverWeapon: return "BT_DeliverWeapon";
    case ECompiledBehaviorType::BT_RecoverUnconscious: return "BT_RecoverUnconscious";
    case ECompiledBehaviorType::BT_GetOutfit: return "BT_GetOutfit";
    case ECompiledBehaviorType::BT_RadioCall: return "BT_RadioCall";
    case ECompiledBehaviorType::BT_EscortOut: return "BT_EscortOut";
    case ECompiledBehaviorType::BT_StashItem: return "BT_StashItem";
    case ECompiledBehaviorType::BT_CautiousSearchPosition: return "BT_CautiousSearchPosition";
    case ECompiledBehaviorType::BT_LockdownWarning: return "BT_LockdownWarning";
    case ECompiledBehaviorType::BT_WakeUpUnconscious: return "BT_WakeUpUnconscious";
    case ECompiledBehaviorType::BT_DeadBodyInvestigate: return "BT_DeadBodyInvestigate";
    case ECompiledBehaviorType::BT_GuardDeadBody: return "BT_GuardDeadBody";
    case ECompiledBehaviorType::BT_DragDeadBody: return "BT_DragDeadBody";
    case ECompiledBehaviorType::BT_CuriousBystander: return "BT_CuriousBystander";
    case ECompiledBehaviorType::BT_DeadBodyBystander: return "BT_DeadBodyBystander";
    case ECompiledBehaviorType::BT_StandOffArrest: return "BT_StandOffArrest";
    case ECompiledBehaviorType::BT_StandOffReposition: return "BT_StandOffReposition";
    case ECompiledBehaviorType::BT_StandAndAim: return "BT_StandAndAim";
    case ECompiledBehaviorType::BT_CloseCombat: return "BT_CloseCombat";
    case ECompiledBehaviorType::BT_MoveToCloseCombat: return "BT_MoveToCloseCombat";
    case ECompiledBehaviorType::BT_MoveAwayFromCloseCombat: return "BT_MoveAwayFromCloseCombat";
    case ECompiledBehaviorType::BT_CoverFightSeasonTwo: return "BT_CoverFightSeasonTwo";
    case ECompiledBehaviorType::BT_ShootFromPosition: return "BT_ShootFromPosition";
    case ECompiledBehaviorType::BT_StandAndShoot: return "BT_StandAndShoot";
    case ECompiledBehaviorType::BT_CheckLastPosition: return "BT_CheckLastPosition";
    case ECompiledBehaviorType::BT_ProtoSearchIdle: return "BT_ProtoSearchIdle";
    case ECompiledBehaviorType::BT_ProtoApproachSearchArea: return "BT_ProtoApproachSearchArea";
    case ECompiledBehaviorType::BT_ProtoSearchPosition: return "BT_ProtoSearchPosition";
    case ECompiledBehaviorType::BT_ShootTarget: return "BT_ShootTarget";
    case ECompiledBehaviorType::BT_TriggerAlarm: return "BT_TriggerAlarm";
    case ECompiledBehaviorType::BT_MoveInCover: return "BT_MoveInCover";
    case ECompiledBehaviorType::BT_MoveToCover: return "BT_MoveToCover";
    case ECompiledBehaviorType::BT_HomeAttackOrigin: return "BT_HomeAttackOrigin";
    case ECompiledBehaviorType::BT_Shoot: return "BT_Shoot";
    case ECompiledBehaviorType::BT_Aim: return "BT_Aim";
    case ECompiledBehaviorType::BT_MoveToRandomNeighbourNode: return "BT_MoveToRandomNeighbourNode";
    case ECompiledBehaviorType::BT_MoveToRandomNeighbourNodeAiming: return "BT_MoveToRandomNeighbourNodeAiming";
    case ECompiledBehaviorType::BT_MoveToAndPlayCombatPositionAct: return "BT_MoveToAndPlayCombatPositionAct";
    case ECompiledBehaviorType::BT_MoveToAimingAndPlayCombatPositionAct: return
        "BT_MoveToAimingAndPlayCombatPositionAct";
    case ECompiledBehaviorType::BT_PlayJumpyReaction: return "BT_PlayJumpyReaction";
    case ECompiledBehaviorType::BT_JumpyInvestigation: return "BT_JumpyInvestigation";
    case ECompiledBehaviorType::BT_AgitatedPatrol: return "BT_AgitatedPatrol";
    case ECompiledBehaviorType::BT_AgitatedGuard: return "BT_AgitatedGuard";
    case ECompiledBehaviorType::BT_HeroEscort: return "BT_HeroEscort";
    case ECompiledBehaviorType::BT_Escort: return "BT_Escort";
    case ECompiledBehaviorType::BT_ControlledFormationMove: return "BT_ControlledFormationMove";
    case ECompiledBehaviorType::BT_EscortSearch: return "BT_EscortSearch";
    case ECompiledBehaviorType::BT_LeadEscort: return "BT_LeadEscort";
    case ECompiledBehaviorType::BT_LeadEscort2: return "BT_LeadEscort2";
    case ECompiledBehaviorType::BT_AimReaction: return "BT_AimReaction";
    case ECompiledBehaviorType::BT_FollowHitman: return "BT_FollowHitman";
    case ECompiledBehaviorType::BT_RideTheLightning: return "BT_RideTheLightning";
    case ECompiledBehaviorType::BT_Scared: return "BT_Scared";
    case ECompiledBehaviorType::BT_Flee: return "BT_Flee";
    case ECompiledBehaviorType::BT_AgitatedBystander: return "BT_AgitatedBystander";
    case ECompiledBehaviorType::BT_SentryFrisk: return "BT_SentryFrisk";
    case ECompiledBehaviorType::BT_SentryIdle: return "BT_SentryIdle";
    case ECompiledBehaviorType::BT_SentryWarning: return "BT_SentryWarning";
    case ECompiledBehaviorType::BT_SentryCheckItem: return "BT_SentryCheckItem";
    case ECompiledBehaviorType::BT_VIPScared: return "BT_VIPScared";
    case ECompiledBehaviorType::BT_VIPSafeRoomTrespasser: return "BT_VIPSafeRoomTrespasser";
    case ECompiledBehaviorType::BT_DefendVIP: return "BT_DefendVIP";
    case ECompiledBehaviorType::BT_CautiousVIP: return "BT_CautiousVIP";
    case ECompiledBehaviorType::BT_CautiousGuardVIP: return "BT_CautiousGuardVIP";
    case ECompiledBehaviorType::BT_InfectedConfused: return "BT_InfectedConfused";
    case ECompiledBehaviorType::BT_EnterInfected: return "BT_EnterInfected";
    case ECompiledBehaviorType::BT_CureInfected: return "BT_CureInfected";
    case ECompiledBehaviorType::BT_SickActInfected: return "BT_SickActInfected";
    case ECompiledBehaviorType::BT_Smart: return "BT_Smart";
    case ECompiledBehaviorType::BT_Controlled: return "BT_Controlled";
    case ECompiledBehaviorType::BT_SpeakTest: return "BT_SpeakTest";
    case ECompiledBehaviorType::BT_Conversation: return "BT_Conversation";
    case ECompiledBehaviorType::BT_RunToHelp: return "BT_RunToHelp";
    case ECompiledBehaviorType::BT_WaitForDialog: return "BT_WaitForDialog";
    case ECompiledBehaviorType::BT_WaitForConfiguredAct: return "BT_WaitForConfiguredAct";
    case ECompiledBehaviorType::BT_TestFlashbangGrenadeThrow: return "BT_TestFlashbangGrenadeThrow";
    case ECompiledBehaviorType::BT_BEHAVIORS_END: return "BT_BEHAVIORS_END";
    case ECompiledBehaviorType::BT_RenewEvent: return "BT_RenewEvent";
    case ECompiledBehaviorType::BT_ExpireEvent: return "BT_ExpireEvent";
    case ECompiledBehaviorType::BT_ExpireEvents: return "BT_ExpireEvents";
    case ECompiledBehaviorType::BT_SetEventHandled: return "BT_SetEventHandled";
    case ECompiledBehaviorType::BT_RenewSharedEvent: return "BT_RenewSharedEvent";
    case ECompiledBehaviorType::BT_ExpireSharedEvent: return "BT_ExpireSharedEvent";
    case ECompiledBehaviorType::BT_ExpireAllEvents: return "BT_ExpireAllEvents";
    case ECompiledBehaviorType::BT_CreateOrJoinSituation: return "BT_CreateOrJoinSituation";
    case ECompiledBehaviorType::BT_JoinSituation: return "BT_JoinSituation";
    case ECompiledBehaviorType::BT_ForceActorToJoinSituation: return "BT_ForceActorToJoinSituation";
    case ECompiledBehaviorType::BT_JoinSituationWithActor: return "BT_JoinSituationWithActor";
    case ECompiledBehaviorType::BT_LeaveSituation: return "BT_LeaveSituation";
    case ECompiledBehaviorType::BT_Escalate: return "BT_Escalate";
    case ECompiledBehaviorType::BT_GotoPhase: return "BT_GotoPhase";
    case ECompiledBehaviorType::BT_RenewGoal: return "BT_RenewGoal";
    case ECompiledBehaviorType::BT_ExpireGoal: return "BT_ExpireGoal";
    case ECompiledBehaviorType::BT_RenewGoalOf: return "BT_RenewGoalOf";
    case ECompiledBehaviorType::BT_ExpireGoalOf: return "BT_ExpireGoalOf";
    case ECompiledBehaviorType::BT_SetTension: return "BT_SetTension";
    case ECompiledBehaviorType::BT_TriggerSpotted: return "BT_TriggerSpotted";
    case ECompiledBehaviorType::BT_CopyKnownLocation: return "BT_CopyKnownLocation";
    case ECompiledBehaviorType::BT_UpdateKnownLocation: return "BT_UpdateKnownLocation";
    case ECompiledBehaviorType::BT_TransferKnownObjectPositions: return "BT_TransferKnownObjectPositions";
    case ECompiledBehaviorType::BT_WitnessAttack: return "BT_WitnessAttack";
    case ECompiledBehaviorType::BT_Speak: return "BT_Speak";
    case ECompiledBehaviorType::BT_StartDynamicEnforcer: return "BT_StartDynamicEnforcer";
    case ECompiledBehaviorType::BT_StopDynamicEnforcer: return "BT_StopDynamicEnforcer";
    case ECompiledBehaviorType::BT_StartRangeBasedDynamicEnforcer: return "BT_StartRangeBasedDynamicEnforcer";
    case ECompiledBehaviorType::BT_StopRangeBasedDynamicEnforcerForLocation: return
        "BT_StopRangeBasedDynamicEnforcerForLocation";
    case ECompiledBehaviorType::BT_StopRangeBasedDynamicEnforcer: return "BT_StopRangeBasedDynamicEnforcer";
    case ECompiledBehaviorType::BT_SetDistracted: return "BT_SetDistracted";
    case ECompiledBehaviorType::BT_IgnoreAllDistractionsExceptTheNewest: return
        "BT_IgnoreAllDistractionsExceptTheNewest";
    case ECompiledBehaviorType::BT_IgnoreDistractions: return "BT_IgnoreDistractions";
    case ECompiledBehaviorType::BT_PerceptibleEntityNotifyWillReact: return "BT_PerceptibleEntityNotifyWillReact";
    case ECompiledBehaviorType::BT_PerceptibleEntityNotifyReacted: return "BT_PerceptibleEntityNotifyReacted";
    case ECompiledBehaviorType::BT_PerceptibleEntityNotifyInvestigating: return
        "BT_PerceptibleEntityNotifyInvestigating";
    case ECompiledBehaviorType::BT_PerceptibleEntityNotifyInvestigated: return
        "BT_PerceptibleEntityNotifyInvestigated";
    case ECompiledBehaviorType::BT_PerceptibleEntityNotifyTerminate: return "BT_PerceptibleEntityNotifyTerminate";
    case ECompiledBehaviorType::BT_LeaveDistractionAssistantRole: return "BT_LeaveDistractionAssistantRole";
    case ECompiledBehaviorType::BT_LeaveDistractionAssitingGuardRole: return "BT_LeaveDistractionAssitingGuardRole";
    case ECompiledBehaviorType::BT_RequestSuitcaseAssistanceOverRadio: return
        "BT_RequestSuitcaseAssistanceOverRadio";
    case ECompiledBehaviorType::BT_RequestSuitcaseAssistanceFaceToFace: return
        "BT_RequestSuitcaseAssistanceFaceToFace";
    case ECompiledBehaviorType::BT_ExpireArrestReasons: return "BT_ExpireArrestReasons";
    case ECompiledBehaviorType::BT_SetDialogSwitch_NPCID: return "BT_SetDialogSwitch_NPCID";
    case ECompiledBehaviorType::BT_InfectedAssignToFollowPlayer: return "BT_InfectedAssignToFollowPlayer";
    case ECompiledBehaviorType::BT_InfectedRemoveFromFollowPlayer: return "BT_InfectedRemoveFromFollowPlayer";
    case ECompiledBehaviorType::BT_Log: return "BT_Log";
    case ECompiledBehaviorType::BT_COMMANDS_END: return "BT_COMMANDS_END";
    case ECompiledBehaviorType::BT_Invalid: return "BT_Invalid";
    default: return "<unknown>";
    }
}

DEFINE_PLUGIN_DETOUR(DebugMod, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData&) {
    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(DebugMod, void, OnClearScene, ZEntitySceneContext* th, bool forReload) {
    return HookResult<void>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(DebugMod);
