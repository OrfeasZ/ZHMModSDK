#pragma once

#include <shared_mutex>
#include <random>
#include <unordered_map>
#include <sstream>
#include <thread> 

#include "IPluginInterface.h"


class LogPins : public IPluginInterface
{
public:
	void PreInit() override;
	void OnDraw3D(IRenderer* p_Renderer) override;

private:
	DEFINE_PLUGIN_DETOUR(LogPins, bool, SignalInputPin, ZEntityRef, uint32_t, const ZObjectRef&);
	DEFINE_PLUGIN_DETOUR(LogPins, bool, SignalOutputPin, ZEntityRef, uint32_t, const ZObjectRef&);
	
private:
	std::unordered_map<std::string, bool> m_knownInputs;
	std::unordered_map<std::string, bool> m_knownOutputs;

	struct sockaddr_in si_other;
	int s, slen;

	std::shared_mutex m_EntityMutex;
	std::unordered_map<uint64_t, ZEntityRef> m_EntitiesToTrack;

	void DumpDetails(ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef);
	int SendToSocket(std::string);

	static void ReceiveFromSocket();
	static LogPins* instance;
	static std::vector<std::string> messages;
	int lastIndex = 0;
};

DEFINE_ZHM_PLUGIN(LogPins)
