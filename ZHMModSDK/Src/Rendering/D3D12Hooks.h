#pragma once

#include <optional>
#include <d3d12.h>
#include <dxgi.h>
#include <unordered_map>
#include <vector>

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
		enum class Function
		{
			IDXGISwapChain_Present = 8,
			IDXGISwapChain_ResizeBuffers = 13,
			IDXGISwapChain_ResizeTarget = 14,
			ID3D12CommandQueue_ExecuteCommandLists = 10,
		};

	private:
		struct VTables
		{
			void* IDXGIFactory1Vtbl;
			void* IDXGIAdapterVtbl;
			void* ID3D12DeviceVtbl;
			void* ID3D12CommandQueueVtbl;
			void* ID3D12CommandAllocatorVtbl;
			void* ID3D12GraphicsCommandListVtbl;
			void* IDXGISwapChainVtbl;
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
		void InstallHooks();
		void RemoveHooks();

	private:
		std::optional<VTables> GetVTables();
		void InstallHook(void* p_VTable, int p_Index, void* p_Detour, void** p_Original);
		void RemoveHook(const InstalledHook& p_Hook);

		DECLARE_D3D12_HOOK(HRESULT, IDXGISwapChain, Present, UINT SyncInterval, UINT Flags);
		DECLARE_D3D12_HOOK(HRESULT, IDXGISwapChain, ResizeBuffers, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
		DECLARE_D3D12_HOOK(HRESULT, IDXGISwapChain, ResizeTarget, const DXGI_MODE_DESC* pNewTargetParameters);
		DECLARE_D3D12_HOOK(void, ID3D12CommandQueue, ExecuteCommandLists, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);

	private:
		std::vector<InstalledHook> m_InstalledHooks;
	};
}
