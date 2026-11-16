#pragma once

#include "TArray.h"
#include "ZEntity.h"

class ZDelayedExplodeInfo {
public:
    PAD(0x08);
    ZEntityRef m_Target; // 0x08
    PAD(0x340); // 0x10
};

class ZMassImpulseCharacterExploder {
public:
    TArray<ZDelayedExplodeInfo> m_aDelayedTargets; // 0x00
    PAD(0x08); // 0x18
};

class ZMassImpulseCollisionChecker {
public:
    struct SCollisionCheck {
        ZDelayedExplodeInfo m_ExplodeInfo; // 0x00
        PAD(0x90); // 0x350, TMaxArray<IAsyncRayHandle*, 16>
    };

    PAD(0x08);
    TArray<SCollisionCheck> m_aCollisionChecks; // 0x08
};

static_assert(offsetof(ZDelayedExplodeInfo, m_Target) == 0x08);
static_assert(sizeof(ZDelayedExplodeInfo) == 0x350);
static_assert(sizeof(ZMassImpulseCharacterExploder) == 0x20);
static_assert(sizeof(ZMassImpulseCollisionChecker::SCollisionCheck) == 0x3E0);
static_assert(offsetof(ZMassImpulseCollisionChecker, m_aCollisionChecks) == 0x08);
