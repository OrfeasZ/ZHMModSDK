#pragma once

#include "Common.h"
#include "Enums.h"

class ZActionMapTree;

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
