#pragma once

#include "Common.h"
#include "ZGrid.h"

class ZKnowledge {
public:
    PAD(0x20);
    ZGridNodeRef m_actorGridNode; // 0x20
    PAD(0xD0);
};

static_assert(sizeof(ZKnowledge) == 256);