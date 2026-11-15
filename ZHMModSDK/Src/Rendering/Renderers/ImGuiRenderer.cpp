#include <directx/d3dx12.h>

#include "ImGuiRenderer.h"

#include <d3dcompiler.h>
#include <dxgi1_4.h>

#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>

#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include <ResourceUploadBatch.h>
#include <DirectXHelpers.h>

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZApplicationEngineWin32.h>
#include <Glacier/ZInputActionManager.h>

#include "D3DUtils.h"
#include "Fonts.h"
#include "Functions.h"
#include "HookImpl.h"
#include "ModSDK.h"
#include "Glacier/ZRender.h"
#include "Util/ProcessUtils.h"

#include <IconsMaterialDesign.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace Rendering::Renderers;

ImGuiRenderer::DrawDataSnapshot::DrawDataSnapshot(const ImDrawData* p_Source) : m_DrawData(*p_Source) {
    // Textures are updated on the window thread before the snapshot is taken.
    m_DrawData.Textures = nullptr;

    for (auto& s_DrawList : m_DrawData.CmdLists) {
        s_DrawList = s_DrawList->CloneOutput();

        // Resolve texture ids now so the snapshot doesn't read ImTextureData that the window thread owns.
        for (auto& s_Command : s_DrawList->CmdBuffer) {
            s_Command.TexRef = ImTextureRef(s_Command.GetTexID());
        }
    }
}

ImGuiRenderer::DrawDataSnapshot::~DrawDataSnapshot() {
    for (auto* s_DrawList : m_DrawData.CmdLists) {
        IM_DELETE(s_DrawList);
    }
}

ImGuiRenderer::ImGuiRenderer() {
    IMGUI_CHECKVERSION();
    m_ImGuiContext = ImGui::CreateContext();
    m_ImPlotContext = ImPlot::CreateContext();

    ImGuiIO& s_ImGuiIO = ImGui::GetIO();
    s_ImGuiIO.IniFilename = nullptr;
    s_ImGuiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    s_ImGuiIO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    s_ImGuiIO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    s_ImGuiIO.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    s_ImGuiIO.BackendRendererName = "imgui_impl_dx12";

    // Here we merge the material icon glyphs into each of our other fonts.
    ImFontConfig s_IconsConfig {};
    s_IconsConfig.MergeMode = true;
    s_IconsConfig.GlyphOffset = { 0.f, 6.f };

    // Unicode ranges used by ImGui font
    static constexpr ImWchar c_TextRanges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin-1 Supplement
        0x2010, 0x2027, // Punctuation
        0
    };
    static constexpr ImWchar c_IconRanges[] = { ICON_MIN_MD, ICON_MAX_16_MD, 0 };

    m_FontLight = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        RobotoLight_compressed_data, RobotoLight_compressed_size, 28.f, nullptr, c_TextRanges
    );
    s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        MaterialIconsRegular_compressed_data, MaterialIconsRegular_compressed_size, 28.f, &s_IconsConfig, c_IconRanges
    );

    m_FontRegular = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        RobotoRegular_compressed_data, RobotoRegular_compressed_size, 28.f, nullptr, c_TextRanges
    );
    s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        MaterialIconsRegular_compressed_data, MaterialIconsRegular_compressed_size, 28.f, &s_IconsConfig, c_IconRanges
    );

    m_FontMedium = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        RobotoMedium_compressed_data, RobotoMedium_compressed_size, 28.f, nullptr, c_TextRanges
    );
    s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        MaterialIconsRegular_compressed_data, MaterialIconsRegular_compressed_size, 28.f, &s_IconsConfig, c_IconRanges
    );

    m_FontBold = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        RobotoBold_compressed_data, RobotoBold_compressed_size, 28.f, nullptr, c_TextRanges
    );
    s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        MaterialIconsRegular_compressed_data, MaterialIconsRegular_compressed_size, 28.f, &s_IconsConfig, c_IconRanges
    );

    m_FontBlack = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        RobotoBlack_compressed_data, RobotoBlack_compressed_size, 28.f, nullptr, c_TextRanges
    );
    s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        MaterialIconsRegular_compressed_data, MaterialIconsRegular_compressed_size, 28.f, &s_IconsConfig, c_IconRanges
    );

    s_ImGuiIO.FontDefault = m_FontRegular;

    SetupStyles();

    m_FrameMessage = RegisterWindowMessageW(L"ZHMModSDK_ImGuiFrame");
}

ImGuiRenderer::~ImGuiRenderer() {
    TeardownRenderer();

    HookRegistry::ClearDetoursWithContext(this);

    ImGui::DestroyContext(m_ImGuiContext);
    ImPlot::DestroyContext(m_ImPlotContext);

    m_ImGuiContext = nullptr;
    m_ImPlotContext = nullptr;
}

