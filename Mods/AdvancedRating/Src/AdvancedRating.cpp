#include "AdvancedRating.h"

#include "Hooks.h"
#include "Logging.h"
#include "Functions.h"

#include <Glacier/ZScene.h>
#include <Glacier/ZAIGameState.h>
#include <Glacier/SOnlineEvent.h>

#include "json.hpp"

void AdvancedRating::PreInit()
{
	InitializeSRWLock(&m_EventLock);

	// Register all the rating events. New events should be added here.
	RegisterEvent(RatingEventType::RecordingsRemoved, -6);
	RegisterEvent(RatingEventType::WitnessEliminatedAccident, -2);
	RegisterEvent(RatingEventType::WitnessEliminatedMurder, -1);
	RegisterEvent(RatingEventType::ActorPacified, 1);
	RegisterEvent(RatingEventType::CaughtTrespassing, 2);
	RegisterEvent(RatingEventType::GunshotHeard, 3);
	RegisterEvent(RatingEventType::BulletImpactNoticed, 3);
	RegisterEvent(RatingEventType::UnconsciousBodyFound, 4);
	RegisterEvent(RatingEventType::AlarmTriggered, 5);
	RegisterEvent(RatingEventType::GuardsAlerted, 5);
	RegisterEvent(RatingEventType::DeadBodyFound, 6);
	RegisterEvent(RatingEventType::GuardKilled, 6);
	RegisterEvent(RatingEventType::CaughtOnCamera, 6);
	RegisterEvent(RatingEventType::CaughtCommitingCrime, 7);
	RegisterEvent(RatingEventType::CivilianKilled, 8);
	
	Hooks::ZGameStatsManager_SendAISignals01->AddDetour(this, &AdvancedRating::ZGameStatsManager_SendAISignals);
	Hooks::ZGameStatsManager_SendAISignals02->AddDetour(this, &AdvancedRating::ZGameStatsManager_SendAISignals);
	Hooks::ZAchievementManagerSimple_OnEventSent->AddDetour(this, &AdvancedRating::ZAchievementManagerSimple_OnEventSent);
}

