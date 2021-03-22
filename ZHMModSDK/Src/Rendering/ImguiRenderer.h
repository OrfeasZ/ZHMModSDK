#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>

#include "Hooks.h"

namespace Rendering
{
	class ImguiRenderer
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

	public:
		static ImguiRenderer* GetInstance();

	private:
		static ImguiRenderer* m_Instance;

	public:
		void Init();
		void Shutdown();

	private:
		void ResetRenderer();
		void OnPresent(IDXGISwapChain3* p_SwapChain);
		bool SetupRenderer(IDXGISwapChain3* p_SwapChain);
		void SetCommandQueue(ID3D12CommandQueue* p_CommandQueue);
		void WaitForGpu(FrameContext* p_Frame);
		void ExecuteCmdList(FrameContext* p_Frame);
		void Draw();

	private:
		DEFINE_DETOUR_WITH_CONTEXT(ImguiRenderer, LRESULT, WndProc, ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM)

	private:
		bool m_RendererSetup = false;
		bool m_ImguiInitialized = false;

		UINT m_BufferCount = 0;
		ID3D12DescriptorHeap* m_RtvDescriptorHeap = nullptr;
		ID3D12DescriptorHeap* m_SrvDescriptorHeap = nullptr;
		ID3D12CommandQueue* m_CommandQueue = nullptr;
		FrameContext* m_FrameContext = nullptr;
		IDXGISwapChain3* m_SwapChain = nullptr;
		HWND m_Hwnd = nullptr;

		int64_t m_Time = 0;
		int64_t m_TicksPerSecond = 0;

		volatile bool m_ImguiHasFocus = false;

		friend class D3D12Hooks;
	};
}
