#pragma once

#include "Common.h"
#include "Enums.h"

class ZActionMapTree
{
public:
	enum Type
	{
		eUNDEFINED = 0,
		eGET = 1,
		eCONSTANT = 2,
		eHOLD = 3,
		eREPEAT = 4,
		eTAP = 5,
		eRELEASE = 6,
		eDOWNEDGE = 7,
		eFASTTAP = 8,
		eHOLDDOWN = 9,
		eFIREONCEHOLDDOWN = 10,
		eCLICKHOLD = 11,
		ePRESS = 12,
		eAND = 13,
		eOR = 14,
		eGT = 15,
		eLT = 16,
		eSEQUENCE = 17,
		eANALOG = 18,
		eANALOGRAW = 19,
		eRELATIVE = 20,
		ePLUS = 21,
		eMINUS = 22,
		eMULT = 23
	};
};

class ZInputAction
{
public:
    ZInputAction(const char* p_Name) : m_szName(p_Name) {}

public:
    const char* m_szName;
    ZActionMapTree* m_pkMap = nullptr;
    int m_iSeq = -1;
};

class ZInputBinding
{
public:
    InputControlNamesp_eHM5InputAction m_eInputAction;
    ZInputAction m_Action;
};

class ZInputTokenStream
{
public:
	class ZTokenData
	{
	public:
		ZActionMapTree::Type eType;
		float fVal;
		char acVal[64];
	};

	const char* m_szData;
	ZTokenData kaTokens[1];
};
