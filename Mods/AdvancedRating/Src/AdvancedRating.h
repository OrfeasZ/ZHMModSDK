#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

class AdvancedRating : public IPluginInterface
{
private:
	enum class RatingEventType
	{
		RecordingsRemoved,
		WitnessEliminatedAccident,
		WitnessEliminatedMurder,
		ActorPacified,
		CaughtTrespassing,
		GunshotHeard,
		BulletImpactNoticed,
		UnconsciousBodyFound,
		AlarmTriggered,
		GuardsAlerted,
		DeadBodyFound,
		GuardKilled,
		CaughtOnCamera,
		CaughtCommitingCrime,
		CivilianKilled,
	};
	
	struct RatingEvent
	{
		RatingEventType Type;
		int64_t Points;

		ZString TypeToString() const
		{
			switch (Type)
			{
				case RatingEventType::RecordingsRemoved:
					return "RecordingsRemoved";
				case RatingEventType::WitnessEliminatedAccident:
					return "WitnessEliminatedAccident";
				case RatingEventType::WitnessEliminatedMurder:
					return "WitnessEliminatedMurder";
				case RatingEventType::ActorPacified:
					return "ActorPacified";
				case RatingEventType::CaughtTrespassing:
					return "CaughtTrespassing";
				case RatingEventType::GunshotHeard:
					return "GunshotHeard";
				case RatingEventType::BulletImpactNoticed:
					return "BulletImpactNoticed";
				case RatingEventType::UnconsciousBodyFound:
					return "UnconsciousBodyFound";
				case RatingEventType::AlarmTriggered:
					return "AlarmTriggered";
				case RatingEventType::GuardsAlerted:
					return "GuardsAlerted";
				case RatingEventType::DeadBodyFound:
					return "DeadBodyFound";
				case RatingEventType::GuardKilled:
					return "GuardKilled";
				case RatingEventType::CaughtOnCamera:
					return "CaughtOnCamera";
				case RatingEventType::CaughtCommitingCrime:
					return "CaughtCommitingCrime";
				case RatingEventType::CivilianKilled:
					return "CivilianKilled";
			}
			
			return "Unknown";
		}
	};
	
public:
	void Init() override;
	void OnDrawUI(bool p_HasFocus) override;

private:
	void OnEvent(RatingEventType p_EventType);
	ZString GetCurrentRating() const;
	void RegisterEvent(RatingEventType p_EventType, int64_t p_Points);
	void Reset();

private:
	DEFINE_PLUGIN_DETOUR(AdvancedRating, void, ZGameStatsManager_SendAISignals, ZGameStatsManager* th);
	DEFINE_PLUGIN_DETOUR(AdvancedRating, void, ZAchievementManagerSimple_OnEventSent, ZAchievementManagerSimple* th, uint32_t eventIndex, const ZDynamicObject& event);

private:
	SRWLOCK m_EventLock = {};
	int64_t m_CurrentPoints = 0;
	std::unordered_map<RatingEventType, RatingEvent> m_RegisteredEvents;
	std::vector<RatingEvent> m_EventHistory;
};

DEFINE_ZHM_PLUGIN(AdvancedRating)
