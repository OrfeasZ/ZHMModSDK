#pragma once

#include <directx/d3d12.h>
#include <dxgi.h>

#include "ZMath.h"
#include "Reflection.h"
#include "ZObjectPool.h"
#include "TObjectPool.h"
#include "ZResource.h"

class ZRenderTexture2D;
class ZEntityRef;
class ZRenderInputLayout;
class ZRenderDepthStencilState;
class ZRenderBlendState;
class ZRenderRasterizerState;
class ZRenderTargetView;
class ZRenderUnorderedAccessView;
class ZRenderableEntity;

enum ERenderFormat : int16 {
    RENDER_FORMAT_NONE = 0x0,
    RENDER_FORMAT_UNKNOWN = 0x0,
    RENDER_FORMAT_R32G32B32A32_TYPELESS = 0x1,
    RENDER_FORMAT_R32G32B32A32_FLOAT = 0x2,
    RENDER_FORMAT_R32G32B32A32_UINT = 0x3,
    RENDER_FORMAT_R32G32B32A32_SINT = 0x4,
    RENDER_FORMAT_R32G32B32_TYPELESS = 0x5,
    RENDER_FORMAT_R32G32B32_FLOAT = 0x6,
    RENDER_FORMAT_R32G32B32_UINT = 0x7,
    RENDER_FORMAT_R32G32B32_SINT = 0x8,
    RENDER_FORMAT_R16G16B16A16_TYPELESS = 0x9,
    RENDER_FORMAT_R16G16B16A16_FLOAT = 0xA,
    RENDER_FORMAT_R16G16B16A16_UNORM = 0xB,
    RENDER_FORMAT_R16G16B16A16_UINT = 0xC,
    RENDER_FORMAT_R16G16B16A16_SNORM = 0xD,
    RENDER_FORMAT_R16G16B16A16_SINT = 0xE,
    RENDER_FORMAT_R32G32_TYPELESS = 0xF,
    RENDER_FORMAT_R32G32_FLOAT = 0x10,
    RENDER_FORMAT_R32G32_UINT = 0x11,
    RENDER_FORMAT_R32G32_SINT = 0x12,
    RENDER_FORMAT_R32G8X24_TYPELESS = 0x13,
    RENDER_FORMAT_D32_FLOAT_S8X24_UINT = 0x14,
    RENDER_FORMAT_R32_FLOAT_X8X24_TYPELESS = 0x15,
    RENDER_FORMAT_X32_TYPELESS_G8X24_UINT = 0x16,
    RENDER_FORMAT_R10G10B10A2_TYPELESS = 0x17,
    RENDER_FORMAT_R10G10B10A2_UNORM = 0x18,
    RENDER_FORMAT_R10G10B10A2_UINT = 0x19,
    RENDER_FORMAT_R11G11B10_FLOAT = 0x1A,
    RENDER_FORMAT_R8G8B8A8_TYPELESS = 0x1B,
    RENDER_FORMAT_R8G8B8A8_UNORM = 0x1C,
    RENDER_FORMAT_R8G8B8A8_UNORM_SRGB = 0x1D,
    RENDER_FORMAT_R8G8B8A8_UINT = 0x1E,
    RENDER_FORMAT_R8G8B8A8_SNORM = 0x1F,
    RENDER_FORMAT_R8G8B8A8_SINT = 0x20,
    RENDER_FORMAT_R16G16_TYPELESS = 0x21,
    RENDER_FORMAT_R16G16_FLOAT = 0x22,
    RENDER_FORMAT_R16G16_UNORM = 0x23,
    RENDER_FORMAT_R16G16_UINT = 0x24,
    RENDER_FORMAT_R16G16_SNORM = 0x25,
    RENDER_FORMAT_R16G16_SINT = 0x26,
    RENDER_FORMAT_R32_TYPELESS = 0x27,
    RENDER_FORMAT_D32_FLOAT = 0x28,
    RENDER_FORMAT_R32_FLOAT = 0x29,
    RENDER_FORMAT_R32_UINT = 0x2A,
    RENDER_FORMAT_R32_SINT = 0x2B,
    RENDER_FORMAT_R24G8_TYPELESS = 0x2C,
    RENDER_FORMAT_D24_UNORM_S8_UINT = 0x2D,
    RENDER_FORMAT_R24_UNORM_X8_TYPELESS = 0x2E,
    RENDER_FORMAT_X24_TYPELESS_G8_UINT = 0x2F,
    RENDER_FORMAT_R9G9B9E5_SHAREDEXP = 0x30,
    RENDER_FORMAT_R8G8_B8G8_UNORM = 0x31,
    RENDER_FORMAT_G8R8_G8B8_UNORM = 0x32,
    RENDER_FORMAT_R8G8_TYPELESS = 0x33,
    RENDER_FORMAT_R8G8_UNORM = 0x34,
    RENDER_FORMAT_R8G8_UINT = 0x35,
    RENDER_FORMAT_R8G8_SNORM = 0x36,
    RENDER_FORMAT_R8G8_SINT = 0x37,
    RENDER_FORMAT_R16_TYPELESS = 0x38,
    RENDER_FORMAT_R16_FLOAT = 0x39,
    RENDER_FORMAT_D16_UNORM = 0x3A,
    RENDER_FORMAT_R16_UNORM = 0x3B,
    RENDER_FORMAT_R16_UINT = 0x3C,
    RENDER_FORMAT_R16_SNORM = 0x3D,
    RENDER_FORMAT_R16_SINT = 0x3E,
    RENDER_FORMAT_B5G6R5_UNORM = 0x3F,
    RENDER_FORMAT_B5G5R5A1_UNORM = 0x40,
    RENDER_FORMAT_R8_TYPELESS = 0x41,
    RENDER_FORMAT_R8_UNORM = 0x42,
    RENDER_FORMAT_R8_UINT = 0x43,
    RENDER_FORMAT_R8_SNORM = 0x44,
    RENDER_FORMAT_R8_SINT = 0x45,
    RENDER_FORMAT_A8_UNORM = 0x46,
    RENDER_FORMAT_R1_UNORM = 0x47,
    RENDER_FORMAT_BC1_TYPELESS = 0x48,
    RENDER_FORMAT_BC1_UNORM = 0x49,
    RENDER_FORMAT_BC1_UNORM_SRGB = 0x4A,
    RENDER_FORMAT_BC2_TYPELESS = 0x4B,
    RENDER_FORMAT_BC2_UNORM = 0x4C,
    RENDER_FORMAT_BC2_UNORM_SRGB = 0x4D,
    RENDER_FORMAT_BC3_TYPELESS = 0x4E,
    RENDER_FORMAT_BC3_UNORM = 0x4F,
    RENDER_FORMAT_BC3_UNORM_SRGB = 0x50,
    RENDER_FORMAT_BC4_TYPELESS = 0x51,
    RENDER_FORMAT_BC4_UNORM = 0x52,
    RENDER_FORMAT_BC4_SNORM = 0x53,
    RENDER_FORMAT_BC5_TYPELESS = 0x54,
    RENDER_FORMAT_BC5_UNORM = 0x55,
    RENDER_FORMAT_BC5_SNORM = 0x56,
    RENDER_FORMAT_BC6H_UF16 = 0x57,
    RENDER_FORMAT_BC6H_SF16 = 0x58,
    RENDER_FORMAT_BC7_TYPELESS = 0x59,
    RENDER_FORMAT_BC7_UNORM = 0x5A,
    RENDER_FORMAT_BC7_UNORM_SRGB = 0x5B,
    RENDER_FORMAT_R16G16B16_FLOAT = 0x5C,
    RENDER_FORMAT_INDEX_32 = 0x5D,
    RENDER_FORMAT_INDEX_16 = 0x5E,
    RENDER_FORMAT_LE_X2R10G10B10_UNORM = 0x5F,
    RENDER_FORMAT_LE_X8R8G8B8_UNORM = 0x60,
    RENDER_FORMAT_X16Y16Z16_SNORM = 0x61,
    RENDER_FORMAT_B8G8R8A8_UNORM = 0x62,
    RENDER_FORMAT_B8G8R8A8_UNORM_SRGB = 0x63,
    NUM_RENDER_FORMATS = 0x64
};

