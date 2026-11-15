#pragma once

#include "Common.h"
#include "ZPrimitives.h"

struct SAISoundConnectorEdge {
    uint16 m_nToWaypoint;
    float32 m_fDistance;
};

struct SAISoundConnectorNode {
    PAD(0x10);
    SAISoundConnectorNode* m_pLeft; // 0x10
    SAISoundConnectorNode* m_pRight; // 0x18
    uint16 m_nFromWaypoint; // 0x20
    SAISoundConnectorEdge m_Edge; // 0x24
};

static_assert(offsetof(SAISoundConnectorNode, m_Edge) == 0x24);

struct SAISoundConnectorTree {
    PAD(0x18);
    SAISoundConnectorNode* m_pRoot; // 0x18
};

class ZSharedSoundSensor {
public:
    PAD(0x18);
    SAISoundConnectorTree m_SoundConnectors; // 0x18, keyed by source waypoint
};

static_assert(offsetof(ZSharedSoundSensor, m_SoundConnectors) == 0x18);
