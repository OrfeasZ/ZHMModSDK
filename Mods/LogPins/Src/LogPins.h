#pragma once

#include "IPluginInterface.h"

class LogPins : public IPluginInterface
{
private:
	DEFINE_PLUGIN_DETOUR(LogPins, bool, SignalInputPin, ZEntityRef zEntityRef, uint32_t pinId, const ZObjectRef& zObjectRef);
	DEFINE_PLUGIN_DETOUR(LogPins, bool, SignalOutputPin, ZEntityRef zEntityRef, uint32_t pinId, const ZObjectRef& zObjectRef);
};

DEFINE_ZHM_PLUGIN(LogPins)
