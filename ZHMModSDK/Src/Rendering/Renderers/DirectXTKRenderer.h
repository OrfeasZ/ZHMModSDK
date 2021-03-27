#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <memory>

#include <GraphicsMemory.h>
#include <wrl/client.h>

#include "DescriptorHeap.h"
#include "Effects.h"
#include "PrimitiveBatch.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "VertexTypes.h"
#include "Hooks.h"
#include "SpriteFont.h"

class SGameUpdateEvent;

namespace Rendering::Renderers
{
	class DirectXTKRenderer
	{
	private:
		struct FrameContext
		{
			size_t Index = 0;

			ID3D12CommandAllocator* CommandAllocator = nullptr;
			ID3D12GraphicsCommandList* CommandList = nullptr;

			ID3D12Resource* BackBuffer = nullptr;
			D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = { 0 };

			ID3D12Fence* Fence = nullptr;
			HANDLE FenceEvent = nullptr;
			volatile uint64_t FenceValue = 0;

			volatile bool Recording = false;
		};

		enum class Descriptors : int
		{
			FontRegular,
			FontBold,
			Count
		};

	public:
		static void Init();
		static void OnEngineInit();
		static void Shutdown();

	public:
		static void OnPresent(IDXGISwapChain3* p_SwapChain);
		static void PostPresent(IDXGISwapChain3* p_SwapChain);
		static void SetCommandQueue(ID3D12CommandQueue* p_CommandQueue);
		static void OnReset();

	private:
		static bool SetupRenderer(IDXGISwapChain3* p_SwapChain);
		static void WaitForGpu(FrameContext* p_Frame);
		static void ExecuteCmdList(FrameContext* p_Frame);
		static void Draw(FrameContext* p_Frame);

	private:
		DEFINE_STATIC_DETOUR(void, DrawScaleform, void* a1, void* a2, void* a3, void* a4, void* a5);

	private:
		static bool m_RendererSetup;
		static UINT m_BufferCount;
		static ID3D12DescriptorHeap* m_RtvDescriptorHeap;
		static ID3D12CommandQueue* m_CommandQueue;
		static FrameContext* m_FrameContext;
		static IDXGISwapChain3* m_SwapChain;
		static HWND m_Hwnd;
		static float m_WindowWidth;
		static float m_WindowHeight;
		static bool m_Shutdown;

	private:
		static std::unique_ptr<DirectX::GraphicsMemory> m_GraphicsMemory;
		static std::unique_ptr<DirectX::BasicEffect> m_LineEffect;
		static std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_LineBatch;
		
		static DirectX::SimpleMath::Matrix m_World;
		static DirectX::SimpleMath::Matrix m_View;
		static DirectX::SimpleMath::Matrix m_Projection;
		static DirectX::SimpleMath::Matrix m_ViewProjection;

		static std::unique_ptr<DirectX::DescriptorHeap> m_ResourceDescriptors;
		static std::unique_ptr<DirectX::SpriteFont> m_Font;
		static std::unique_ptr<DirectX::SpriteBatch> m_SpriteBatch;
	};
}
