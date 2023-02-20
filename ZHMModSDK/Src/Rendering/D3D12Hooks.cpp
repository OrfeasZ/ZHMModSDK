#include "D3D12Hooks.h"

#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <filesystem>


#include "D3D12SwapChain.h"
#include "D3DUtils.h"
#include "Logging.h"
#include "MinHook.h"
#include "ModSDK.h"
#include "Renderers/DirectXTKRenderer.h"

#include <HookImpl.h>

using namespace Rendering;

DEFINE_D3D12_HOOK(IDXGIFactory, CreateSwapChain);
DEFINE_D3D12_HOOK(ID3D12CommandQueue, ExecuteCommandLists);

D3D12Hooks::~D3D12Hooks()
{
    RemoveHooks();
    HookRegistry::ClearDetoursWithContext(this);
}

void D3D12Hooks::Startup()
{
    Hooks::D3D12CreateDevice->AddDetour(this, &D3D12Hooks::D3D12CreateDevice);
}

void D3D12Hooks::Install()
{
    if (m_Installed)
    {
        Logger::Warn("Already installed. Skipping.");
        return;
    }

    const auto s_VTables = &m_VTables;

    INSTALL_D3D12_HOOK(IDXGIFactory, CreateSwapChain);
    INSTALL_D3D12_HOOK(ID3D12CommandQueue, ExecuteCommandLists);

    m_Installed = true;

    Logger::Debug("Installed D3D hooks.");
}

void D3D12Hooks::RemoveHooks()
{
    for (auto& s_Hook : m_InstalledHooks)
        RemoveHook(s_Hook);

    m_InstalledHooks.clear();
    m_Installed = true;
}

HRESULT D3D12Hooks::Detour_IDXGIFactory_CreateSwapChain(IDXGIFactory* th, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
    Logger::Debug("[D3D12Hooks] Creating swap chain.");

    IDXGISwapChain* s_SwapChain = nullptr;

    auto s_Result = Original_IDXGIFactory_CreateSwapChain(th, pDevice, pDesc, &s_SwapChain);

    if (s_Result != S_OK)
        return s_Result;

    ScopedD3DRef<IDXGISwapChain3> s_SwapChain3;

    if (s_SwapChain->QueryInterface(REF_IID_PPV_ARGS(s_SwapChain3)) != S_OK)
    {
        Logger::Warn("[D3D12Hooks] Swap chain was not version 3. Not touching.");
        *ppSwapChain = s_SwapChain;
        return S_OK;
    }

    Logger::Debug("[D3D12Hooks] Wrapping swap chain.");
    *ppSwapChain = new D3D12SwapChain(s_SwapChain3.Ref);
    (*ppSwapChain)->AddRef();

    return S_OK;
}

