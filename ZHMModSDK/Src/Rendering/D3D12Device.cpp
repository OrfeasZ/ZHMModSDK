#include "D3D12Device.h"

#include "Logging.h"
#include "ModSDK.h"

using namespace Rendering;

D3D12Device::D3D12Device(ID3D12Device10* p_Target) :
    m_Target(p_Target)
{
    m_Target->AddRef();
}

D3D12Device::~D3D12Device()
{
    m_Target->Release();
}

ULONG D3D12Device::AddRef()
{
    InterlockedIncrement(&m_RefCount);
    return m_RefCount;
}

ULONG D3D12Device::Release()
{
    // Decrement the object's internal counter.
    const ULONG s_NewRefCount = InterlockedDecrement(&m_RefCount);

    if (s_NewRefCount == 0)
        delete this;

    return s_NewRefCount;
}

HRESULT D3D12Device::QueryInterface(const IID& riid, void** ppvObject)
{
    if (!ppvObject)
        return E_INVALIDARG;

    *ppvObject = nullptr;

    if (riid == IID_IUnknown ||
        riid == __uuidof(ID3D12Device) ||
        riid == __uuidof(ID3D12Device1) ||
        riid == __uuidof(ID3D12Device2) ||
        riid == __uuidof(ID3D12Device3) ||
        riid == __uuidof(ID3D12Device4) ||
        riid == __uuidof(ID3D12Device5) ||
        riid == __uuidof(ID3D12Device6) ||
        riid == __uuidof(ID3D12Device7) ||
        riid == __uuidof(ID3D12Device8) ||
        riid == __uuidof(ID3D12Device9) ||
        riid == __uuidof(ID3D12Device10))
    {
        *ppvObject = this;
        AddRef();

        return NOERROR;
    }

    return E_NOINTERFACE;
}

/*
HRESULT D3D12Device::CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC* pDesc, const IID& riid, void** ppCommandQueue) {
	const auto s_Result = m_Target->CreateCommandQueue(pDesc, riid, ppCommandQueue);

	Logger::Debug("Created command queue {}", fmt::ptr(*ppCommandQueue));

	return s_Result;
}

HRESULT D3D12Device::CreateCommandQueue1(
	const D3D12_COMMAND_QUEUE_DESC* pDesc,
	const IID& CreatorID,
	const IID& riid,
	void** ppCommandQueue
) {
	const auto s_Result = m_Target->CreateCommandQueue1(pDesc, CreatorID, riid, ppCommandQueue);

	Logger::Debug("Created command queue1 {}", fmt::ptr(*ppCommandQueue));

	return s_Result;
}*/
