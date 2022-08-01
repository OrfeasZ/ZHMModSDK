#pragma once

#include "ZPrimitives.h"

#include <d3d12.h>
#include <dxgi.h>

#include "ZMath.h"

class IRenderDestinationEntity :
	public IComponentInterface
{
public:
	virtual ZEntityRef* GetSource() = 0;
	virtual void IRenderDestinationEntity_unk06() = 0;
	virtual void IRenderDestinationEntity_unk07() = 0;
	virtual void IRenderDestinationEntity_unk08() = 0;
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
	PAD(0x540);
	ZRenderSwapChain* m_pSwapChain; // 0x548, look for ZRenderSwapChain constructor; ZRenderSwapChain_Resize called from ZRenderDevice function
	ID3D12Device* m_pDevice; // 0x550
	PAD(0x30E9D90); // 0x558
	ID3D12CommandQueue* m_pCommandQueue; // 0x30EA2E8, look for "m_pFrameHeapCBVSRVUAV" string, first vtable call with + 128
};

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
	PAD(0x14170);
	ZRenderDevice* m_pDevice; // 0x14178, look for ZRenderDevice constructor
	PAD(0x1E0); // 0x14180
	ZRenderContext* m_pRenderContext; // 0x14360, look for "ZRenderManager::RenderThread" string, first thing being constructed and assigned
};
