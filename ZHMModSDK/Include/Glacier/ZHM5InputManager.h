#pragma once

#include "Reflection.h"

class ZHMInputManager : public IComponentInterface {
public:
};

class ZHM5InputControl {
public:
    PAD(0x138);
    bool m_bActive; // 0x130
};