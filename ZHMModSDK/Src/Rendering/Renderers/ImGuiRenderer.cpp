#include <directx/d3dx12.h>

#include "ImGuiRenderer.h"

#include <d3dcompiler.h>
#include <dxgi1_4.h>

#include "ImGuiImpl.h"
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include <ResourceUploadBatch.h>
#include <DirectXHelpers.h>
#include <windowsx.h>

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

#include <IconsMaterialDesign.h>

using namespace Rendering::Renderers;

ImGuiRenderer::ImGuiRenderer() {
    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&m_TicksPerSecond));
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_Time));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImPlot::CreateContext();

    ImGuiIO& s_ImGuiIO = ImGui::GetIO();
    s_ImGuiIO.IniFilename = nullptr;
    s_ImGuiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    s_ImGuiIO.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values (optional)
    s_ImGuiIO.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    // We can honor io.WantSetMousePos requests (optional, rarely used)
    s_ImGuiIO.BackendPlatformName = "imgui_impl_win32";
    s_ImGuiIO.BackendRendererName = "imgui_impl_dx12";
    s_ImGuiIO.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    // Here we merge the material icon glyphs into each of our other fonts.
    ImFontConfig s_IconsConfig {};
    s_IconsConfig.MergeMode = true;
    s_IconsConfig.GlyphOffset = {0.f, 6.f};

    // Unicode ranges used by ImGui font
    static constexpr ImWchar c_TextRanges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin-1 Supplement
        0x2010, 0x2027, // Punctuation
        0
    };
    static constexpr ImWchar c_IconRanges[] = {ICON_MIN_MD, ICON_MAX_16_MD, 0};

    m_FontLight = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        RobotoLight_compressed_data, RobotoLight_compressed_size, 28.f, nullptr, c_TextRanges
    );
    s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        MaterialIconsRegular_compressed_data, MaterialIconsRegular_compressed_size, 28.f, &s_IconsConfig, c_IconRanges
    );
    s_ImGuiIO.Fonts->Build();

    m_FontRegular = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        RobotoRegular_compressed_data, RobotoRegular_compressed_size, 28.f, nullptr, c_TextRanges
    );
    s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        MaterialIconsRegular_compressed_data, MaterialIconsRegular_compressed_size, 28.f, &s_IconsConfig, c_IconRanges
    );
    s_ImGuiIO.Fonts->Build();

    m_FontMedium = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        RobotoMedium_compressed_data, RobotoMedium_compressed_size, 28.f, nullptr, c_TextRanges
    );
    s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        MaterialIconsRegular_compressed_data, MaterialIconsRegular_compressed_size, 28.f, &s_IconsConfig, c_IconRanges
    );
    s_ImGuiIO.Fonts->Build();

    m_FontBold = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        RobotoBold_compressed_data, RobotoBold_compressed_size, 28.f, nullptr, c_TextRanges
    );
    s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        MaterialIconsRegular_compressed_data, MaterialIconsRegular_compressed_size, 28.f, &s_IconsConfig, c_IconRanges
    );
    s_ImGuiIO.Fonts->Build();

    m_FontBlack = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        RobotoBlack_compressed_data, RobotoBlack_compressed_size, 28.f, nullptr, c_TextRanges
    );
    s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(
        MaterialIconsRegular_compressed_data, MaterialIconsRegular_compressed_size, 28.f, &s_IconsConfig, c_IconRanges
    );
    s_ImGuiIO.Fonts->Build();

    s_ImGuiIO.FontDefault = m_FontRegular;

    SetupStyles();
}

ImGuiRenderer::~ImGuiRenderer() {
    if (m_RendererSetup)
        WaitForCurrentFrameToFinish();

    HookRegistry::ClearDetoursWithContext(this);
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
    s_Style.IndentSpacing = 10.f;
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

void ImGuiRenderer::OnEngineInit() {
    Hooks::ZApplicationEngineWin32_MainWindowProc->AddDetour(this, &ImGuiRenderer::WndProc);
    Hooks::ZKeyboardWindows_Update->AddDetour(this, &ImGuiRenderer::ZKeyboardWindows_Update);
    Hooks::ZInputAction_Analog->AddDetour(this, &ImGuiRenderer::ZInputAction_Analog);
}

void ImGuiRenderer::Draw() {
    ImGui_ImplDX12_NewFrame();

    ImGuiIO& s_ImGuiIO = ImGui::GetIO();

    int64_t s_CurrentTime = 0;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&s_CurrentTime));

    s_ImGuiIO.DeltaTime = static_cast<float>(s_CurrentTime - m_Time) / m_TicksPerSecond;
    m_Time = s_CurrentTime;

    UpdateMouseData(s_ImGuiIO);
    ProcessKeyEventsWorkarounds(s_ImGuiIO);

    // Construct the UI.
    ImGui::NewFrame();

    ImGui::GetStyle().Alpha = m_ImguiHasFocus ? 1.f : 0.3f;

    ModSDK::GetInstance()->OnDrawUI(m_ImguiHasFocus);

    if (m_ShowingUiToggleWarning) {
        const auto s_Center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(s_Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushFont(SDK()->GetImGuiBlackFont());
        const auto s_Expanded = ImGui::Begin("Warning", &m_ShowingUiToggleWarning);
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
                ModSDK::GetInstance()->SetHasShownUiToggleWarning();
                m_ImguiVisible = false;
                m_ShowingUiToggleWarning = false;
                m_ImguiHasFocus = false;
            }

            ImGui::EndDisabled();

            ImGui::SameLine();

            if (ImGui::Button("Cancel")) {
                m_ShowingUiToggleWarning = false;
            }
        }

        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
    }
}