void ImGuiRenderer::SetupStyles() {
    auto& s_Style = ImGui::GetStyle();

    s_Style.ChildRounding = 0.f;
    s_Style.FrameRounding = 0.f;
    s_Style.GrabRounding = 0.f;
    s_Style.PopupRounding = 0.f;
    s_Style.ScrollbarRounding = 0.f;
    s_Style.TabRounding = 0.f;
    s_Style.WindowRounding = 0.f;
    s_Style.WindowBorderSize = 0.f;

    s_Style.WindowPadding = ImVec2(12.f, 12.f);
    s_Style.FramePadding = ImVec2(6.f, 6.f);
    s_Style.CellPadding = ImVec2(6.f, 3.f);
    s_Style.ItemSpacing = ImVec2(10.f, 6.f);
    s_Style.ItemInnerSpacing = ImVec2(10.f, 10.f);
    s_Style.TouchExtraPadding = ImVec2(0.f, 0.f);
    s_Style.IndentSpacing = 34.f;
    s_Style.ScrollbarSize = 12.f;
    s_Style.GrabMinSize = 12.f;

    s_Style.WindowBorderSize = 0.f;
    s_Style.ChildBorderSize = 0.f;
    s_Style.PopupBorderSize = 0.f;
    s_Style.FrameBorderSize = 0.f;
    s_Style.TabBorderSize = 0.f;

    ImVec4* s_Colors = s_Style.Colors;
    s_Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    s_Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    s_Colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);
    s_Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    s_Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    s_Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    s_Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    s_Colors[ImGuiCol_FrameBg] = ImVec4(0.06f, 0.05f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.13f);
    s_Colors[ImGuiCol_FrameBgActive] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_TitleBg] = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);
    s_Colors[ImGuiCol_TitleBgActive] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);
    s_Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    s_Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    s_Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    s_Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    s_Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    s_Colors[ImGuiCol_CheckMark] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_CheckboxSelectedBg] = ImVec4(0.06f, 0.05f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_SliderGrab] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_Button] = ImVec4(0.55f, 0.11f, 0.13f, 1.00f);
    s_Colors[ImGuiCol_ButtonHovered] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.05f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_Header] = ImVec4(0.55f, 0.11f, 0.13f, 1.00f);
    s_Colors[ImGuiCol_HeaderHovered] = ImVec4(0.98f, 0.00f, 0.00f, 1.00f);
    s_Colors[ImGuiCol_HeaderActive] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    s_Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    s_Colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    s_Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.09f);
    s_Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.55f, 0.11f, 0.13f, 1.00f);
    s_Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_Tab] = ImVec4(0.55f, 0.11f, 0.13f, 1.00f);
    s_Colors[ImGuiCol_TabHovered] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_TabActive] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
    s_Colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    s_Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    s_Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    s_Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    s_Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    s_Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    s_Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    s_Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    s_Colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    s_Colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    s_Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    s_Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    s_Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    s_Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    s_Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    s_Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    s_Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void ImGuiRenderer::OnEngineInitialized() {
    Hooks::ZApplicationEngineWin32_MainWindowProc->AddDetour(this, &ImGuiRenderer::WndProc);
    Hooks::ZKeyboardWindows_Update->AddDetour(this, &ImGuiRenderer::ZKeyboardWindows_Update);

    m_WndProcHooked.store(true, std::memory_order_release);
}

void ImGuiRenderer::SetSwapChain(IDXGISwapChain3* p_SwapChain) {
    if (p_SwapChain == m_SwapChain.m_Ref) {
        return;
    }

    m_SwapChain = p_SwapChain;
}

void ImGuiRenderer::SetCommandQueue(ID3D12CommandQueue* p_CommandQueue) {
    if (p_CommandQueue == m_CommandQueue.m_Ref) {
        return;
    }

    m_CommandQueue = p_CommandQueue;
}

void ImGuiRenderer::OnPresent(IDXGISwapChain3* p_SwapChain) {
    if (!m_CommandQueue) {
        return;
    }

    if (!SetupRenderer(p_SwapChain)) {
        Logger::Error("[ImGuiRenderer] Failed to set up renderer.");
        return;
    }

    // The ImGui frame runs on the thread that owns the game window, since the platform windows ImGui creates must
    // belong to that thread too. Owned windows on another thread share its input queue and can deadlock with it.
    if (m_WndProcHooked.load(std::memory_order_acquire) &&
        !m_FrameRequested.exchange(true, std::memory_order_acq_rel)) {
        if (!PostMessageW(m_Hwnd, m_FrameMessage, 0, 0)) {
            m_FrameRequested.store(false, std::memory_order_release);
        }
    }

    if (!m_IsImGuiVisible.load(std::memory_order_acquire)) {
        m_RenderSnapshot.reset();
        return;
    }

    std::unique_ptr<DrawDataSnapshot> s_OldSnapshot;

    {
        std::scoped_lock s_Lock(m_SnapshotMutex);

        // Free the old snapshjot after the lock is released, so the window thread doesn't wait for it.
        if (m_PendingSnapshot) {
            s_OldSnapshot = std::exchange(m_RenderSnapshot, std::move(m_PendingSnapshot));
        }
    }

    if (!m_RenderSnapshot) {
        return;
    }

    const auto s_NextFrame = (++m_FrameCounter) % m_FrameContext.size();
    auto& s_FrameCtx = m_FrameContext[s_NextFrame];

    const std::uint64_t s_PendingValue = s_FrameCtx.m_FenceValue.load(std::memory_order_acquire);

    if (s_PendingValue != 0 && s_PendingValue > m_Fence->GetCompletedValue()) {
        BreakIfFailed(m_Fence->SetEventOnCompletion(s_PendingValue, m_FenceEvent.m_Handle));
        WaitForSingleObject(m_FenceEvent.m_Handle, INFINITE);
    }

    ReleaseDeferredResources(s_FrameCtx.m_DeferredResources);

    const auto s_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    s_FrameCtx.m_CommandAllocator->Reset();
    BreakIfFailed(m_CommandList->Reset(s_FrameCtx.m_CommandAllocator, nullptr));

    D3D12_RESOURCE_BARRIER s_RtBarrier {};
    s_RtBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    s_RtBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    s_RtBarrier.Transition.pResource = m_BackBuffers[s_BackBufferIndex];
    s_RtBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    s_RtBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    s_RtBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    m_CommandList->ResourceBarrier(1, &s_RtBarrier);

    const auto s_RTVHandle = m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    const D3D12_CPU_DESCRIPTOR_HANDLE s_RTVDescriptor { s_RTVHandle.ptr + s_BackBufferIndex * m_RTVDescriptorSize };

    m_CommandList->OMSetRenderTargets(1, &s_RTVDescriptor, FALSE, nullptr);
    m_CommandList->SetDescriptorHeaps(1, &m_SRVDescriptorHeap.m_Ref);

    {
        std::scoped_lock s_Lock(m_BackendMutex);
        ImGui_ImplDX12_RenderDrawData(&m_RenderSnapshot->m_DrawData, m_CommandList);
    }

    s_FrameCtx.m_DeferredResources = TakeDeferredResources();

    D3D12_RESOURCE_BARRIER s_PresentBarrier {};
    s_PresentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    s_PresentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    s_PresentBarrier.Transition.pResource = m_BackBuffers[s_BackBufferIndex];
    s_PresentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    s_PresentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    s_PresentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    m_CommandList->ResourceBarrier(1, &s_PresentBarrier);
    BreakIfFailed(m_CommandList->Close());

    ID3D12CommandList* s_Lists[] = { m_CommandList.m_Ref };
    m_CommandQueue->ExecuteCommandLists(1, s_Lists);
}

