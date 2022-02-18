#pragma once

#include <shared_mutex>
#include <random>
#include <unordered_map>
#include <sstream>
#include <thread> 
#include <deque>

#include "IPluginInterface.h"

class LogPins : public IPluginInterface
{
public:
	void PreInit() override;
	void OnDraw3D(IRenderer* p_Renderer) override;
	void OnDrawMenu() override;

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

	void UpdateTrackableMap(ZEntityRef entityRef);
	void DumpDetails(ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef);
	int AddToSendList(std::string);
	void ProcessSocketMessageQueue();

	static void SendToSocketThread();
	static void ReceiveFromSocketThread();

	static LogPins* instance;
	static std::deque<std::string> receivedMessages;
	static std::deque<std::string> sendingMessages;

	static bool sendingPinsEnabled;
	static std::mutex sendingMutex;
	static std::condition_variable sendingCV;

	int lastIndex = 0;

	uint64_t entityToTrack = 0;

	bool drawCoverPlane;
	SMatrix coverPlanePos;
	SVector3 coverPlaneSize;

	int lastKnownActorId = 0;

	// ZEntityType** ref;
	ZEntityRef* ref;
};

DEFINE_ZHM_PLUGIN(LogPins)
