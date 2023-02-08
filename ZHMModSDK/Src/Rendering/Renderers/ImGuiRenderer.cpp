#include <directx/d3dx12.h>

#include "ImGuiRenderer.h"

#include <d3dcompiler.h>
#include <dxgi1_4.h>

#include <imgui.h>
#include <backends/imgui_impl_dx12.h>
#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>
#include <DirectXHelpers.h>

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZApplicationEngineWin32.h>

#include "UI/Console.h"
#include "UI/MainMenu.h"
#include "UI/ModSelector.h"

#include "Rendering/D3DUtils.h"
#include "Fonts.h"
#include "Globals.h"
#include "HookImpl.h"
#include "ModSDK.h"
#include "Glacier/ZRender.h"


using namespace Rendering::Renderers;

ImGuiRenderer::ImGuiRenderer()
{
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&m_TicksPerSecond));
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_Time));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& s_ImGuiIO = ImGui::GetIO();
	s_ImGuiIO.IniFilename = nullptr;

	ImGui::StyleColorsDark();

	s_ImGuiIO.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
	s_ImGuiIO.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
	s_ImGuiIO.BackendPlatformName = "imgui_impl_win32";

	// Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
	s_ImGuiIO.KeyMap[ImGuiKey_Tab] = VK_TAB;
	s_ImGuiIO.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	s_ImGuiIO.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	s_ImGuiIO.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	s_ImGuiIO.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	s_ImGuiIO.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	s_ImGuiIO.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	s_ImGuiIO.KeyMap[ImGuiKey_Home] = VK_HOME;
	s_ImGuiIO.KeyMap[ImGuiKey_End] = VK_END;
	s_ImGuiIO.KeyMap[ImGuiKey_Insert] = VK_INSERT;
	s_ImGuiIO.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	s_ImGuiIO.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	s_ImGuiIO.KeyMap[ImGuiKey_Space] = VK_SPACE;
	s_ImGuiIO.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	s_ImGuiIO.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
	s_ImGuiIO.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
	s_ImGuiIO.KeyMap[ImGuiKey_A] = 'A';
	s_ImGuiIO.KeyMap[ImGuiKey_C] = 'C';
	s_ImGuiIO.KeyMap[ImGuiKey_V] = 'V';
	s_ImGuiIO.KeyMap[ImGuiKey_X] = 'X';
	s_ImGuiIO.KeyMap[ImGuiKey_Y] = 'Y';
	s_ImGuiIO.KeyMap[ImGuiKey_Z] = 'Z';

	s_ImGuiIO.BackendRendererName = "imgui_impl_dx12";
	s_ImGuiIO.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	m_FontLight = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(RobotoLight_compressed_data, RobotoLight_compressed_size, 32.f);
	m_FontRegular = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(RobotoRegular_compressed_data, RobotoRegular_compressed_size, 32.f);
	m_FontMedium = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(RobotoMedium_compressed_data, RobotoMedium_compressed_size, 32.f);
	m_FontBold = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(RobotoBold_compressed_data, RobotoBold_compressed_size, 32.f);
	m_FontBlack = s_ImGuiIO.Fonts->AddFontFromMemoryCompressedTTF(RobotoBlack_compressed_data, RobotoBlack_compressed_size, 32.f);

	s_ImGuiIO.FontDefault = m_FontRegular;

	SetupStyles();
}

ImGuiRenderer::~ImGuiRenderer()
{
	if (m_RendererSetup)
		WaitForCurrentFrameToFinish();

	HookRegistry::ClearDetoursWithContext(this);
}

