// Thanks to SpecialK source code and its creator (https://discourse.differentk.fyi/)
// for providing some helpful pointers on how to properly sync rendering.

#include "ImguiRenderer.h"

#include <d3dcompiler.h>
#include <dxgi1_4.h>

#include <imgui.h>
#include <backends/imgui_impl_dx12.h>

#include "D3D12Hooks.h"
#include "D3DUtils.h"
#include "Hooks.h"
#include "Logging.h"
#include "HookImpl.h"

#include <Glacier/ZApplicationEngineWin32.h>

#include <UI/Console.h>

using namespace Rendering;

ImguiRenderer* ImguiRenderer::m_Instance = nullptr;

ImguiRenderer* ImguiRenderer::GetInstance()
{
    if (m_Instance == nullptr)
        m_Instance = new ImguiRenderer();

    return m_Instance;
}

void ImguiRenderer::Init()
{
    D3D12Hooks::InstallHooks();
	
    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&m_TicksPerSecond));
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_Time));

	// If we're running in debug mode we should enable the D3D debug layer.
#if _DEBUG
    Hooks::D3D12CreateDevice->AddDetour(this, [](void*, Hook<HRESULT(IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice)>* p_Hook, IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice)
    {
        Logger::Debug("Creating D3D device. Enabling debug layer.");

        ID3D12Debug* s_Debug = nullptr;
    	
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&s_Debug))))
        {
            s_Debug->EnableDebugLayer();
        }
    	
        const auto s_Result = p_Hook->CallOriginal(pAdapter, MinimumFeatureLevel, riid, ppDevice);

    	if (s_Debug)
    	{
            ID3D12InfoQueue* s_InfoQueue = nullptr;
            (*reinterpret_cast<ID3D12Device**>(ppDevice))->QueryInterface(IID_PPV_ARGS(&s_InfoQueue));
    		
            s_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
            s_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            s_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
            s_InfoQueue->Release();

            s_Debug->Release();
    	}

        return HookResult<HRESULT>(HookAction::Return(), s_Result);
    });
#endif

    Hooks::ZApplicationEngineWin32_MainWindowProc->AddDetour(this, &ImguiRenderer::WndProc);
}

void ImguiRenderer::Shutdown()
{
    D3D12Hooks::RemoveHooks();
	
    ResetRenderer();
}

void ImguiRenderer::Draw()
{
    ImGui_ImplDX12_NewFrame();

    ImGuiIO& s_ImGuiIO = ImGui::GetIO();

    int64_t s_CurrentTime = 0;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&s_CurrentTime));
	
    s_ImGuiIO.DeltaTime = static_cast<float>(s_CurrentTime - m_Time) / m_TicksPerSecond;
    m_Time = s_CurrentTime;

    s_ImGuiIO.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
    s_ImGuiIO.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
    s_ImGuiIO.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
    s_ImGuiIO.KeySuper = false;

    // Set mouse position
    if (m_ImguiHasFocus)
    {
        s_ImGuiIO.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

        if (auto* s_ForegroundWnd = GetForegroundWindow())
        {
            if (s_ForegroundWnd == m_Hwnd || IsChild(s_ForegroundWnd, m_Hwnd))
            {
                POINT s_CursorPos;

                if (GetCursorPos(&s_CursorPos) && ScreenToClient(m_Hwnd, &s_CursorPos))
                    s_ImGuiIO.MousePos = ImVec2(static_cast<float>(s_CursorPos.x), static_cast<float>(s_CursorPos.y));
            }
        }
    }

	// Construct the UI.
    ImGui::NewFrame();

    if (m_ImguiHasFocus)
    {
        UI::Console::Draw();
    }
}