void ImGuiRenderer::PostPresent(IDXGISwapChain3* p_SwapChain, HRESULT p_PresentResult) {
    if (!m_CommandQueue || !m_RendererSetup) {
        return;
    }

    if (p_PresentResult == DXGI_ERROR_DEVICE_REMOVED || p_PresentResult == DXGI_ERROR_DEVICE_RESET) {
        Logger::Error("[ImGuiRenderer] Device lost after Present (hr={:#x}).", static_cast<std::uint32_t>(p_PresentResult));
        return;
    }

    auto& s_FrameCtx = m_FrameContext[m_FrameCounter.load(std::memory_order_acquire) % c_MaxRenderedFrames];
    const std::uint64_t s_NewFence = ++m_FenceValue;
    s_FrameCtx.m_FenceValue.store(s_NewFence, std::memory_order_release);

    BreakIfFailed(m_CommandQueue->Signal(m_Fence, s_NewFence));
}

void ImGuiRenderer::WaitForCurrentFrameToFinish() const {
    const std::uint64_t s_Fence = m_FenceValue.load(std::memory_order_acquire);

    if (s_Fence != 0 && s_Fence > m_Fence->GetCompletedValue()) {
        BreakIfFailed(m_Fence->SetEventOnCompletion(s_Fence, m_FenceEvent.m_Handle));
        WaitForSingleObject(m_FenceEvent.m_Handle, INFINITE);
    }
}

void ImGuiRenderer::AllocateSRVDescriptor(
    D3D12_CPU_DESCRIPTOR_HANDLE* p_OutCpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE* p_OutGpuHandle
) {
    std::scoped_lock s_Lock(m_DescriptorMutex);

    UINT s_Index;

    if (!m_FreeSRVDescriptorIndices.empty()) {
        s_Index = m_FreeSRVDescriptorIndices.back();
        m_FreeSRVDescriptorIndices.pop_back();
    }
    else {
        s_Index = m_NextSRVDescriptorIndex++;
    }

    *p_OutCpuHandle = m_SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    p_OutCpuHandle->ptr += s_Index * m_SRVDescriptorSize;

    *p_OutGpuHandle = m_SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    p_OutGpuHandle->ptr += s_Index * m_SRVDescriptorSize;
}

void ImGuiRenderer::FreeSRVDescriptor(
    D3D12_CPU_DESCRIPTOR_HANDLE p_CpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE
) {
    const auto s_HeapStart =
        m_SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    const UINT s_Index =
        static_cast<UINT>((p_CpuHandle.ptr - s_HeapStart.ptr) / m_SRVDescriptorSize);

    std::scoped_lock s_Lock(m_DescriptorMutex);
    m_FreeSRVDescriptorIndices.push_back(s_Index);
}

std::vector<ImGuiRenderer::DeferredResource> ImGuiRenderer::TakeDeferredResources() {
    std::scoped_lock s_Lock(m_DescriptorMutex);
    return std::exchange(m_PendingDeferredResources, {});
}

void ImGuiRenderer::ReleaseDeferredResources(
    std::vector<DeferredResource>& p_Resources
) {
    for (auto& s_Resource : p_Resources) {
        if (s_Resource.m_ImGuiTexture.id) {
            FreeSRVDescriptor(
                s_Resource.m_ImGuiTexture.srvCPUDescriptor,
                s_Resource.m_ImGuiTexture.srvGPUDescriptor
            );
        }
    }

    p_Resources.clear();
}

