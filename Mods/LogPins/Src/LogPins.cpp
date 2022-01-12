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

// variadic template
template < typename... Args >
std::string sstr(Args &&... args)
{
	std::ostringstream sstr;
	// fold expression
	(sstr << std::dec << ... << args);
	return sstr.str();
}

DECLARE_PLUGIN_DETOUR(LogPins, bool, SignalInputPin, ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
{
	auto it = m_knownInputs.find(sstr(pinId, "_", (*entityRef.m_pEntity)->m_nEntityId));
	if (it == m_knownInputs.end())
	{
		Logger::Info("Pin Input: {} on {}", pinId, (*entityRef.m_pEntity)->m_nEntityId);

		auto pInterface = (*(*entityRef.m_pEntity)->m_pInterfaces)[0];

		if (!pInterface.m_pTypeId ||
			!pInterface.m_pTypeId->m_pType ||
			!pInterface.m_pTypeId->m_pType->m_pTypeName
			) {
		}
		else
		{
			Logger::Info("Pin entity class: {}", pInterface.m_pTypeId->m_pType->m_pTypeName);
		}

		if (objectRef.IsEmpty())
		{
			Logger::Info("Parameter type: None");
		}
		else
		{
			Logger::Info("Parameter type: {}", objectRef.m_pTypeID->m_pType->m_pTypeName);
		}
		
		m_knownInputs[sstr(pinId, "_", (*entityRef.m_pEntity)->m_nEntityId)] = true;
	}

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(LogPins, bool, SignalOutputPin, ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
{
	auto it = m_knownOutputs.find(sstr(pinId, "_", (*entityRef.m_pEntity)->m_nEntityId));
	if (it == m_knownOutputs.end())
	{
		Logger::Info("Pin Output: {} on {}", pinId, (*entityRef.m_pEntity)->m_nEntityId);

		auto pInterface = (*(*entityRef.m_pEntity)->m_pInterfaces)[0];

		if (!pInterface.m_pTypeId ||
			!pInterface.m_pTypeId->m_pType ||
			!pInterface.m_pTypeId->m_pType->m_pTypeName
			) {
		}
		else
		{
			Logger::Info("Pin entity class: {}", pInterface.m_pTypeId->m_pType->m_pTypeName);
		}

		if (objectRef.IsEmpty())
		{
			Logger::Info("Parameter type: None");
		}
		else
		{
			Logger::Info("Parameter type: {}", objectRef.m_pTypeID->m_pType->m_pTypeName);
		}

		m_knownOutputs[sstr(pinId, "_", (*entityRef.m_pEntity)->m_nEntityId)] = true;
	}

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(LogPins);
