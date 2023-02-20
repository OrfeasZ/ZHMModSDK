#pragma once

#include "ZRender.h"
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
public:
    ZResourcePtr m_VertexPaintData; // 0x170
    ZString m_sVertexPaintSourceResourceId; // 0x178
    ZResourcePtr m_ResourceID; // 0x188
    SVector3 m_PrimitiveScale; // 0x190
    ESeamFixMode m_eSeamFix; // 0x19C
};

static_assert(sizeof(ZRenderableEntity) == 0xD0);
static_assert(offsetof(ZGeomEntity, m_ResourceID) == 0x188);

class IRenderDestinationTextureEntity : public IRenderDestinationEntity
{
public:
    virtual void IRenderDestinationTextureEntity_unk32() = 0;
    virtual TArray<ZEntityRef>* GetClients() = 0;
    virtual void AddClient(const ZEntityRef&) = 0;
    virtual void RemoveClient(const ZEntityRef&) = 0;
    virtual void IRenderDestinationTextureEntity_unk36() = 0;
};

class ZRenderDestinationTextureEntity : public ZRenderableEntity, public IRenderDestinationTextureEntity
{
public:
    TArray<TEntityRef<ZRenderableEntity>> m_aMultiSource; // 0xD8
    int32 m_nSelectedSource; // 0xF0
    uint32 m_nWidth; // 0xF4
    uint32 m_nHeight; // 0xF8
    float32 m_fOutputScale; // 0xFC
    bool m_bUseSimpleRendering; // 0x100
    bool m_bIsHDR; // 0x101
    bool m_bSharedResource; // 0x102
    bool m_bNeedsGamma; // 0x103
    bool m_bUseBGRA; // 0x104
    bool m_bIsPIP; // 0x105
    bool m_bDrawShadows; // 0x106
    bool m_bDrawCrowds; // 0x107
    bool m_bDrawWater; // 0x108
    bool m_bDrawDecals; // 0x109
    bool m_bDrawScatter; // 0x10A
    bool m_bDrawTransparent; // 0x10B
    bool m_bDrawGates; // 0x10C
    int32 m_nGateTraversalDepth; // 0x110
    bool m_bDrawRefractions; // 0x114
    bool m_bDrawAtmosphericScattering; // 0x115
    bool m_bDrawUI; // 0x116
};

static_assert(offsetof(ZRenderDestinationTextureEntity, m_aMultiSource) == 0xD8);
static_assert(offsetof(ZRenderDestinationTextureEntity, m_bDrawUI) == 0x116);
