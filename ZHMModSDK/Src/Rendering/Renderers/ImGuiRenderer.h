#pragma once

#include <vector>
#include <directx/d3d12.h>
#include <dxgi1_4.h>

#include <imgui.h>

#include <implot.h>

#include "Hooks.h"
#include "Glacier/ZInput.h"
#include "D3DUtils.h"
#include "IRenderer.h"
#include "ResourceUploadBatch.h"

struct ImFont;

namespace Rendering::Renderers {
    class ImGuiRenderer {
    public:
        struct FrameContext {
            ScopedD3DRef<ID3D12CommandAllocator> m_CommandAllocator;
            std::atomic<std::uint64_t> m_FenceValue { 0 };
        };

        ImGuiRenderer();
        ~ImGuiRenderer();

        void OnEngineInitialized();

        void SetSwapChain(IDXGISwapChain3* p_SwapChain);
        void SetCommandQueue(ID3D12CommandQueue* p_CommandQueue);
        void OnPresent(IDXGISwapChain3* p_SwapChain);
        void PostPresent(IDXGISwapChain3* p_SwapChain, HRESULT p_PresentResult);
        void OnReset(IDXGISwapChain3* p_SwapChain);
        void PostReset(IDXGISwapChain3* p_SwapChain);

        bool IsVisible() const { return m_IsImGuiVisible.load(std::memory_order_acquire); }

        ImGuiContext* GetContext() const {
            return m_ImGuiContext;
        }

        ImPlotContext* GetImPlotContext() const {
            return m_ImPlotContext;
        }

        ImFont* GetFontLight() { return m_FontLight; }
        ImFont* GetFontRegular() { return m_FontRegular; }
        ImFont* GetFontMedium() { return m_FontMedium; }
        ImFont* GetFontBold() { return m_FontBold; }
        ImFont* GetFontBlack() { return m_FontBlack; }

        void SetFocus(bool p_HasFocus) { m_ImGuiHasFocus.store(p_HasFocus, std::memory_order_release); }

        bool CreateDDSTextureFromMemory(
            const void* p_Data,
            size_t p_DataSize,
            ScopedD3DRef<ID3D12Resource>& p_OutTexture,
            ImGuiTexture& p_OutImGuiTexture
        );

        bool CreateDDSTextureFromFile(
            const std::string& p_FilePath,
            ScopedD3DRef<ID3D12Resource>& p_OutTexture,
            ImGuiTexture& p_OutImGuiTexture
        );

        bool CreateWICTextureFromMemory(
            const void* p_Data,
            size_t p_DataSize,
            ScopedD3DRef<ID3D12Resource>& p_OutTexture,
            ImGuiTexture& p_OutImGuiTexture
        );

        bool CreateWICTextureFromFile(
            const std::string& p_FilePath,
            ScopedD3DRef<ID3D12Resource>& p_OutTexture,
            ImGuiTexture& p_OutImGuiTexture
        );

        void SetGameDescriptorHeap();
        void ResetDescriptorHeap();

    private:
        bool SetupRenderer(IDXGISwapChain3* p_SwapChain);
        void TeardownRenderer();
        void Draw();
        void SetupStyles();
        void WaitForCurrentFrameToFinish() const;

        void AllocateSRVDescriptor(
            D3D12_CPU_DESCRIPTOR_HANDLE* p_OutCPUHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE* p_OutGPUHandle
        );
        void FreeSRVDescriptor(
            D3D12_CPU_DESCRIPTOR_HANDLE p_CPUHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE p_GPUHandle
        );

        // Input plumbing helpers (ported from ZHM's WndProc handler).
        static ImGuiMouseSource GetMouseSourceFromMessageExtraInfo();
        static bool IsVkDown(int p_Vk);
        static void UpdateKeyModifiers(ImGuiIO& p_ImGuiIO);
        void UpdateKeyboardCodePage();
        static ImGuiKey KeyEventToImGuiKey(WPARAM p_Wparam, LPARAM p_Lparam);
        static void AddKeyEvent(ImGuiIO& p_ImGuiIO, ImGuiKey p_Key, bool p_Down, int p_NativeKeycode, int p_NativeScancode = -1);
        void UpdateMouseData(ImGuiIO& p_ImGuiIO);
        void ProcessKeyEventsWorkarounds(ImGuiIO& p_ImGuiIO);

        bool CreateTexture(
            std::function<HRESULT(ScopedD3DRef<ID3D12Device>&, DirectX::ResourceUploadBatch&, ID3D12Resource**)> p_Loader,
            ScopedD3DRef<ID3D12Resource>& p_OutTexture,
            ImGuiTexture& p_OutImGuiTexture
        );

        DECLARE_DETOUR_WITH_CONTEXT(
            ImGuiRenderer,
            LRESULT,
            WndProc,
            ZApplicationEngineWin32*,
            HWND,
            UINT,
            WPARAM,
            LPARAM
        );

        DECLARE_DETOUR_WITH_CONTEXT(ImGuiRenderer, void, ZKeyboardWindows_Update, ZKeyboardWindows*, bool);

        // Maximum number of SRV (Shader Resource View) descriptors that can
        // be allocated in the global CBV/SRV/UAV descriptor heap.
        static constexpr std::size_t c_MaxSRVDescriptors = 1024;
        // The maximum number of frames that can be buffered for render.
        static constexpr std::size_t c_MaxRenderedFrames = 4;

        bool m_RendererSetup = false;

        ScopedD3DRef<IDXGISwapChain3> m_SwapChain;
        ScopedD3DRef<ID3D12CommandQueue> m_CommandQueue;
        HWND m_Hwnd = nullptr;

        std::uint32_t m_RTVDescriptorSize = 0;
        ScopedD3DRef<ID3D12DescriptorHeap> m_RTVDescriptorHeap;

        ScopedD3DRef<ID3D12DescriptorHeap> m_SRVDescriptorHeap;
        UINT m_SRVDescriptorSize = 0;
        UINT m_NextSRVDescriptorIndex = 0;
        std::vector<UINT> m_FreeSRVDescriptorIndices;

        std::array<FrameContext, c_MaxRenderedFrames> m_FrameContext {};
        std::vector<ScopedD3DRef<ID3D12Resource>> m_BackBuffers;
        ScopedD3DRef<ID3D12GraphicsCommandList> m_CommandList;

        ScopedD3DRef<ID3D12Fence> m_Fence;
        SafeHandle m_FenceEvent;

        std::atomic<std::uint32_t> m_FrameCounter { 0 };
        std::atomic<std::uint64_t> m_FenceValue { 0 };

        std::int64_t m_Time = 0;
        std::int64_t m_TicksPerSecond = 0;

        ImGuiContext* m_ImGuiContext = nullptr;
        ImPlotContext* m_ImPlotContext = nullptr;

        ImFont* m_FontLight = nullptr;
        ImFont* m_FontRegular = nullptr;
        ImFont* m_FontMedium = nullptr;
        ImFont* m_FontBold = nullptr;
        ImFont* m_FontBlack = nullptr;

        std::atomic<bool> m_ImGuiHasFocus { false };
        std::atomic<bool> m_IsImGuiVisible { true };
        std::atomic<bool> m_UIToggleWarningRequested { false };
        bool m_ShowingUIToggleWarning = false;

        // Input state for WndProc handler.
        HWND m_MouseHwnd = nullptr;
        int m_MouseTrackedArea = 0; // 0: not tracked, 1: client area, 2: non-client area
        int m_MouseButtonsDown = 0;
        UINT32 m_KeyboardCodePage = CP_ACP;
    };
}