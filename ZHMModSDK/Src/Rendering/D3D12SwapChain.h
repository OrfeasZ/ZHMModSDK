#pragma once
#include <cstdint>
#include <dxgi1_4.h>

namespace Rendering
{
	class D3D12SwapChain : public IDXGISwapChain3
	{
	public:
		D3D12SwapChain(IDXGISwapChain3* p_Target);
		virtual ~D3D12SwapChain();

		HRESULT QueryInterface(const IID& riid, void** ppvObject) override;
		ULONG AddRef() override;
		ULONG Release() override;
		HRESULT Present(UINT SyncInterval, UINT Flags) override;
		HRESULT Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters) override;
		HRESULT ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) override;
		HRESULT ResizeBuffers1(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue) override;
		HRESULT ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters) override;

		HRESULT SetPrivateData(const GUID& Name, UINT DataSize, const void* pData) override { return m_Target->SetPrivateData(Name, DataSize, pData); }
		HRESULT SetPrivateDataInterface(const GUID& Name, const IUnknown* pUnknown) override { return m_Target->SetPrivateDataInterface(Name, pUnknown); }
		HRESULT GetPrivateData(const GUID& Name, UINT* pDataSize, void* pData) override { return m_Target->GetPrivateData(Name, pDataSize, pData); }
		HRESULT GetParent(const IID& riid, void** ppParent) override { return m_Target->GetParent(riid, ppParent); }
		HRESULT GetDevice(const IID& riid, void** ppDevice) override { return m_Target->GetDevice(riid, ppDevice); }
		HRESULT GetBuffer(UINT Buffer, const IID& riid, void** ppSurface) override { return m_Target->GetBuffer(Buffer, riid, ppSurface); }
		HRESULT SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget) override { return m_Target->SetFullscreenState(Fullscreen, pTarget); }
		HRESULT GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget) override { return m_Target->GetFullscreenState(pFullscreen, ppTarget); }
		HRESULT GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc) override { return m_Target->GetDesc(pDesc); }
		HRESULT GetContainingOutput(IDXGIOutput** ppOutput) override { return m_Target->GetContainingOutput(ppOutput); }
		HRESULT GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats) override { return m_Target->GetFrameStatistics(pStats); }
		HRESULT GetLastPresentCount(UINT* pLastPresentCount) override { return m_Target->GetLastPresentCount(pLastPresentCount); }
		HRESULT GetDesc1(DXGI_SWAP_CHAIN_DESC1* pDesc) override { return m_Target->GetDesc1(pDesc); }
		HRESULT GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc) override { return m_Target->GetFullscreenDesc(pDesc); }
		HRESULT GetHwnd(HWND* pHwnd) override { return m_Target->GetHwnd(pHwnd); }
		HRESULT GetCoreWindow(const IID& refiid, void** ppUnk) override { return m_Target->GetCoreWindow(refiid, ppUnk); }
		BOOL IsTemporaryMonoSupported() override { return m_Target->IsTemporaryMonoSupported(); }
		HRESULT GetRestrictToOutput(IDXGIOutput** ppRestrictToOutput) override { return m_Target->GetRestrictToOutput(ppRestrictToOutput); }
		HRESULT SetBackgroundColor(const DXGI_RGBA* pColor) override { return m_Target->SetBackgroundColor(pColor); }
		HRESULT GetBackgroundColor(DXGI_RGBA* pColor) override { return m_Target->GetBackgroundColor(pColor); }
		HRESULT SetRotation(DXGI_MODE_ROTATION Rotation) override { return m_Target->SetRotation(Rotation); }
		HRESULT GetRotation(DXGI_MODE_ROTATION* pRotation) override { return m_Target->GetRotation(pRotation); }
		HRESULT SetSourceSize(UINT Width, UINT Height) override { return m_Target->SetSourceSize(Width, Height); }
		HRESULT GetSourceSize(UINT* pWidth, UINT* pHeight) override { return m_Target->GetSourceSize(pWidth, pHeight); }
		HRESULT SetMaximumFrameLatency(UINT MaxLatency) override { return m_Target->SetMaximumFrameLatency(MaxLatency); }
		HRESULT GetMaximumFrameLatency(UINT* pMaxLatency) override { return m_Target->GetMaximumFrameLatency(pMaxLatency); }
		HANDLE GetFrameLatencyWaitableObject() override { return m_Target->GetFrameLatencyWaitableObject(); }
		HRESULT SetMatrixTransform(const DXGI_MATRIX_3X2_F* pMatrix) override { return m_Target->SetMatrixTransform(pMatrix); }
		HRESULT GetMatrixTransform(DXGI_MATRIX_3X2_F* pMatrix) override { return m_Target->GetMatrixTransform(pMatrix); }
		UINT GetCurrentBackBufferIndex() override { return m_Target->GetCurrentBackBufferIndex(); }
		HRESULT CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT* pColorSpaceSupport) override { return m_Target->CheckColorSpaceSupport(ColorSpace, pColorSpaceSupport); }
		HRESULT SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace) override { return m_Target->SetColorSpace1(ColorSpace); }

	private:
		IDXGISwapChain3* m_Target;
		volatile ULONG m_RefCount = 0;
	};
}
