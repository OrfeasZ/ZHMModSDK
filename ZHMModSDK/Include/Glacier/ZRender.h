#pragma once

#include <directx/d3d12.h>
#include <dxgi.h>

#include "ZMath.h"
#include "Reflection.h"
#include "ZObjectPool.h"
#include "ZResource.h"

template <class T, bool U>
class TRenderReferencedCountedImpl : public T {
public:
    int32_t m_ReferenceCount;
};

class ZRenderTexture2D;
class ZEntityRef;

class ZRenderTargetView;
class ZRenderUnorderedAccessView;

class ZRenderDepthStencilView {
public:
    virtual ~ZRenderDepthStencilView() = 0;

public:
    PAD(0x20);
    ZRenderTexture2D* m_pTexture;
    PAD(0x08);
};

static_assert(sizeof(ZRenderDepthStencilView) == 56);
static_assert(offsetof(ZRenderDepthStencilView, m_pTexture) == 0x28);

class ZRenderShaderResourceView {
public:
    virtual ~ZRenderShaderResourceView() = 0;

public:
    PAD(0xC); // 0x08
    int32_t m_nHeapDescriptorIndex; // 0x14
    PAD(0x20); // 0x18
    D3D12_CPU_DESCRIPTOR_HANDLE m_Handle; // 0x38
};

static_assert(offsetof(ZRenderShaderResourceView, m_nHeapDescriptorIndex) == 0x14);
static_assert(offsetof(ZRenderShaderResourceView, m_Handle) == 0x38);

struct SD3D12ObjectPools {
    PAD(0x5b0);
    TObjectPool<ZRenderTargetView> RenderTargetViews;
    TObjectPool<ZRenderDepthStencilView> DepthStencilViews;
    TObjectPool<ZRenderShaderResourceView> ShaderResourceViews;
    TObjectPool<ZRenderUnorderedAccessView> UnorderedAccessViews;
};

class ZRenderSharedResources {
public:
    PAD(0x4680);
    ZRenderDepthStencilView* m_pDepthStencilView;
};

class IRenderRefCount {
public:
    virtual ~IRenderRefCount() = 0;
    virtual void AddRef() = 0;
    virtual uint32_t Release() = 0;
};

class IRenderDestination : public IRenderRefCount {
public:
};

class IRenderDestinationEntity :
        public IComponentInterface {
public:
    virtual ZEntityRef* GetSource() = 0;
    virtual void IRenderDestinationEntity_unk6() = 0;
    virtual IRenderDestination* GetRenderDestination() const = 0;
    virtual void IRenderDestinationEntity_unk8() = 0;
    virtual void SetSource(ZEntityRef*) = 0;
    virtual void IRenderDestinationEntity_unk10() = 0;
    virtual void IRenderDestinationEntity_unk11() = 0;
    virtual void IRenderDestinationEntity_unk12() = 0;
    virtual void IRenderDestinationEntity_unk13() = 0;
    virtual void IRenderDestinationEntity_unk14() = 0;
    virtual void IRenderDestinationEntity_unk15() = 0;
    virtual void IRenderDestinationEntity_unk16() = 0;
    virtual void IRenderDestinationEntity_unk17() = 0;
    virtual void IRenderDestinationEntity_unk18() = 0;
    virtual void IRenderDestinationEntity_unk19() = 0;
    virtual void IRenderDestinationEntity_unk20() = 0;
    virtual void IRenderDestinationEntity_unk21() = 0;
    virtual void IRenderDestinationEntity_unk22() = 0;
    virtual void IRenderDestinationEntity_unk23() = 0;
    virtual void IRenderDestinationEntity_unk24() = 0;
    virtual void IRenderDestinationEntity_unk25() = 0;
    virtual void IRenderDestinationEntity_unk26() = 0;
    virtual void IRenderDestinationEntity_unk27() = 0;
    virtual void IRenderDestinationEntity_unk28() = 0;
    virtual void IRenderDestinationEntity_unk29() = 0;
    virtual void IRenderDestinationEntity_unk30() = 0;
    virtual void IRenderDestinationEntity_unk31() = 0;
};

class ZRenderSwapChain {
public:
    virtual ~ZRenderSwapChain();

public:
    IDXGIFactory1* m_pFactory;
    IDXGISwapChain* m_pSwapChain;
};

