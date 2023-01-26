#pragma once

#include "ZSpatialEntity.h"
#include "ZResource.h"

class ZRenderableEntity : public ZBoundedEntity //Size: 0xD0
{
	PAD(0x18);
};

class ZPrimitiveContainerEntity : public ZRenderableEntity //Size: 0x170
{
	PAD(0xA0);
};

class ZGeomEntity : public ZPrimitiveContainerEntity //Size: 0x1B0
{
	PAD(0x8); //TResourcePtr m_VertexPaintData; // 0x170
	ZString m_sVertexPaintSourceResourceId; // 0x178
	ZResourcePtr m_ResourceID; // 0x188
	SVector3 m_PrimitiveScale; // 0x190
	ESeamFixMode m_eSeamFix; // 0x19C
};

static_assert(sizeof(ZRenderableEntity) == 0xD0);