void ImGuiRenderer::SetupStyles()
{
	auto& s_Style = ImGui::GetStyle();

	s_Style.ChildRounding = 0.f;
	s_Style.FrameRounding = 0.f;
	s_Style.GrabRounding = 0.f;
	s_Style.PopupRounding = 0.f;
	s_Style.ScrollbarRounding = 0.f;
	s_Style.TabRounding = 0.f;
	s_Style.WindowRounding = 0.f;
	s_Style.WindowBorderSize = 0.f;

	s_Style.WindowPadding = ImVec2(20.f, 20.f);
	s_Style.FramePadding = ImVec2(10.f, 10.f);
	s_Style.CellPadding = ImVec2(5.f, 5.f);
	s_Style.ItemSpacing = ImVec2(20.f, 10.f);
	s_Style.ItemInnerSpacing = ImVec2(10.f, 10.f);
	s_Style.TouchExtraPadding = ImVec2(0.f, 0.f);
	s_Style.IndentSpacing = 20.f;
	s_Style.ScrollbarSize = 20.f;
	s_Style.GrabMinSize = 20.f;

	s_Style.WindowBorderSize = 0.f;
	s_Style.ChildBorderSize = 0.f;
	s_Style.PopupBorderSize = 0.f;
	s_Style.FrameBorderSize = 0.f;
	s_Style.TabBorderSize = 0.f;

	ImVec4* s_Colors = s_Style.Colors;
	s_Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	s_Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	s_Colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);
	s_Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	s_Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	s_Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	s_Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	s_Colors[ImGuiCol_FrameBg] = ImVec4(0.06f, 0.05f, 0.05f, 1.00f);
	s_Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.13f);
	s_Colors[ImGuiCol_FrameBgActive] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
	s_Colors[ImGuiCol_TitleBg] = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);
	s_Colors[ImGuiCol_TitleBgActive] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
	s_Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);
	s_Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	s_Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	s_Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	s_Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	s_Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	s_Colors[ImGuiCol_CheckMark] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
	s_Colors[ImGuiCol_SliderGrab] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
	s_Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.05f, 1.00f);
	s_Colors[ImGuiCol_Button] = ImVec4(0.55f, 0.11f, 0.13f, 1.00f);
	s_Colors[ImGuiCol_ButtonHovered] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
	s_Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.05f, 0.05f, 1.00f);
	s_Colors[ImGuiCol_Header] = ImVec4(0.55f, 0.11f, 0.13f, 1.00f);
	s_Colors[ImGuiCol_HeaderHovered] = ImVec4(0.98f, 0.00f, 0.00f, 1.00f);
	s_Colors[ImGuiCol_HeaderActive] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
	s_Colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	s_Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	s_Colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	s_Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.09f);
	s_Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.55f, 0.11f, 0.13f, 1.00f);
	s_Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
	s_Colors[ImGuiCol_Tab] = ImVec4(0.55f, 0.11f, 0.13f, 1.00f);
	s_Colors[ImGuiCol_TabHovered] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
	s_Colors[ImGuiCol_TabActive] = ImVec4(0.98f, 0.00f, 0.05f, 1.00f);
	s_Colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
	s_Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
	s_Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	s_Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	s_Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	s_Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	s_Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	s_Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	s_Colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	s_Colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	s_Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	s_Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	s_Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	s_Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	s_Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	s_Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	s_Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void ImGuiRenderer::OnEngineInit()
{
	Hooks::ZApplicationEngineWin32_MainWindowProc->AddDetour(this, &ImGuiRenderer::WndProc);
	Hooks::ZKeyboardWindows_Update->AddDetour(this, &ImGuiRenderer::ZKeyboardWindows_Update);
	Hooks::ZInputAction_Analog->AddDetour(this, &ImGuiRenderer::ZInputAction_Analog);
}

void ImGuiRenderer::Draw()
{
	ImGui_ImplDX12_NewFrame();

	ImGuiIO& s_ImGuiIO = ImGui::GetIO();

	int64_t s_CurrentTime = 0;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&s_CurrentTime));

	s_ImGuiIO.DeltaTime = static_cast<float>(s_CurrentTime - m_Time) / m_TicksPerSecond;
	m_Time = s_CurrentTime;

	s_ImGuiIO.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
	s_ImGuiIO.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
	s_ImGuiIO.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
	s_ImGuiIO.KeySuper = false;

	// Set mouse position
	if (m_ImguiHasFocus)
	{
		s_ImGuiIO.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

		if (auto* s_ForegroundWnd = GetForegroundWindow())
		{
			if (s_ForegroundWnd == m_Hwnd || IsChild(s_ForegroundWnd, m_Hwnd))
			{
				POINT s_CursorPos;

				if (GetCursorPos(&s_CursorPos) && ScreenToClient(m_Hwnd, &s_CursorPos))
					s_ImGuiIO.MousePos = ImVec2(static_cast<float>(s_CursorPos.x), static_cast<float>(s_CursorPos.y));
			}
		}
	}

	// Construct the UI.
	ImGui::NewFrame();

	ImGui::GetStyle().Alpha = m_ImguiHasFocus ? 1.f : 0.3f;

	ModSDK::GetInstance()->OnDrawUI(m_ImguiHasFocus);
}