void ImGuiRenderer::OnPresent(IDXGISwapChain3* p_SwapChain) {
    if (!m_CommandQueue)
        return;

    if (!SetupRenderer(p_SwapChain)) {
        Logger::Error("Failed to set up ImGui renderer.");
        return;
    }

    if (!m_ImguiVisible)
        return;

    Draw();

    ImGui::Render();

    // Get context of next frame to render.
    auto& s_FrameCtx = m_FrameContext[++m_FrameCounter % m_FrameContext.size()];

    // If this context is still being rendered, we should wait for it.
    if (s_FrameCtx.FenceValue != 0 && s_FrameCtx.FenceValue > m_Fence->GetCompletedValue()) {
        BreakIfFailed(m_Fence->SetEventOnCompletion(s_FrameCtx.FenceValue, m_FenceEvent.Handle));
        WaitForSingleObject(m_FenceEvent.Handle, INFINITE);
    }

    const auto s_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    // Reset command list and allocator.
    s_FrameCtx.CommandAllocator->Reset();
    BreakIfFailed(m_CommandList->Reset(s_FrameCtx.CommandAllocator, nullptr));

    // Transition the render target into the correct state to allow for drawing into it.
    const D3D12_RESOURCE_BARRIER s_RTBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_BackBuffers[s_BackBufferIndex],
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    m_CommandList->ResourceBarrier(1, &s_RTBarrier);

    const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    const CD3DX12_CPU_DESCRIPTOR_HANDLE s_RtvDescriptor(s_RtvHandle, s_BackBufferIndex, m_RtvDescriptorSize);

    m_CommandList->OMSetRenderTargets(1, &s_RtvDescriptor, FALSE, nullptr);
    m_CommandList->SetDescriptorHeaps(1, &m_SrvDescriptorHeap.Ref);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList, m_SrvDescriptorHeap);

    const D3D12_RESOURCE_BARRIER s_PresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_BackBuffers[s_BackBufferIndex],
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );

    m_CommandList->ResourceBarrier(1, &s_PresentBarrier);
    BreakIfFailed(m_CommandList->Close());

    m_CommandQueue->ExecuteCommandLists(1, CommandListCast(&m_CommandList.Ref));
}

void ImGuiRenderer::PostPresent(IDXGISwapChain3* p_SwapChain, HRESULT p_PresentResult) {
    if (!m_CommandQueue || !m_RendererSetup)
        return;

    if (p_PresentResult == DXGI_ERROR_DEVICE_REMOVED || p_PresentResult == DXGI_ERROR_DEVICE_RESET) {
        Logger::Error("Device lost after present.");
        abort();
    }
    else {
        FrameContext& s_FrameCtx = m_FrameContext[m_FrameCounter % MaxRenderedFrames];

        // Update the fence value for this frame and ask to receive a signal with this
        // fence value as soon as the GPU has finished rendering the frame. We update this
        // monotonically in order to always have the latest number represent the most
        // recently submitted frame, and in order to avoid having multiple frames share
        // the same fence value.
        s_FrameCtx.FenceValue = ++m_FenceValue;
        BreakIfFailed(m_CommandQueue->Signal(m_Fence, s_FrameCtx.FenceValue));
    }
}

void ImGuiRenderer::WaitForCurrentFrameToFinish() const {
    if (m_FenceValue != 0 && m_FenceValue > m_Fence->GetCompletedValue()) {
        BreakIfFailed(m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent.Handle));
        WaitForSingleObject(m_FenceEvent.Handle, INFINITE);
    }
}

