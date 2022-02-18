#include "LogPins.h"
#include "SArrayToProp.h"

#include "Hooks.h"
#include "Logging.h"
#include "Functions.h"

#include <Glacier/ZScene.h>
#include <Glacier/ZEntity.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZObject.h>
#include <Glacier/ZActor.h>

#define DEFAULT_SERVER "127.0.0.1"
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 37275

#pragma comment(lib, "Ws2_32.lib")

#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

class IItem : public ZEntityImpl {};
class ZEntityBaseReplica : public ZEntityImpl {};

LogPins* LogPins::instance;
std::deque<std::string> LogPins::receivedMessages;
std::deque<std::string> LogPins::sendingMessages;

std::mutex LogPins::sendingMutex;
std::condition_variable LogPins::sendingCV;
bool LogPins::sendingPinsEnabled = false;

void LogPins::PreInit()
{
	Hooks::SignalInputPin->AddDetour(this, &LogPins::SignalInputPin);
	Hooks::SignalOutputPin->AddDetour(this, &LogPins::SignalOutputPin);

	s, slen = sizeof(si_other);
	WSADATA wsa;

	//Initialise winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		Logger::Info("Failed to initialise socket");
	}

	instance = this;

	//create socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		Logger::Info("Failed to create socket");
	}
	else
	{
		//setup address structure
		memset((char*)&si_other, 0, sizeof(si_other));
		si_other.sin_family = AF_INET;
		si_other.sin_port = htons(DEFAULT_PORT);
		si_other.sin_addr.S_un.S_addr = inet_addr(DEFAULT_SERVER);

		LogPins::AddToSendList("hello\r\n");

		std::thread sendToSocketThread(&LogPins::SendToSocketThread);
		sendToSocketThread.detach();

		std::thread receiveFromSocketThread(&LogPins::ReceiveFromSocketThread);
		receiveFromSocketThread.detach();
	}


	/*
		closesocket(s);
		WSACleanup();

	*/
}

void LogPins::OnDraw3D(IRenderer* p_Renderer)
{
	LogPins::ProcessSocketMessageQueue();

	for (int i = lastKnownActorId; i < *Globals::NextActorId; ++i)
	{
		auto* s_Actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
		ZEntityRef s_Ref;
		s_Actor->GetID(&s_Ref);

		LogPins::UpdateTrackableMap(s_Ref);
	}

	m_EntityMutex.lock_shared();

	//for (auto& s_Entity : m_EntitiesToTrack)

	auto it = m_EntitiesToTrack.find(entityToTrack);
	if (it != m_EntitiesToTrack.end())
	{
		auto& s_Entity = m_EntitiesToTrack[entityToTrack];

		auto* s_SpatialEntity = s_Entity.QueryInterface<ZSpatialEntity>();

		SMatrix s_Transform;
		Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

		float4 s_Min, s_Max;

		s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

		if (s_Min.x == 0 && s_Min.y == 0 && s_Min.z == 0 && s_Max.x == 0 && s_Max.y == 0 && s_Max.z == 0)
		{
			s_Max = float4(.1f, .1f, .1f, s_Max.w);
		}

		p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z + .1f), s_Transform, SVector4(0.f, 0.f, 1.f, 1.f));
	}

	m_EntityMutex.unlock_shared();

	if (drawCoverPlane)
	{
		p_Renderer->DrawOBB3D(SVector3(-coverPlaneSize.x / 2.f, -coverPlaneSize.y / 2.f, -coverPlaneSize.z / 2.f), SVector3(coverPlaneSize.x / 2.f, coverPlaneSize.y / 2.f, coverPlaneSize.z / 2.f), coverPlanePos, SVector4(0.f, 0.f, 1.f, 1.f));
	}
}

