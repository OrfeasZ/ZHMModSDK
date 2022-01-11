#include "LogPins.h"

#include "Hooks.h"
#include "Logging.h"
#include "Functions.h"

#include <Glacier/ZScene.h>
#include <Glacier/ZAIGameState.h>
#include <Glacier/SOnlineEvent.h>

#include "json.hpp"

void LogPins::PreInit()
{
	Hooks::SignalInputPin->AddDetour(this, &LogPins::SignalInputPin);
	Hooks::SignalOutputPin->AddDetour(this, &LogPins::SignalOutputPin);
}


DEFINE_PLUGIN_DETOUR(LogPins, bool, SignalInputPin, ZEntityRef zEntityRef, uint32_t pinId, const ZObjectRef& zObjectRef)
{
	Logger::Debug("Pin Input: {}", pinId);
}

DEFINE_PLUGIN_DETOUR(LogPins, bool, SignalOutputPin, ZEntityRef zEntityRef, uint32_t pinId, const ZObjectRef& zObjectRef);
{
	Logger::Debug("Pin Output: {}", pinId);
}

DECLARE_ZHM_PLUGIN(LogPins);