void ImGuiRenderer::OnPresent(IDXGISwapChain3* p_SwapChain)
{
	if (!m_CommandQueue)
		return;
	
	if (!SetupRenderer(p_SwapChain))
	{
		Logger::Error("Failed to set up ImGui renderer.");
		return;
	}
	
	Draw();

	ImGui::Render();

	// Get context of next frame to render.
	auto& s_FrameCtx = m_FrameContext[++m_FrameCounter % m_FrameContext.size()];

	// If this context is still being rendered, we should wait for it.
	if (s_FrameCtx.FenceValue != 0 && s_FrameCtx.FenceValue > m_Fence->GetCompletedValue())
	{
		BreakIfFailed(m_Fence->SetEventOnCompletion(s_FrameCtx.FenceValue, m_FenceEvent.Handle));
		WaitForSingleObject(m_FenceEvent.Handle, INFINITE);
	}

	const auto s_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	// Reset command list and allocator.
	s_FrameCtx.CommandAllocator->Reset();
	BreakIfFailed(m_CommandList->Reset(s_FrameCtx.CommandAllocator, nullptr));

	// Transition the render target into the correct state to allow for drawing into it.
	const D3D12_RESOURCE_BARRIER s_RTBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_BackBuffers[s_BackBufferIndex],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	m_CommandList->ResourceBarrier(1, &s_RTBarrier);

	const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	const CD3DX12_CPU_DESCRIPTOR_HANDLE s_RtvDescriptor(s_RtvHandle, s_BackBufferIndex, m_RtvDescriptorSize);

	m_CommandList->OMSetRenderTargets(1, &s_RtvDescriptor, FALSE, nullptr);
	m_CommandList->SetDescriptorHeaps(1, &m_SrvDescriptorHeap.Ref);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList);

	const D3D12_RESOURCE_BARRIER s_PresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_BackBuffers[s_BackBufferIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	m_CommandList->ResourceBarrier(1, &s_PresentBarrier);
	BreakIfFailed(m_CommandList->Close());

	m_CommandQueue->ExecuteCommandLists(1, CommandListCast(&m_CommandList.Ref));
}

void ImGuiRenderer::PostPresent(IDXGISwapChain3* p_SwapChain, HRESULT p_PresentResult)
{
	if (!m_CommandQueue || !m_RendererSetup)
		return;

	if (p_PresentResult == DXGI_ERROR_DEVICE_REMOVED || p_PresentResult == DXGI_ERROR_DEVICE_RESET)
	{
		Logger::Error("Device lost after present.");
		abort();
	}
	else
	{
		FrameContext& s_FrameCtx = m_FrameContext[m_FrameCounter % MaxRenderedFrames];

		// Update the fence value for this frame and ask to receive a signal with this
		// fence value as soon as the GPU has finished rendering the frame. We update this
		// monotonically in order to always have the latest number represent the most
		// recently submitted frame, and in order to avoid having multiple frames share
		// the same fence value.
		s_FrameCtx.FenceValue = ++m_FenceValue;
		BreakIfFailed(m_CommandQueue->Signal(m_Fence, s_FrameCtx.FenceValue));
	}
}

void ImGuiRenderer::WaitForCurrentFrameToFinish() const
{
	if (m_FenceValue != 0 && m_FenceValue > m_Fence->GetCompletedValue())
	{
		BreakIfFailed(m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent.Handle));
		WaitForSingleObject(m_FenceEvent.Handle, INFINITE);
	}
}

