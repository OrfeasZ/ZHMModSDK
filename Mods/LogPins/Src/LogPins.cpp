#include "LogPins.h"

#include "Hooks.h"
#include "Logging.h"
#include "Functions.h"

#include <Glacier/ZScene.h>
#include <Glacier/ZEntity.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZObject.h>

#define DEFAULT_SERVER "127.0.0.1"
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27015

#pragma comment(lib, "Ws2_32.lib")

#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

LogPins* LogPins::instance;
std::vector<std::string> LogPins::messages;

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
		memset((char *)&si_other, 0, sizeof(si_other));
		si_other.sin_family = AF_INET;
		si_other.sin_port = htons(DEFAULT_PORT);
		si_other.sin_addr.S_un.S_addr = inet_addr(DEFAULT_SERVER);

		LogPins::SendToSocket("hello\r\n");

		std::thread receiveFromSocketThread(&LogPins::ReceiveFromSocket);
		receiveFromSocketThread.detach();
	}


	/*
		closesocket(s);
		WSACleanup();

	*/
}

void LogPins::OnDraw3D(IRenderer* p_Renderer)
{
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

		p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(0.f, 0.f, 1.f, 1.f));
	}

	m_EntityMutex.unlock_shared();

	if (lastIndex < LogPins::messages.size())
	{
		std::string currentMessage = LogPins::messages[lastIndex];

		Logger::Info("Received From Socket: {}", currentMessage);

		std::string delimiter = "_";

		std::vector<std::string> params;

		size_t last = 0;
		size_t next = 0;
		while ((next = currentMessage.find(delimiter, last)) != std::string::npos)
		{
			params.push_back(currentMessage.substr(last, next - last));
			last = next + 1;
		}
		params.push_back(currentMessage.substr(last));

		if (currentMessage.length() > 0)
		{
			uint64_t requestedEntityId;
			std::istringstream(params[1].c_str()) >> requestedEntityId;

			if (currentMessage.at(0) == 'H')
			{
				entityToTrack = requestedEntityId;
			}
			else if (currentMessage.at(0) == 'P')
			{
				entityToTrack = requestedEntityId;
				auto it = m_EntitiesToTrack.find(requestedEntityId);
				if (it != m_EntitiesToTrack.end())
				{
					auto requestedEnt = m_EntitiesToTrack[requestedEntityId];

					// m_EntitiesToTrack[requestedEntityId].GetBaseEntity()->Deactivate(0);



					auto s_EntitySpatial = requestedEnt.QueryInterface<ZSpatialEntity>();
					// s_EntitySpatial->m_mTransform.Trans.x += 2.f;

					SMatrix s_WorldTransform;
					Functions::ZSpatialEntity_WorldTransform->Call(s_EntitySpatial, &s_WorldTransform);

					float32 x;
					std::istringstream(params[2].c_str()) >> x;
					float32 y;
					std::istringstream(params[3].c_str()) >> y;
					float32 z;
					std::istringstream(params[4].c_str()) >> z;

					SMatrix43 newTrans = SMatrix43();
					newTrans.Trans = SVector3(
						x, y, z
						/*
						(s_EntitySpatial->m_mTransform.Trans.x - s_WorldTransform.mat[2].x),
						(s_EntitySpatial->m_mTransform.Trans.y - s_WorldTransform.mat[2].y),
						(s_EntitySpatial->m_mTransform.Trans.z - s_WorldTransform.mat[2].z)
						*/
					);

					// ZObjectRef transObj = ZObjectRef();

					requestedEnt.SetProperty("m_mTransform", newTrans);

					auto baseEnt = requestedEnt.GetBaseEntity();

					auto baseInt = (*baseEnt->m_pType->m_pInterfaces)[0];

					auto baseIntName = baseInt.m_pTypeId->m_pType->m_pTypeName;

					// Logger::Info("baseEnt: {}", baseEnt);
				}
			}
		}

		lastIndex++;
	}
}

