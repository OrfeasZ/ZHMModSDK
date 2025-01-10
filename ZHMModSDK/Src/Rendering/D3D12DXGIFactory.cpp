#include "D3D12DXGIFactory.h"

#include "Logging.h"
#include "ModSDK.h"
#include "D3DUtils.h"
#include "D3D12SwapChain.h"

using namespace Rendering;

D3D12DXGIFactory::D3D12DXGIFactory(IDXGIFactory4* p_Target) :
    m_Target(p_Target) {
    m_Target->AddRef();
}

D3D12DXGIFactory::~D3D12DXGIFactory() {
    m_Target->Release();
}

ULONG D3D12DXGIFactory::AddRef() {
    InterlockedIncrement(&m_RefCount);
    return m_RefCount;
}

ULONG D3D12DXGIFactory::Release() {
    // Decrement the object's internal counter.
    const ULONG s_NewRefCount = InterlockedDecrement(&m_RefCount);

    if (s_NewRefCount == 0)
        delete this;

    return s_NewRefCount;
}

HRESULT D3D12DXGIFactory::QueryInterface(const IID& riid, void** ppvObject) {
    if (!ppvObject)
        return E_INVALIDARG;

    *ppvObject = nullptr;

    if (riid == IID_IUnknown ||
        riid == __uuidof(IDXGIFactory) ||
        riid == __uuidof(IDXGIFactory1) ||
        riid == __uuidof(IDXGIFactory2) ||
        riid == __uuidof(IDXGIFactory3) ||
        riid == __uuidof(IDXGIFactory4)) {
        *ppvObject = this;
        AddRef();

        return NOERROR;
    }

    return E_NOINTERFACE;
}

HRESULT D3D12DXGIFactory::CreateSwapChain(
    IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain
) {
    Logger::Debug("[D3D12Hooks] Creating swap chain.");

    IDXGISwapChain* s_SwapChain = nullptr;

    auto s_Result = m_Target->CreateSwapChain(pDevice, pDesc, &s_SwapChain);

    if (s_Result != S_OK)
        return s_Result;

    ScopedD3DRef<IDXGISwapChain3> s_SwapChain3;

    if (s_SwapChain->QueryInterface(REF_IID_PPV_ARGS(s_SwapChain3)) != S_OK) {
        Logger::Warn("[D3D12Hooks] Swap chain was not version 3. Not touching.");
        *ppSwapChain = s_SwapChain;
        return S_OK;
    }

    Logger::Debug("[D3D12Hooks] Wrapping swap chain.");
    auto s_WrappedSwapChain = new D3D12SwapChain(s_SwapChain3.Ref);
    s_WrappedSwapChain->AddRef();

    *ppSwapChain = s_WrappedSwapChain;

    ModSDK::GetInstance()->SetSwapChain(s_WrappedSwapChain);

    ID3D12CommandQueue* s_CommandQueue = nullptr;
    pDevice->QueryInterface(IID_PPV_ARGS(&s_CommandQueue));

    ModSDK::GetInstance()->SetCommandQueue(s_CommandQueue);

    return S_OK;
}