void LogPins::ProcessSocketMessageQueue()
{
	if (!LogPins::receivedMessages.empty())
	{
		std::string currentMessage = LogPins::receivedMessages.front();
		LogPins::receivedMessages.pop_front();

		Logger::Info("Received From Socket: {}", currentMessage);

		std::string delimiter = "|";

		std::vector<std::string> params;

		size_t last = 0;
		size_t next = 0;
		while ((next = currentMessage.find(delimiter, last)) != std::string::npos)
		{
			params.push_back(currentMessage.substr(last, next - last));
			last = next + 1;
		}

		uint64_t requestedEntityId;
		if (last > 0)
		{
			params.push_back(currentMessage.substr(last));
			std::istringstream(params[1].c_str()) >> requestedEntityId;
		}
		else
		{
			params.push_back(currentMessage);
		}

		if (currentMessage.length() > 0)
		{
			if (params[0] == "H")
			{
				entityToTrack = requestedEntityId;
			}
			else if (params[0] == "P")
			{
				auto it = m_EntitiesToTrack.find(requestedEntityId);
				if (it != m_EntitiesToTrack.end())
				{
					auto requestedEnt = m_EntitiesToTrack[requestedEntityId];

					auto s_EntitySpatial = requestedEnt.QueryInterface<ZSpatialEntity>();

					SMatrix s_WorldTransform;
					Functions::ZSpatialEntity_WorldTransform->Call(s_EntitySpatial, &s_WorldTransform);
				
					requestedEnt.SetProperty("m_mTransform", EularToMatrix43(
						std::stod(params[2].c_str()),
						std::stod(params[3].c_str()),
						std::stod(params[4].c_str()),
						std::stod(params[5].c_str()),
						std::stod(params[6].c_str()),
						std::stod(params[7].c_str())
					));
				}
			}
			else if (params[0] == "C")
			{
				float32 sX;
				std::istringstream(params[8].c_str()) >> sX;
				float32 sY;
				std::istringstream(params[9].c_str()) >> sY;
				float32 sZ;
				std::istringstream(params[10].c_str()) >> sZ;
				coverPlaneSize = SVector3(sX, sY, sZ);

				SMatrix43 newTrans = EularToMatrix43(
					std::stod(params[2].c_str()),
					std::stod(params[3].c_str()),
					std::stod(params[4].c_str()),
					std::stod(params[5].c_str()),
					std::stod(params[6].c_str()),
					std::stod(params[7].c_str())
				);

				Matrix43ToSMatrix(newTrans, &coverPlanePos);

				drawCoverPlane = true;
			}
			else if (params[0] == "UpdateProperty")
			{
				auto it = m_EntitiesToTrack.find(requestedEntityId);
				if (it != m_EntitiesToTrack.end())
				{
					auto requestedEnt = m_EntitiesToTrack[requestedEntityId];

					SetPropertyFromVectorString(requestedEnt, params[2].c_str(), params[3].c_str(), params, 4, &m_EntitiesToTrack);
				}
			}
			else if (params[0] == "GetHeroPosition")
			{
				TEntityRef<ZHitman5> s_LocalHitman;
				Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);
				const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

				std::ostringstream ss;
				ss << "GetHeroPosition_" << s_HitmanSpatial->m_mTransform.Trans.x << "_" << s_HitmanSpatial->m_mTransform.Trans.y << "_" << s_HitmanSpatial->m_mTransform.Trans.z;
				LogPins::AddToSendList(ss.str());
			}
			else if (params[0] == "SetHeroPosition")
			{
				TEntityRef<ZHitman5> s_LocalHitman;
				Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);
				const auto s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

				s_LocalHitman.m_ref.GetBaseEntity()->Deactivate(6000);

				s_HitmanSpatial->m_mTransform = EularToMatrix43(
					std::stoul(params[2]),
					std::stoul(params[3]),
					std::stoul(params[4]),
					std::stoul(params[5]),
					std::stoul(params[6]),
					std::stoul(params[7])
				);
			}
		}
	}
}

int LogPins::AddToSendList(std::string message)
{
	LogPins::sendingMessages.push_back(message);
	LogPins::sendingCV.notify_one();

	return 0;
}

void LogPins::SendToSocketThread()
{
	bool runServer = true;

	while (runServer)
	{
		std::unique_lock<std::mutex> lk(LogPins::sendingMutex);
		while (LogPins::sendingMessages.empty())
		{
			LogPins::sendingCV.wait(lk);
			// std::this_thread::yield();
		}

		std::string currentMessage = LogPins::sendingMessages.front();
		LogPins::sendingMessages.pop_front();

		//start communication
		//send the message
		if (sendto(LogPins::instance->s, currentMessage.c_str(), currentMessage.length(), 0, (struct sockaddr*)&LogPins::instance->si_other, LogPins::instance->slen) == SOCKET_ERROR)
		{
			Logger::Info("AddToSendList() failed with error code : {}", WSAGetLastError());
		}
	}
}

void LogPins::ReceiveFromSocketThread()
{
	bool shouldReceive = true;
	char buf[DEFAULT_BUFLEN];
	std::string pingGameResponse = "PingGame";

	while (shouldReceive)
	{
		memset(buf, '\0', DEFAULT_BUFLEN);
		//try to receive some data, this is a blocking call
		if (recvfrom(LogPins::instance->s, buf, DEFAULT_BUFLEN, 0, (struct sockaddr*)&LogPins::instance->si_other, &LogPins::instance->slen) == SOCKET_ERROR)
		{
			// noError = false;
		}
		else
		{
			std::string message = buf;

			if (message == "Exit")
			{
				shouldReceive = false;
			}
			else if (message == "Ping")
			{
				if (sendto(LogPins::instance->s, pingGameResponse.c_str(), pingGameResponse.length(), 0, (struct sockaddr*)&LogPins::instance->si_other, LogPins::instance->slen) == SOCKET_ERROR)
				{
					Logger::Info("AddToSendList() failed with error code : {}", WSAGetLastError());
				}
			}
			else
			{
				LogPins::receivedMessages.push_back(message);
			}
		}
	}
}