enum ERenderResourceType {
    RENDER_RESOURCE_TYPE_TEXTURE2D = 1,
    RENDER_RESOURCE_TYPE_TEXTURE3D = 2,
    RENDER_RESOURCE_TYPE_BUFFER = 3
};

template <typename T, bool B>
class TRenderReferencedCountedImpl : public T {
public:
    int32_t m_ReferenceCount;
};

template <typename T, ERenderResourceType ResourceType>
class TRenderResourceImpl : public TRenderReferencedCountedImpl<T, false> {
};

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
    PAD(0xD0); //0x0
    TObjectPool<ZRenderInputLayout> RenderInputLayouts; // 0xD0
    PAD(0x50); //0x150
    TObjectPool<ZRenderDepthStencilState> RenderDepthStencilStates; // 0x1A0
    PAD(0x50); //0x220
    TObjectPool<ZRenderBlendState> RenderBlendStates; // 0x270
    PAD(0x50); //0x2F0
    TObjectPool<ZRenderRasterizerState> RenderRasterizerStates; // 0x340
    PAD(0x1F0); //0x3C0
    TObjectPool<ZRenderTargetView> RenderTargetViews; // 0x5B0
    TObjectPool<ZRenderDepthStencilView> RenderDepthStencilViews; // 0x630
    TObjectPool<ZRenderShaderResourceView> RenderShaderResourceViews; // 0x6B0
    TObjectPool<ZRenderUnorderedAccessView> RenderUnorderedAccessViews; // 0x730
};