bool ImGuiRenderer::SetupRenderer(IDXGISwapChain3* p_SwapChain) {
    if (m_RendererSetup) {
        return true;
    }

    ScopedD3DRef<ID3D12Device> s_Device;

    if (p_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK) {
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 s_SwapChainDesc {};

    if (p_SwapChain->GetDesc1(&s_SwapChainDesc) != S_OK) {
        return false;
    }

    m_SwapChain = p_SwapChain;

    const auto s_BufferCount = s_SwapChainDesc.BufferCount;

    {
        D3D12_DESCRIPTOR_HEAP_DESC s_RTVHeapDesc {};
        s_RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        s_RTVHeapDesc.NumDescriptors = s_BufferCount;
        s_RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        if (s_Device->CreateDescriptorHeap(&s_RTVHeapDesc, IID_PPV_ARGS(m_RTVDescriptorHeap.ReleaseAndGetPtr())) != S_OK) {
            return false;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC s_SRVHeapDesc {};
        s_SRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        s_SRVHeapDesc.NumDescriptors = c_MaxSRVDescriptors;
        s_SRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        if (s_Device->CreateDescriptorHeap(&s_SRVHeapDesc, IID_PPV_ARGS(m_SRVDescriptorHeap.ReleaseAndGetPtr())) != S_OK) {
            return false;
        }
    }

    for (UINT i = 0; i < c_MaxRenderedFrames; ++i) {
        auto& s_FrameCtx = m_FrameContext[i];

        if (s_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(s_FrameCtx.m_CommandAllocator.ReleaseAndGetPtr()))
            != S_OK) {
            return false;
        }

        s_FrameCtx.m_FenceValue.store(0);
    }

    m_BackBuffers.clear();
    m_BackBuffers.resize(s_BufferCount);

    m_RTVDescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_SRVDescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    const auto s_RTVHandle = m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < s_BufferCount; ++i) {
        if (p_SwapChain->GetBuffer(i, IID_PPV_ARGS(m_BackBuffers[i].ReleaseAndGetPtr())) != S_OK) {
            return false;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE s_Descriptor { s_RTVHandle.ptr + i * m_RTVDescriptorSize };

        s_Device->CreateRenderTargetView(m_BackBuffers[i], nullptr, s_Descriptor);
    }

    if (s_Device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_FrameContext[0].m_CommandAllocator, nullptr, IID_PPV_ARGS(m_CommandList.ReleaseAndGetPtr())
    ) != S_OK
        || m_CommandList->Close() != S_OK) {
        return false;
    }

    if (s_Device->CreateFence(m_FenceValue.load(), D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.ReleaseAndGetPtr())) != S_OK) {
        return false;
    }

    m_FenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    if (!m_FenceEvent) {
        return false;
    }

    if (p_SwapChain->GetHwnd(&m_Hwnd) != S_OK) {
        return false;
    }

    ImGui_ImplDX12_InitInfo s_InitInfo {};
    s_InitInfo.Device = s_Device;
    s_InitInfo.CommandQueue = m_CommandQueue;
    s_InitInfo.NumFramesInFlight = c_MaxRenderedFrames;
    s_InitInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    s_InitInfo.SrvDescriptorHeap = m_SRVDescriptorHeap;
    s_InitInfo.UserData = this;

    s_InitInfo.SrvDescriptorAllocFn = [](
        ImGui_ImplDX12_InitInfo* p_InitInfo,
        D3D12_CPU_DESCRIPTOR_HANDLE* p_OutCPUHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE* p_OutGPUHandle) {
            auto* s_Renderer = static_cast<ImGuiRenderer*>(p_InitInfo->UserData);
            s_Renderer->AllocateSRVDescriptor(p_OutCPUHandle, p_OutGPUHandle);
        };

    s_InitInfo.SrvDescriptorFreeFn = [](
        ImGui_ImplDX12_InitInfo* p_InitInfo,
        D3D12_CPU_DESCRIPTOR_HANDLE p_CPUHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE p_GPUHandle) {
            auto* s_Renderer = static_cast<ImGuiRenderer*>(p_InitInfo->UserData);
            s_Renderer->FreeSRVDescriptor(p_CPUHandle, p_GPUHandle);
        };

    if (!ImGui_ImplDX12_Init(&s_InitInfo)) {
        Logger::Error("[ImGuiRenderer] ImGui_ImplDX12_Init failed.");
        return false;
    }

    if (!ImGui_ImplDX12_CreateDeviceObjects()) {
        Logger::Error("[ImGuiRenderer] ImGui_ImplDX12_CreateDeviceObjects failed.");
        return false;
    }

    auto& s_PlatformIO = ImGui::GetPlatformIO();

    m_RendererCreateWindow = s_PlatformIO.Renderer_CreateWindow;
    m_RendererDestroyWindow = s_PlatformIO.Renderer_DestroyWindow;
    m_RendererSetWindowSize = s_PlatformIO.Renderer_SetWindowSize;
    m_RendererRenderWindow = s_PlatformIO.Renderer_RenderWindow;

    s_PlatformIO.Renderer_CreateWindow = [](ImGuiViewport* p_Viewport) {
        const auto s_Renderer = ModSDK::GetInstance()->GetImguiRenderer();

        std::scoped_lock s_Lock(s_Renderer->m_BackendMutex);
        s_Renderer->m_RendererCreateWindow(p_Viewport);
    };

    s_PlatformIO.Renderer_DestroyWindow = [](ImGuiViewport* p_Viewport) {
        const auto s_Renderer = ModSDK::GetInstance()->GetImguiRenderer();
        std::scoped_lock s_Lock(s_Renderer->m_BackendMutex);
        s_Renderer->m_RendererDestroyWindow(p_Viewport);
    };

    s_PlatformIO.Renderer_SetWindowSize = [](ImGuiViewport* p_Viewport, ImVec2 p_Size) {
        const auto s_Renderer = ModSDK::GetInstance()->GetImguiRenderer();
        std::scoped_lock s_Lock(s_Renderer->m_BackendMutex);
        s_Renderer->m_RendererSetWindowSize(p_Viewport, p_Size);
    };

    s_PlatformIO.Renderer_RenderWindow = [](ImGuiViewport* p_Viewport, void* p_RenderArg) {
        const auto s_Renderer = ModSDK::GetInstance()->GetImguiRenderer();
        std::scoped_lock s_Lock(s_Renderer->m_BackendMutex);
        s_Renderer->m_RendererRenderWindow(p_Viewport, p_RenderArg);
    };

    m_RendererSetup = true;

    Logger::Info("[ImGuiRenderer] Renderer ready (hwnd={}).", static_cast<void*>(m_Hwnd));

    return true;
}

void ImGuiRenderer::TeardownRenderer() {
    if (!m_RendererSetup) {
        return;
    }

    ReleaseFrameResources();

    // The wrappers reach this renderer through ModSDK, which may already be tearing it down.
    auto& s_PlatformIO = ImGui::GetPlatformIO();
    s_PlatformIO.Renderer_CreateWindow = m_RendererCreateWindow;
    s_PlatformIO.Renderer_DestroyWindow = m_RendererDestroyWindow;
    s_PlatformIO.Renderer_SetWindowSize = m_RendererSetWindowSize;
    s_PlatformIO.Renderer_RenderWindow = m_RendererRenderWindow;

    ImGui_ImplDX12_Shutdown();

    if (m_PlatformSetup) {
        ImGui_ImplWin32_Shutdown();
        m_PlatformSetup = false;
    }

    m_BackBuffers.clear();
    m_CommandList.Reset();
    m_Fence.Reset();
    m_FenceEvent.Reset();
    m_RTVDescriptorHeap.Reset();
    m_SRVDescriptorHeap.Reset();
    m_SwapChain.Reset();
    m_CommandQueue.Reset();
    m_RendererSetup = false;
}

