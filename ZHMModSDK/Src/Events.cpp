#include "Events.h"

#include "EventDispatcherImpl.h"

std::unordered_set<EventDispatcherBase*>* EventDispatcherRegistry::g_Dispatchers = nullptr;

DEFINE_EVENT(OnConsoleCommand, void)