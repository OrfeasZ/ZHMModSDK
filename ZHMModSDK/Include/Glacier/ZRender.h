#pragma once

#include <d3d12.h>
#include <dxgi.h>

#include "ZMath.h"
#include "Reflection.h"

class ZEntityRef;

class IRenderRefCount
{
public:
    virtual ~IRenderRefCount() = 0;
    virtual void AddRef() = 0;
    virtual uint32_t Release() = 0;
};

class IRenderDestination : public IRenderRefCount
{
public:
};

class IRenderDestinationEntity :
    public IComponentInterface
{
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

class ZRenderSwapChain
{
public:
    virtual ~ZRenderSwapChain();

public:
    IDXGIFactory1* m_pFactory;
    IDXGISwapChain* m_pSwapChain;
};

class ZRenderDevice
{
public:
    virtual ~ZRenderDevice() = default;

public:
    PAD(0x10A08);
    ZRenderSwapChain* m_pSwapChain; // 0x10A10, look for ZRenderSwapChain constructor
    PAD(0x08); // 0x10A18
    ID3D12Device* m_pDevice; // 0x10A20
    PAD(0x30E9D88); // 0x10A28
    ID3D12CommandQueue* m_pCommandQueue; // 0x30FA7B0, look for "m_pFrameHeapCBVSRVUAV" string, first vtable call with + 128
    PAD(0x180FC8); // 0x30FA7B8
    ID3D12DescriptorHeap* m_pFrameHeapCBVSRVUAV; // 0x327B780, look for "m_pFrameHeapCBVSRVUAV" string, first argument
};

static_assert(offsetof(ZRenderDevice, m_pSwapChain) == 0x10A10);
static_assert(offsetof(ZRenderDevice, m_pCommandQueue) == 0x30FA7B0);
static_assert(offsetof(ZRenderDevice, m_pFrameHeapCBVSRVUAV) == 0x327B780);

class ZRenderContext
{
public:
    PAD(0x240);
    SMatrix m_mWorldToView; // 0x240, function called by ZRenderContext_Unknown01, second pair of 4
    PAD(0xC0);
    SMatrix m_mViewToProjection; // 0x340, function called by ZRenderContext_Unknown01, first pair of 4
};

class ZRenderManager
{
public:
    virtual ~ZRenderManager() = default;

public:
    PAD(0x14178);
    ZRenderDevice* m_pDevice; // 0x14180, look for ZRenderDevice constructor
    PAD(0xF8); // 0x14188
    ZRenderContext* m_pRenderContext; // 0x14280, look for "ZRenderManager::RenderThread" string, first thing being constructed and assigned
};


class ZRenderTexture2D
{
public:
    virtual ~ZRenderTexture2D() = 0;

public:
    ID3D12Resource* m_pResource;
};

class ZRenderTargetView;

class ZRenderShaderResourceView
{
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

class ZRenderDestination : public IRenderDestination
{
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
