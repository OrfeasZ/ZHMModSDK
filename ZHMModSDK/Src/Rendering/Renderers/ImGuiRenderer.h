#pragma once

#include <vector>
#include <d3d12.h>
#include <dxgi1_4.h>

#include "Hooks.h"
#include "Glacier/ZInput.h"

struct ImFont;

namespace Rendering::Renderers
{
	class ImGuiRenderer
	{
	public:
		struct FrameContext
		{
			size_t Index = 0;
			ID3D12CommandAllocator* CommandAllocator = nullptr;			
			volatile uint64_t FenceValue = 0;
			volatile bool Recording = false;
		};

		ImGuiRenderer();
		~ImGuiRenderer();

	public:
		void OnEngineInit();
		void Shutdown();
		
	public:
		void OnPresent(IDXGISwapChain3* p_SwapChain);
		void PostPresent(IDXGISwapChain3* p_SwapChain);
		void SetCommandQueue(ID3D12CommandQueue* p_CommandQueue);
		void OnReset();
		void PostReset();

	public:
		ImFont* GetFontLight() { return m_FontLight; }
		ImFont* GetFontRegular() { return m_FontRegular; }
		ImFont* GetFontMedium() { return m_FontMedium; }
		ImFont* GetFontBold() { return m_FontBold; }
		ImFont* GetFontBlack() { return m_FontBlack; }

		void SetFocus(bool p_Focused) { m_ImguiHasFocus = p_Focused; }
		
	private:
		bool SetupRenderer(IDXGISwapChain3* p_SwapChain);
		void Draw();
		void SetupStyles();

	private:
		DEFINE_DETOUR_WITH_CONTEXT(ImGuiRenderer, LRESULT, WndProc, ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM);
		DEFINE_DETOUR_WITH_CONTEXT(ImGuiRenderer, void, ZKeyboardWindows_Update, ZKeyboardWindows*, bool);
		DEFINE_DETOUR_WITH_CONTEXT(ImGuiRenderer, double, ZInputAction_Analog, ZInputAction*, int);

	private:
		bool m_RendererSetup = false;

		IDXGISwapChain3* m_SwapChain = nullptr;
		HANDLE m_FrameLatencyWaitable = nullptr;
		ID3D12CommandQueue* m_CommandQueue = nullptr;
		HWND m_Hwnd = nullptr;

		ID3D12DescriptorHeap* m_RtvDescriptorHeap = nullptr;
		ID3D12DescriptorHeap* m_SrvDescriptorHeap = nullptr;

		constexpr static int FRAME_COUNT = 2;

		FrameContext m_FrameContext[FRAME_COUNT] = { };

		ID3D12GraphicsCommandList* m_CommandList = nullptr;

		ID3D12Fence* m_Fence = nullptr;
		HANDLE m_FenceEvent = nullptr;

		UINT m_BufferCount = 0;
		ID3D12Resource** m_BackBuffers = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE* m_DescriptorHandles = nullptr;

		int64_t m_Time = 0;
		int64_t m_TicksPerSecond = 0;

		volatile uint64_t m_FenceLastSignaledValue = 0;
		volatile uint32_t m_FrameIndex = 0;

		ImFont* m_FontLight = nullptr;
		ImFont* m_FontRegular = nullptr;
		ImFont* m_FontMedium = nullptr;
		ImFont* m_FontBold = nullptr;
		ImFont* m_FontBlack = nullptr;

		volatile bool m_ImguiHasFocus = false;
	};
}
