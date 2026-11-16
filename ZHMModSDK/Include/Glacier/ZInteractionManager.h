#pragma once

#include "TArray.h"
#include "ZEntity.h"

class ZInteractionData {
public:
    PAD(0x138);
    ZEntityRef m_Entity; // 0x138
    void* m_pInterface; // 0x140
    PAD(0x08); // 0x148
};

static_assert(sizeof(ZInteractionData) == 0x150);

// Deleted entities stay in m_aInteractions until the next rebuild.
class ZInteractionManager {
public:
    PAD(0x38);
    TArray<ZInteractionData> m_aInteractions; // 0x38
};
static_assert(offsetof(ZInteractionManager, m_aInteractions) == 0x38);