class ZRenderSharedResources {
public:
    PAD(0x1A0);
    ZRenderTexture2D* m_pBoxReflectionCubeTexture[2]; // 0x1A0
    ZRenderTexture2D* m_pBoxReflectionDiffuseCubeTexture[2]; // 0x1B0
    ZRenderShaderResourceView* m_pBoxReflectionCubeTextureSRV[2]; // 0x1C0
    ZRenderShaderResourceView* m_pBoxReflectionDiffuseCubeTextureSRV[2]; // 0x1D0
    PAD(0x40); // 0x1E0
    int32_t m_nBoxReflectionCubeRenderTargetChunks; // 0x220
    PAD(0x449C); // 0x224
    uint32 m_nBoxReflectionMaxCubeMaps; // 0x46C0
    uint32 m_nBoxReflectionResolution; // 0x46C4
    uint32 m_nBoxReflectionRenderResolution; // 0x46C8
    uint32 m_nBoxReflectionMips; // 0x46CC
    uint32 m_nBoxReflectionUseBC6; // 0x46D0
    PAD(0x34DC); // 0x46D4
    bool m_SplitBoxReflectionCubeRenderTargets; // 0x7BB0
};

class IRenderRefCount {
public:
    virtual ~IRenderRefCount() = 0;
    virtual void AddRef() = 0;
    virtual uint32_t Release() = 0;
    virtual int32_t GetRefCount() const = 0;
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
    virtual ~ZRenderSwapChain() = 0;

public:
    IDXGIFactory1* m_pFactory;
    IDXGISwapChain* m_pSwapChain;
};

struct StructType {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
};

struct BaseStructure {
    BaseStructure* next;
    StructType structType;
    size_t structVersion;
};

struct slConstants : BaseStructure {
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
    bool orthographicProjection;
    bool motionVectorsDilated;
    bool motionVectorsJittered;
};

class ZRenderDeviceBase {
    virtual ~ZRenderDeviceBase() = 0;
};