bool ImGuiRenderer::SetupRenderer(IDXGISwapChain3* p_SwapChain) {
    if (m_RendererSetup)
        return true;

    Logger::Debug("Setting up ImGui renderer.");

    ScopedD3DRef<ID3D12Device> s_Device;

    if (p_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK)
        return false;

    DXGI_SWAP_CHAIN_DESC1 s_SwapChainDesc;

    if (p_SwapChain->GetDesc1(&s_SwapChainDesc) != S_OK)
        return false;

    m_SwapChain = p_SwapChain;

    const auto s_BufferCount = s_SwapChainDesc.BufferCount;

    {
        D3D12_DESCRIPTOR_HEAP_DESC s_Desc = {};
        s_Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        s_Desc.NumDescriptors = s_BufferCount;
        s_Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        s_Desc.NodeMask = 0;

        if (s_Device->CreateDescriptorHeap(&s_Desc, IID_PPV_ARGS(m_RtvDescriptorHeap.ReleaseAndGetPtr())) != S_OK)
            return false;

        D3D_SET_OBJECT_NAME_A(m_RtvDescriptorHeap, "ZHMModSDK ImGui Rtv Descriptor Heap");
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC s_Desc = {};
        s_Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        s_Desc.NumDescriptors = MaxSRVDescriptors;
        s_Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        s_Desc.NodeMask = 0;

        if (s_Device->CreateDescriptorHeap(&s_Desc, IID_PPV_ARGS(m_SrvDescriptorHeap.ReleaseAndGetPtr())) != S_OK)
            return false;

        D3D_SET_OBJECT_NAME_A(m_SrvDescriptorHeap, "ZHMModSDK ImGui Srv Descriptor Heap");
    }

    m_FrameContext.clear();

    for (UINT i = 0; i < MaxRenderedFrames; ++i) {
        FrameContext s_Frame {};

        if (s_Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(s_Frame.CommandAllocator.ReleaseAndGetPtr())
        ) != S_OK)
            return false;

        char s_CmdAllocDebugName[128];
        sprintf_s(s_CmdAllocDebugName, sizeof(s_CmdAllocDebugName), "ZHMModSDK ImGui Command Allocator #%u", i);
        D3D_SET_OBJECT_NAME_A(s_Frame.CommandAllocator, s_CmdAllocDebugName);

        s_Frame.FenceValue = 0;

        m_FrameContext.push_back(std::move(s_Frame));
    }

    // Create RTVs for back buffers.
    m_BackBuffers.clear();
    m_BackBuffers.resize(s_BufferCount);

    m_RtvDescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < s_BufferCount; ++i) {
        if (p_SwapChain->GetBuffer(i, IID_PPV_ARGS(m_BackBuffers[i].ReleaseAndGetPtr())) != S_OK)
            return false;

        const CD3DX12_CPU_DESCRIPTOR_HANDLE s_RtvDescriptor(s_RtvHandle, i, m_RtvDescriptorSize);
        s_Device->CreateRenderTargetView(m_BackBuffers[i], nullptr, s_RtvDescriptor);
    }

    if (s_Device->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_FrameContext[0].CommandAllocator, nullptr,
            IID_PPV_ARGS(m_CommandList.ReleaseAndGetPtr())
        ) != S_OK ||
        m_CommandList->Close() != S_OK)
        return false;

    char s_CmdListDebugName[128];
    sprintf_s(s_CmdListDebugName, sizeof(s_CmdListDebugName), "ZHMModSDK ImGui Command List");
    D3D_SET_OBJECT_NAME_A(m_CommandList, s_CmdListDebugName);

    if (s_Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.ReleaseAndGetPtr())) != S_OK)
        return false;

    char s_FenceDebugName[128];
    sprintf_s(s_FenceDebugName, sizeof(s_FenceDebugName), "ZHMModSDK ImGui Fence");
    D3D_SET_OBJECT_NAME_A(m_Fence, s_FenceDebugName);

    m_FenceEvent = CreateEventW(nullptr, false, false, nullptr);

    if (!m_FenceEvent)
        return false;

    if (p_SwapChain->GetHwnd(&m_Hwnd) != S_OK)
        return false;

    if (!ImGui_ImplDX12_Init(
        s_Device, MaxRenderedFrames, DXGI_FORMAT_R8G8B8A8_UNORM, m_SrvDescriptorHeap,
        m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
    ))
        return false;

    if (!ImGui_ImplDX12_CreateDeviceObjects())
        return false;

    SetupStyles();

    ImGuiIO& s_ImGuiIO = ImGui::GetIO();

    RECT s_Rect = {0, 0, 0, 0};
    GetClientRect(m_Hwnd, &s_Rect);

    s_ImGuiIO.DisplaySize = ImVec2(
        static_cast<float>(s_Rect.right - s_Rect.left), static_cast<float>(s_Rect.bottom - s_Rect.top)
    );
    s_ImGuiIO.FontGlobalScale = (s_ImGuiIO.DisplaySize.y / 1800.f);
    ImGui::GetMainViewport()->PlatformHandleRaw = m_Hwnd;

    m_RendererSetup = true;

    Logger::Debug("ImGui renderer successfully set up.");

    return true;
}

void ImGuiRenderer::OnReset() {
    if (!m_RendererSetup)
        return;

    WaitForCurrentFrameToFinish();

    // Reset all fence values to latest fence value since we don't
    // really care about tracking any previous frames after a reset.
    // We only care about the last submitted frame having completed
    // (which means that all the previous ones have too).
    for (auto& s_Frame : m_FrameContext)
        s_Frame.FenceValue = m_FenceValue;

    m_BackBuffers.clear();

    ImGui_ImplDX12_InvalidateDeviceObjects();
}

void ImGuiRenderer::PostReset() {
    if (!m_RendererSetup)
        return;

    DXGI_SWAP_CHAIN_DESC1 s_SwapChainDesc;

    if (m_SwapChain->GetDesc1(&s_SwapChainDesc) != S_OK)
        return;

    ScopedD3DRef<ID3D12Device> s_Device;

    if (m_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK)
        return;

    // Reset the back buffers.
    m_BackBuffers.resize(s_SwapChainDesc.BufferCount);

    m_RtvDescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < m_BackBuffers.size(); ++i) {
        if (m_SwapChain->GetBuffer(i, IID_PPV_ARGS(m_BackBuffers[i].ReleaseAndGetPtr())) != S_OK)
            return;

        const CD3DX12_CPU_DESCRIPTOR_HANDLE s_RtvDescriptor(s_RtvHandle, i, m_RtvDescriptorSize);
        s_Device->CreateRenderTargetView(m_BackBuffers[i], nullptr, s_RtvDescriptor);
    }

    // Re-create the ImGui D3D12 device objects.
    ImGui_ImplDX12_CreateDeviceObjects();

    // Set scaling parameters based on new view height.
    ImGuiIO& s_ImGuiIO = ImGui::GetIO();

    RECT s_Rect = {0, 0, 0, 0};
    GetClientRect(m_Hwnd, &s_Rect);

    s_ImGuiIO.DisplaySize = ImVec2(
        static_cast<float>(s_Rect.right - s_Rect.left), static_cast<float>(s_Rect.bottom - s_Rect.top)
    );
    s_ImGuiIO.FontGlobalScale = (s_ImGuiIO.DisplaySize.y / 1800.f);
    ImGui::GetMainViewport()->PlatformHandleRaw = m_Hwnd;
}

void ImGuiRenderer::SetCommandQueue(ID3D12CommandQueue* p_CommandQueue) {
    if (m_CommandQueue == p_CommandQueue)
        return;

    if (m_CommandQueue) {
        m_CommandQueue->Release();
        m_CommandQueue = nullptr;
    }

    Logger::Debug("Setting up ImGui command queue.");
    m_CommandQueue = p_CommandQueue;
}

