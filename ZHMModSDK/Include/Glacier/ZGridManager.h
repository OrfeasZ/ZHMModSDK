#pragma once

#include "Reflection.h"
#include "SReasoningGrid.h"
#include "Globals.h"

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