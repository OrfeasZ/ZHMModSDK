#include "D3D12Hooks.h"

#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <filesystem>


#include "D3DUtils.h"
#include "Logging.h"
#include "MinHook.h"
#include "ModSDK.h"
#include "Renderers/DirectXTKRenderer.h"
#include "Util/ProcessUtils.h"

#include "Renderers/ImGuiRenderer.h"

using namespace Rendering;

DEFINE_D3D12_HOOK(IDXGISwapChain, Present);
DEFINE_D3D12_HOOK(IDXGISwapChain, ResizeBuffers);
DEFINE_D3D12_HOOK(IDXGISwapChain, ResizeTarget);
DEFINE_D3D12_HOOK(ID3D12CommandQueue, ExecuteCommandLists);

D3D12Hooks::~D3D12Hooks()
{
	RemoveHooks();
}

void D3D12Hooks::InstallHooks()
{
	const auto s_VTables = GetVTables();

	if (!s_VTables)
	{
		Logger::Error("Could not get D3D vtables. Custom rendering will not work.");
		return;
	}

	Util::ProcessUtils::SuspendAllThreadsButCurrent();

	INSTALL_D3D12_HOOK(IDXGISwapChain, Present);
	INSTALL_D3D12_HOOK(IDXGISwapChain, ResizeBuffers);
	INSTALL_D3D12_HOOK(IDXGISwapChain, ResizeTarget);
	INSTALL_D3D12_HOOK(ID3D12CommandQueue, ExecuteCommandLists);

	Util::ProcessUtils::ResumeSuspendedThreads();

	Logger::Debug("Installed D3D hooks.");
}

void D3D12Hooks::RemoveHooks()
{
	Util::ProcessUtils::SuspendAllThreadsButCurrent();

	for (auto& s_Hook : m_InstalledHooks)
		RemoveHook(s_Hook);

	m_InstalledHooks.clear();

	Util::ProcessUtils::ResumeSuspendedThreads();
}

HRESULT D3D12Hooks::Detour_IDXGISwapChain_Present(IDXGISwapChain* th, UINT SyncInterval, UINT Flags)
{
	ScopedD3DRef<IDXGISwapChain3> s_SwapChain3;

	if (th->QueryInterface(REF_IID_PPV_ARGS(s_SwapChain3)) != S_OK)
		return Original_IDXGISwapChain_Present(th, SyncInterval, Flags);

	ModSDK::GetInstance()->OnPresent(s_SwapChain3);

	auto s_Result = Original_IDXGISwapChain_Present(th, SyncInterval, Flags);

	ModSDK::GetInstance()->PostPresent(s_SwapChain3);

	return s_Result;
}

