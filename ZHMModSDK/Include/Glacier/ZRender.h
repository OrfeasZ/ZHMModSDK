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
	PAD(0x3F8);
	ZRenderSwapChain* m_pSwapChain; // 0x400
	ID3D12Device* m_pDevice; // 0x408
	PAD(0x30E9C88); // 0x410
	ID3D12CommandQueue* m_pCommandQueue; // 0x30EA098
};

class ZRenderContext
{
public:
	PAD(0x240);
	SMatrix m_mWorldToView; // 0x240
	PAD(0xC0);
	SMatrix m_mViewToProjection; // 0x340
};

class ZRenderManager
{
public:
	virtual ~ZRenderManager() = default;

public:
	PAD(0x14150);
	ZRenderDevice* m_pDevice; // 0x14158
	PAD(0x1B8); // 0x14160
	ZRenderContext* m_pRenderContext; // 0x14318
};
