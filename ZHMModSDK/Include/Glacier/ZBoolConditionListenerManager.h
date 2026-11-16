#pragma once

#include "THashMap.h"
#include "TArray.h"
#include "ZEntity.h"

class IBoolCondition;

class ZBoolConditionListenerManager {
public:
    PAD(0x08);
    THashMap<IBoolCondition*, TArray<ZEntityRef>> m_Listeners; // 0x08
};
