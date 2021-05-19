#pragma once

#include "Common.h"
#include "Enums.h"

class ZInputAction
{
public:
	const char* m_szName;
	PAD(0x10);
};

class ZInputBinding
{
public:
	InputControlNamesp_eHM5InputAction m_eInputAction;
	ZInputAction m_Action;
};