void D3D12Hooks::Detour_ID3D12CommandQueue_ExecuteCommandLists(ID3D12CommandQueue* th, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
{
    ModSDK::GetInstance()->SetCommandQueue(th);

    Original_ID3D12CommandQueue_ExecuteCommandLists(th, NumCommandLists, ppCommandLists);
}

/// Internal implementation below.

struct ScopedWindowClass
{
    ScopedWindowClass() : Class({}) {}

    ~ScopedWindowClass()
    {
        UnregisterClassA(Class.lpszClassName, Class.hInstance);
    }

    WNDCLASSEX* operator->()
    {
        return &Class;
    }

    operator WNDCLASSEX()
    {
        return Class;
    }

    WNDCLASSEX Class;
};

struct ScopedWindow
{
    ScopedWindow(HWND p_Window) : Window(p_Window) {}

    ~ScopedWindow()
    {
        if (Window != nullptr)
            DestroyWindow(Window);
    }

    operator HWND()
    {
        return Window;
    }

    operator bool()
    {
        return Window != nullptr;
    }

    HWND Window;
};

/**
 * This function creates some mock D3D12 devices and such so we can
 * grab the addresses to their vtables which we then use to install our
 * custom hooks and do custom rendering stuffs. These are immediately
 * destroyed before this function returns.
 */
bool D3D12Hooks::GetVTables(ID3D12Device* p_Device)
{
    Logger::Debug("[D3D12Hooks] Locating D3D12 vtable addresses.");

#if _DEBUG
    /*ID3D12Debug* s_Debug = nullptr;
    uint32_t s_ThingResult = D3D12GetDebugInterface(IID_PPV_ARGS(&s_Debug));

    if (SUCCEEDED(s_ThingResult))
    {
        Logger::Debug("[D3D12Hooks] Enabling D3D12 debug layer.");

        s_Debug->EnableDebugLayer();

        ID3D12Debug1* s_Debug1 = nullptr;

        if (SUCCEEDED(s_Debug->QueryInterface(IID_PPV_ARGS(&s_Debug1))))
        {
            Logger::Debug("[D3D12Hooks] Enabling D3D12 gpu-based validation.");

            s_Debug1->SetEnableGPUBasedValidation(true);
            s_Debug1->Release();
        }

        s_Debug->Release();
    }
    else
    {
        Logger::Error("[D3D12Hooks] Could not get debug interface with error: {:X}", s_ThingResult);
    }

    ID3D12DeviceRemovedExtendedDataSettings* s_DredSettings;

    // Turn on auto-breadcrumbs and page fault reporting.
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&s_DredSettings))))
    {
        Logger::Debug("[D3D12Hooks] Enabling D3D12 breadcrumbs and page fault reporting.");
        s_DredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        s_DredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        s_DredSettings->Release();
    }*/
#endif
    Logger::Debug("[D3D12Hooks] Creating command queue.");

    D3D12_COMMAND_QUEUE_DESC s_CommandQueueDesc {};
    ScopedD3DRef<ID3D12CommandQueue> s_CommandQueue;

    if (p_Device->CreateCommandQueue(&s_CommandQueueDesc, REF_IID_PPV_ARGS(s_CommandQueue)) != S_OK)
        return false;

    Logger::Debug("[D3D12Hooks] Creating command allocator.");

    ScopedD3DRef<ID3D12CommandAllocator> s_CommandAllocator;

    if (p_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, REF_IID_PPV_ARGS(s_CommandAllocator)) != S_OK)
        return false;

    Logger::Debug("[D3D12Hooks] Creating command list.");

    ScopedD3DRef<ID3D12GraphicsCommandList> s_GraphicsCommandList;

    if (p_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, s_CommandAllocator, nullptr, REF_IID_PPV_ARGS(s_GraphicsCommandList)) != S_OK)
        return false;

    // Create a temporary window for our swap chain.
    ScopedWindowClass s_WindowClass;
    s_WindowClass->cbSize = sizeof(WNDCLASSEX);
    s_WindowClass->style = CS_HREDRAW | CS_VREDRAW;
    s_WindowClass->lpfnWndProc = DefWindowProcA;
    s_WindowClass->hInstance = GetModuleHandleA(nullptr);
    s_WindowClass->lpszClassName = "ZHMModSDK";

    RegisterClassExA(&s_WindowClass.Class);

    Logger::Debug("[D3D12Hooks] Creating fake window for swap chain.");

    ScopedWindow s_Window = CreateWindowA(s_WindowClass->lpszClassName, "ZHMModSDK_D3D", WS_OVERLAPPEDWINDOW, 0, 0, 256, 256, nullptr, nullptr, s_WindowClass->hInstance, nullptr);

    if (!s_Window)
        return false;

    DXGI_RATIONAL s_RefreshRateRational {};
    s_RefreshRateRational.Numerator = 60;
    s_RefreshRateRational.Denominator = 1;

    DXGI_MODE_DESC s_BufferDesc {};
    s_BufferDesc.Width = 256;
    s_BufferDesc.Height = 256;
    s_BufferDesc.RefreshRate = s_RefreshRateRational;
    s_BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    DXGI_SAMPLE_DESC s_SampleDesc {};
    s_SampleDesc.Count = 1;
    s_SampleDesc.Quality = 0;

    DXGI_SWAP_CHAIN_DESC s_SwapChainDesc {};
    s_SwapChainDesc.BufferDesc = s_BufferDesc;
    s_SwapChainDesc.SampleDesc = s_SampleDesc;
    s_SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    s_SwapChainDesc.BufferCount = 2;
    s_SwapChainDesc.OutputWindow = s_Window;
    s_SwapChainDesc.Windowed = 1;
    s_SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    s_SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ScopedD3DRef<IDXGIFactory1> s_Factory;

    if (Hooks::CreateDXGIFactory1->Call(REF_IID_PPV_ARGS(s_Factory)) != S_OK)
        return false;

    ScopedD3DRef<IDXGISwapChain> s_SwapChain;

    Logger::Debug("[D3D12Hooks] Creating swap chain.");

    if (s_Factory->CreateSwapChain(s_CommandQueue, &s_SwapChainDesc, &s_SwapChain.Ref) != S_OK)
        return false;

    m_VTables.IDXGIFactoryVtbl = s_Factory.VTable();
    m_VTables.ID3D12DeviceVtbl = *reinterpret_cast<void**>(p_Device);
    m_VTables.ID3D12CommandQueueVtbl = s_CommandQueue.VTable();
    m_VTables.ID3D12CommandAllocatorVtbl = s_CommandAllocator.VTable();
    m_VTables.ID3D12GraphicsCommandListVtbl = s_GraphicsCommandList.VTable();
    m_VTables.IDXGISwapChainVtbl = s_SwapChain.VTable();

    Logger::Debug("[D3D12Hooks] Located all D3D12 vtable addresses.");

    return true;
}

void D3D12Hooks::InstallHook(void* p_VTable, int p_Index, void* p_Detour, void** p_Original)
{
    auto s_VTableEntries = static_cast<void**>(p_VTable);
    auto s_OriginalAddr = s_VTableEntries[p_Index];

    auto s_Result = MH_CreateHook(s_OriginalAddr, p_Detour, p_Original);

    if (s_Result != MH_OK)
    {
        Logger::Error("Could not create D3D12 vtable hook at address {}. Error code: {}.", fmt::ptr(s_OriginalAddr), s_Result);
        return;
    }

    s_Result = MH_EnableHook(s_OriginalAddr);

    if (s_Result != MH_OK)
    {
        Logger::Error("Could install detour for D3D12 vtable hook at address {}. Error code: {}.", fmt::ptr(s_OriginalAddr), s_Result);
        return;
    }

    InstalledHook s_Hook;
    s_Hook.VTable = s_VTableEntries;
    s_Hook.Index = p_Index;
    s_Hook.OriginalAddr = s_OriginalAddr;

    m_InstalledHooks.push_back(s_Hook);

    Logger::Debug("Successfully installed detour for D3D12 vtable hook address {}.", fmt::ptr(s_OriginalAddr));
}

void D3D12Hooks::RemoveHook(const InstalledHook& p_Hook)
{
    MH_DisableHook(p_Hook.OriginalAddr);
    MH_RemoveHook(p_Hook.OriginalAddr);
}

DECLARE_DETOUR_WITH_CONTEXT(D3D12Hooks, HRESULT, D3D12CreateDevice, IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice)
{
    const auto s_Result = p_Hook->CallOriginal(pAdapter, MinimumFeatureLevel, riid, ppDevice);

    if (!m_Installed)
    {
        GetVTables(static_cast<ID3D12Device*>(*ppDevice));
        Install();
    }

    return HookResult(HookAction::Return(), s_Result);
}