int LogPins::SendToSocket(std::string message)
{
	//start communication
	//send the message
	if (sendto(s, message.c_str(), message.length(), 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
	{
		Logger::Info("SendToSocket() failed with error code : {}", WSAGetLastError());
	}

	return 0;
}

void LogPins::ReceiveFromSocket()
{
	bool noError = true;
	char buf[DEFAULT_BUFLEN];

	while (noError)
	{
		memset(buf, '\0', DEFAULT_BUFLEN);
		//try to receive some data, this is a blocking call
		if (recvfrom(LogPins::instance->s, buf, DEFAULT_BUFLEN, 0, (struct sockaddr *) &LogPins::instance->si_other, &LogPins::instance->slen) == SOCKET_ERROR)
		{
			// noError = false;
		}

		std::string message = buf;

		if (message.rfind("exit") == 0) noError = false;

		LogPins::messages.push_back(message);
	}
}

void LogPins::DumpDetails(ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
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

DECLARE_PLUGIN_DETOUR(LogPins, bool, SignalInputPin, ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
{
	std::ostringstream ss;
	ss << "I_" << pinId << "_" << (*entityRef.m_pEntity)->m_nEntityId;
	std::string s = ss.str();

	LogPins::SendToSocket(s);

	auto it = m_knownInputs.find(s);
	if (it == m_knownInputs.end())
	{
		Logger::Info("Pin Input: {} on {}", pinId, (*entityRef.m_pEntity)->m_nEntityId);

		DumpDetails(entityRef, pinId, objectRef);

		m_knownInputs[s] = true;
	}

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_PLUGIN_DETOUR(LogPins, bool, SignalOutputPin, ZEntityRef entityRef, uint32_t pinId, const ZObjectRef& objectRef)
{
	std::ostringstream ss;
	ss << "O_" << pinId << "_" << (*entityRef.m_pEntity)->m_nEntityId;
	std::string s = ss.str();

	LogPins::SendToSocket(s);

	auto it = m_knownOutputs.find(s);
	if (it == m_knownOutputs.end())
	{
		Logger::Info("Pin Output: {} on {}", pinId, (*entityRef.m_pEntity)->m_nEntityId);

		DumpDetails(entityRef, pinId, objectRef);

		m_knownOutputs[s] = true;
	}

	if ((*entityRef.m_pEntity)->m_nEntityId == 12379823793659994311)
	{
		auto entRefT = objectRef.As<TEntityRef<ZSpatialEntity>>();
		auto entRef = entRefT->m_ref;
		uint64_t entId = (*entRef.m_pEntity)->m_nEntityId;
		Logger::Info("Test {}", entId);

		// m_Entity
		for (auto& s_PropertyType : *(*entityRef.m_pEntity)->m_pProperties01)
		{
			if (s_PropertyType.m_nPropertyId != 1125585824)
				continue;

			auto* s_Type = s_PropertyType.m_pType->getPropertyInfo()->m_pType;

			auto* s_Data = (*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
				s_Type->typeInfo()->m_nTypeSize,
				s_Type->typeInfo()->m_nTypeAlignment
			);

			Logger::Info("Got prop {}", s_PropertyType.m_nPropertyId);
			// TODO: Re-implement this.
			//Hooks::GetPropertyValue->Call(*this, p_PropertyId, s_Data);
		}


		auto it = m_EntitiesToTrack.find(entId);
		if (it == m_EntitiesToTrack.end())
		{
			m_EntityMutex.lock();

			m_EntitiesToTrack[entId] = entRef;

			m_EntityMutex.unlock();
		}
	}

	// if ((*entityRef.m_pEntity)->m_nEntityId == entityToTrack)
	// if(entityToTrack > 0)
	{
		auto it = m_EntitiesToTrack.find((*entityRef.m_pEntity)->m_nEntityId);
		if (it == m_EntitiesToTrack.end())
		{
			m_EntityMutex.lock();

			m_EntitiesToTrack[(*entityRef.m_pEntity)->m_nEntityId] = entityRef;

			m_EntityMutex.unlock();
		}
	}

	return HookResult<bool>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(LogPins);
