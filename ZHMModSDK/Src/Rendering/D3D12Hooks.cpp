#include "D3D12Hooks.h"

#include <directx/d3d12.h>
#include <filesystem>


#include "D3D12SwapChain.h"
#include "D3DUtils.h"
#include "Logging.h"
#include "MinHook.h"
#include "ModSDK.h"
#include "Renderers/DirectXTKRenderer.h"
#include "D3D12DXGIFactory.h"
#include "D3D12Device.h"

#include <HookImpl.h>

using namespace Rendering;

D3D12Hooks::~D3D12Hooks()
{
    HookRegistry::ClearDetoursWithContext(this);
}

void D3D12Hooks::Startup()
{
    Hooks::D3D12CreateDevice->AddDetour(this, &D3D12Hooks::D3D12CreateDevice);
    Hooks::CreateDXGIFactory1->AddDetour(this, &D3D12Hooks::CreateDXGIFactory1);
}

DEFINE_DETOUR_WITH_CONTEXT(D3D12Hooks, HRESULT, D3D12CreateDevice, IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice) {
#if _DEBUG
	/*ID3D12Debug* s_Debug = nullptr;
	uint32_t s_ThingResult = D3D12GetDebugInterface(IID_PPV_ARGS(&s_Debug));

	if (SUCCEEDED(s_ThingResult)) {
		Logger::Debug("[D3D12Hooks] Enabling D3D12 debug layer.");

		s_Debug->EnableDebugLayer();

		ID3D12Debug1* s_Debug1 = nullptr;

		if (SUCCEEDED(s_Debug->QueryInterface(IID_PPV_ARGS(&s_Debug1)))) {
			Logger::Debug("[D3D12Hooks] Enabling D3D12 gpu-based validation.");

			//s_Debug1->SetEnableGPUBasedValidation(true);
			s_Debug1->Release();
		}

		s_Debug->Release();
	} else {
		Logger::Error("[D3D12Hooks] Could not get debug interface with error: {:X}", s_ThingResult);
	}

	ID3D12DeviceRemovedExtendedDataSettings* s_DredSettings;

	// Turn on auto-breadcrumbs and page fault reporting.
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&s_DredSettings)))) {
		Logger::Debug("[D3D12Hooks] Enabling D3D12 breadcrumbs and page fault reporting.");
		s_DredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		s_DredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		s_DredSettings->Release();
	}*/
#endif

	Logger::Debug("[D3D12Hooks] Creating D3D12 device.");

	ID3D12Device* s_Device = nullptr;
	auto s_Result = p_Hook->CallOriginal(pAdapter, MinimumFeatureLevel, riid, (void**)&s_Device);

	if (s_Result != S_OK)
		return { HookAction::Return(), s_Result };

	ScopedD3DRef<ID3D12Device10> s_Device10;

	if (s_Device->QueryInterface(REF_IID_PPV_ARGS(s_Device10)) != S_OK)
	{
		Logger::Warn("[D3D12Hooks] D3D12 device was not version 10. Not touching.");
		*ppDevice = s_Device;
		return { HookAction::Return(), S_OK };
	}

	Logger::Debug("[D3D12Hooks] Wrapping D3D12 device.");
	auto s_WrappedFactory = new D3D12Device(s_Device10.Ref);
	s_WrappedFactory->AddRef();

	*ppDevice = s_WrappedFactory;

	return { HookAction::Return(), S_OK };
}

DEFINE_DETOUR_WITH_CONTEXT(D3D12Hooks, HRESULT, CreateDXGIFactory1, REFIID riid, void** ppFactory) {
	Logger::Debug("[D3D12Hooks] Creating DXGI factory.");

	IDXGIFactory* s_Factory = nullptr;
	auto s_Result = p_Hook->CallOriginal(riid, (void**)&s_Factory);

	if (s_Result != S_OK)
		return { HookAction::Return(), s_Result };

	ScopedD3DRef<IDXGIFactory4> s_Factory4;

	if (s_Factory->QueryInterface(REF_IID_PPV_ARGS(s_Factory4)) != S_OK)
	{
		Logger::Warn("[D3D12Hooks] DXGI factory was not version 4. Not touching.");
		*ppFactory = s_Factory;
		return { HookAction::Return(), S_OK };
	}

	Logger::Debug("[D3D12Hooks] Wrapping DXGI factory.");
	auto s_WrappedFactory = new D3D12DXGIFactory(s_Factory4.Ref);
	s_WrappedFactory->AddRef();

	*ppFactory = s_WrappedFactory;

	return { HookAction::Return(), S_OK };
}