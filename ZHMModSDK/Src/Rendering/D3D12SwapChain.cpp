#include "D3D12SwapChain.h"

#include "Logging.h"
#include "ModSDK.h"

using namespace Rendering;

D3D12SwapChain::D3D12SwapChain(IDXGISwapChain3* p_Target) :
	m_Target(p_Target)
{
	m_Target->AddRef();
}

D3D12SwapChain::~D3D12SwapChain()
{
	m_Target->Release();
}

ULONG D3D12SwapChain::AddRef()
{
	InterlockedIncrement(&m_RefCount);
	return m_RefCount;
}

ULONG D3D12SwapChain::Release()
{
	// Decrement the object's internal counter.
	const ULONG s_NewRefCount = InterlockedDecrement(&m_RefCount);

	if (s_NewRefCount == 0)
		delete this;

	return s_NewRefCount;
}

HRESULT D3D12SwapChain::QueryInterface(const IID& riid, void** ppvObject)
{
	if (!ppvObject)
		return E_INVALIDARG;

	*ppvObject = nullptr;

	if (riid == IID_IUnknown ||
		riid == __uuidof(IDXGISwapChain) ||
		riid == __uuidof(IDXGISwapChain1) ||
		riid == __uuidof(IDXGISwapChain2) ||
		riid == __uuidof(IDXGISwapChain3))
	{
		*ppvObject = this;
		AddRef();

		return NOERROR;
	}

	return E_NOINTERFACE;
}

HRESULT D3D12SwapChain::Present(UINT SyncInterval, UINT Flags)
{
	ModSDK::GetInstance()->OnPresent(m_Target);
	const auto s_Result = m_Target->Present(SyncInterval, Flags);
	ModSDK::GetInstance()->PostPresent(m_Target, s_Result);
	return s_Result;
}

HRESULT D3D12SwapChain::Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
	ModSDK::GetInstance()->OnPresent(m_Target);
	const auto s_Result = m_Target->Present1(SyncInterval, PresentFlags, pPresentParameters);
	ModSDK::GetInstance()->PostPresent(m_Target, s_Result);
	return s_Result;
}

HRESULT D3D12SwapChain::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	ModSDK::GetInstance()->OnReset(m_Target);
	const auto s_Result = m_Target->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
	ModSDK::GetInstance()->PostReset(m_Target);
	return s_Result;
}

HRESULT D3D12SwapChain::ResizeBuffers1(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue)
{
	ModSDK::GetInstance()->OnReset(m_Target);
	const auto s_Result = m_Target->ResizeBuffers1(BufferCount, Width, Height, Format, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
	ModSDK::GetInstance()->PostReset(m_Target);
	return s_Result;
}

HRESULT D3D12SwapChain::ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters)
{
	ModSDK::GetInstance()->OnReset(m_Target);
	const auto s_Result = m_Target->ResizeTarget(pNewTargetParameters);
	ModSDK::GetInstance()->PostReset(m_Target);
	return s_Result;
}