bool ImGuiRenderer::SetupRenderer(IDXGISwapChain3* p_SwapChain)
{
	if (m_RendererSetup)
		return true;

	Logger::Debug("Setting up ImGui renderer.");

	ScopedD3DRef<ID3D12Device> s_Device;

	if (p_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK)
		return false;
	
	DXGI_SWAP_CHAIN_DESC1 s_SwapChainDesc;

	if (p_SwapChain->GetDesc1(&s_SwapChainDesc) != S_OK)
		return false;

	m_SwapChain = p_SwapChain;

	const auto s_BufferCount = s_SwapChainDesc.BufferCount;
	
	{
		D3D12_DESCRIPTOR_HEAP_DESC s_Desc = {};
		s_Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		s_Desc.NumDescriptors = s_BufferCount;
		s_Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		s_Desc.NodeMask = 0;

		if (s_Device->CreateDescriptorHeap(&s_Desc, IID_PPV_ARGS(m_RtvDescriptorHeap.ReleaseAndGetPtr())) != S_OK)
			return false;

		D3D_SET_OBJECT_NAME_A(m_RtvDescriptorHeap, "ZHMModSDK ImGui Rtv Descriptor Heap");
	}

	{
		D3D12_DESCRIPTOR_HEAP_DESC s_Desc = {};
		s_Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		s_Desc.NumDescriptors = s_BufferCount; // TODO: This looks like "total texture / shared resource view count" so we should increase based on number of textures we want to render.
		s_Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		s_Desc.NodeMask = 0;

		if (s_Device->CreateDescriptorHeap(&s_Desc, IID_PPV_ARGS(m_SrvDescriptorHeap.ReleaseAndGetPtr())) != S_OK)
			return false;

		D3D_SET_OBJECT_NAME_A(m_SrvDescriptorHeap, "ZHMModSDK ImGui Srv Descriptor Heap");
	}

	m_FrameContext.clear();

	for (UINT i = 0; i < MaxRenderedFrames; ++i)
	{
		FrameContext s_Frame {};
		
		if (s_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(s_Frame.CommandAllocator.ReleaseAndGetPtr())) != S_OK)
			return false;

		char s_CmdAllocDebugName[128];
		sprintf_s(s_CmdAllocDebugName, sizeof(s_CmdAllocDebugName), "ZHMModSDK ImGui Command Allocator #%u", i);
		D3D_SET_OBJECT_NAME_A(s_Frame.CommandAllocator, s_CmdAllocDebugName);

		s_Frame.FenceValue = 0;

		m_FrameContext.push_back(std::move(s_Frame));
	}

	// Create RTVs for back buffers.
	m_BackBuffers.clear();
	m_BackBuffers.resize(s_BufferCount);

	m_RtvDescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < s_BufferCount; ++i)
	{
		if (p_SwapChain->GetBuffer(i, IID_PPV_ARGS(m_BackBuffers[i].ReleaseAndGetPtr())) != S_OK)
			return false;

		const CD3DX12_CPU_DESCRIPTOR_HANDLE s_RtvDescriptor(s_RtvHandle, i, m_RtvDescriptorSize);
		s_Device->CreateRenderTargetView(m_BackBuffers[i], nullptr, s_RtvDescriptor);
	}

	if (s_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_FrameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(m_CommandList.ReleaseAndGetPtr())) != S_OK ||
		m_CommandList->Close() != S_OK)
		return false;

	char s_CmdListDebugName[128];
	sprintf_s(s_CmdListDebugName, sizeof(s_CmdListDebugName), "ZHMModSDK ImGui Command List");
	D3D_SET_OBJECT_NAME_A(m_CommandList, s_CmdListDebugName);

	if (s_Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.ReleaseAndGetPtr())) != S_OK)
		return false;

	char s_FenceDebugName[128];
	sprintf_s(s_FenceDebugName, sizeof(s_FenceDebugName), "ZHMModSDK ImGui Fence");
	D3D_SET_OBJECT_NAME_A(m_Fence, s_FenceDebugName);

	m_FenceEvent = CreateEventW(nullptr, false, false, nullptr);

	if (!m_FenceEvent)
		return false;

	if (p_SwapChain->GetHwnd(&m_Hwnd) != S_OK)
		return false;

	if (!ImGui_ImplDX12_Init(s_Device, MaxRenderedFrames, DXGI_FORMAT_R8G8B8A8_UNORM, m_SrvDescriptorHeap, m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()))
		return false;

	if (!ImGui_ImplDX12_CreateDeviceObjects())
		return false;

	SetupStyles();
	
	ImGuiIO& s_ImGuiIO = ImGui::GetIO();

	RECT s_Rect = { 0, 0, 0, 0 };
	GetClientRect(m_Hwnd, &s_Rect);
	
	s_ImGuiIO.DisplaySize = ImVec2(static_cast<float>(s_Rect.right - s_Rect.left), static_cast<float>(s_Rect.bottom - s_Rect.top));
	s_ImGuiIO.ImeWindowHandle = m_Hwnd;

	s_ImGuiIO.FontGlobalScale = (s_ImGuiIO.DisplaySize.y / 2048.f);
	
	m_RendererSetup = true;

	Logger::Debug("ImGui renderer successfully set up.");

	return true;
}

