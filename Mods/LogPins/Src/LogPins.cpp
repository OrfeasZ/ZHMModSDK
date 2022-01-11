#include "LogPins.h"

#include "Hooks.h"
#include "Logging.h"
#include "Functions.h"

#include <Glacier/ZScene.h>

void LogPins::PreInit()
{
	Hooks::SignalInputPin->AddDetour(this, &LogPins::SignalInputPin);
	Hooks::SignalOutputPin->AddDetour(this, &LogPins::SignalOutputPin);
}

DECLARE_PLUGIN_DETOUR(LogPins, bool, SignalInputPin, ZEntityRef zEntityRef, uint32_t pinId, const ZObjectRef& zObjectRef)
{
	auto it = m_knownInputs.find(pinId);
	if (it == m_knownInputs.end())
	{
		Logger::Info("Pin Input: {}", pinId);
		
		m_knownInputs[pinId] = true;
	}

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(LogPins, bool, SignalOutputPin, ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
{
	auto it = m_knownOutputs.find(pinId);
	if (it == m_knownOutputs.end())
	{
		Logger::Info("Pin Output: {}", pinId);
		
		m_knownOutputs[pinId] = true;
	}

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(LogPins);