void ImguiRenderer::OnPresent(IDXGISwapChain3* p_SwapChain)
{
    if (!SetupRenderer(p_SwapChain))
    {
        Logger::Error("Failed to set up ImGui renderer.");
        ResetRenderer();
        return;
    }

    if (m_SwapChain != p_SwapChain)
        return;
	
    if (!m_CommandQueue)
        return;

    const auto s_BackBufferIndex = p_SwapChain->GetCurrentBackBufferIndex();
    auto& s_Frame = m_FrameContext[s_BackBufferIndex];

    if (s_Frame.Fence->GetCompletedValue() < s_Frame.FenceValue)
    {
        if (SUCCEEDED(s_Frame.Fence->SetEventOnCompletion(s_Frame.FenceValue, s_Frame.FenceEvent)))
        {
            WaitForSingleObject(s_Frame.FenceEvent, INFINITE);
        }
    }

	if (!s_Frame.Recording)
	{
        s_Frame.CommandAllocator->Reset();

        s_Frame.Recording = SUCCEEDED(s_Frame.CommandList->Reset(s_Frame.CommandAllocator, nullptr));

        if (s_Frame.Recording)
            s_Frame.CommandList->SetPredication(nullptr, 0, D3D12_PREDICATION_OP_EQUAL_ZERO);

        if (!s_Frame.Recording)
        {
            Logger::Warn("Could not reset command list of frame {}.", s_BackBufferIndex);
            return;
        }
	}

    {
        D3D12_RESOURCE_BARRIER s_Barrier = {};
        s_Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        s_Barrier.Transition.pResource = s_Frame.BackBuffer;
        s_Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        s_Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        s_Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        s_Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

        s_Frame.CommandList->ResourceBarrier(1, &s_Barrier);
    }

	// Render
    s_Frame.CommandList->OMSetRenderTargets(1, &s_Frame.DescriptorHandle, false, nullptr);
    s_Frame.CommandList->SetDescriptorHeaps(1, &m_SrvDescriptorHeap);

    Draw();
	
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), s_Frame.CommandList);

    {
        D3D12_RESOURCE_BARRIER s_Barrier = {};
        s_Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        s_Barrier.Transition.pResource = s_Frame.BackBuffer;
        s_Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        s_Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        s_Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        s_Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

        s_Frame.CommandList->ResourceBarrier(1, &s_Barrier);
    }

    ExecuteCmdList(&s_Frame);

    const auto s_SyncValue = s_Frame.FenceValue + 1;

	if (SUCCEEDED(m_CommandQueue->Signal(s_Frame.Fence, s_SyncValue)))
        s_Frame.FenceValue = s_SyncValue;
}