void ImGuiRenderer::OnReset()
{
	if (!m_RendererSetup)
		return;

	WaitForCurrentFrameToFinish();

	// Reset all fence values to latest fence value since we don't
	// really care about tracking any previous frames after a reset.
	// We only care about the last submitted frame having completed
	// (which means that all the previous ones have too).
	for (auto& s_Frame : m_FrameContext)
		s_Frame.FenceValue = m_FenceValue;

	m_BackBuffers.clear();

	ImGui_ImplDX12_InvalidateDeviceObjects();
}

void ImGuiRenderer::PostReset()
{
	if (!m_RendererSetup)
		return;
	
	DXGI_SWAP_CHAIN_DESC1 s_SwapChainDesc;

	if (m_SwapChain->GetDesc1(&s_SwapChainDesc) != S_OK)
		return;

	ScopedD3DRef<ID3D12Device> s_Device;

	if (m_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK)
		return;

	// Reset the back buffers.
	m_BackBuffers.resize(s_SwapChainDesc.BufferCount);
	
	m_RtvDescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < m_BackBuffers.size(); ++i)
	{
		if (m_SwapChain->GetBuffer(i, IID_PPV_ARGS(m_BackBuffers[i].ReleaseAndGetPtr())) != S_OK)
			return;

		const CD3DX12_CPU_DESCRIPTOR_HANDLE s_RtvDescriptor(s_RtvHandle, i, m_RtvDescriptorSize);
		s_Device->CreateRenderTargetView(m_BackBuffers[i], nullptr, s_RtvDescriptor);
	}

	// Re-create the ImGui D3D12 device objects.
	ImGui_ImplDX12_CreateDeviceObjects();

	// Set scaling parameters based on new view height.
	ImGuiIO& s_ImGuiIO = ImGui::GetIO();

	RECT s_Rect = { 0, 0, 0, 0 };
	GetClientRect(m_Hwnd, &s_Rect);

	s_ImGuiIO.DisplaySize = ImVec2(static_cast<float>(s_Rect.right - s_Rect.left), static_cast<float>(s_Rect.bottom - s_Rect.top));
	s_ImGuiIO.ImeWindowHandle = m_Hwnd;

	s_ImGuiIO.FontGlobalScale = (s_ImGuiIO.DisplaySize.y / 2048.f);
}

void ImGuiRenderer::SetCommandQueue(ID3D12CommandQueue* p_CommandQueue)
{
	if (m_CommandQueue == p_CommandQueue)
		return;

	if (m_CommandQueue)
	{
		m_CommandQueue->Release();
		m_CommandQueue = nullptr;
	}

	Logger::Debug("Setting up ImGui command queue.");
	m_CommandQueue = p_CommandQueue;
	m_CommandQueue->AddRef();
}

