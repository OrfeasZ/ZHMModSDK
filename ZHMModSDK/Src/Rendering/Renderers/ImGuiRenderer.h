#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
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
        struct DeferredResource {
            ScopedD3DRef<ID3D12Resource> m_Texture;
            ImGuiTexture m_ImGuiTexture;
        };

        struct FrameContext {
            ScopedD3DRef<ID3D12CommandAllocator> m_CommandAllocator;
            std::atomic<std::uint64_t> m_FenceValue { 0 };

            std::vector<DeferredResource> m_DeferredResources;
        };

        // A copy of a viewport's draw data that the render thread can use while the window thread builds the next frame.
        struct DrawDataSnapshot {
            explicit DrawDataSnapshot(const ImDrawData* p_Source);
            ~DrawDataSnapshot();

            DrawDataSnapshot(const DrawDataSnapshot&) = delete;
            DrawDataSnapshot& operator=(const DrawDataSnapshot&) = delete;

            ImDrawData m_DrawData;
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

        bool CreateImGuiTextureSRV(ID3D12Resource* p_Texture, ImGuiTexture& p_OutImGuiTexture);
        void DestroyImGuiTextureSRV(ImGuiTexture& p_Texture);

        void DestroyImGuiTexture(
            ScopedD3DRef<ID3D12Resource>& p_Texture,
            ImGuiTexture& p_ImGuiTexture
        );

        void SetGameDescriptorHeap();
        void ResetDescriptorHeap();

    private:
        struct TimingStat {
            void Add(std::chrono::steady_clock::time_point p_Start);
            std::string Format() const;

            double m_TotalMs = 0.0;
            double m_MaxMs = 0.0;
            std::uint32_t m_Count = 0;
        };

        // Traces where the window thread and the render thread spend time.
        struct WindowThreadTimings {
            TimingStat m_MessageGap;
            TimingStat m_FrameDelay;
            TimingStat m_FrameInterval;
            TimingStat m_Draw;
            TimingStat m_UpdatePlatformWindows;
            TimingStat m_RenderPlatformWindows;
            TimingStat m_ViewportLock;
            TimingStat m_ViewportRender;
            TimingStat m_ViewportSwap;

            // Frames submitted to the game's command queue that the GPU hasn't finished yet.
            std::uint64_t m_MaxGameQueueBacklog = 0;

            bool m_MovedViewport = false;
            bool m_MovedGameWindow = false;
        };

        struct RenderThreadTimings {
            TimingStat m_BackendLock;
            TimingStat m_FenceWait;
            std::uint32_t m_PresentCount = 0;
        };

        void FlushWindowThreadTimings();
        void FlushRenderThreadTimings();

        bool SetupRenderer(IDXGISwapChain3* p_SwapChain);
        void TeardownRenderer();
        void RunFrame();
        void StartWindowManagerMove(HWND p_Hwnd);
        void Draw();
        void SetupStyles();
        void WaitForCurrentFrameToFinish() const;

        bool AllocateSRVDescriptor(
            D3D12_CPU_DESCRIPTOR_HANDLE* p_OutCPUHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE* p_OutGPUHandle
        );
        void FreeSRVDescriptor(
            D3D12_CPU_DESCRIPTOR_HANDLE p_CPUHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE p_GPUHandle
        );

        void ReleaseDeferredResources(std::vector<DeferredResource>& p_Resources);

        std::vector<DeferredResource> TakeDeferredResources();

        // Waits for the GPU, then releases deferred textures and draw data snapshots before textures are destroyed.
        void ReleaseFrameResources();

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
        // ZApplicationEngineWin32_MainWindowProc applies its cursor clip when it gets this message.
        static constexpr UINT c_GameUpdateCursorClipMessage = WM_USER + 1;

        bool m_RendererSetup = false;

        ScopedD3DRef<IDXGISwapChain3> m_SwapChain;
        ScopedD3DRef<ID3D12CommandQueue> m_CommandQueue;
        ScopedD3DRef<ID3D12CommandQueue> m_BackendCommandQueue;
        HWND m_Hwnd = nullptr;
        bool m_PlatformSetup = false;

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

        // Guards the SRV descriptor free list and m_PendingDeferredResources.
        std::mutex m_DescriptorMutex;
        std::vector<DeferredResource> m_PendingDeferredResources;

        // The ImGui frame runs on the thread that owns the game window, and the main viewport is rendered on the
        // present thread. The DX12 backend has shared state (for example Renderer_RenderState), so every call into it
        // holds this mutex. Never make Win32 calls that can send messages to other threads while holding it.
        std::mutex m_BackendMutex;

        // DX12 backend viewport callbacks.
        void (*m_RendererCreateWindow)(ImGuiViewport*) = nullptr;
        void (*m_RendererDestroyWindow)(ImGuiViewport*) = nullptr;
        void (*m_RendererSetWindowSize)(ImGuiViewport*, ImVec2) = nullptr;
        void (*m_RendererRenderWindow)(ImGuiViewport*, void*) = nullptr;
        void (*m_RendererSwapBuffers)(ImGuiViewport*, void*) = nullptr;

        // Win32 backend show callbac.
        void (*m_PlatformShowWindow)(ImGuiViewport*) = nullptr;

        UINT m_FrameMessage = 0;
        std::atomic<bool> m_WndProcHooked { false };
        std::atomic<bool> m_FrameRequested { false };
        bool m_InFrame = false;
        HWND m_WindowManagerMoveHwnd = nullptr;

        WindowThreadTimings m_WindowThreadTimings;
        RenderThreadTimings m_RenderThreadTimings;
        std::chrono::steady_clock::time_point m_LastMessage;
        std::chrono::steady_clock::time_point m_LastFrame;
        std::chrono::steady_clock::time_point m_LastWindowThreadFlush;
        std::chrono::steady_clock::time_point m_LastRenderThreadFlush;
        std::atomic<std::chrono::steady_clock::time_point> m_FramePostedAt;
        std::atomic<bool> m_HasDetachedViewports { false };
        bool m_IsGameWindowInMoveLoop = false;

        std::mutex m_SnapshotMutex;
        std::unique_ptr<DrawDataSnapshot> m_PendingSnapshot;
        std::unique_ptr<DrawDataSnapshot> m_RenderSnapshot;

        // ImGui keeps a pointer to this, so it has to live as long as the context.
        std::string m_IniFilePath;

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
    };
}