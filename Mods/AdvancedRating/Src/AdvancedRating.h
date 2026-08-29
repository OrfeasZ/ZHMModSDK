#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

class AdvancedRating : public IPluginInterface {
private:
    enum class RatingEventType {
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

    struct RatingEvent {
        RatingEventType Type;
        int64_t Points;

        ZString TypeToString() const {
            switch (Type) {
                case RatingEventType::RecordingsRemoved:
                    return "Recordings removed";
                case RatingEventType::WitnessEliminatedAccident:
                    return "Witness eliminated accident";
                case RatingEventType::WitnessEliminatedMurder:
                    return "Witness eliminated murder";
                case RatingEventType::ActorPacified:
                    return "Actor pacified";
                case RatingEventType::CaughtTrespassing:
                    return "Caught trespassing";
                case RatingEventType::GunshotHeard:
                    return "Gunshot heard";
                case RatingEventType::BulletImpactNoticed:
                    return "Bullet impact noticed";
                case RatingEventType::UnconsciousBodyFound:
                    return "Unconscious body found";
                case RatingEventType::AlarmTriggered:
                    return "Alarm triggered";
                case RatingEventType::GuardsAlerted:
                    return "Guards alerted";
                case RatingEventType::DeadBodyFound:
                    return "Dead body found";
                case RatingEventType::GuardKilled:
                    return "Guard killed";
                case RatingEventType::CaughtOnCamera:
                    return "Caught on camera";
                case RatingEventType::CaughtCommitingCrime:
                    return "Caught commiting crime";
                case RatingEventType::CivilianKilled:
                    return "Civilian killed";
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
    DECLARE_PLUGIN_DETOUR(AdvancedRating, void, ZGameStatsManager_SendAISignals, ZGameStatsManager* th);
    DECLARE_PLUGIN_DETOUR(
        AdvancedRating, void, ZAchievementManagerSimple_OnEventSent, ZAchievementManagerSimple* th, uint32_t eventIndex,
        const ZDynamicObject& event
    );

private:
    SRWLOCK m_EventLock = {};
    int64_t m_CurrentPoints = 0;
    std::unordered_map<RatingEventType, RatingEvent> m_RegisteredEvents;
    std::vector<RatingEvent> m_EventHistory;
};

DECLARE_ZHM_PLUGIN(AdvancedRating)