class ZRenderDevice : public ZRenderDeviceBase {
public:
    virtual ~ZRenderDevice() = 0;

public:
    PAD(0x3F8); // 0x08
    slConstants m_Constants; // 0x400
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

static_assert(offsetof(ZRenderDevice, m_Constants) == 0x400);
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

class ZRenderManager : public IComponentInterface {
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

struct SRenderTexture2DDesc {
    uint32 nWidth; // 0x0
    uint32 nHeight; // 0x4
    uint32 nMipLevels; // 0x8
    PAD(0xE); //0xC
    uint16 nArraySize; // 0x1A
    ERenderFormat eFormat; // 0x1C
    PAD(0x4); // 0x20
};

class IRenderResource : public IRenderRefCount {
public:
    virtual ERenderResourceType GetResourceType() const = 0;
};

class IRenderResourceD3D12 : public IRenderResource {
public:
    ID3D12Resource* m_pResource; // 0x8
    uint8_t m_Unknown0;         // 0x10
    int32_t m_Unknown1;         // 0x14
    uint64_t m_Unknown2;        // 0x18
};

static_assert(sizeof(IRenderResourceD3D12) == 0x20);

class ZRenderTexture2D : public TRenderResourceImpl<IRenderResourceD3D12, ERenderResourceType::RENDER_RESOURCE_TYPE_TEXTURE2D> {
public:
    virtual ~ZRenderTexture2D() = 0;

public:
    SRenderTexture2DDesc m_Description; // 0x28
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
    FX2_SHADER_TYPE_HULL_SHADER,
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

class ZVTablePaddingRemover {
public:
    virtual ~ZVTablePaddingRemover() = 0;
};

class ZRenderGraphNode : public ZVTablePaddingRemover {
public:
    enum TYPE : uint8 {
        GEOM = 0,
        LINKED = 1,
        PARTICLEEMITTER = 2,
        SPEEDTREE = 3,
        SPATIAL = 4,
        LIGHT = 5,
        CAMERA = 6,
        MATERIAL = 7,
        POSTFILTER = 8,
        RAIN = 9,
        RAINMODIFIER = 10,
        RAINSIMULATION = 11,
        COMPOSITOR = 12,
        DESTINATION = 13,
        SPLITSCREEN = 14,
        VIDEO_PLAYER = 15,
        UI = 16,
        VOLUMELIGHT = 17,
        FOGBOX = 18,
        CROWDENTITY = 19,
        SCATTER = 20,
        MIRROR = 21,
        BOXREFLECTION = 22,
        RENDERGLOBAL = 23,
        WATERPARAMETERS = 24,
        ATMOSPHERICSCATTERING = 25,
        GUIGROUP = 26,
        TYPE_SIZE = 27,
        RENDERABLE_TYPE_MASK = 15,
        RENDERABLE_TYPE_FIRST = GEOM,
        RENDERABLE_TYPE_LAST = SPEEDTREE
    };

    ZRenderableEntity* m_pRenderableEntity; // 0x8
    PAD(0x8); // 0x10
    uint32 m_nGridIndex; // 0x18
    int32 m_Base; // 0x1C
    uint16 m_nRoomID; // 0x20
    uint16 m_nRoomIDOverlap[2]; // 0x22
    PAD(0x2); // 0x26
    TYPE m_nType; // 0x28
};

class ZRenderGraphNodeBoxReflection : public ZRenderGraphNode {
public:
    PAD(0xC4); // 0x30
    int32 m_nId; // 0xF4
};

class IRenderGraphManager : public IComponentInterface {};

class ZRenderGraphManager : public IRenderGraphManager {
public:
    PAD(0x578); // 0x8
    TMaxArray<ZRenderGraphNodeBoxReflection*, 682> m_BoxReflections; // 0x580
};

class ZRenderBoxReflectionCacheResource {
public:
    const uint8_t* GetEntryData(const SVector3& p_Position) {
        if (m_Entries.size() == 1) {
            return m_Entries[0].m_pData;
        }

        for (const SEntry& s_Entry : m_Entries) {
            if (std::abs(s_Entry.m_Position.x - p_Position.x) <= 0.001f &&
                std::abs(s_Entry.m_Position.y - p_Position.y) <= 0.001f &&
                std::abs(s_Entry.m_Position.z - p_Position.z) <= 0.001f) {
                return s_Entry.m_pData;
            }
        }

        return nullptr;
    }

    struct SEntry {
        SVector3 m_Position;
        const uint8_t* m_pData;
    };

    TArray<SEntry> m_Entries;
    const uint8_t* m_pResourceData;
    ZResourceReaderPtr m_pResourceReader;
};