void AdvancedRating::OnDrawUI(bool p_HasFocus)
{
	char s_CurrentRating[256];
	sprintf_s(s_CurrentRating, sizeof(s_CurrentRating), "RATING: %s (%lld)###AdvancedRating", GetCurrentRating().c_str(), m_CurrentPoints);
	
	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	const auto s_WindowExpanded = ImGui::Begin(s_CurrentRating, nullptr);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	if (s_WindowExpanded)
	{
		ImGui::BeginTable("RatingTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY);

		AcquireSRWLockShared(&m_EventLock);

		for (auto& s_Event : m_EventHistory)
		{
			auto s_TypeName = s_Event.TypeToString();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(s_TypeName.c_str(), s_TypeName.c_str() + s_TypeName.size());
			ImGui::TableNextColumn();
			ImGui::Text("%lld", s_Event.Points);
		}

		ReleaseSRWLockShared(&m_EventLock);

		ImGui::EndTable();
	}

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}

void AdvancedRating::OnEvent(RatingEventType p_EventType)
{
	const auto s_Event = m_RegisteredEvents[p_EventType];

	AcquireSRWLockExclusive(&m_EventLock);
	
	m_CurrentPoints += s_Event.Points;
	m_EventHistory.push_back(s_Event);

	if (m_CurrentPoints < 0)
		m_CurrentPoints = 0;

	ReleaseSRWLockExclusive(&m_EventLock);
}

ZString AdvancedRating::GetCurrentRating() const
{
	if (m_CurrentPoints < 10)
		return "Silent Assassin";

	if (m_CurrentPoints < 20)
		return "Professional";
	
	if (m_CurrentPoints < 30)
		return "Expert";

	if (m_CurrentPoints < 40)
		return "Contract Killer";

	if (m_CurrentPoints < 50)
		return "Thug";

	if (m_CurrentPoints < 60)
		return "Loose Cannon";

	if (m_CurrentPoints < 100)
		return "Madman";
	
	return "Mass Murderer";
}

void AdvancedRating::RegisterEvent(RatingEventType p_EventType, int64_t p_Points)
{
	m_RegisteredEvents[p_EventType] = RatingEvent { p_EventType, p_Points };
}

void AdvancedRating::Reset()
{
	AcquireSRWLockExclusive(&m_EventLock);

	m_CurrentPoints = 0;
	m_EventHistory.clear();
	
	ReleaseSRWLockExclusive(&m_EventLock);
}

DECLARE_PLUGIN_DETOUR(AdvancedRating, void, ZAchievementManagerSimple_OnEventSent, ZAchievementManagerSimple* th, uint32_t eventId, const ZDynamicObject& event)
{
	ZString s_EventData;
	Functions::ZDynamicObject_ToString->Call(const_cast<ZDynamicObject*>(&event), &s_EventData);

	Logger::Debug("Achievement Event Sent: {} - {}", eventId, s_EventData);

	auto s_JsonEvent = nlohmann::json::parse(s_EventData.c_str(), s_EventData.c_str() + s_EventData.size());

	const std::string s_EventName = s_JsonEvent["Name"];
	
	if (s_EventName == "SecuritySystemRecorder")
	{
		if (s_JsonEvent["Value"]["event"] == "spotted")
			OnEvent(RatingEventType::CaughtOnCamera);
		else if (s_JsonEvent["Value"]["event"] == "destroyed")
			OnEvent(RatingEventType::RecordingsRemoved);
	}
	else if (s_EventName == "Kill")
	{
		const bool s_Target = s_JsonEvent["Value"]["IsTarget"];
		const std::string s_ID = s_JsonEvent["Value"]["RepositoryId"];
		const auto s_ActorType = static_cast<EActorType>(s_JsonEvent["Value"]["ActorType"]);

		if (!s_Target && s_ActorType == EActorType::eAT_Civilian)
			OnEvent(RatingEventType::CivilianKilled);

		if (!s_Target && s_ActorType == EActorType::eAT_Guard)
			OnEvent(RatingEventType::GuardKilled);
	}
	else if (s_EventName == "Pacify")
	{
		const bool s_Target = s_JsonEvent["Value"]["IsTarget"];
		const std::string s_ID = s_JsonEvent["Value"]["RepositoryId"];
		const auto s_ActorType = static_cast<EActorType>(s_JsonEvent["Value"]["ActorType"]);

		if (!s_Target)
			OnEvent(RatingEventType::ActorPacified);
	}
	else if (s_EventName == "ContractStart")
	{
		Reset();
	}
	
	return HookResult<void>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(AdvancedRating, void, ZGameStatsManager_SendAISignals, ZGameStatsManager* th)
{
	if (!th->m_oldGameState.m_bHitmanTrespassingSpotted && th->m_gameState.m_bHitmanTrespassingSpotted)
		OnEvent(RatingEventType::CaughtTrespassing);

	if (!th->m_oldGameState.m_bBodyFound && th->m_gameState.m_bBodyFound && th->m_gameState.m_bBodyFoundPacify)
		OnEvent(RatingEventType::UnconsciousBodyFound);
	
	if (!th->m_oldGameState.m_bBodyFound && th->m_gameState.m_bBodyFound && th->m_gameState.m_bBodyFoundMurder)
		OnEvent(RatingEventType::DeadBodyFound);

	if (!th->m_oldGameState.m_bDeadBodySeen && th->m_gameState.m_bDeadBodySeen)
		OnEvent(RatingEventType::DeadBodyFound);

	if (th->m_oldGameState.m_actorCounts.m_nEnemiesIsAlerted == 0 && th->m_gameState.m_actorCounts.m_nEnemiesIsAlerted > 0)
		OnEvent(RatingEventType::GuardsAlerted);

	return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(AdvancedRating);