ImGuiMouseSource ImGuiRenderer::GetMouseSourceFromMessageExtraInfo() {
    const auto s_ExtraInfo = ::GetMessageExtraInfo();

    if ((s_ExtraInfo & 0xFFFFFF80) == 0xFF515700) {
        return ImGuiMouseSource_Pen;
    }

    if ((s_ExtraInfo & 0xFFFFFF80) == 0xFF515780) {
        return ImGuiMouseSource_TouchScreen;
    }

    return ImGuiMouseSource_Mouse;
}

bool ImGuiRenderer::IsVkDown(int p_Vk) {
    return (::GetKeyState(p_Vk) & 0x8000) != 0;
}

void ImGuiRenderer::UpdateKeyModifiers(ImGuiIO& p_ImGuiIO) {
    p_ImGuiIO.AddKeyEvent(ImGuiMod_Ctrl, IsVkDown(VK_CONTROL));
    p_ImGuiIO.AddKeyEvent(ImGuiMod_Shift, IsVkDown(VK_SHIFT));
    p_ImGuiIO.AddKeyEvent(ImGuiMod_Alt, IsVkDown(VK_MENU));
    p_ImGuiIO.AddKeyEvent(ImGuiMod_Super, IsVkDown(VK_LWIN) || IsVkDown(VK_RWIN));
}

void ImGuiRenderer::UpdateKeyboardCodePage() {
    // Retrieve keyboard code page, required for handling of non-Unicode Windows.
    const auto s_KeyboardLayout = ::GetKeyboardLayout(0);
    const auto s_KeyboardLcid = MAKELCID(HIWORD(s_KeyboardLayout), SORT_DEFAULT);

    if (::GetLocaleInfoA(
        s_KeyboardLcid, (LOCALE_RETURN_NUMBER | LOCALE_IDEFAULTANSICODEPAGE),
        reinterpret_cast<LPSTR>(&m_KeyboardCodePage),
        sizeof(m_KeyboardCodePage)
    ) == 0) {
        m_KeyboardCodePage = CP_ACP; // Fallback to default ANSI code page when fails.
    }
}

