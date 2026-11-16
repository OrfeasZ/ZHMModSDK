#pragma once

#include "TArray.h"
#include "ZEntity.h"

class ZDistanceConditionManager {
public:
    struct SDistanceConditionNewData {
        ZEntityRef m_Entity; // 0x00
        void* m_pCondition; // 0x08
        bool m_bActivated; // 0x10
    };

    PAD(0x08);
    TArray<SDistanceConditionNewData> m_aPendingChanges; // 0x08
};

static_assert(sizeof(ZDistanceConditionManager::SDistanceConditionNewData) == 0x18);
