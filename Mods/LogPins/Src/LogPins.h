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
};

DEFINE_ZHM_PLUGIN(LogPins)