ImGuiKey ImGuiRenderer::KeyEventToImGuiKey(WPARAM p_Wparam, LPARAM p_Lparam) {
    // There is no distinct VK_xxx for keypad enter, instead it is VK_RETURN + KF_EXTENDED.
    if ((p_Wparam == VK_RETURN) && (HIWORD(p_Lparam) & KF_EXTENDED))
        return ImGuiKey_KeypadEnter;

    const int s_Scancode = LOBYTE(HIWORD(p_Lparam));

    switch (p_Wparam) {
        case VK_TAB: return ImGuiKey_Tab;
        case VK_LEFT: return ImGuiKey_LeftArrow;
        case VK_RIGHT: return ImGuiKey_RightArrow;
        case VK_UP: return ImGuiKey_UpArrow;
        case VK_DOWN: return ImGuiKey_DownArrow;
        case VK_PRIOR: return ImGuiKey_PageUp;
        case VK_NEXT: return ImGuiKey_PageDown;
        case VK_HOME: return ImGuiKey_Home;
        case VK_END: return ImGuiKey_End;
        case VK_INSERT: return ImGuiKey_Insert;
        case VK_DELETE: return ImGuiKey_Delete;
        case VK_BACK: return ImGuiKey_Backspace;
        case VK_SPACE: return ImGuiKey_Space;
        case VK_RETURN: return ImGuiKey_Enter;
        case VK_ESCAPE: return ImGuiKey_Escape;
        //case VK_OEM_7: return ImGuiKey_Apostrophe;
        case VK_OEM_COMMA: return ImGuiKey_Comma;
        //case VK_OEM_MINUS: return ImGuiKey_Minus;
        case VK_OEM_PERIOD: return ImGuiKey_Period;
        //case VK_OEM_2: return ImGuiKey_Slash;
        //case VK_OEM_1: return ImGuiKey_Semicolon;
        //case VK_OEM_PLUS: return ImGuiKey_Equal;
        //case VK_OEM_4: return ImGuiKey_LeftBracket;
        //case VK_OEM_5: return ImGuiKey_Backslash;
        //case VK_OEM_6: return ImGuiKey_RightBracket;
        //case VK_OEM_3: return ImGuiKey_GraveAccent;
        case VK_CAPITAL: return ImGuiKey_CapsLock;
        case VK_SCROLL: return ImGuiKey_ScrollLock;
        case VK_NUMLOCK: return ImGuiKey_NumLock;
        case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
        case VK_PAUSE: return ImGuiKey_Pause;
        case VK_NUMPAD0: return ImGuiKey_Keypad0;
        case VK_NUMPAD1: return ImGuiKey_Keypad1;
        case VK_NUMPAD2: return ImGuiKey_Keypad2;
        case VK_NUMPAD3: return ImGuiKey_Keypad3;
        case VK_NUMPAD4: return ImGuiKey_Keypad4;
        case VK_NUMPAD5: return ImGuiKey_Keypad5;
        case VK_NUMPAD6: return ImGuiKey_Keypad6;
        case VK_NUMPAD7: return ImGuiKey_Keypad7;
        case VK_NUMPAD8: return ImGuiKey_Keypad8;
        case VK_NUMPAD9: return ImGuiKey_Keypad9;
        case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
        case VK_DIVIDE: return ImGuiKey_KeypadDivide;
        case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case VK_ADD: return ImGuiKey_KeypadAdd;
        case VK_LSHIFT: return ImGuiKey_LeftShift;
        case VK_LCONTROL: return ImGuiKey_LeftCtrl;
        case VK_LMENU: return ImGuiKey_LeftAlt;
        case VK_LWIN: return ImGuiKey_LeftSuper;
        case VK_RSHIFT: return ImGuiKey_RightShift;
        case VK_RCONTROL: return ImGuiKey_RightCtrl;
        case VK_RMENU: return ImGuiKey_RightAlt;
        case VK_RWIN: return ImGuiKey_RightSuper;
        case VK_APPS: return ImGuiKey_Menu;
        case '0': return ImGuiKey_0;
        case '1': return ImGuiKey_1;
        case '2': return ImGuiKey_2;
        case '3': return ImGuiKey_3;
        case '4': return ImGuiKey_4;
        case '5': return ImGuiKey_5;
        case '6': return ImGuiKey_6;
        case '7': return ImGuiKey_7;
        case '8': return ImGuiKey_8;
        case '9': return ImGuiKey_9;
        case 'A': return ImGuiKey_A;
        case 'B': return ImGuiKey_B;
        case 'C': return ImGuiKey_C;
        case 'D': return ImGuiKey_D;
        case 'E': return ImGuiKey_E;
        case 'F': return ImGuiKey_F;
        case 'G': return ImGuiKey_G;
        case 'H': return ImGuiKey_H;
        case 'I': return ImGuiKey_I;
        case 'J': return ImGuiKey_J;
        case 'K': return ImGuiKey_K;
        case 'L': return ImGuiKey_L;
        case 'M': return ImGuiKey_M;
        case 'N': return ImGuiKey_N;
        case 'O': return ImGuiKey_O;
        case 'P': return ImGuiKey_P;
        case 'Q': return ImGuiKey_Q;
        case 'R': return ImGuiKey_R;
        case 'S': return ImGuiKey_S;
        case 'T': return ImGuiKey_T;
        case 'U': return ImGuiKey_U;
        case 'V': return ImGuiKey_V;
        case 'W': return ImGuiKey_W;
        case 'X': return ImGuiKey_X;
        case 'Y': return ImGuiKey_Y;
        case 'Z': return ImGuiKey_Z;
        case VK_F1: return ImGuiKey_F1;
        case VK_F2: return ImGuiKey_F2;
        case VK_F3: return ImGuiKey_F3;
        case VK_F4: return ImGuiKey_F4;
        case VK_F5: return ImGuiKey_F5;
        case VK_F6: return ImGuiKey_F6;
        case VK_F7: return ImGuiKey_F7;
        case VK_F8: return ImGuiKey_F8;
        case VK_F9: return ImGuiKey_F9;
        case VK_F10: return ImGuiKey_F10;
        case VK_F11: return ImGuiKey_F11;
        case VK_F12: return ImGuiKey_F12;
        case VK_F13: return ImGuiKey_F13;
        case VK_F14: return ImGuiKey_F14;
        case VK_F15: return ImGuiKey_F15;
        case VK_F16: return ImGuiKey_F16;
        case VK_F17: return ImGuiKey_F17;
        case VK_F18: return ImGuiKey_F18;
        case VK_F19: return ImGuiKey_F19;
        case VK_F20: return ImGuiKey_F20;
        case VK_F21: return ImGuiKey_F21;
        case VK_F22: return ImGuiKey_F22;
        case VK_F23: return ImGuiKey_F23;
        case VK_F24: return ImGuiKey_F24;
        case VK_BROWSER_BACK: return ImGuiKey_AppBack;
        case VK_BROWSER_FORWARD: return ImGuiKey_AppForward;
        default: break;
    }

    // Fallback to scancode
    // https://handmade.network/forums/t/2011-keyboard_inputs_-_scancodes,_raw_input,_text_input,_key_names
    switch (s_Scancode) {
        case 41: return ImGuiKey_GraveAccent;
        // VK_OEM_8 in EN-UK, VK_OEM_3 in EN-US, VK_OEM_7 in FR, VK_OEM_5 in DE, etc.
        case 12: return ImGuiKey_Minus;
        case 13: return ImGuiKey_Equal;
        case 26: return ImGuiKey_LeftBracket;
        case 27: return ImGuiKey_RightBracket;
        case 86: return ImGuiKey_Oem102;
        case 43: return ImGuiKey_Backslash;
        case 39: return ImGuiKey_Semicolon;
        case 40: return ImGuiKey_Apostrophe;
        case 51: return ImGuiKey_Comma;
        case 52: return ImGuiKey_Period;
        case 53: return ImGuiKey_Slash;
        default: break;
    }

    return ImGuiKey_None;
}

void ImGuiRenderer::AddKeyEvent(
    ImGuiIO& p_ImGuiIO, ImGuiKey p_Key, bool p_Down, int p_NativeKeycode, int p_NativeScancode
) {
    p_ImGuiIO.AddKeyEvent(p_Key, p_Down);
    p_ImGuiIO.SetKeyEventNativeData(p_Key, p_NativeKeycode, p_NativeScancode);
}