HRESULT D3D12Hooks::Detour_IDXGISwapChain_ResizeBuffers(IDXGISwapChain* th, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	ModSDK::GetInstance()->OnReset();

	return Original_IDXGISwapChain_ResizeBuffers(th, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

HRESULT D3D12Hooks::Detour_IDXGISwapChain_ResizeTarget(IDXGISwapChain* th, const DXGI_MODE_DESC* pNewTargetParameters)
{
	ModSDK::GetInstance()->OnReset();

	return Original_IDXGISwapChain_ResizeTarget(th, pNewTargetParameters);
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
std::optional<D3D12Hooks::VTables> D3D12Hooks::GetVTables()
{
	Logger::Debug("[D3D12Hooks] Locating D3D12 vtable addresses.");

#if _DEBUG
	// See if we already have a cached version of these for this process.
	char s_SharedMemoryName[256];
	sprintf_s(s_SharedMemoryName, sizeof(s_SharedMemoryName), "ZHModSDK_D3D12Hooks_Vtbl_%llu_%lu", sizeof(VTables), GetCurrentProcessId());

	{
		auto* s_Mapping = OpenFileMappingA(FILE_MAP_READ, false, s_SharedMemoryName);

		if (s_Mapping != nullptr)
		{
			auto* s_Buffer = MapViewOfFile(s_Mapping, FILE_MAP_READ, 0, 0, sizeof(VTables));

			if (s_Buffer != nullptr)
			{
				Logger::Debug("[D3D12Hooks] Found cached vtable info. Re-using.");

				VTables s_VTables {};
				memcpy(&s_VTables, s_Buffer, sizeof(VTables));

				UnmapViewOfFile(s_Buffer);
				CloseHandle(s_Mapping);

				return s_VTables;
			}

			CloseHandle(s_Mapping);
		}
	}
#endif

	static wchar_t s_SystemDir[8192];

	if (!GetSystemDirectoryW(s_SystemDir, sizeof(s_SystemDir) / sizeof(wchar_t)))
		return std::nullopt;

	std::filesystem::path s_DxgiPath = s_SystemDir;
	s_DxgiPath += "/dxgi.dll";

	std::filesystem::path s_D3D12Path = s_SystemDir;
	s_D3D12Path += "/d3d12.dll";

	auto s_DxgiModule = LoadLibraryW(canonical(s_DxgiPath).c_str());
	auto s_D3D12Module = LoadLibraryW(canonical(s_D3D12Path).c_str());

	if (s_DxgiModule == nullptr || s_D3D12Module == nullptr)
		return std::nullopt;

	typedef HRESULT (WINAPI* CreateDXGIFactory1_t)(REFIID riid, _COM_Outptr_ void** ppFactory);
	typedef HRESULT (WINAPI* D3D12CreateDevice_t)(_In_opt_ IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, _In_ REFIID riid, _COM_Outptr_opt_ void** ppDevice);

	auto s_CreateDXGIFactory1 = reinterpret_cast<CreateDXGIFactory1_t>(GetProcAddress(s_DxgiModule, "CreateDXGIFactory1"));
	auto s_D3D12CreateDevice = reinterpret_cast<D3D12CreateDevice_t>(GetProcAddress(s_D3D12Module, "D3D12CreateDevice"));

	if (s_CreateDXGIFactory1 == nullptr || s_D3D12CreateDevice == nullptr)
		return std::nullopt;

	// If we don't, try to find them from scratch.
	ScopedD3DRef<IDXGIFactory1> s_Factory;

	if (s_CreateDXGIFactory1(REF_IID_PPV_ARGS(s_Factory)) != S_OK)
		return std::nullopt;

	ScopedD3DRef<IDXGIAdapter> s_Adapter;

	if (s_Factory->EnumAdapters(0, &s_Adapter.Ref) == DXGI_ERROR_NOT_FOUND)
		return std::nullopt;

	Logger::Debug("[D3D12Hooks] Creating D3D12 device.");

#if _DEBUG
	/*ID3D12Debug* s_Debug = nullptr;

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&s_Debug))))
		s_Debug->EnableDebugLayer();

	s_Debug->Release();*/
#endif

	ScopedD3DRef<ID3D12Device> s_Device;

	if (s_D3D12CreateDevice(s_Adapter, D3D_FEATURE_LEVEL_12_0, REF_IID_PPV_ARGS(s_Device)) != S_OK)
		return std::nullopt;

	Logger::Debug("[D3D12Hooks] Creating command queue.");

	D3D12_COMMAND_QUEUE_DESC s_CommandQueueDesc {};
	ScopedD3DRef<ID3D12CommandQueue> s_CommandQueue;

	if (s_Device->CreateCommandQueue(&s_CommandQueueDesc, REF_IID_PPV_ARGS(s_CommandQueue)) != S_OK)
		return std::nullopt;

	Logger::Debug("[D3D12Hooks] Creating command allocator.");

	ScopedD3DRef<ID3D12CommandAllocator> s_CommandAllocator;

	if (s_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, REF_IID_PPV_ARGS(s_CommandAllocator)) != S_OK)
		return std::nullopt;

	Logger::Debug("[D3D12Hooks] Creating command list.");

	ScopedD3DRef<ID3D12GraphicsCommandList> s_GraphicsCommandList;

	if (s_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, s_CommandAllocator, nullptr, REF_IID_PPV_ARGS(s_GraphicsCommandList)) != S_OK)
		return std::nullopt;

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
		return std::nullopt;

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

	ScopedD3DRef<IDXGISwapChain> s_SwapChain;

	Logger::Debug("[D3D12Hooks] Creating swap chain.");

	if (s_Factory->CreateSwapChain(s_CommandQueue, &s_SwapChainDesc, &s_SwapChain.Ref) != S_OK)
		return std::nullopt;

	VTables s_VTables {};
	s_VTables.IDXGIFactory1Vtbl = s_Factory.VTable();
	s_VTables.IDXGIAdapterVtbl = s_Adapter.VTable();
	s_VTables.ID3D12DeviceVtbl = s_Device.VTable();
	s_VTables.ID3D12CommandQueueVtbl = s_CommandQueue.VTable();
	s_VTables.ID3D12CommandAllocatorVtbl = s_CommandAllocator.VTable();
	s_VTables.ID3D12GraphicsCommandListVtbl = s_GraphicsCommandList.VTable();
	s_VTables.IDXGISwapChainVtbl = s_SwapChain.VTable();

	Logger::Debug("[D3D12Hooks] Located all D3D12 vtable addresses.");

#if _DEBUG
	// Cache this in case we need to reload.
	auto* s_Mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(VTables), s_SharedMemoryName);

	if (s_Mapping != nullptr)
	{
		auto* s_Buffer = MapViewOfFile(s_Mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(VTables));

		if (s_Buffer != nullptr)
		{
			memcpy(s_Buffer, &s_VTables, sizeof(VTables));
			UnmapViewOfFile(s_Buffer);

			Logger::Debug("[D3D12Hooks] Cached vtable addresses in shared memory.");
		}

		// NOTE: We don't close the handle since we want this to be found in the future.
	}
#endif	

	return s_VTables;
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

