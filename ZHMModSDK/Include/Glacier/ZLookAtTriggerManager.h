#pragma once

#include "THashMap.h"
#include "ZEntity.h"

// Registering adds to m_PendingTriggers, and the frame update moves it to m_Triggers.
class ZLookAtTriggerManager {
public:
    struct SPendingTrigger {
        PAD(0x28);
    };

    struct STrigger {
        PAD(0x18);
    };

    PAD(0x08);
    THashMap<ZEntityRef, SPendingTrigger> m_PendingTriggers; // 0x08
    THashMap<ZEntityRef, STrigger> m_Triggers; // 0x28
};

static_assert(offsetof(ZLookAtTriggerManager, m_PendingTriggers) == 0x08);
static_assert(offsetof(ZLookAtTriggerManager, m_Triggers) == 0x28);
static_assert(sizeof(THashMapNode<TPair<ZEntityRef, ZLookAtTriggerManager::SPendingTrigger>>) == 0x38);
static_assert(sizeof(THashMapNode<TPair<ZEntityRef, ZLookAtTriggerManager::STrigger>>) == 0x28);