void ImGuiRenderer::UpdateMouseData(ImGuiIO& p_ImGuiIO) {
    const auto s_FocusedWindow = ::GetForegroundWindow();
    const bool s_IsAppFocused = (s_FocusedWindow == m_Hwnd);

    if (s_IsAppFocused) {
        // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when io.ConfigNavMoveSetMousePos is enabled by user)
        if (p_ImGuiIO.WantSetMousePos) {
            POINT s_Pos = {static_cast<int>(p_ImGuiIO.MousePos.x), static_cast<int>(p_ImGuiIO.MousePos.y)};
            if (::ClientToScreen(m_Hwnd, &s_Pos)) {
                ::SetCursorPos(s_Pos.x, s_Pos.y);
            }
        }

        // (Optional) Fallback to provide mouse position when focused (WM_MOUSEMOVE already provides this when hovered or captured)
        // This also fills a short gap when clicking non-client area: WM_NCMOUSELEAVE -> modal OS move -> gap -> WM_NCMOUSEMOVE
        if (!p_ImGuiIO.WantSetMousePos && m_MouseTrackedArea == 0) {
            POINT s_Pos;
            if (::GetCursorPos(&s_Pos) && ::ScreenToClient(m_Hwnd, &s_Pos)) {
                p_ImGuiIO.AddMousePosEvent(static_cast<float>(s_Pos.x), static_cast<float>(s_Pos.y));
            }
        }
    }
}

void ImGuiRenderer::ProcessKeyEventsWorkarounds(ImGuiIO& io) {
    // Left & right Shift keys: when both are pressed together, Windows tend to not generate the WM_KEYUP event for the first released one.
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) && !IsVkDown(VK_LSHIFT)) {
        AddKeyEvent(io, ImGuiKey_LeftShift, false, VK_LSHIFT);
    }

    if (ImGui::IsKeyDown(ImGuiKey_RightShift) && !IsVkDown(VK_RSHIFT)) {
        AddKeyEvent(io, ImGuiKey_RightShift, false, VK_RSHIFT);
    }

    // Sometimes WM_KEYUP for Win key is not passed down to the app (e.g. for Win+V on some setups, according to GLFW).
    if (ImGui::IsKeyDown(ImGuiKey_LeftSuper) && !IsVkDown(VK_LWIN)) {
        AddKeyEvent(io, ImGuiKey_LeftSuper, false, VK_LWIN);
    }

    if (ImGui::IsKeyDown(ImGuiKey_RightSuper) && !IsVkDown(VK_RWIN)) {
        AddKeyEvent(io, ImGuiKey_RightSuper, false, VK_RWIN);
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

    HRESULT s_Result = p_Loader(s_Device, s_UploadBatch, &s_Texture.Ref);

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

    UINT s_SRVIndex = m_NextSRVIndex++;
    UINT s_DescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_CPU_DESCRIPTOR_HANDLE s_CPUHandle = m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    s_CPUHandle.ptr += s_SRVIndex * s_DescriptorSize;

    D3D12_GPU_DESCRIPTOR_HANDLE s_GPUHandle = m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    s_GPUHandle.ptr += s_SRVIndex * s_DescriptorSize;

    DirectX::CreateShaderResourceView(s_Device, p_OutTexture, s_CPUHandle);

    p_OutImGuiTexture.id = s_GPUHandle.ptr;

    Logger::Info("Created texture ({}x{}) in SRV slot {}.", s_TextureDescription.Width, s_TextureDescription.Height, m_NextSRVIndex);

    return true;
}

