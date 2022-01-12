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
	auto pinIt = m_knownInputPins.find(pinId);
	auto entIt = m_knownInputEntities.find(entityRef);

	if (pinIt == m_knownInputPins.end() && entIt == m_knownInputEntities.end())
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
		
		m_knownInputPins[pinId] = true;
		m_knownInputEntities[entityRef] = true;
	}

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(LogPins, bool, SignalOutputPin, ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
{
	auto pinIt = m_knownOutputPins.find(pinId);
	auto entIt = m_knownOutputEntities.find(entityRef);

	if (pinIt == m_knownOutputPins.end() && entIt == m_knownOutputEntities.end())
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

		m_knownOutputPins[pinId] = true;
		m_knownOutputEntities[entityRef] = true;
	}

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(LogPins);
