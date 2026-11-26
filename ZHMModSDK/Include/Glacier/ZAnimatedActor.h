#pragma once

#include "ZPathfinder.h"

class ZNPCMorphemeNetworkController;

struct SAnimatedActorNavState {
    PAD(0x24);
    ZPFLocation m_closestLocationOnNavmesh; // 0x24
    ZPFLocation m_lastValidLocationOnNavmesh; // 0x44
    PAD(0x16C); // 0x64
};

static_assert(offsetof(SAnimatedActorNavState, m_closestLocationOnNavmesh) == 0x24);
static_assert(offsetof(SAnimatedActorNavState, m_lastValidLocationOnNavmesh) == 0x44);
static_assert(sizeof(SAnimatedActorNavState) == 0x1D0);

class ZAnimatedActor {
public:
    PAD(0xD8);
    SAnimatedActorNavState* m_pNavState; // 0xD8
};

class ZAnimProgramBase {
public:
    PAD(0x8);
    ZNPCMorphemeNetworkController* m_pController; // 0x8
    ZAnimatedActor* m_pAnimatedActor; // 0x10
};

class ZMoveAnimProgram : public ZAnimProgramBase {};
