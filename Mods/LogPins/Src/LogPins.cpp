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

	//create socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		Logger::Info("Failed to create socket");
	}

	//setup address structure
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(DEFAULT_PORT);
	si_other.sin_addr.S_un.S_addr = inet_addr(DEFAULT_SERVER);

	LogPins::SendToSocket("hello\r\n");

	/*
		closesocket(s);
		WSACleanup();

	*/
}

void LogPins::OnDraw3D(IRenderer* p_Renderer)
{
	m_EntityMutex.lock_shared();

	//for (auto& s_Entity : m_EntitiesToTrack)

	uint64_t s = 12379766437458850639;

	auto it = m_EntitiesToTrack.find(s);
	if (it != m_EntitiesToTrack.end())
	{
		auto& s_Entity = m_EntitiesToTrack[s];

		auto* s_SpatialEntity = s_Entity.QueryInterface<ZSpatialEntity>();

		SMatrix s_Transform;
		Functions::ZSpatialEntity_WorldTransform->Call(s_SpatialEntity, &s_Transform);

		float4 s_Min, s_Max;

		s_SpatialEntity->CalculateBounds(s_Min, s_Max, 1, 0);

		p_Renderer->DrawOBB3D(SVector3(s_Min.x, s_Min.y, s_Min.z), SVector3(s_Max.x, s_Max.y, s_Max.z), s_Transform, SVector4(0.f, 0.f, 1.f, 1.f));
	}

	m_EntityMutex.unlock_shared();
}

int LogPins::SendToSocket(std::string message)
{
	//start communication
	//send the message
	if (sendto(s, message.c_str(), message.length(), 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
	{
		Logger::Info("sendto() failed with error code : {}", WSAGetLastError());
	}

	return 0;
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

	/*
	std::ostringstream ss;

	auto& s_Properties1 = (*entityRef.m_pEntity)->m_pProperties01;
	auto& s_Properties2 = (*entityRef.m_pEntity)->m_pProperties02;

	if (!s_Properties1)
	{
		ss << "";
	}
	else
	{
		size_t aSize = (*entityRef.m_pEntity)->m_pProperties01->size();
		size_t aCap = (*entityRef.m_pEntity)->m_pProperties01->capacity();

		ss << aSize << "/" << aCap;

		for (auto s_Property = (*entityRef.m_pEntity)->m_pProperties01->begin(); s_Property != (*entityRef.m_pEntity)->m_pProperties01->end(); ++s_Property)
		{
			if (!!s_Property->m_pType && !!s_Property->m_pType->getPropertyInfo() && !!s_Property->m_pType->getPropertyInfo()->m_pName)
			{
				ss << " " << s_Property->m_pType->getPropertyInfo()->m_pName;
			}
		}
	}

	ss << " | ";

	if (!s_Properties2)
	{
		ss << "";
	}
	else
	{
		for (auto s_Property = (*entityRef.m_pEntity)->m_pProperties02->begin(); s_Property != (*entityRef.m_pEntity)->m_pProperties02->end(); ++s_Property)
		{
			if (!!s_Property->m_pType && !!s_Property->m_pType->getPropertyInfo() && !!s_Property->m_pType->getPropertyInfo()->m_pName)
			{
				ss << " " << s_Property->m_pType->getPropertyInfo()->m_pName;
			}
		}
	}

	Logger::Info("Entity props:{}", ss.str());
	*/
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

	uint64_t entId = 12379766437458850639;

	if ((*entityRef.m_pEntity)->m_nEntityId == entId)
	{
		auto it = m_EntitiesToTrack.find(entId);
		if (it != m_EntitiesToTrack.end())
		{
			m_EntityMutex.lock();

			m_EntitiesToTrack[entId] = entityRef;

			m_EntityMutex.unlock();
		}
	}

	return HookResult<bool>(HookAction::Continue());
}


DECLARE_ZHM_PLUGIN(LogPins);
