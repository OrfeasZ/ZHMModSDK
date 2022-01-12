#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"

#include <sstream>

class LogPins : public IPluginInterface
{
public:
	void PreInit() override;

private:
	DEFINE_PLUGIN_DETOUR(LogPins, bool, SignalInputPin, ZEntityRef, uint32_t, const ZObjectRef&);
	DEFINE_PLUGIN_DETOUR(LogPins, bool, SignalOutputPin, ZEntityRef, uint32_t, const ZObjectRef&);
	
private:
	std::unordered_map<std::string, bool> m_knownInputs;
	std::unordered_map<std::string, bool> m_knownOutputs;

	static void DumpDetails(ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
	{
		auto pInterface = (*(*entityRef.m_pEntity)->m_pInterfaces)[0];

		if (!pInterface.m_pTypeId ||
			!pInterface.m_pTypeId->m_pType ||
			!pInterface.m_pTypeId->m_pType->m_pTypeName
			) {
			Logger::Info("Pin entity class: UNKNOWN");
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
	}
};

DEFINE_ZHM_PLUGIN(LogPins)
