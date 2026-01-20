#pragma once
#include <cstdint>
#include <dxgi1_4.h>

namespace Rendering {
    class D3D12DXGIFactory : public IDXGIFactory4 {
    public:
        D3D12DXGIFactory(IDXGIFactory4* p_Target);
        virtual ~D3D12DXGIFactory();

        HRESULT QueryInterface(const IID& riid, void** ppvObject) override;
        ULONG AddRef() override;
        ULONG Release() override;

        HRESULT SetPrivateData(REFGUID Name, UINT DataSize, const void* pData) override {
            return m_Target->SetPrivateData(Name, DataSize, pData);
        }

        HRESULT SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown) override {
            return m_Target->SetPrivateDataInterface(Name, pUnknown);
        }

        HRESULT GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData) override {
            return m_Target->GetPrivateData(Name, pDataSize, pData);
        }

        HRESULT GetParent(REFIID riid, void** ppParent) override { return m_Target->GetParent(riid, ppParent); }

        HRESULT EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter) override {
            return m_Target->EnumAdapters(Adapter, ppAdapter);
        }

        HRESULT MakeWindowAssociation(HWND WindowHandle, UINT Flags) override {
            return m_Target->MakeWindowAssociation(WindowHandle, Flags);
        }

        HRESULT GetWindowAssociation(HWND* pWindowHandle) override {
            return m_Target->GetWindowAssociation(pWindowHandle);
        }

        HRESULT CreateSwapChain(IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) override;

        HRESULT CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter) override {
            return m_Target->CreateSoftwareAdapter(Module, ppAdapter);
        }

        HRESULT EnumAdapters1(UINT Adapter, IDXGIAdapter1** ppAdapter) override {
            return m_Target->EnumAdapters1(Adapter, ppAdapter);
        }

        BOOL IsCurrent(void) override { return m_Target->IsCurrent(); }
        BOOL IsWindowedStereoEnabled(void) override { return m_Target->IsWindowedStereoEnabled(); }

        HRESULT CreateSwapChainForHwnd(
            IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc,
            const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput,
            IDXGISwapChain1** ppSwapChain
        ) override {
            return m_Target->CreateSwapChainForHwnd(
                pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain
            );
        }

        HRESULT CreateSwapChainForCoreWindow(
            IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput,
            IDXGISwapChain1** ppSwapChain
        ) override {
            return m_Target->CreateSwapChainForCoreWindow(pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
        }

        HRESULT GetSharedResourceAdapterLuid(HANDLE hResource, LUID* pLuid) override {
            return m_Target->GetSharedResourceAdapterLuid(hResource, pLuid);
        }

        HRESULT RegisterStereoStatusWindow(HWND WindowHandle, UINT wMsg, DWORD* pdwCookie) override {
            return m_Target->RegisterStereoStatusWindow(WindowHandle, wMsg, pdwCookie);
        }

        HRESULT RegisterStereoStatusEvent(HANDLE hEvent, DWORD* pdwCookie) override {
            return m_Target->RegisterStereoStatusEvent(hEvent, pdwCookie);
        }

        void UnregisterStereoStatus(DWORD dwCookie) override { return m_Target->UnregisterStereoStatus(dwCookie); }

        HRESULT RegisterOcclusionStatusWindow(HWND WindowHandle, UINT wMsg, DWORD* pdwCookie) override {
            return m_Target->RegisterOcclusionStatusWindow(WindowHandle, wMsg, pdwCookie);
        }

        HRESULT RegisterOcclusionStatusEvent(HANDLE hEvent, DWORD* pdwCookie) override {
            return m_Target->RegisterOcclusionStatusEvent(hEvent, pdwCookie);
        }

        void UnregisterOcclusionStatus(DWORD dwCookie) override {
            return m_Target->UnregisterOcclusionStatus(dwCookie);
        }

        HRESULT CreateSwapChainForComposition(
            IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput,
            IDXGISwapChain1** ppSwapChain
        ) override { return m_Target->CreateSwapChainForComposition(pDevice, pDesc, pRestrictToOutput, ppSwapChain); }

        UINT GetCreationFlags(void) override { return m_Target->GetCreationFlags(); }

        HRESULT EnumAdapterByLuid(LUID AdapterLuid, REFIID riid, void** ppvAdapter) override {
            return m_Target->EnumAdapterByLuid(AdapterLuid, riid, ppvAdapter);
        }

        HRESULT EnumWarpAdapter(REFIID riid, void** ppvAdapter) override {
            return m_Target->EnumWarpAdapter(riid, ppvAdapter);
        }

    private:
        IDXGIFactory4* m_Target;
        volatile ULONG m_RefCount = 0;
    };
}