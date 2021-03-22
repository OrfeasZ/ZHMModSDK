#pragma once

#include "ZPrimitives.h"

#include <d3d12.h>

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

class ZRenderManager
{
public:
	virtual ~ZRenderManager() = default;

public:
	PAD(0x14150);
	ZRenderDevice* m_pDevice; // 0x14158
};