bool ImguiRenderer::SetupRenderer(IDXGISwapChain3* p_SwapChain)
{
    if (m_RendererSetup)
        return true;

    Logger::Debug("Setting up ImGui renderer.");

    m_SwapChain = p_SwapChain;

    ScopedD3DRef<ID3D12Device> s_Device;

    if (p_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK)
        return false;

    DXGI_SWAP_CHAIN_DESC1 s_SwapChainDesc;

    if (p_SwapChain->GetDesc1(&s_SwapChainDesc) != S_OK)
        return false;

    m_BufferCount = s_SwapChainDesc.BufferCount;
    m_FrameContext = new FrameContext[m_BufferCount];

    {
        D3D12_DESCRIPTOR_HEAP_DESC s_Desc = {};
        s_Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        s_Desc.NumDescriptors = m_BufferCount;
        s_Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        s_Desc.NodeMask = 0;

        if (s_Device->CreateDescriptorHeap(&s_Desc, IID_PPV_ARGS(&m_RtvDescriptorHeap)) != S_OK)
            return false;

        D3D_SET_OBJECT_NAME_A(m_RtvDescriptorHeap, "ZHMModSDK ImGui Descriptor Heap");
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC s_Desc = {};
        s_Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        s_Desc.NumDescriptors = m_BufferCount; // TODO: This can probably be 1
        s_Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        s_Desc.NodeMask = 0;

        if (s_Device->CreateDescriptorHeap(&s_Desc, IID_PPV_ARGS(&m_SrvDescriptorHeap)) != S_OK)
            return false;

        D3D_SET_OBJECT_NAME_A(m_SrvDescriptorHeap, "ZHMModSDK Backbuffer Descriptor Heap");
    }

    const auto s_RtvDescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for (size_t i = 0; i < m_BufferCount; ++i)
    {
        auto& s_Frame = m_FrameContext[i];

        s_Frame.Index = i;
    	
        if (s_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&s_Frame.Fence)) != S_OK)
            return false;

        char s_FenceDebugName[128];
        sprintf_s(s_FenceDebugName, sizeof(s_FenceDebugName), "ZHMModSDK ImGui Fence #%llu", i);
        D3D_SET_OBJECT_NAME_A(s_Frame.Fence, s_FenceDebugName);

        if (s_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s_Frame.CommandAllocator)) != S_OK)
            return false;

        char s_CmdAllocDebugName[128];
        sprintf_s(s_CmdAllocDebugName, sizeof(s_CmdAllocDebugName), "ZHMModSDK ImGui Command Allocator #%llu", i);
        D3D_SET_OBJECT_NAME_A(s_Frame.CommandAllocator, s_CmdAllocDebugName);

        if (s_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, s_Frame.CommandAllocator, nullptr, IID_PPV_ARGS(&s_Frame.CommandList)) != S_OK ||
            s_Frame.CommandList->Close() != S_OK)
            return false;

        char s_CmdListDebugName[128];
        sprintf_s(s_CmdListDebugName, sizeof(s_CmdListDebugName), "ZHMModSDK ImGui Command List #%llu", i);
        D3D_SET_OBJECT_NAME_A(s_Frame.CommandList, s_CmdListDebugName);
    	
        if (p_SwapChain->GetBuffer(i, IID_PPV_ARGS(&s_Frame.BackBuffer)) != S_OK)
            return false;

        s_Frame.DescriptorHandle.ptr = s_RtvHandle.ptr + (i * s_RtvDescriptorSize);

        s_Device->CreateRenderTargetView(s_Frame.BackBuffer, nullptr, s_Frame.DescriptorHandle);

        s_Frame.FenceValue = 0;
        s_Frame.FenceEvent = CreateEventA(nullptr, false, false, nullptr);

        s_Frame.Recording = false;
    }
	
    if (p_SwapChain->GetHwnd(&m_Hwnd) != S_OK)
        return false;

    if (!m_ImguiInitialized)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
    	
        ImGuiIO& s_ImGuiIO = ImGui::GetIO();
        ImGui::StyleColorsDark();

        s_ImGuiIO.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
        s_ImGuiIO.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
        s_ImGuiIO.BackendPlatformName = "imgui_impl_win32";
        s_ImGuiIO.ImeWindowHandle = m_Hwnd;

        // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
        s_ImGuiIO.KeyMap[ImGuiKey_Tab] = VK_TAB;
        s_ImGuiIO.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
        s_ImGuiIO.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
        s_ImGuiIO.KeyMap[ImGuiKey_UpArrow] = VK_UP;
        s_ImGuiIO.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
        s_ImGuiIO.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
        s_ImGuiIO.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
        s_ImGuiIO.KeyMap[ImGuiKey_Home] = VK_HOME;
        s_ImGuiIO.KeyMap[ImGuiKey_End] = VK_END;
        s_ImGuiIO.KeyMap[ImGuiKey_Insert] = VK_INSERT;
        s_ImGuiIO.KeyMap[ImGuiKey_Delete] = VK_DELETE;
        s_ImGuiIO.KeyMap[ImGuiKey_Backspace] = VK_BACK;
        s_ImGuiIO.KeyMap[ImGuiKey_Space] = VK_SPACE;
        s_ImGuiIO.KeyMap[ImGuiKey_Enter] = VK_RETURN;
        s_ImGuiIO.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
        s_ImGuiIO.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
        s_ImGuiIO.KeyMap[ImGuiKey_A] = 'A';
        s_ImGuiIO.KeyMap[ImGuiKey_C] = 'C';
        s_ImGuiIO.KeyMap[ImGuiKey_V] = 'V';
        s_ImGuiIO.KeyMap[ImGuiKey_X] = 'X';
        s_ImGuiIO.KeyMap[ImGuiKey_Y] = 'Y';
        s_ImGuiIO.KeyMap[ImGuiKey_Z] = 'Z';

        s_ImGuiIO.BackendRendererName = "imgui_impl_dx12";
        s_ImGuiIO.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

        m_ImguiInitialized = true;
    }

    ImGuiIO& s_ImGuiIO = ImGui::GetIO();
	
    RECT s_Rect = { 0, 0, 0, 0 };
    GetClientRect(m_Hwnd, &s_Rect);
    s_ImGuiIO.DisplaySize = ImVec2(static_cast<float>(s_Rect.right - s_Rect.left), static_cast<float>(s_Rect.bottom - s_Rect.top));

    if (!ImGui_ImplDX12_Init(s_Device, m_BufferCount, DXGI_FORMAT_R8G8B8A8_UNORM, m_SrvDescriptorHeap, m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()))
        return false;

    if (!ImGui_ImplDX12_CreateDeviceObjects())
        return false;
	
    m_RendererSetup = true;

    Logger::Debug("ImGui renderer successfully set up.");

    return true;
}