DEFINE_DETOUR_WITH_CONTEXT(
    ImGuiRenderer, LRESULT, WndProc, ZApplicationEngineWin32* th, HWND p_Hwnd, UINT p_Message, WPARAM p_Wparam,
    LPARAM p_Lparam
) {
    if (ImGui::GetCurrentContext() == nullptr)
        return HookResult<LRESULT>(HookAction::Continue());

    auto s_ScanCode = static_cast<uint8_t>(p_Lparam >> 16);

    // Toggle imgui input when user presses the console key.
    if (s_ScanCode == ModSDK::GetInstance()->GetConsoleScanCode() && (p_Message == WM_KEYDOWN || p_Message ==
        WM_SYSKEYDOWN)) {
        m_ImguiHasFocus = !m_ImguiHasFocus;

        if (m_ImguiHasFocus) {
            // Set the GUI to visible again if we toggle it on.
            m_ImguiVisible = true;
        }

        if (!m_ImguiHasFocus) {
            DWORD s_EventCount = 256;
            DIDEVICEOBJECTDATA s_Buffer[256];
            ZKeyboardWindows* s_KeyboardWindows = static_cast<ZKeyboardWindows*>(
                Globals::InputDeviceManager->m_devices[4]
            );

            if (s_KeyboardWindows->dif.m_pDev) {
                // Prevents buffered events from being processed by the game after imgui is closed
                s_KeyboardWindows->dif.m_pDev->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), s_Buffer, &s_EventCount, 0);
            }
        }
    }

    if (s_ScanCode == ModSDK::GetInstance()->GetUiToggleScanCode() && (p_Message == WM_KEYDOWN || p_Message ==
        WM_SYSKEYDOWN)) {
        if (!ModSDK::GetInstance()->HasShownUiToggleWarning()) {
            m_ShowingUiToggleWarning = true;
            m_ImguiHasFocus = true;
        }
        else {
            m_ImguiVisible = !m_ImguiVisible;

            if (!m_ImguiVisible) {
                m_ImguiHasFocus = false;
            }
        }
    }

    //Globals::InputActionManager->m_bDebugKeys = true;
    Globals::InputActionManager->m_bEnabled = !m_ImguiHasFocus;

    if (!m_ImguiHasFocus)
        return HookResult<LRESULT>(HookAction::Continue());

    // If we got a quit / close message then return control back to the process.
    if (p_Message == WM_QUIT || p_Message == WM_DESTROY || p_Message == WM_NCDESTROY || p_Message == WM_CLOSE) {
        m_ImguiHasFocus = false;
        return HookResult<LRESULT>(HookAction::Continue());
    }

    // Pass resizing messages down to the process.
    if (p_Message == WM_SIZE)
        return HookResult<LRESULT>(HookAction::Continue());

    ImGuiIO& s_ImGuiIO = ImGui::GetIO();

    switch (p_Message) {
        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE: {
            // We need to call TrackMouseEvent in order to receive WM_MOUSELEAVE events
            const auto s_MouseSource = GetMouseSourceFromMessageExtraInfo();
            const int s_Area = (p_Message == WM_MOUSEMOVE) ? 1 : 2;
            m_MouseHwnd = p_Hwnd;

            if (m_MouseTrackedArea != s_Area) {
                TRACKMOUSEEVENT s_TmeCancel = {sizeof(s_TmeCancel), TME_CANCEL, p_Hwnd, 0};
                TRACKMOUSEEVENT s_TmeTrack = {
                    sizeof(s_TmeTrack), static_cast<DWORD>(s_Area == 2 ? (TME_LEAVE | TME_NONCLIENT) : TME_LEAVE),
                    p_Hwnd, 0
                };

                if (m_MouseTrackedArea != 0) {
                    ::TrackMouseEvent(&s_TmeCancel);
                }

                ::TrackMouseEvent(&s_TmeTrack);
                m_MouseTrackedArea = s_Area;
            }

            POINT s_MousePos = {static_cast<LONG>(GET_X_LPARAM(p_Lparam)), static_cast<LONG>(GET_Y_LPARAM(p_Lparam))};

            if (p_Message == WM_NCMOUSEMOVE && ::ScreenToClient(p_Hwnd, &s_MousePos) == FALSE) {
                // WM_NCMOUSEMOVE are provided in absolute coordinates.
                break;
            }

            s_ImGuiIO.AddMouseSourceEvent(s_MouseSource);
            s_ImGuiIO.AddMousePosEvent(static_cast<float>(s_MousePos.x), static_cast<float>(s_MousePos.y));

            break;
        }

        case WM_MOUSELEAVE:
        case WM_NCMOUSELEAVE: {
            if (const int s_Area = (p_Message == WM_MOUSELEAVE) ? 1 : 2; m_MouseTrackedArea == s_Area) {
                if (m_MouseHwnd == p_Hwnd) {
                    m_MouseHwnd = nullptr;
                }

                m_MouseTrackedArea = 0;
                s_ImGuiIO.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
            }

            break;
        }

        case WM_DESTROY:
            if (m_MouseHwnd == p_Hwnd && m_MouseTrackedArea != 0) {
                TRACKMOUSEEVENT s_TmeCancel = {sizeof(s_TmeCancel), TME_CANCEL, p_Hwnd, 0};
                ::TrackMouseEvent(&s_TmeCancel);
                m_MouseHwnd = nullptr;
                m_MouseTrackedArea = 0;
                s_ImGuiIO.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
            }

            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK: {
            const auto s_MouseSource = GetMouseSourceFromMessageExtraInfo();
            int s_Button = 0;

            if (p_Message == WM_LBUTTONDOWN || p_Message == WM_LBUTTONDBLCLK) { s_Button = 0; }
            if (p_Message == WM_RBUTTONDOWN || p_Message == WM_RBUTTONDBLCLK) { s_Button = 1; }
            if (p_Message == WM_MBUTTONDOWN || p_Message == WM_MBUTTONDBLCLK) { s_Button = 2; }
            if (p_Message == WM_XBUTTONDOWN || p_Message == WM_XBUTTONDBLCLK) {
                s_Button = GET_XBUTTON_WPARAM(p_Wparam) == XBUTTON1 ? 3 : 4;
            }

            HWND s_HwndWithCapture = ::GetCapture();

            if (m_MouseButtonsDown != 0 && s_HwndWithCapture != p_Hwnd) {
                // Did we externally lose capture?
                m_MouseButtonsDown = 0;
            }

            if (m_MouseButtonsDown == 0 && s_HwndWithCapture == nullptr) {
                ::SetCapture(p_Hwnd);
            }

            // Allow us to read mouse coordinates when dragging mouse outside our window bounds.
            m_MouseButtonsDown |= 1 << s_Button;
            s_ImGuiIO.AddMouseSourceEvent(s_MouseSource);
            s_ImGuiIO.AddMouseButtonEvent(s_Button, true);

            break;
        }

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP: {
            const auto s_MouseSource = GetMouseSourceFromMessageExtraInfo();
            int s_Button = 0;

            if (p_Message == WM_LBUTTONUP) { s_Button = 0; }
            if (p_Message == WM_RBUTTONUP) { s_Button = 1; }
            if (p_Message == WM_MBUTTONUP) { s_Button = 2; }
            if (p_Message == WM_XBUTTONUP) { s_Button = (GET_XBUTTON_WPARAM(p_Wparam) == XBUTTON1) ? 3 : 4; }

            m_MouseButtonsDown &= ~(1 << s_Button);

            if (m_MouseButtonsDown == 0 && ::GetCapture() == p_Hwnd) {
                ::ReleaseCapture();
            }

            s_ImGuiIO.AddMouseSourceEvent(s_MouseSource);
            s_ImGuiIO.AddMouseButtonEvent(s_Button, false);

            break;
        }

        case WM_MOUSEWHEEL:
            s_ImGuiIO.AddMouseWheelEvent(
                0.0f, static_cast<float>(GET_WHEEL_DELTA_WPARAM(p_Wparam)) / static_cast<float>(WHEEL_DELTA)
            );
            break;

        case WM_MOUSEHWHEEL:
            s_ImGuiIO.AddMouseWheelEvent(
                -static_cast<float>(GET_WHEEL_DELTA_WPARAM(p_Wparam)) / static_cast<float>(WHEEL_DELTA), 0.0f
            );
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP: {
            const bool s_IsKeyDown = (p_Message == WM_KEYDOWN || p_Message == WM_SYSKEYDOWN);

            if (p_Wparam < 256) {
                // Submit modifiers
                UpdateKeyModifiers(s_ImGuiIO);

                // Obtain virtual key code and convert to ImGuiKey
                const ImGuiKey s_Key = KeyEventToImGuiKey(p_Wparam, p_Lparam);
                const int s_Vk = static_cast<int>(p_Wparam);
                const int s_Scancode = LOBYTE(HIWORD(p_Lparam));

                // Special behavior for VK_SNAPSHOT / ImGuiKey_PrintScreen as Windows doesn't emit the key down event.
                if (s_Key == ImGuiKey_PrintScreen && !s_IsKeyDown) {
                    AddKeyEvent(s_ImGuiIO, s_Key, true, s_Vk, s_Scancode);
                }

                // Submit key event
                if (s_Key != ImGuiKey_None) {
                    AddKeyEvent(s_ImGuiIO, s_Key, s_IsKeyDown, s_Vk, s_Scancode);
                }

                // Submit individual left/right modifier events
                if (s_Vk == VK_SHIFT) {
                    // Important: Shift keys tend to get stuck when pressed together, missing key-up events are corrected in ImGui_ImplWin32_ProcessKeyEventsWorkarounds()
                    if (IsVkDown(VK_LSHIFT) == s_IsKeyDown) {
                        AddKeyEvent(s_ImGuiIO, ImGuiKey_LeftShift, s_IsKeyDown, VK_LSHIFT, s_Scancode);
                    }
                    if (IsVkDown(VK_RSHIFT) == s_IsKeyDown) {
                        AddKeyEvent(s_ImGuiIO, ImGuiKey_RightShift, s_IsKeyDown, VK_RSHIFT, s_Scancode);
                    }
                }
                else if (s_Vk == VK_CONTROL) {
                    if (IsVkDown(VK_LCONTROL) == s_IsKeyDown) {
                        AddKeyEvent(s_ImGuiIO, ImGuiKey_LeftCtrl, s_IsKeyDown, VK_LCONTROL, s_Scancode);
                    }
                    if (IsVkDown(VK_RCONTROL) == s_IsKeyDown) {
                        AddKeyEvent(s_ImGuiIO, ImGuiKey_RightCtrl, s_IsKeyDown, VK_RCONTROL, s_Scancode);
                    }
                }
                else if (s_Vk == VK_MENU) {
                    if (IsVkDown(VK_LMENU) == s_IsKeyDown) {
                        AddKeyEvent(s_ImGuiIO, ImGuiKey_LeftAlt, s_IsKeyDown, VK_LMENU, s_Scancode);
                    }
                    if (IsVkDown(VK_RMENU) == s_IsKeyDown) {
                        AddKeyEvent(s_ImGuiIO, ImGuiKey_RightAlt, s_IsKeyDown, VK_RMENU, s_Scancode);
                    }
                }
            }
            break;
        }

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            s_ImGuiIO.AddFocusEvent(p_Message == WM_SETFOCUS);
            break;

        case WM_INPUTLANGCHANGE:
            UpdateKeyboardCodePage();
            break;

        case WM_CHAR:
            if (::IsWindowUnicode(p_Hwnd)) {
                // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
                if (p_Wparam > 0 && p_Wparam < 0x10000) {
                    s_ImGuiIO.AddInputCharacterUTF16(static_cast<unsigned short>(p_Wparam));
                }
            }
            else {
                wchar_t s_Wch = 0;
                ::MultiByteToWideChar(
                    m_KeyboardCodePage, MB_PRECOMPOSED, reinterpret_cast<char*>(&p_Wparam), 1, &s_Wch, 1
                );

                s_ImGuiIO.AddInputCharacter(s_Wch);
            }

            break;
    }

    // Don't call the original function so input isn't passed down to the game.
    return {HookAction::Return(), DefWindowProcW(p_Hwnd, p_Message, p_Wparam, p_Lparam)};
}