struct slConstants {
    char structTypeUuid[16];
    size_t structVersion;
    SMatrix44 cameraViewToClip;
    SMatrix44 clipToCameraView;
    SMatrix44 clipToLensClip;
    SMatrix44 clipToPrevClip;
    SMatrix44 prevClipToClip;
    SVector2 jitterOffset;
    SVector2 mvecScale;
    SVector2 cameraPinholeOffset;
    SVector3 cameraPos;
    SVector3 cameraUp;
    SVector3 cameraRight;
    SVector3 cameraFwd;
    float cameraNear;
    float cameraFar;
    float cameraFOV;
    float cameraAspectRatio;
    float motionVectorsInvalidValue;
    bool depthInverted;
    bool cameraMotionIncluded;
    bool motionVectors3D;
    bool reset;
    bool renderingGameFrames;
    bool orthographicProjection;
    bool motionVectorsDilated;
    bool motionVectorsJittered;
};

class ZRenderDevice {
public:
    virtual ~ZRenderDevice() = default;

public:
    PAD(0x400); // 0x08
    slConstants m_Constants; // 0x408
    PAD(0x104E0); // 0x5C8
    ZRenderSwapChain* m_pSwapChain; // 0x10AA8, look for ZRenderSwapChain constructor
    PAD(0x08); // 0x10A18
    ID3D12Device* m_pDevice; // 0x10A20
    PAD(0x30E9D88); // 0x10A28

    ID3D12CommandQueue* m_pCommandQueue;
    // 0x30FA848, look for "m_pFrameHeapCBVSRVUAV" string, first vtable call with + 128

    PAD(0x180f50); // 0x30FA850

    ID3D12DescriptorHeap* m_pDescriptorHeapDSV;
    // 0x327B7A0, look for "m_pDescriptorHeapDSV" string, argument to the left of it

    PAD(0x70); // 0x327B7A8

    ID3D12DescriptorHeap* m_pFrameHeapCBVSRVUAV;
    // 0x327B818, look for "m_pFrameHeapCBVSRVUAV" string, argument to the left of it
};

static_assert(offsetof(ZRenderDevice, m_Constants) == 0x408);
static_assert(offsetof(ZRenderDevice, m_pSwapChain) == 0x10AA8);
static_assert(offsetof(ZRenderDevice, m_pCommandQueue) == 0x30FA848);
static_assert(offsetof(ZRenderDevice, m_pDescriptorHeapDSV) == 0x327B7A0);
static_assert(offsetof(ZRenderDevice, m_pFrameHeapCBVSRVUAV) == 0x327B818);

class ZRenderContext {
public:
    PAD(0x200);
    SMatrix m_mViewToWorld; // 0x200
    SMatrix m_mWorldToView; // 0x240, function called by ZRenderContext_Unknown01, second pair of 4
    PAD(0xC0);
    SMatrix m_mViewToProjection; // 0x340, function called by ZRenderContext_Unknown01, first pair of 4
};

class ZRenderGraphNodeCamera {
public:
    virtual ~ZRenderGraphNodeCamera() = 0;

    PAD(0x5E0); // 0x8
    uint16 m_nStickyRoomId; // 0x5E8
    uint16 m_nNumOverlappingRooms; // 0x5EA
    uint16 m_aOverlappingRooms[32]; // 0x5EC
};

class ZRenderManager {
public:
    virtual ~ZRenderManager() = default;

public:
    PAD(0x14178);
    ZRenderDevice* m_pDevice; // 0x14180, look for ZRenderDevice constructor
    PAD(0x08); // 0x14188
    ZRenderSharedResources* m_pSharedResources; // 0x14190
    PAD(0x38); // 0x14198
    ZRenderGraphNodeCamera* m_pCurrentCamera; // 0x141D0
    PAD(0xA8);
    ZRenderContext* m_pRenderContext; // 0x14280
    // 0x14280, look for "ZRenderManager::RenderThread" string, first thing being constructed and assigned
};

static_assert(offsetof(ZRenderManager, m_pDevice) == 0x14180);
static_assert(offsetof(ZRenderManager, m_pSharedResources) == 0x14190);
static_assert(offsetof(ZRenderManager, m_pRenderContext) == 0x14280);

class ZRenderTexture2D {
public:
    virtual ~ZRenderTexture2D() = 0;

public:
    ID3D12Resource* m_pResource;
};

class ZRenderDestination : public IRenderDestination {
public:
    uint32_t m_nRefCount; // 0x08
    PAD(0x60); // 0x10
    ZRenderDevice* m_pDevice; // 0x70
    PAD(0x68); // 0x78
    ZRenderTexture2D* m_pTexture2D; // 0xE0 look for ZRenderDestination destructor, vtable check at the end
    ZRenderTargetView* m_pRenderTargetView; // 0xE8
    PAD(0x98); // 0xF0
    ZRenderShaderResourceView* m_pSRV; // 0x188
};

static_assert(offsetof(ZRenderDestination, m_pDevice) == 0x70);
static_assert(offsetof(ZRenderDestination, m_pTexture2D) == 0xE0);
static_assert(offsetof(ZRenderDestination, m_pSRV) == 0x188);

