#pragma once

#include "ZPrimitives.h"
#include "TArray.h"

class ZKnowledge {};

struct SBehaviorBase {
    ECompiledBehaviorType eBehaviorType;
};

class ZBehaviorService {
public:
    virtual ~ZBehaviorService() = 0;

    struct SBehaviorState {
        PAD(0xA0); // 0x0
        SBehaviorBase* m_pCurrentBehavior; // 0xA0
        PAD(0x50); // 0xA8
    };

    TFixedArray<SBehaviorState, 500> m_aBehaviorStates;
};

static_assert(sizeof(ZBehaviorService::SBehaviorState) == 248);