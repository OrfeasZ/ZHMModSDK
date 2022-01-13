#include "LogPins.h"

#include "Hooks.h"
#include "Logging.h"
#include "Functions.h"

#include <Glacier/ZScene.h>
#include <Glacier/ZEntity.h>
#include <Glacier/ZObject.h>

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")


#define DEFAULT_SERVER "127.0.0.1"
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27015


#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

void LogPins::PreInit()
{
	Hooks::SignalInputPin->AddDetour(this, &LogPins::SignalInputPin);
	Hooks::SignalOutputPin->AddDetour(this, &LogPins::SignalOutputPin);

	LogPins::SendToSocket("hello\r\n");
}

int LogPins::SendToSocket(std::string message)
{
	struct sockaddr_in si_other;
	int s, slen = sizeof(si_other);
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

	//start communication
	//send the message
	if (sendto(s, message.c_str(), message.length, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
	{
		Logger::Info("sendto() failed with error code : {}", WSAGetLastError());
	}

	closesocket(s);
	WSACleanup();

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
	ss << "" << pinId << "_" << (*entityRef.m_pEntity)->m_nEntityId;
	std::string s = ss.str();

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
	ss << "" << pinId << "_" << (*entityRef.m_pEntity)->m_nEntityId;
	std::string s = ss.str();

	auto it = m_knownOutputs.find(s);
	if (it == m_knownOutputs.end())
	{
		Logger::Info("Pin Output: {} on {}", pinId, (*entityRef.m_pEntity)->m_nEntityId);

		DumpDetails(entityRef, pinId, objectRef);

		m_knownOutputs[s] = true;
	}

	return HookResult<bool>(HookAction::Continue());
}


DECLARE_ZHM_PLUGIN(LogPins);