void ImGuiRenderer::ReleaseFrameResources() {
    WaitForCurrentFrameToFinish();

    for (auto& s_FrameCtx : m_FrameContext) {
        ReleaseDeferredResources(s_FrameCtx.m_DeferredResources);

        s_FrameCtx.m_FenceValue.store(m_FenceValue.load(std::memory_order_acquire));
    }

    auto s_PendingResources = TakeDeferredResources();
    ReleaseDeferredResources(s_PendingResources);

    m_RenderSnapshot.reset();

    std::scoped_lock s_Lock(m_SnapshotMutex);
    m_PendingSnapshot.reset();
}

void ImGuiRenderer::OnReset(IDXGISwapChain3* p_SwapChain) {
    if (!m_RendererSetup) {
        return;
    }

    ReleaseFrameResources();

    m_BackBuffers.clear();

    std::scoped_lock s_Lock(m_BackendMutex);
    ImGui_ImplDX12_InvalidateDeviceObjects();
}

void ImGuiRenderer::PostReset(IDXGISwapChain3* p_SwapChain) {
    if (!m_RendererSetup) {
        return;
    }

    DXGI_SWAP_CHAIN_DESC1 s_SwapChainDesc {};

    if (p_SwapChain->GetDesc1(&s_SwapChainDesc) != S_OK) {
        return;
    }

    ScopedD3DRef<ID3D12Device> s_Device;

    if (p_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK) {
        return;
    }

    m_BackBuffers.resize(s_SwapChainDesc.BufferCount);

    m_RTVDescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    const auto s_RTVHandle = m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < m_BackBuffers.size(); ++i) {
        if (p_SwapChain->GetBuffer(i, IID_PPV_ARGS(m_BackBuffers[i].ReleaseAndGetPtr())) != S_OK) {
            return;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE s_Descriptor { s_RTVHandle.ptr + i * m_RTVDescriptorSize };

        s_Device->CreateRenderTargetView(m_BackBuffers[i], nullptr, s_Descriptor);
    }

    std::scoped_lock s_Lock(m_BackendMutex);

    ImGui_ImplDX12_CreateDeviceObjects();
}

void ImGuiRenderer::RunFrame() {
    m_FrameRequested.store(false, std::memory_order_release);

    if (m_InFrame) {
        return;
    }

    if (!m_PlatformSetup) {
        if (!ImGui_ImplWin32_Init(m_Hwnd)) {
            Logger::Error("[ImGuiRenderer] ImGui_ImplWin32_Init failed.");
            return;
        }

        ImGui_ImplWin32_SetArrowCursor((*Globals::ApplicationEngineWin32)->m_hDefaultCursor);

        // ImGui shows windows dragged out of the game without activating them, so they end up behind the game
        // window, which stays active. Popups, menus and tooltips have no taskbar icon and keep that behavior.
        auto& s_PlatformIO = ImGui::GetPlatformIO();
        m_PlatformShowWindow = s_PlatformIO.Platform_ShowWindow;

        s_PlatformIO.Platform_ShowWindow = [](ImGuiViewport* p_Viewport) {
            ModSDK::GetInstance()->GetImguiRenderer()->m_PlatformShowWindow(p_Viewport);

            if (!(p_Viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon)) {
                ImGui::GetPlatformIO().Platform_SetWindowFocus(p_Viewport);
            }
        };

        m_PlatformSetup = true;
    }

    if (!m_IsImGuiVisible.load(std::memory_order_acquire)) {
        ImGui::DestroyPlatformWindows();
        return;
    }

    // Wait until the game window has a non zero size.
    RECT s_ClientRect {};

    if (IsIconic(m_Hwnd) || !GetClientRect(m_Hwnd, &s_ClientRect) || s_ClientRect.right <= 0 || s_ClientRect.bottom <= 0) {
        return;
    }

    m_InFrame = true;

    {
        std::scoped_lock s_Lock(m_BackendMutex);
        ImGui_ImplDX12_NewFrame();
    }

    ImGui_ImplWin32_NewFrame();

    auto& s_IO = ImGui::GetIO();
    s_IO.FontGlobalScale = s_IO.DisplaySize.y / 1800.f;

    // While the window manager moves a window, Wine gets no pointer updates, so we work out the mouse position from
    // where the window is and where on it the drag started. ImGui then places the window where it already is, and can
    // find the docking target under the mouse. These events come after the Win32 backend's, which use the stale
    // cursor position.
    if (m_WindowManagerMoveHwnd) {
        const ImVec2 s_ClickOffset = ImGui::GetCurrentContext()->ActiveIdClickOffset;
        POINT s_MovePoint {};
        ClientToScreen(m_WindowManagerMoveHwnd, &s_MovePoint);
        s_MovePoint.x += static_cast<LONG>(s_ClickOffset.x);
        s_MovePoint.y += static_cast<LONG>(s_ClickOffset.y);

        // The moved window is transparent to hit tests while it is dragged, so this finds the window under it.
        const ImGuiViewport* s_HoveredViewport = ImGui::FindViewportByPlatformHandle(WindowFromPoint(s_MovePoint));

        s_IO.AddMousePosEvent(static_cast<float>(s_MovePoint.x), static_cast<float>(s_MovePoint.y));
        s_IO.AddMouseViewportEvent(s_HoveredViewport ? s_HoveredViewport->ID : 0);
    }

    ImGui::NewFrame();

    Draw();

    ImGui::Render();

    {
        std::scoped_lock s_Lock(m_BackendMutex);

        for (ImTextureData* s_Texture : ImGui::GetPlatformIO().Textures) {
            if (s_Texture->Status != ImTextureStatus_OK) {
                ImGui_ImplDX12_UpdateTexture(s_Texture);
            }
        }
    }

    auto s_Snapshot = std::make_unique<DrawDataSnapshot>(ImGui::GetDrawData());

    {
        std::scoped_lock s_Lock(m_SnapshotMutex);
        m_PendingSnapshot = std::move(s_Snapshot);
    }

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();

    m_InFrame = false;

    // X11 window managers like KWin keep windows that a program moves inside the work area, but not windows that
    // the user drags. So under Wine we hand the drag of a detached window over to the window manager.
    static const bool s_IsWine = Util::ProcessUtils::IsRunningUnderWine();
    const ImGuiWindow* s_MovingWindow = ImGui::GetCurrentContext()->MovingWindow;

    if (!s_IsWine || m_WindowManagerMoveHwnd || !s_MovingWindow || !s_MovingWindow->ViewportOwned) {
        return;
    }

    const auto s_MovingHwnd = static_cast<HWND>(s_MovingWindow->Viewport->PlatformHandleRaw);

    if (s_MovingHwnd && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
        StartWindowManagerMove(s_MovingHwnd);
    }
}

