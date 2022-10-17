#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>

#include "Hooks.h"
#include "Glacier/ZInput.h"

struct ImFont;

namespace Rendering::Renderers
{
	class ImGuiRenderer
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
		ImGuiRenderer();
		~ImGuiRenderer();

	public:
		void OnEngineInit();
		void Shutdown();
		
	public:
		void OnPresent(IDXGISwapChain3* p_SwapChain);
		void SetCommandQueue(ID3D12CommandQueue* p_CommandQueue);
		void OnReset();

	public:
		ImFont* GetFontLight() { return m_FontLight; }
		ImFont* GetFontRegular() { return m_FontRegular; }
		ImFont* GetFontMedium() { return m_FontMedium; }
		ImFont* GetFontBold() { return m_FontBold; }
		ImFont* GetFontBlack() { return m_FontBlack; }

		void SetFocus(bool p_Focused) { m_ImguiHasFocus = p_Focused; }
		
	private:
		bool SetupRenderer(IDXGISwapChain3* p_SwapChain);
		void WaitForGpu(FrameContext* p_Frame);
		void ExecuteCmdList(FrameContext* p_Frame);
		void Draw();
		void SetupStyles();

	private:
		DEFINE_DETOUR_WITH_CONTEXT(ImGuiRenderer, LRESULT, WndProc, ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM);
		DEFINE_DETOUR_WITH_CONTEXT(ImGuiRenderer, void, ZKeyboardWindows_Update, ZKeyboardWindows*, bool);
		DEFINE_DETOUR_WITH_CONTEXT(ImGuiRenderer, double, ZInputAction_Analog, ZInputAction*, int);

	private:
		bool m_RendererSetup = false;

		UINT m_BufferCount = 0;
		ID3D12DescriptorHeap* m_RtvDescriptorHeap = nullptr;
		ID3D12DescriptorHeap* m_SrvDescriptorHeap = nullptr;
		//ID3D12CommandQueue* m_CommandQueue;
		FrameContext* m_FrameContext = nullptr;
		IDXGISwapChain3* m_SwapChain = nullptr;
		HWND m_Hwnd = nullptr;

		int64_t m_Time = 0;
		int64_t m_TicksPerSecond = 0;

		ImFont* m_FontLight = nullptr;
		ImFont* m_FontRegular = nullptr;
		ImFont* m_FontMedium = nullptr;
		ImFont* m_FontBold = nullptr;
		ImFont* m_FontBlack = nullptr;

		volatile bool m_ImguiHasFocus = false;

		SRWLOCK m_Lock {};
	};
}
