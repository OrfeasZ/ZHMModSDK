#pragma once

#include "SReasoningGrid.h"
#include "ZPathfinder.h"

#include "Reflection.h"
#include "Globals.h"

class ZGridNodeRef {
public:
    ZGridNodeRef(unsigned short nNodeIndex) {
        m_nNodeIndex = nNodeIndex;
        m_nRoomID = -1;
        m_pNode = (*Globals::ActiveGrid)->GetNode(nNodeIndex);
    }

    bool CheckVisibility(const ZGridNodeRef& pOther, bool bLow, bool bCheckDoors) const {
        if (!m_pNode || !pOther.m_pNode) {
            return false;
        }

        unsigned int fromNodeIndex = m_nNodeIndex;
        unsigned int toNodeIndex = pOther.m_nNodeIndex;

        if (bLow) {
            return (*Globals::ActiveGrid)->HasVisibilityLow(fromNodeIndex, toNodeIndex);
        }

        return (*Globals::ActiveGrid)->HasVisibilityHigh(fromNodeIndex, toNodeIndex);
    }

    const SGWaypoint* GetNode() const {
        return m_pNode;
    }

    const SGWaypoint* m_pNode;
    uint16 m_nNodeIndex;
    uint16 m_nRoomID;
};

class ZHM5GridManager : public IComponentInterface {
public:
    PAD(0x40);
    ZPFLocation m_HitmanPFLocation; // 0x48
    ZGridNodeRef m_HitmanNode; //0x68
};