void ImguiRenderer::ResetRenderer()
{
    Logger::Debug("Resetting renderer.");
	
    m_RendererSetup = false;

    if (m_RendererSetup)
    {
        ImGui_ImplDX12_InvalidateDeviceObjects();
        ImGui_ImplDX12_Shutdown();
    }

    for (UINT i = 0; i < m_BufferCount; i++)
    {
        auto& s_Frame = m_FrameContext[i];

        if (m_SwapChain && m_SwapChain->GetCurrentBackBufferIndex() != s_Frame.Index)
            s_Frame.Recording = false;
    	
        WaitForGpu(&s_Frame);

        if (s_Frame.CommandList)
        {
            s_Frame.CommandList->Release();
            s_Frame.CommandList = nullptr;
        }

        if (s_Frame.CommandAllocator)
        {
            s_Frame.CommandAllocator->Release();
            s_Frame.CommandAllocator = nullptr;
        }
    	
        if (s_Frame.BackBuffer)
        {
            s_Frame.BackBuffer->Release();
            s_Frame.BackBuffer = nullptr;
        }

    	if (s_Frame.Fence)
    	{
            s_Frame.Fence->Release();
            s_Frame.Fence = nullptr;
    	}

    	if (s_Frame.FenceEvent)
    	{
            CloseHandle(s_Frame.FenceEvent);
            s_Frame.FenceEvent = nullptr;
    	}

        s_Frame.FenceValue = 0;
        s_Frame.Recording = false;
        s_Frame.DescriptorHandle = {0};
    }
	
    delete[] m_FrameContext;
    m_FrameContext = nullptr;
    
    m_CommandQueue = nullptr;

    if (m_RtvDescriptorHeap)
    {
        m_RtvDescriptorHeap->Release();
        m_RtvDescriptorHeap = nullptr;
    }

    if (m_SrvDescriptorHeap)
    {
        m_SrvDescriptorHeap->Release();
        m_SrvDescriptorHeap = nullptr;
    }
    
    m_SwapChain = nullptr;
    m_BufferCount = 0;
    m_Hwnd = nullptr;
}

void ImguiRenderer::SetCommandQueue(ID3D12CommandQueue* p_CommandQueue)
{
    if (!m_RendererSetup || m_CommandQueue || !m_SwapChain)
        return;

    m_CommandQueue = p_CommandQueue;
}

void ImguiRenderer::WaitForGpu(FrameContext* p_Frame)
{
    if (p_Frame->Recording)
        ExecuteCmdList(p_Frame);

    if (!p_Frame->FenceEvent || !m_CommandQueue)
        return;

    const auto s_SyncValue = p_Frame->FenceValue + 1;

    if (FAILED(m_CommandQueue->Signal(p_Frame->Fence, s_SyncValue)))
        return;

    if (SUCCEEDED(p_Frame->Fence->SetEventOnCompletion(s_SyncValue, p_Frame->FenceEvent)))
        WaitForSingleObject(p_Frame->FenceEvent, INFINITE);

    p_Frame->FenceValue = s_SyncValue;
}

