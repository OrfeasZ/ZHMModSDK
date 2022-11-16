#pragma once

#include "ZString.h"
#include "TArray.h"
#include "TMap.h"
#include "ZBuffer.h"

class ZHttpResultBase;

class ZHttpHeader
{
public:
	virtual ~ZHttpHeader() {}

public:
	ZString m_key;
	ZString m_value;
};

class ZHttpUrl
{
public:
	enum class EVerb : int32_t
	{
		eNONE,
		eGET,
		ePOST,
		ePUT,
		eHEAD,
	};

	static ZString EVerbToStr(EVerb p_Verb)
	{
		switch (p_Verb)
		{
			case EVerb::eGET:
				return "GET";

			case EVerb::ePOST:
				return "POST";

			case EVerb::ePUT:
				return "PUT";

			case EVerb::eHEAD:
				return "HEAD";

			default:
				return "UNKNOWN";
		}
	}

public:
	virtual ~ZHttpUrl() {}

public:
	/*ZString fullUrl() const
	{
		std::string s_URL;
		s_URL = m_protocol.c_str();
		s_URL += "://";
		s_URL += m_host.c_str();
		s_URL += m_uri.c_str();

		for (auto it = m_queryArgs.begin(); it != m_queryArgs.end(); ++it)
		{
			if (it == m_queryArgs.begin())
				s_URL += "?";
			else
				s_URL += "&";

			s_URL += it->first.c_str();
			s_URL += "=";
			s_URL += it->second.c_str();
		}

		return s_URL;
	}*/

public:
	EVerb m_eVerb;
	ZString m_protocol;
	ZString m_host;
	uint16_t m_nPort;
	ZString m_uri;
	TArray<TPair<ZString, ZString>> m_queryArgs;
};

class SHttpRequestBehavior
{
public:
	void (*unk00)();
	void (*unk01)();
	void (*unk02)();
	void (*unk03)();
	void (*unk04)();
	void (*unk05)();
};

class ZHttpRequestParams
{
public:
	ZHttpUrl m_url;
	ZString m_body;
	PAD(0x08);
	TArray<ZHttpHeader> m_headers;
	SHttpRequestBehavior* m_pHttpRequestBehavior;
	PAD(0x08);
	ZHttpResultBase* m_pResult;
};

class IHttpRequest
{
public:
	virtual ~IHttpRequest() = 0;

public:
	int32_t m_nUnk00;
	int32_t m_nUnk01;
	bool m_bUnk02;
	int32_t m_nUnk03;
	PAD(0x1C);
	ZHttpUrl m_url;
	PAD(0x08);
	SHttpRequestBehavior* N00000350;
};

class ZHttpRequestWindows :
	public IHttpRequest
{

};

class ZHttpResultDynamicObject
{
public:
	virtual ~ZHttpResultDynamicObject() = default;

public:
	PAD(0x38);
	ZBuffer m_buffer;
};
