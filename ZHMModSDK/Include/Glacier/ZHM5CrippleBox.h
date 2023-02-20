#pragma once

#include "ZEntity.h"
#include "ZHitman5.h"

class ZHeroReference : public ZEntityImpl
{
public:
    TEntityRef<ZHitman5> m_rHitmanCharacter; // 0x18
};

class ZHM5CrippleBox : public ZHeroReference, public ISavableEntity
{
public:
    PAD(0x8);
    bool m_bInSequence; // 0x38
    bool m_bAllowBaseMovements; // 0x39
    bool m_bSequenceAllowCamera; // 0x3A
    bool m_bActivateOnStart; // 0x3B
    bool m_bLedges; // 0x3C
    bool m_bCover; // 0x3D
    bool m_bTakeClothes; // 0x3E
    bool m_bLadder; // 0x3F
    bool m_bPullVictimFromWindow; // 0x40
    bool m_bFiberWireKill; // 0x41
    bool m_bClimbWindow; // 0x42
    bool m_bThrowBodyOverRail; // 0x43
    bool m_bDumpBody; // 0x44
    bool m_bOperateCPDoor; // 0x45
    bool m_bHideInCloset; // 0x46
    bool m_bCloseCombat; // 0x47
    bool m_bGrabVictim; // 0x48
    bool m_bPushVictimThroughWindowAndRail; // 0x49
    bool m_bKickVictimOverLedge; // 0x4A
    bool m_bPickupItem; // 0x4B
    bool m_bDropItem; // 0x4C
    bool m_bDragBody; // 0x4D
    bool m_bThrowItem; // 0x4E
    bool m_bPlaceItem; // 0x4F
    bool m_bInteractions; // 0x50
    bool m_bUseDeathAnimation; // 0x51
    bool m_bRun; // 0x52
    bool m_bTurn; // 0x53
    bool m_bSneak; // 0x54
    bool m_bNoSnapSneak; // 0x55
    bool m_bStandUp; // 0x56
    bool m_bFastWalk; // 0x57
    bool m_bMovementAllowed; // 0x58
    bool m_bIdleAnimationsAllowed; // 0x59
    bool m_bItems; // 0x5A
    bool m_bCanHolsterItems; // 0x5B
    bool m_bCoverTakedown; // 0x5C
    bool m_bCoverScale; // 0x5D
    bool m_bCoverToCover; // 0x5E
    bool m_bCloseCombatSnapNeck; // 0x5F
    bool m_bCloseCombatChoke; // 0x60
    bool m_bCloseCombatPistolFinish; // 0x61
    bool m_bCloseCombatProps; // 0x62
    bool m_bCloseCombatStandard; // 0x63
    bool m_bCloseCombatFakeSwing; // 0x64
    bool m_bGameCameraAutoAlign; // 0x65
    bool m_bCameraSide; // 0x66
    bool m_bNotSelfieMode; // 0x67
    bool m_bInstinct; // 0x68
    bool m_bBlindFire; // 0x69
    bool m_bAim; // 0x6A
    bool m_bHairTrigger; // 0x6B
    bool m_bFire; // 0x6C
    bool m_bLimitedAmmo; // 0x6D
    bool m_bOpenLogbook; // 0x6E
    bool m_bOpenPauseMenu; // 0x6F
    PAD(0x1D);
};