DECLARE_DETOUR_WITH_CONTEXT(ImGuiRenderer, LRESULT, WndProc, ZApplicationEngineWin32* th, HWND p_Hwnd, UINT p_Message, WPARAM p_Wparam, LPARAM p_Lparam)
{
	if (ImGui::GetCurrentContext() == nullptr)
		return HookResult<LRESULT>(HookAction::Continue());

	auto s_ScanCode = static_cast<uint8_t>(p_Lparam >> 16);

	// Toggle imgui input when user presses the grave / tilde key.
	if (s_ScanCode == 0x29 && (p_Message == WM_KEYDOWN || p_Message == WM_SYSKEYDOWN))
		m_ImguiHasFocus = !m_ImguiHasFocus;

	if (!m_ImguiHasFocus)
		return HookResult<LRESULT>(HookAction::Continue());

	// If we got a quit / close message then return control back to the process.
	if (p_Message == WM_QUIT || p_Message == WM_DESTROY || p_Message == WM_NCDESTROY || p_Message == WM_CLOSE)
	{
		m_ImguiHasFocus = false;
		return HookResult<LRESULT>(HookAction::Continue());
	}
	
	// Pass resizing messages down to the process.
	if (p_Message == WM_SIZE)
		return HookResult<LRESULT>(HookAction::Continue());
	
	ImGuiIO& s_ImGuiIO = ImGui::GetIO();

	switch (p_Message)
	{
		case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
		{
			int s_Button = 0;

			if (p_Message == WM_LBUTTONDOWN || p_Message == WM_LBUTTONDBLCLK)
				s_Button = 0;
			if (p_Message == WM_RBUTTONDOWN || p_Message == WM_RBUTTONDBLCLK)
				s_Button = 1;
			if (p_Message == WM_MBUTTONDOWN || p_Message == WM_MBUTTONDBLCLK)
				s_Button = 2;
			if (p_Message == WM_XBUTTONDOWN || p_Message == WM_XBUTTONDBLCLK)
				s_Button = (GET_XBUTTON_WPARAM(p_Wparam) == XBUTTON1) ? 3 : 4;

			if (!ImGui::IsAnyMouseDown() && GetCapture() == nullptr)
				SetCapture(p_Hwnd);

			s_ImGuiIO.MouseDown[s_Button] = true;

			break;
		}

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
		{
			int s_Button = 0;

			if (p_Message == WM_LBUTTONUP)
				s_Button = 0;
			if (p_Message == WM_RBUTTONUP)
				s_Button = 1;
			if (p_Message == WM_MBUTTONUP)
				s_Button = 2;

			if (p_Message == WM_XBUTTONUP)
				s_Button = (GET_XBUTTON_WPARAM(p_Wparam) == XBUTTON1) ? 3 : 4;

			s_ImGuiIO.MouseDown[s_Button] = false;

			if (!ImGui::IsAnyMouseDown() && GetCapture() == p_Hwnd)
				ReleaseCapture();

			break;
		}
		case WM_MOUSEWHEEL:
			s_ImGuiIO.MouseWheel += static_cast<float>(GET_WHEEL_DELTA_WPARAM(p_Wparam)) / static_cast<float>(WHEEL_DELTA);
			break;

		case WM_MOUSEHWHEEL:
			s_ImGuiIO.MouseWheelH += static_cast<float>(GET_WHEEL_DELTA_WPARAM(p_Wparam)) / static_cast<float>(WHEEL_DELTA);
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (p_Wparam < 256)
				s_ImGuiIO.KeysDown[p_Wparam] = true;

			break;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (p_Wparam < 256)
				s_ImGuiIO.KeysDown[p_Wparam] = false;

			break;

		case WM_CHAR:
			// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
			if (p_Wparam > 0 && p_Wparam < 0x10000)
				s_ImGuiIO.AddInputCharacterUTF16(static_cast<unsigned short>(p_Wparam));

			break;
	}

	// Don't call the original function so input isn't passed down to the game.
	return HookResult<LRESULT>(HookAction::Return(), DefWindowProcW(p_Hwnd, p_Message, p_Wparam, p_Lparam));
}

DECLARE_DETOUR_WITH_CONTEXT(ImGuiRenderer, void, ZKeyboardWindows_Update, ZKeyboardWindows*, bool)
{
	// Don't process input while the imgui overlay has focus.
	if (m_ImguiHasFocus)
		return HookResult<void>(HookAction::Return());

	return HookResult<void>(HookAction::Continue());
}

DECLARE_DETOUR_WITH_CONTEXT(ImGuiRenderer, double, ZInputAction_Analog, ZInputAction* th, int a2)
{
	static std::unordered_set<std::string> s_BlockedInputs = {
		"eIAKBMLookHorizontal",
		"eIAKBMLookVertical",
		"TiltCamera",
		"TurnCamera",
		"AnalogLeftX",
		"AnalogLeftY",
		"AnalogRightY",
		"AnalogRightX",
		"eIAStickRightHorizontal_Analog",
		"eIAStickRightVertical_Analog",
		"eIAStickLeftHorizontal_Analog",
		"eIAStickLeftVertical_Analog",
		"eIAStickLeftHorizontal_Raw",
		"eIAStickLeftVertical_Raw",
		"eIAStickRightHorizontal_Raw",
		"eIAStickRightVertical_Raw",
	};

	// Don't allow moving the camera / character while the imgui overlay has focus.
	if (m_ImguiHasFocus)
	{
		if (s_BlockedInputs.contains(th->m_szName))
			return HookResult(HookAction::Return(), 0.0);
	}

	return HookResult<double>(HookAction::Continue());
}
