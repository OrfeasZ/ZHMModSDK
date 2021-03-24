#include "D3D12Renderer.h"
#include "D3D12Hooks.h"
#include "Renderers/ImGuiRenderer.h"

using namespace Rendering;

void D3D12Renderer::Init()
{
	Renderers::ImGuiRenderer::Init();
	
	// Init the D3D12 hooks.
	D3D12Hooks::InstallHooks();

	// If we're running in debug mode we should enable the D3D debug layer.
#if _DEBUG
	/*Hooks::D3D12CreateDevice->AddDetour(this, [](void*, Hook<HRESULT(IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice)>* p_Hook, IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice)
		{
			Logger::Debug("Creating D3D device. Enabling debug layer.");

			ID3D12Debug* s_Debug = nullptr;

			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&s_Debug))))
			{
				s_Debug->EnableDebugLayer();
			}

			const auto s_Result = p_Hook->CallOriginal(pAdapter, MinimumFeatureLevel, riid, ppDevice);

			if (s_Debug)
			{
				ID3D12InfoQueue* s_InfoQueue = nullptr;
				(*reinterpret_cast<ID3D12Device**>(ppDevice))->QueryInterface(IID_PPV_ARGS(&s_InfoQueue));

				s_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
				s_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
				s_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
				s_InfoQueue->Release();

				s_Debug->Release();
			}

			return HookResult<HRESULT>(HookAction::Return(), s_Result);
		});*/
#endif
}

void D3D12Renderer::Shutdown()
{
	D3D12Hooks::RemoveHooks();
	
	Renderers::ImGuiRenderer::Shutdown();
}
