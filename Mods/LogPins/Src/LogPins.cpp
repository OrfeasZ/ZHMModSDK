#include "LogPins.h"

#include "Hooks.h"
#include "Logging.h"
#include "Functions.h"

#include <Glacier/ZScene.h>
#include <Glacier/ZEntity.h>
#include <Glacier/ZObject.h>

void LogPins::PreInit()
{
	Hooks::SignalInputPin->AddDetour(this, &LogPins::SignalInputPin);
	Hooks::SignalOutputPin->AddDetour(this, &LogPins::SignalOutputPin);
}

DECLARE_PLUGIN_DETOUR(LogPins, bool, SignalInputPin, ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
{
	auto it = m_knownInputs.find(pinId);
	if (it == m_knownInputs.end())
	{
		Logger::Info("Pin Input: {} on {}", pinId, (*entityRef.m_pEntity)->m_nEntityId);

		m_knownInputs[pinId] = true;
	}

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(LogPins, bool, SignalOutputPin, ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
{
	auto it = m_knownOutputs.find(pinId);
	if (it == m_knownOutputs.end())
	{
		Logger::Info("Pin Output: {} on {}", pinId, (*entityRef.m_pEntity)->m_nEntityId);
		Logger::Info("Pin entity interfaces include:");

		for (auto pInterface : *(*entityRef.m_pEntity)->m_pInterfaces)
		{
			if (!pInterface.m_pTypeId ||
				!pInterface.m_pTypeId->m_pType ||
				!pInterface.m_pTypeId->m_pType->m_pTypeName
			)
				continue;
			Logger::Info("{}", pInterface.m_pTypeId->m_pType->m_pTypeName);
		}

		m_knownOutputs[pinId] = true;
	}

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(LogPins);