void ImGuiRenderer::StartWindowManagerMove(HWND p_Hwnd) {
    POINT s_Cursor {};
    GetCursorPos(&s_Cursor);

    // ImGui captures the mouse while a button is down, and the move can't start while another window holds it.
    ReleaseCapture();

    m_WindowManagerMoveHwnd = p_Hwnd;
    ClipCursor(nullptr);

    // This runs a modal loop until the button is released. The loop still dispatches our frame messages, so ImGui
    // keeps running and can show the docking targets.
    SendMessageW(p_Hwnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(s_Cursor.x, s_Cursor.y));

    m_WindowManagerMoveHwnd = nullptr;
    PostMessageW(m_Hwnd, c_GameUpdateCursorClipMessage, 0, 0);

    // The window manager can swallow the button release, which would leave ImGui dragging the window.
    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    }
}

void ImGuiRenderer::Draw() {
    const bool s_HasFocus = m_ImGuiHasFocus.load(std::memory_order_acquire);

    ImGui::GetStyle().Alpha = s_HasFocus ? 1.f : 0.3f;

    ModSDK::GetInstance()->OnDrawUI(s_HasFocus);

    if (m_UIToggleWarningRequested.exchange(false, std::memory_order_acquire)) {
        m_ShowingUIToggleWarning = true;
    }

    if (m_ShowingUIToggleWarning) {
        const auto s_Center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(s_Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushFont(SDK()->GetImGuiBlackFont());
        const auto s_Expanded = ImGui::Begin("Warning", &m_ShowingUIToggleWarning);
        ImGui::PushFont(SDK()->GetImGuiRegularFont());

        if (s_Expanded) {
            ImGui::Text("You have pressed the UI toggle key (F11 by default), which will HIDE the SDK UI.");
            ImGui::Text("You must press this key again to show the SDK UI.");
            ImGui::Text("If you want to change this key, you can do so in the mods.ini file.");
            ImGui::Text("See the SDK readme for more information. This warning will not appear again.");

            ImGui::NewLine();

            static bool s_HasConfirmed = false;

            ImGui::Checkbox(
                "I understand I'm hiding the UI and that I must press this key to show it again", &s_HasConfirmed
            );

            ImGui::NewLine();

            ImGui::BeginDisabled(!s_HasConfirmed);

            if (ImGui::Button("Continue")) {
                ModSDK::GetInstance()->SetHasShownUIToggleWarning();
                m_IsImGuiVisible.store(false, std::memory_order_release);
                m_ShowingUIToggleWarning = false;
                m_ImGuiHasFocus.store(false, std::memory_order_release);
            }

            ImGui::EndDisabled();

            ImGui::SameLine();

            if (ImGui::Button("Cancel")) {
                m_ShowingUIToggleWarning = false;
            }
        }

        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
    }
}

bool ImGuiRenderer::CreateDDSTextureFromMemory(
    const void* p_Data,
    size_t p_DataSize,
    ScopedD3DRef<ID3D12Resource>& p_OutTexture,
    ImGuiTexture& p_OutImGuiTexture
) {
    return CreateTexture(
        [p_Data, p_DataSize](ScopedD3DRef<ID3D12Device>& device,
            DirectX::ResourceUploadBatch& batch,
            ID3D12Resource** texture) {
                return DirectX::CreateDDSTextureFromMemory(
                    device,
                    batch,
                    reinterpret_cast<const uint8_t*>(p_Data),
                    p_DataSize,
                    texture
                );
    },
        p_OutTexture,
        p_OutImGuiTexture
    );
}

bool ImGuiRenderer::CreateDDSTextureFromFile(
    const std::string& p_FilePath,
    ScopedD3DRef<ID3D12Resource>& p_OutTexture,
    ImGuiTexture& p_OutImGuiTexture
) {
    return CreateTexture(
        [p_FilePath](ScopedD3DRef<ID3D12Device>& device,
            DirectX::ResourceUploadBatch& batch,
            ID3D12Resource** texture) {
                const std::wstring s_FilePath2(p_FilePath.begin(), p_FilePath.end());

                return DirectX::CreateDDSTextureFromFile(device, batch, s_FilePath2.c_str(), texture);
    },
        p_OutTexture,
        p_OutImGuiTexture
    );
}

bool ImGuiRenderer::CreateWICTextureFromMemory(
    const void* p_Data,
    size_t p_DataSize,
    ScopedD3DRef<ID3D12Resource>& p_OutTexture,
    ImGuiTexture& p_OutImGuiTexture
) {
    return CreateTexture(
        [p_Data, p_DataSize](ScopedD3DRef<ID3D12Device>& device,
            DirectX::ResourceUploadBatch& batch,
            ID3D12Resource** texture) {
                return DirectX::CreateWICTextureFromMemory(device, batch, reinterpret_cast<const uint8_t*>(p_Data), p_DataSize, texture);
    },
        p_OutTexture,
        p_OutImGuiTexture
    );
}

bool ImGuiRenderer::CreateWICTextureFromFile(
    const std::string& p_FilePath,
    ScopedD3DRef<ID3D12Resource>& p_OutTexture,
    ImGuiTexture& p_OutImGuiTexture
) {
    return CreateTexture(
        [p_FilePath](ScopedD3DRef<ID3D12Device>& device,
            DirectX::ResourceUploadBatch& batch,
            ID3D12Resource** texture) {
                const std::wstring s_FilePath2(p_FilePath.begin(), p_FilePath.end());

                return DirectX::CreateWICTextureFromFile(device, batch, s_FilePath2.c_str(), texture);
    },
        p_OutTexture,
        p_OutImGuiTexture
    );
}

bool ImGuiRenderer::CreateImGuiTextureSRV(ID3D12Resource* p_Texture, ImGuiTexture& p_OutImGuiTexture) {
    if (!p_Texture) {
        return false;
    }

    if (!m_RendererSetup) {
        Logger::Error("Failed to create texture - ImGui renderer is not set up!");

        return false;
    }

    ScopedD3DRef<ID3D12Device> s_Device;

    if (m_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK) {
        Logger::Error("Failed to retrieve D3D12 device from swap chain in CreateImGuiTexture!");

        return false;
    }

    const auto s_Description = p_Texture->GetDesc();

    p_OutImGuiTexture.width = static_cast<UINT>(s_Description.Width);
    p_OutImGuiTexture.height = s_Description.Height;

    AllocateSRVDescriptor(
        &p_OutImGuiTexture.srvCPUDescriptor,
        &p_OutImGuiTexture.srvGPUDescriptor
    );

    D3D12_SHADER_RESOURCE_VIEW_DESC s_SRVDescription {};
    s_SRVDescription.Format = s_Description.Format;
    s_SRVDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    s_SRVDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    s_SRVDescription.Texture2D.MostDetailedMip = 0;
    s_SRVDescription.Texture2D.MipLevels = s_Description.MipLevels;
    s_SRVDescription.Texture2D.PlaneSlice = 0;
    s_SRVDescription.Texture2D.ResourceMinLODClamp = 0.f;

    s_Device->CreateShaderResourceView(
        p_Texture,
        &s_SRVDescription,
        p_OutImGuiTexture.srvCPUDescriptor
    );

    p_OutImGuiTexture.id = p_OutImGuiTexture.srvGPUDescriptor.ptr;

    return true;
}

void ImGuiRenderer::DestroyImGuiTextureSRV(ImGuiTexture& p_Texture) {
    if (!p_Texture.id) {
        return;
    }

    std::scoped_lock s_Lock(m_DescriptorMutex);

    m_PendingDeferredResources.push_back({
        .m_Texture = {},
        .m_ImGuiTexture = p_Texture
    });

    p_Texture = {};
}

void ImGuiRenderer::DestroyImGuiTexture(ScopedD3DRef<ID3D12Resource>& p_Texture, ImGuiTexture& p_ImGuiTexture) {
    if (!p_Texture && !p_ImGuiTexture.id) {
        return;
    }

    std::scoped_lock s_Lock(m_DescriptorMutex);

    m_PendingDeferredResources.push_back({
        std::move(p_Texture),
        p_ImGuiTexture
    });

    p_ImGuiTexture = {};
}

bool ImGuiRenderer::CreateTexture(
    std::function<HRESULT(ScopedD3DRef<ID3D12Device>&, DirectX::ResourceUploadBatch&, ID3D12Resource**)> p_Loader,
    ScopedD3DRef<ID3D12Resource>& p_OutTexture,
    ImGuiTexture& p_OutImGuiTexture
) {
    if (!m_RendererSetup) {
        Logger::Error("Failed to create texture - ImGui renderer is not set up!");

        return false;
    }

    ScopedD3DRef<ID3D12Device> s_Device;

    if (m_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK) {
        Logger::Error("Failed to retrieve D3D12 device from swap chain in CreateTexture!");

        return false;
    }

    DirectX::ResourceUploadBatch s_UploadBatch(s_Device);

    s_UploadBatch.Begin();

    ScopedD3DRef<ID3D12Resource> s_Texture;

    HRESULT s_Result = p_Loader(s_Device, s_UploadBatch, &s_Texture.m_Ref);

    if (FAILED(s_Result)) {
        Logger::Error("Failed to create texture via loader function!");

        return false;
    }

    auto s_Finish = s_UploadBatch.End(m_CommandQueue);

    s_Finish.wait();

    p_OutTexture = std::move(s_Texture);

    auto s_TextureDescription = p_OutTexture->GetDesc();

    p_OutImGuiTexture.width = static_cast<UINT>(s_TextureDescription.Width);
    p_OutImGuiTexture.height = static_cast<UINT>(s_TextureDescription.Height);

    AllocateSRVDescriptor(
        &p_OutImGuiTexture.srvCPUDescriptor,
        &p_OutImGuiTexture.srvGPUDescriptor
    );

    DirectX::CreateShaderResourceView(
        s_Device,
        p_OutTexture,
        p_OutImGuiTexture.srvCPUDescriptor
    );

    p_OutImGuiTexture.id = p_OutImGuiTexture.srvGPUDescriptor.ptr;

    const auto s_HeapStart = m_SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    const auto s_SRVDescriptorIndex =
        (p_OutImGuiTexture.srvCPUDescriptor.ptr - s_HeapStart.ptr) / m_SRVDescriptorSize;

    Logger::Info(
        "Created texture ({}x{}) in SRV slot {}.",
        s_TextureDescription.Width,
        s_TextureDescription.Height,
        s_SRVDescriptorIndex
    );

    return true;
}

void ImGuiRenderer::SetGameDescriptorHeap() {
    const auto s_RenderState =
        static_cast<ImGui_ImplDX12_RenderState*>(ImGui::GetPlatformIO().Renderer_RenderState);

    if (!s_RenderState) {
        return;
    }

    auto s_DescriptorHeap = Globals::RenderManager->m_pDevice->m_pFrameHeapCBVSRVUAV;
    s_RenderState->CommandList->SetDescriptorHeaps(1, &s_DescriptorHeap);
}

void ImGuiRenderer::ResetDescriptorHeap() {
    const auto s_RenderState =
        static_cast<ImGui_ImplDX12_RenderState*>(ImGui::GetPlatformIO().Renderer_RenderState);

    if (!s_RenderState) {
        return;
    }

    s_RenderState->CommandList->SetDescriptorHeaps(1, &m_SRVDescriptorHeap.m_Ref);
}

DEFINE_DETOUR_WITH_CONTEXT(
    ImGuiRenderer, LRESULT, WndProc, ZApplicationEngineWin32* th, HWND p_Hwnd, UINT p_Message, WPARAM p_Wparam,
    LPARAM p_Lparam
) {
    if (ImGui::GetCurrentContext() == nullptr)
        return HookResult<LRESULT>(HookAction::Continue());

    if (p_Message == m_FrameMessage) {
        RunFrame();
        return { HookAction::Return(), 0 };
    }

    // Fix weird window clipping behavior under Wine.
    if (p_Message == c_GameUpdateCursorClipMessage && m_WindowManagerMoveHwnd) {
        return { HookAction::Return(), 0 };
    }

    // Layout-independent tilde toggle: compare the hardware scancode
    // (lParam bits 16-23) rather than VK_OEM_3, which moves per layout.
    const uint8_t s_ScanCode = static_cast<uint8_t>(p_Lparam >> 16);

    if (s_ScanCode == ModSDK::GetInstance()->GetConsoleScanCode() && (p_Message == WM_KEYDOWN || p_Message ==
        WM_SYSKEYDOWN)) {
        const bool s_NewFocus = !m_ImGuiHasFocus.load(std::memory_order_relaxed);
        m_ImGuiHasFocus.store(s_NewFocus, std::memory_order_release);

        ZKeyboardWindows* s_KeyboardWindows = static_cast<ZKeyboardWindows*>(
            Globals::InputDeviceManager->m_devices[4]
        );

        if (s_NewFocus) {
            m_IsImGuiVisible.store(true, std::memory_order_release);

            if (s_KeyboardWindows->m_pbDigital) {
                // Clear the keyboard state to prevent held keys from remaining active while ImGui has focus.
                std::fill_n(
                    s_KeyboardWindows->m_pbDigital,
                    s_KeyboardWindows->m_digcount,
                    false
                );
            }
        }
        else {
            DWORD s_EventCount = 256;
            DIDEVICEOBJECTDATA s_Buffer[256];

            if (s_KeyboardWindows->dif.m_pDev) {
                // Prevent buffered events from being processed after ImGui is closed.
                s_KeyboardWindows->dif.m_pDev->GetDeviceData(
                    sizeof(DIDEVICEOBJECTDATA),
                    s_Buffer,
                    &s_EventCount,
                    0
                );
            }
        }
    }

    if (s_ScanCode == ModSDK::GetInstance()->GetUIToggleScanCode() && (p_Message == WM_KEYDOWN || p_Message ==
        WM_SYSKEYDOWN)) {
        if (!ModSDK::GetInstance()->HasShownUIToggleWarning()) {
            m_UIToggleWarningRequested.store(true, std::memory_order_release);
            m_ImGuiHasFocus.store(true, std::memory_order_release);
        }
        else {
            const bool s_NewVisible = !m_IsImGuiVisible.load(std::memory_order_relaxed);
            m_IsImGuiVisible.store(s_NewVisible, std::memory_order_release);

            if (!s_NewVisible) {
                m_ImGuiHasFocus.store(false, std::memory_order_release);
            }
        }
    }

    const bool s_HasFocus = m_ImGuiHasFocus.load(std::memory_order_acquire);

    //Globals::InputActionManager->m_bDebugKeys = true;
    Globals::InputActionManager->m_bEnabled = !s_HasFocus;

    if (!s_HasFocus) {
        return HookResult<LRESULT>(HookAction::Continue());
    }

    // If we got a quit / close message then return control back to the process.
    if (p_Message == WM_QUIT || p_Message == WM_DESTROY || p_Message == WM_NCDESTROY || p_Message == WM_CLOSE) {
        m_ImGuiHasFocus.store(false, std::memory_order_release);
        return HookResult<LRESULT>(HookAction::Continue());
    }

    // Pass resizing messages down to the process.
    if (p_Message == WM_SIZE) {
        return HookResult<LRESULT>(HookAction::Continue());
    }

    if (ImGui_ImplWin32_WndProcHandler(p_Hwnd, p_Message, p_Wparam, p_Lparam)) {
        return { HookAction::Return(), TRUE };
    }

    // Don't call the original function so input isn't passed down to the game.
    return { HookAction::Return(), DefWindowProcW(p_Hwnd, p_Message, p_Wparam, p_Lparam) };
}

DEFINE_DETOUR_WITH_CONTEXT(ImGuiRenderer, void, ZKeyboardWindows_Update, ZKeyboardWindows*, bool) {
    // Don't process input while the imgui overlay has focus.
    if (m_ImGuiHasFocus.load(std::memory_order_acquire)) {
        return HookResult<void>(HookAction::Return());
    }

    return HookResult<void>(HookAction::Continue());
}
