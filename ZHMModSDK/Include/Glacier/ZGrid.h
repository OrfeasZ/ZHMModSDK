#pragma once

#include "ZMath.h"
#include "TArray.h"
#include "ZBitArray.h"
#include "ZPathfinder.h"

struct SGWaypoint {
    union {
        struct {
            short nNeighbor0;
            short nNeighbor1;
            short nNeighbor2;
            short nNeighbor3;
            short nNeighbor4;
            short nNeighbor5;
            short nNeighbor6;
            short nNeighbor7;
        };

        short Neighbors[8];
    };

    float4 vPos;
    unsigned int nVisionDataOffset;
    short nLayerIndex;
};

struct SGProperties {
    float4 vMin;
    float4 vMax;
    int32 nGridWidth;
    float32 fGridSpacing;
    int32 nVisibilityRange;
};

struct SGCellCoords {
    short x;
    short y;
};

struct SReasoningGrid {
    const SGWaypoint* GetNode(unsigned short nIndex) const {
        return &m_WaypointList[nIndex];
    }

    SGCellCoords CalculateCoordinates(const float4& vPos) const {
        const float4 vertexPosition = vPos - m_Properties.vMin;
        const short x = static_cast<short>(vertexPosition.x * (1.f / m_Properties.fGridSpacing));
        const short y = static_cast<short>(vertexPosition.y * (1.f / m_Properties.fGridSpacing));

        return SGCellCoords {x, y};
    }

    bool HasVisibilityLow(unsigned int nFromNode, unsigned int nToNode) const {
        return HasVisibility(nFromNode, nToNode, 1);
    }

    bool HasVisibilityHigh(unsigned int nFromNode, unsigned int nToNode) const {
        return HasVisibility(nFromNode, nToNode, 0);
    }

    bool HasVisibility(unsigned int nFromNode, unsigned int nToNode, unsigned char nLow) const {
        const SGWaypoint* fromWaypoint = &m_WaypointList[nFromNode];
        const SGWaypoint* toWaypoint = &m_WaypointList[nToNode];
        const SGCellCoords fromNodeCellCoordinates = CalculateCoordinates(fromWaypoint->vPos);
        const SGCellCoords toNodeCellCoordinates = CalculateCoordinates(toWaypoint->vPos);

        if (abs(fromNodeCellCoordinates.x - toNodeCellCoordinates.y) > m_Properties.nVisibilityRange ||
            abs(fromNodeCellCoordinates.y - toNodeCellCoordinates.y) > m_Properties.nVisibilityRange) {
            return false;
        }

        const unsigned char* fromVisionData = &m_pVisibilityData[fromWaypoint->nVisionDataOffset];
        const unsigned short layerCount = *reinterpret_cast<const unsigned short*>(fromVisionData);
        const unsigned short layerIndex = toWaypoint->nLayerIndex;
        unsigned short layerDataIndex = 0;

        if (layerIndex != fromWaypoint->nLayerIndex) {
            bool layerFound = false;

            for (unsigned short i = 0; i < layerCount; ++i) {
                const unsigned short currentLayer = *reinterpret_cast<const unsigned short*>(&fromVisionData[2 * i +
                    2]);

                if (currentLayer == layerIndex) {
                    layerDataIndex = i + 1;
                    layerFound = true;

                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        int bitsWidth = 2 * m_Properties.nVisibilityRange + 1;
        int xOffset = m_Properties.nVisibilityRange + toNodeCellCoordinates.x - fromNodeCellCoordinates.x;
        int yOffset = m_Properties.nVisibilityRange + toNodeCellCoordinates.y - fromNodeCellCoordinates.y;
        int layerDepth = nLow + 2 * layerDataIndex;
        int bitIndex = xOffset + bitsWidth * (yOffset + bitsWidth * layerDepth);

        const unsigned char* visibilityArray = &fromVisionData[2 * layerCount + 2];
        ZBitArray bitArray;

        bitArray.m_aBytes.m_pBegin = const_cast<unsigned char*>(visibilityArray);

        bool hasVisibility = bitArray.Get(bitIndex);

        bitArray.m_aBytes.m_pBegin = nullptr;

        return hasVisibility;
    }

    TArray<SGWaypoint> m_WaypointList;
    ZBitArray m_LowVisibilityBits;
    ZBitArray m_HighVisibilityBits;
    SGProperties m_Properties;
    uint32 m_nNodeCount;
    TArray<uint8> m_pVisibilityData;
    ZBitArray m_deadEndData;
};

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

class ZGridManager : public IComponentInterface {
public:
    float4 CellToPosition(int x, int y, float z, const SGProperties& properties) const {
        float4 result;

        result.x = static_cast<float>(x) * properties.fGridSpacing + properties.vMin.x;
        result.y = static_cast<float>(y) * properties.fGridSpacing + properties.vMin.y;
        result.z = z;

        return result;
    }

    float4 GetCellUpperLeft(const float4& vPosition, const SGProperties& properties) const {
        const SGCellCoords cellCoordinates = (*Globals::ActiveGrid)->CalculateCoordinates(vPosition);

        return CellToPosition(cellCoordinates.x, cellCoordinates.y, vPosition.z, properties);
    }

    unsigned int GetHeatmapColorFromRating(float fRating) {
        if (fRating < 0.0f) {
            return 0xFF000000;
        }

        if (fRating > 1.0f) {
            return 0xFFFFFFFF;
        }

        if (fRating < 0.5f) {
            int color = static_cast<int>((fRating * 2.0f) * 255.0f);
            color = std::clamp(color, 0, 255);

            return ((color << 8) - 0xFFFF01);
        }
        else {
            int color = static_cast<int>((1.0f - (fRating - 0.5f) * 2.0f) * 255.0f);
            color = std::clamp(color, 0, 255);

            return color - 0xFF0100;
        }
    }
};