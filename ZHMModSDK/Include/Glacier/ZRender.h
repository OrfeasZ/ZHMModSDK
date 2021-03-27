#pragma once

#include "ZPrimitives.h"

#include <d3d12.h>
#include <dxgi.h>

#include "ZMath.h"

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
	PAD(0x30E9C60);
	ID3D12CommandQueue* m_pCommandQueue; // 0x30EA070
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
	ZRenderContext* m_pRenderContext; //0x14318
};
