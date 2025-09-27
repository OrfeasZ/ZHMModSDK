#pragma once

#include "ZMath.h"

class ZColor
{
public:
	static SVector4 UnpackUnsigned(const unsigned int lPackedVec4)
	{
		const unsigned int alpha = (lPackedVec4 >> 24) & 0xFF;
		const unsigned int blue = (lPackedVec4 >> 16) & 0xFF;
		const unsigned int green = (lPackedVec4 >> 8) & 0xFF;
		const unsigned int red = lPackedVec4 & 0xFF;
        SVector4 result;

		result.x = static_cast<float>(red) / 255.0f;
		result.y = static_cast<float>(green) / 255.0f;
		result.z = static_cast<float>(blue) / 255.0f;
		result.w = static_cast<float>(alpha) / 255.0f;

		return result;
	}
};