void LogPins::DumpDetails(ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
{
	auto pInterface = (*(*entityRef.m_pEntity)->m_pInterfaces)[0];

	if (!pInterface.m_pTypeId ||
		!pInterface.m_pTypeId->m_pType ||
		!pInterface.m_pTypeId->m_pType->m_pTypeName
		)
	{
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

void LogPins::OnDrawMenu()
{
	if (ImGui::Button(LogPins::sendingPinsEnabled ? "DISABLE PINS" : "ENABLE PINS"))
	{
		LogPins::sendingMessages.clear();
		LogPins::sendingPinsEnabled = !LogPins::sendingPinsEnabled;
	}
}

void LogPins::UpdateTrackableMap(ZEntityRef entityRef) // , const ZObjectRef& objectRef)
{
	// Add entity to track
	auto it = m_EntitiesToTrack.find((*entityRef.m_pEntity)->m_nEntityId);
	if (it == m_EntitiesToTrack.end())
	{
		m_EntityMutex.lock();

		m_EntitiesToTrack[(*entityRef.m_pEntity)->m_nEntityId] = entityRef;

		// Add any referenced entities
		TArray<ZEntityProperty>* props = (*entityRef.m_pEntity)->m_pProperties01;

		if (props != nullptr)
		{
			for (auto prop : *props)
			{
				if (prop.m_pType == nullptr || prop.m_pType->getPropertyInfo() == nullptr) continue;
				auto propInfo = prop.m_pType->getPropertyInfo();

				std::string propName, propTypeName;
				bool validPropName = false;
				[&]()
				{
					__try
					{
						[&]()
						{
							propName = propInfo->m_pName;
							propTypeName = propInfo->m_pType->typeInfo()->m_pTypeName;
							validPropName = true;
						}();
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						propTypeName = "";
					}
				}();

				if (propTypeName.empty() || !validPropName) continue;

				if ((*entityRef.m_pEntity)->m_nEntityId == 12379770012806219889 && propName == "m_aValues")
				{
					auto* propValue = reinterpret_cast<TArray<TEntityRef<ZItemRepositoryKeyEntity>>*>(reinterpret_cast<uintptr_t>(entityRef.m_pEntity) + prop.m_nOffset);

					// propValue->clear();
					// propValue->resize(0);

					// (*propValue) = *arr;

					// entityRef.SetProperty(propName.c_str(), *propValue);
				}

				if (propTypeName.starts_with("TEntityRef<") && propTypeName.ends_with("Entity>")) // == "TEntityRef<ZSpatialEntity>")
				{
					auto propValue = reinterpret_cast<ZEntityRef*>(reinterpret_cast<uintptr_t>(entityRef.m_pEntity) + prop.m_nOffset);

					auto propEnt = propValue->m_pEntity;

					if (propEnt == nullptr) continue;

					// Logger::Trace("Value: {}", (*propEnt)->m_nEntityId);

					auto propIt = m_EntitiesToTrack.find((*propEnt)->m_nEntityId);
					if (propIt == m_EntitiesToTrack.end())
					{
						m_EntitiesToTrack[(*propEnt)->m_nEntityId] = *propValue;
					}
				}
				else if (propTypeName.starts_with("TArray<TEntityRef<") && propTypeName.ends_with("Entity>>"))// propTypeName == "TArray<TEntityRef<ZSpatialEntity>>")
				{
					auto propValue = reinterpret_cast<TArray<ZEntityRef>*>(reinterpret_cast<uintptr_t>(entityRef.m_pEntity) + prop.m_nOffset);

					for (auto containedEnt : *propValue)
					{
						auto propEnt = containedEnt.m_pEntity;

						if (propEnt == nullptr) continue;

						auto entIt = (*propEnt)->m_nEntityId;
						auto propIt = m_EntitiesToTrack.find(entIt);
						if (propIt == m_EntitiesToTrack.end())
						{
							m_EntitiesToTrack[entIt] = containedEnt;
						}
					}
				}
			}
		}

		m_EntityMutex.unlock();
	}
}

DECLARE_PLUGIN_DETOUR(LogPins, bool, SignalInputPin, ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
{
	if (LogPins::sendingPinsEnabled)
	{
		std::ostringstream ss;
		ss << "I_" << pinId << "_" << (*entityRef.m_pEntity)->m_nEntityId;
		std::string s = ss.str();

		LogPins::AddToSendList(s);

		auto pinsIt = m_knownInputs.find(s);
		if (pinsIt == m_knownInputs.end())
		{
			// Logger::Info("Pin Input: {} on {}", pinId, (*entityRef.m_pEntity)->m_nEntityId);

			// DumpDetails(entityRef, pinId, objectRef);

			m_knownInputs[s] = true;
		}
	}

	LogPins::UpdateTrackableMap(entityRef);

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(LogPins, bool, SignalOutputPin, ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
{
	if (LogPins::sendingPinsEnabled)
	{
		std::ostringstream ss;
		ss << "O_" << pinId << "_" << (*entityRef.m_pEntity)->m_nEntityId;
		std::string s = ss.str();

		LogPins::AddToSendList(s);

		auto pinsIt = m_knownOutputs.find(s);
		if (pinsIt == m_knownOutputs.end())
		{
			// Logger::Info("Pin Output: {} on {}", pinId, (*entityRef.m_pEntity)->m_nEntityId);

			// DumpDetails(entityRef, pinId, objectRef);

			m_knownOutputs[s] = true;
		}
	}


	LogPins::UpdateTrackableMap(entityRef);

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(LogPins);
