#pragma once

#include "ZRender.h"
#include "ZSpatialEntity.h"
#include "ZResource.h"

class ZRenderableEntity : public ZBoundedEntity //Size: 0xD0
{
    PAD(0x18);
};

// Size: 0x70
class ZRenderGeometryBuffer
{
public:
    virtual ~ZRenderGeometryBuffer() = default; // TODO

public:
    PAD(0x08); // 0x08
    uint32_t m_nSize; // 0x10
    PAD(0x20); // 0x18
    ID3D12Resource* m_pResource; // 0x38
    PAD(0x18); // 0x40
    char* m_pCPUBuffer; // 0x58
    PAD(0x10); // 0x60
};

static_assert(offsetof(ZRenderGeometryBuffer, m_pResource) == 0x38);
static_assert(offsetof(ZRenderGeometryBuffer, m_pCPUBuffer) == 0x58);
static_assert(sizeof(ZRenderGeometryBuffer) == 0x70);

class ZRenderVertexBuffer : public ZRenderGeometryBuffer {};
class ZRenderIndexBuffer : public ZRenderGeometryBuffer {};

template <class T>
class TRenderReferencedCountedImpl : public T
{
public:
    int32_t m_ReferenceCount;
};

class IRenderPrimitive : public TRenderReferencedCountedImpl<IRenderRefCount>
{
public:
    PAD(0x56); // 0x10
    uint16_t m_BufferDataIndex; // 0x66
};

template <class T>
class TRefCountPtrArg
{
public:
    T* m_pObject;
};

template <class T>
class TRefCountPtr : public TRefCountPtrArg<T>
{
};


class ZPrimitiveContainerEntity : public ZRenderableEntity //Size: 0x170
{
public:
    PAD(0x08);
    TArray<TRefCountPtr<IRenderPrimitive>> m_Primitives; // 0xD8
    PAD(0x80); // 0xF0
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

// Size: 32 (0x20)
struct SRenderPrimitiveMeshDesc
{
    uint32_t nNumVertices; // 0x00
    uint32_t mNumIndices; // 0x04
    PAD(0x10); // 0x08
    uint8_t nNumStreams; // 0x18
    uint8_t anStreamStride[4]; // 0x19
    PAD(0x03); // 0x1D
};

static_assert(sizeof(SRenderPrimitiveMeshDesc) == 0x20);

// vvv Members Unverified vvv
struct SRenderInputElementDesc
{
    uint8 nOffset;
    __int32 eFormat : 8;
    __int32 eElement : 8;
    uint8 nElementIndex;
    uint8 nStreamIndex;
    __int32 eClassification : 8;
    uint16 nInstanceDataStepRate;
};

static_assert(sizeof(SRenderInputElementDesc) == 20);

class ZRenderInputLayout
{
public:
    virtual ~ZRenderInputLayout() = default; // TODO

public:
    PAD(0x08); // 0x08
    uint32_t m_nNumElements; // 0x10
    SRenderInputElementDesc m_Elements[16]; // 0x14
    D3D12_INPUT_ELEMENT_DESC m_ElementDesc[16]; // 0x158
};

static_assert(offsetof(ZRenderInputLayout, m_nNumElements) == 0x10);
static_assert(offsetof(ZRenderInputLayout, m_Elements) == 0x14);
static_assert(offsetof(ZRenderInputLayout, m_ElementDesc) == 0x158);

struct SPrimitiveBufferData
{
    SRenderPrimitiveMeshDesc m_MeshDesc; // 0x00
    PAD(0x38); // 0x20
    ZRenderInputLayout* m_pInputLayout; // 0x58
    PAD(0x08); // 0x60
    ZRenderIndexBuffer* m_pIndexBuffer; // 0x68
    ZRenderVertexBuffer* m_pVertexBuffers[4]; // 0x70
    PAD(0x10); // 0x90
};

static_assert(offsetof(SPrimitiveBufferData, m_pInputLayout) == 0x58);
static_assert(offsetof(SPrimitiveBufferData, m_pIndexBuffer) == 0x68);
static_assert(offsetof(SPrimitiveBufferData, m_pVertexBuffers) == 0x70);
static_assert(sizeof(SPrimitiveBufferData) == 160);

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