class RenderReferencedCountedBaseStub {
public:
    virtual ~RenderReferencedCountedBaseStub() = 0;
};

struct SSemanticStringPair {
    ZString m_MaterialPropertyName; // 0x0
    ZString m_ShaderParameterName; // 0x10
    int32_t m_Unk0; // 0x20
    bool m_Unk1; // 0x24
};

enum class ERenderConstBufferType {
    RENDER_CONST_BUFFER_VECTOR_1D = 1,
    RENDER_CONST_BUFFER_VECTOR_2D,
    RENDER_CONST_BUFFER_VECTOR_3D,
    RENDER_CONST_BUFFER_TRANSFORM_2D,
    RENDER_CONST_BUFFER_MATRIX_4X4 = 8,
    RENDER_CONST_BUFFER_TEXTURE_2D,
    RENDER_CONST_BUFFER_TEXTURE_3D,
    RENDER_CONST_BUFFER_TEXTURE_CUBE,
    RENDER_CONST_BUFFER_TEXTURE_CUBE_ARRAY = 13,
    RENDER_CONST_BUFFER_BUFFER = 15,
    RENDER_CONST_BUFFER_TEXTURE_2D_ARRAY
};

struct SRenderConstDesc {
    ERenderConstBufferType nType; // 0x0
    ZString Name; // 0x8
    uint16_t nOffset; // 0x18
    uint32_t nSize; // 0x1C
};

struct SRenderTextureDesc : SRenderConstDesc {
};

struct SRenderConstBufferDesc {
    PAD(0x10); // 0x0
    uint32_t nNumConstants; // 0x10
    uint32_t nNumTextures; // 0x14
    TArray<SRenderConstDesc> Constants; // 0x18
    TArray<SRenderTextureDesc> Textures; // 0x30
};

enum class EFX2ShaderType {
    FX2_SHADER_TYPE_VERTEX_SHADER,
    FX2_SHADER_TYPE_PIXEL_SHADER,
    FX2_SHADER_TYPE_GEOMETRY_SHADER,
    FX2_SHADER_TYPE_DOMAIN_SHADER,
    FX2_SHADER_TYPE_HULL_SHADE,
    FX2_SHADER_TYPE_COMPUTE_SHADER,
    FX2_SHADER_TYPE_RAYTRACING_SHADER,
    FX2_SHADER_TYPE_SIZE
};

class ZRenderShader {
public:
    EFX2ShaderType m_eShaderType; // 0x0
    SRenderConstBufferDesc m_Desc; // 0x8
    PAD(0x8); // 0x50
    const uint8_t* m_pByteCode; // 0x58
    uint32 m_nByteCodeSize; // 0x60
    ZString m_Name; // 0x68
};

class ZRenderEffectTechnique;

class ZRenderEffectPass {
public:
    virtual ~ZRenderEffectPass() = 0;

    ZRenderEffectTechnique* m_pTechnique; // 0x8
    ZRenderShader* m_pShader[static_cast<size_t>(EFX2ShaderType::FX2_SHADER_TYPE_SIZE)]; // 0x10
};

class ZRenderEffect;

class ZRenderEffectTechnique {
public:
    virtual ~ZRenderEffectTechnique() = 0;

    TArray<ZRenderEffectPass*> m_Passes; // 0x8
    ZRenderEffect* m_pEffect; // 0x20
};

class ZRenderEffect : public TRenderReferencedCountedImpl<RenderReferencedCountedBaseStub, false> {
public:
    int32_t m_nId; // 0x10
    THashMap<ZString, ZRenderEffectTechnique*, TDefaultHashMapPolicy<ZString>> m_Techniques; // 0x18
    TArray<ZRenderShader*> m_Programs; // 0x38
    TArray<SSemanticStringPair> m_SemanticStringPairs; // 0x50
};

struct STextureInfo {
    ZString Name; // 0x0
    uint8 nInterpretAs; // 0x10
    uint8 nDimension; // 0x11
    uint8 nResourceOffset; // 0x12
};

class ZRenderMaterialEffectData {
public:
    virtual ~ZRenderMaterialEffectData() = 0;

    ZRenderEffect* m_pRenderEffect; // 0x8
};

class ZRenderMaterialInstance : public TRenderReferencedCountedImpl<RenderReferencedCountedBaseStub, false> {
public:
    PAD(0xDA0); // 0x10
    TMaxArray<STextureInfo, 16> m_TextureInfo; // 0xDB0
    PAD(0x3C); // 0xF38
    TResourcePtr<ZRenderMaterialEffectData> m_pEffectData; // 0xF74
    ZRenderEffect* m_pEffect; // 0xF80
    ZResourcePtr m_pMaterialDescriptor; // 0xF88
};