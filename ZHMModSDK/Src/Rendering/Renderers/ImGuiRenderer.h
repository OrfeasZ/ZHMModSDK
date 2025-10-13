#pragma once

#include <vector>
#include <directx/d3d12.h>
#include <dxgi1_4.h>
#include <imgui.h>

#include "Hooks.h"
#include "Glacier/ZInput.h"
#include "D3DUtils.h"

struct ImFont;

namespace Rendering::Renderers {
    class ImGuiRenderer {
    public:
        struct FrameContext {
            ScopedD3DRef<ID3D12CommandAllocator> CommandAllocator;
            volatile uint64_t FenceValue = 0;
        };

        ImGuiRenderer();
        ~ImGuiRenderer();

    public:
        void OnEngineInit();

    public:
        void OnPresent(IDXGISwapChain3* p_SwapChain);
        void PostPresent(IDXGISwapChain3* p_SwapChain, HRESULT p_PresentResult);
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
        void WaitForCurrentFrameToFinish() const;
        static ImGuiMouseSource GetMouseSourceFromMessageExtraInfo();
        static bool IsVkDown(int p_Vk);
        static void UpdateKeyModifiers(ImGuiIO& p_ImGuiIO);
        void UpdateKeyboardCodePage();
        ImGuiKey KeyEventToImGuiKey(WPARAM p_Wparam, LPARAM p_Lparam);
        void AddKeyEvent(
            ImGuiIO& p_ImGuiIO, ImGuiKey p_Key, bool p_Down, int p_NativeKeycode, int p_NativeScancode = -1
        );
        void UpdateMouseData(ImGuiIO& p_ImGuiIO);
        void ProcessKeyEventsWorkarounds(ImGuiIO& io);

    private:
        DECLARE_DETOUR_WITH_CONTEXT(
            ImGuiRenderer, LRESULT, WndProc, ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM
        );
        DECLARE_DETOUR_WITH_CONTEXT(ImGuiRenderer, void, ZKeyboardWindows_Update, ZKeyboardWindows*, bool);
        DECLARE_DETOUR_WITH_CONTEXT(ImGuiRenderer, double, ZInputAction_Analog, ZInputAction*, int);

    private:
        bool m_RendererSetup = false;

        ScopedD3DRef<IDXGISwapChain3> m_SwapChain;
        ScopedD3DRef<ID3D12CommandQueue> m_CommandQueue;
        HWND m_Hwnd = nullptr;

        uint32_t m_RtvDescriptorSize = 0;
        ScopedD3DRef<ID3D12DescriptorHeap> m_RtvDescriptorHeap;
        ScopedD3DRef<ID3D12DescriptorHeap> m_SrvDescriptorHeap;

        /** The maximum number of frames that can be buffered for render. */
        inline constexpr static size_t MaxRenderedFrames = 4;
        std::vector<FrameContext> m_FrameContext;

        std::vector<ScopedD3DRef<ID3D12Resource>> m_BackBuffers;

        ScopedD3DRef<ID3D12GraphicsCommandList> m_CommandList;

        ScopedD3DRef<ID3D12Fence> m_Fence;
        SafeHandle m_FenceEvent;

        volatile uint32_t m_FrameCounter = 0;
        volatile uint64_t m_FenceValue = 0;

        int64_t m_Time = 0;
        int64_t m_TicksPerSecond = 0;

        ImFont* m_FontLight = nullptr;
        ImFont* m_FontRegular = nullptr;
        ImFont* m_FontMedium = nullptr;
        ImFont* m_FontBold = nullptr;
        ImFont* m_FontBlack = nullptr;

        volatile bool m_ImguiHasFocus = false;
        volatile bool m_ImguiVisible = true;
        bool m_ShowingUiToggleWarning = false;

        HWND m_MouseHwnd;
        int m_MouseTrackedArea; // 0: not tracked, 1: client area, 2: non-client area
        int m_MouseButtonsDown;
        UINT32 m_KeyboardCodePage;
    };
}