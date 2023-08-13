#pragma once

#include <optional>
#include <d3d12.h>
#include <dxgi.h>
#include <unordered_map>
#include <vector>

#include "Hook.h"

#define DECLARE_D3D12_HOOK(ReturnType, ThisType, FuncName, ...) \
    private: \
        static ReturnType Detour_ ## ThisType ## _ ## FuncName(ThisType* th, __VA_ARGS__); \
        typedef ReturnType (*ThisType ## _ ## FuncName ## _t)(ThisType* th, __VA_ARGS__); \
        static ThisType ## _ ## FuncName ## _t Original_ ## ThisType ## _ ## FuncName;

#define DEFINE_D3D12_HOOK(ThisType, FuncName) \
    D3D12Hooks::ThisType ## _ ## FuncName ## _t D3D12Hooks::Original_ ## ThisType ## _ ## FuncName = nullptr;

#define INSTALL_D3D12_HOOK(ThisType, FuncName) InstallHook(s_VTables->ThisType ## Vtbl, static_cast<int>(D3D12Hooks::Function::ThisType ## _ ## FuncName), Detour_ ## ThisType ## _ ## FuncName, reinterpret_cast<void**>(&Original_ ## ThisType ## _ ## FuncName));

namespace Rendering
{
    class D3D12Hooks
    {
    public:
        typedef HRESULT(WINAPI* D3D12CreateDevice_t)(_In_opt_ IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, _In_ REFIID riid, _COM_Outptr_opt_ void** ppDevice);

        enum class Function
        {
            IDXGISwapChain_Present = 8,
            IDXGISwapChain_ResizeBuffers = 13,
            IDXGISwapChain_ResizeTarget = 14,
            ID3D12CommandQueue_ExecuteCommandLists = 10,
            IDXGIFactory_CreateSwapChain = 10,
            IDXGIFactory2_CreateSwapChainForHwnd = 15,
            IDXGIFactory2_CreateSwapChainForCoreWindow = 16,
            IDXGIFactory2_CreateSwapChainForComposition = 24,
            ID3D12Device_CreateDescriptorHeap = 14,
            ID3D12Device_CreateShaderResourceView = 18,
            ID3D12Device_CreateCommittedResource = 27,
        };

    private:
        struct VTables
        {
            void* IDXGIFactoryVtbl = nullptr;
            void* ID3D12DeviceVtbl = nullptr;
            void* ID3D12CommandQueueVtbl = nullptr;
            void* ID3D12CommandAllocatorVtbl = nullptr;
            void* ID3D12GraphicsCommandListVtbl = nullptr;
            void* IDXGISwapChainVtbl = nullptr;
        };

        struct InstalledHook
        {
            void** VTable;
            int Index;
            void* OriginalAddr;
        };

    public:
        ~D3D12Hooks();

    public:
        void Startup();
        void RemoveHooks();

    private:
        void Install();
        bool GetVTables(ID3D12Device* p_Device);
        void InstallHook(void* p_VTable, int p_Index, void* p_Detour, void** p_Original);
        void RemoveHook(const InstalledHook& p_Hook);

        DECLARE_D3D12_HOOK(HRESULT, IDXGIFactory, CreateSwapChain, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);
        DECLARE_D3D12_HOOK(HRESULT, ID3D12Device, CreateCommittedResource, const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, REFIID riidResource, void** ppvResource);
        DECLARE_D3D12_HOOK(void, ID3D12CommandQueue, ExecuteCommandLists, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);

        DECLARE_DETOUR_WITH_CONTEXT(D3D12Hooks, HRESULT, D3D12CreateDevice, IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice);

    private:
        std::vector<InstalledHook> m_InstalledHooks;
        bool m_Installed = false;
        VTables m_VTables = {};
    };
}