void ImguiRenderer::ExecuteCmdList(FrameContext* p_Frame)
{
    if (FAILED(p_Frame->CommandList->Close()))
        return;

    p_Frame->Recording = false;
	
    ID3D12CommandList* const s_CommandLists[] = {
        p_Frame->CommandList,
    };

    m_CommandQueue->ExecuteCommandLists(1, s_CommandLists);
}

DECLARE_DETOUR_WITH_CONTEXT(ImguiRenderer, LRESULT, WndProc, ZApplicationEngineWin32* th, HWND p_Hwnd, UINT p_Message, WPARAM p_Wparam, LPARAM p_Lparam)
{
    if (ImGui::GetCurrentContext() == nullptr)
        return HookResult<LRESULT>(HookAction::Continue());

    auto s_ScanCode = static_cast<uint8_t>(p_Lparam >> 16);

    // Toggle imgui input when user presses the grave / tilde key.
    if (s_ScanCode == 0x29 && (p_Message == WM_KEYDOWN || p_Message == WM_SYSKEYDOWN))
        m_ImguiHasFocus = !m_ImguiHasFocus;

	if (!m_ImguiHasFocus)
        return HookResult<LRESULT>(HookAction::Continue());

    ImGuiIO& s_ImGuiIO = ImGui::GetIO();

    switch (p_Message)
    {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
    {
        int s_Button = 0;
    		
        if (p_Message == WM_LBUTTONDOWN || p_Message == WM_LBUTTONDBLCLK)
            s_Button = 0;
        if (p_Message == WM_RBUTTONDOWN || p_Message == WM_RBUTTONDBLCLK)
            s_Button = 1;
        if (p_Message == WM_MBUTTONDOWN || p_Message == WM_MBUTTONDBLCLK)
            s_Button = 2;    		
        if (p_Message == WM_XBUTTONDOWN || p_Message == WM_XBUTTONDBLCLK) 
            s_Button = (GET_XBUTTON_WPARAM(p_Wparam) == XBUTTON1) ? 3 : 4;

        if (!ImGui::IsAnyMouseDown() && GetCapture() == nullptr)
            SetCapture(p_Hwnd);

        s_ImGuiIO.MouseDown[s_Button] = true;

    	break;
    }

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
    {
        int s_Button = 0;
    		
        if (p_Message == WM_LBUTTONUP)
            s_Button = 0;
        if (p_Message == WM_RBUTTONUP) 
            s_Button = 1;
        if (p_Message == WM_MBUTTONUP) 
            s_Button = 2;
    		
        if (p_Message == WM_XBUTTONUP)
            s_Button = (GET_XBUTTON_WPARAM(p_Wparam) == XBUTTON1) ? 3 : 4;

        s_ImGuiIO.MouseDown[s_Button] = false;

        if (!ImGui::IsAnyMouseDown() && GetCapture() == p_Hwnd)
            ReleaseCapture();

    	break;
    }
    case WM_MOUSEWHEEL:
        s_ImGuiIO.MouseWheel += static_cast<float>(GET_WHEEL_DELTA_WPARAM(p_Wparam)) / static_cast<float>(WHEEL_DELTA);
        break;
    
    case WM_MOUSEHWHEEL:
        s_ImGuiIO.MouseWheelH += static_cast<float>(GET_WHEEL_DELTA_WPARAM(p_Wparam)) / static_cast<float>(WHEEL_DELTA);
        break;
    	
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (p_Wparam < 256)
            s_ImGuiIO.KeysDown[p_Wparam] = true;
  
        break;
    	
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (p_Wparam < 256)
            s_ImGuiIO.KeysDown[p_Wparam] = false;
 
        break;
    	
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (p_Wparam > 0 && p_Wparam < 0x10000)
            s_ImGuiIO.AddInputCharacterUTF16(static_cast<unsigned short>(p_Wparam));

        break;
    }

	// Don't call the original function so input isn't passed down to the game.
	// TODO: This doesn't seem to eat keyboard input.
    return HookResult<LRESULT>(HookAction::Return(), DefWindowProcW(p_Hwnd, p_Message, p_Wparam, p_Lparam));
}