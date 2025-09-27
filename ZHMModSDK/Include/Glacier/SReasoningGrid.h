#pragma once

#include "ZMath.h"
#include "TArray.h"

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

class ZBitArray {
public:
    bool Get(unsigned int nBitIndex) const {
        const unsigned int byteIndex = nBitIndex >> 3;
        const unsigned int bitOffset = nBitIndex & 7;

        return (1 << bitOffset) & m_aBytes[byteIndex];
    }

    TArray<uint8> m_aBytes;
    uint32 m_nSize;
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