DEFINE_DETOUR_WITH_CONTEXT(ImGuiRenderer, void, ZKeyboardWindows_Update, ZKeyboardWindows*, bool) {
    // Don't process input while the imgui overlay has focus.
    if (m_ImguiHasFocus)
        return HookResult<void>(HookAction::Return());

    return HookResult<void>(HookAction::Continue());
}

DEFINE_DETOUR_WITH_CONTEXT(ImGuiRenderer, double, ZInputAction_Analog, ZInputAction* th, int a2) {
    static std::unordered_set<std::string> s_BlockedInputs = {
        "eIAKBMLookHorizontal",
        "eIAKBMLookVertical",
        "TiltCamera",
        "TurnCamera",
        "AnalogLeftX",
        "AnalogLeftY",
        "AnalogRightY",
        "AnalogRightX",
        "eIAStickRightHorizontal_Analog",
        "eIAStickRightVertical_Analog",
        "eIAStickLeftHorizontal_Analog",
        "eIAStickLeftVertical_Analog",
        "eIAStickLeftHorizontal_Raw",
        "eIAStickLeftVertical_Raw",
        "eIAStickRightHorizontal_Raw",
        "eIAStickRightVertical_Raw",
    };

    //// Don't allow moving the camera / character while the imgui overlay has focus.
    //if (m_ImguiHasFocus)
    //{
    //    if (s_BlockedInputs.contains(th->m_szName))
    //        return HookResult(HookAction::Return(), 0.0);
    //}

    return HookResult<double>(HookAction::Continue());
}