#pragma once

#include "ZEntity.h"

class ZCharacterStateMachineCoordinator;
class ZCharacterScheduler;

class ZCharacterSubcontroller : public ZEntityImpl //Size: 0x38
{
};

class ZCharacterSubcontrollerInventory : public ZCharacterSubcontroller //Size: 0x310
{
public:
    PAD(0x140);
    uint32 m_nMaxGunAmmo; // 0x158
    uint32 m_nMaxRevolverAmmo; // 0x15C
    uint32 m_nMaxSMGAmmo; // 0x160
    uint32 m_nMaxRifleAmmo; // 0x164
    uint32 m_nMaxSniperAmmo; // 0x168
    uint32 m_nMaxMGAmmo; // 0x16C
    uint32 m_nMaxRPGAmmo; // 0x170
    uint32 m_nMaxShotgunAmmo; // 0x174
    uint32 m_nMaxTranqAmmo; // 0x178
    uint32 m_nMaxPistolLightAmmo; // 0x17C
    uint32 m_nMaxShotgunBeanbagAmmo; // 0x180
};

class ZCharacterSubcontrollerContainer : public ZEntityImpl //Size: 0x1E0
{
public:
    PAD(0x1B0);
    TArray<TEntityRef<ZCharacterSubcontroller>> m_aReferencedControllers; // 0x1C8
};

class ZCharacter : public ZEntityImpl //Size: 0x48
{
public:
    TEntityRef<ZCharacterStateMachineCoordinator> m_rStateMachineCoordinator; // 0x18
    TEntityRef<ZCharacterSubcontrollerContainer> m_rSubcontrollerContainer; // 0x28
    TEntityRef<ZCharacterScheduler> m_rScheduler; // 0x38
};
