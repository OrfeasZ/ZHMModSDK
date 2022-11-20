// Thanks to SpecialK source code and its creator (https://discourse.differentk.fyi/)
// for providing some helpful pointers on how to properly sync rendering.

#include "ImGuiRenderer.h"

#include <d3dcompiler.h>
#include <dxgi1_4.h>

#include <imgui.h>
#include <backends/imgui_impl_dx12.h>

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
	InitializeSRWLock(&m_Lock);

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
	Shutdown();
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

void ImGuiRenderer::Shutdown()
{
	HookRegistry::ClearDetoursWithContext(this);
	OnReset();
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
	ScopedSharedGuard s_Guard(&m_Lock);
	
	if (!SetupRenderer(p_SwapChain))
	{
		Logger::Error("Failed to set up ImGui renderer.");
		OnReset();
		return;
	}

	if (m_SwapChain != p_SwapChain)
		return;

	if (!Globals::RenderManager->m_pDevice->m_pCommandQueue)
		return;

	const auto s_BackBufferIndex = p_SwapChain->GetCurrentBackBufferIndex();
	auto& s_Frame = m_FrameContext[s_BackBufferIndex];

	if (s_Frame.Fence->GetCompletedValue() < s_Frame.FenceValue)
	{
		if (SUCCEEDED(s_Frame.Fence->SetEventOnCompletion(s_Frame.FenceValue, s_Frame.FenceEvent)))
		{
			WaitForSingleObject(s_Frame.FenceEvent, INFINITE);
		}
	}

	if (!s_Frame.Recording)
	{
		s_Frame.CommandAllocator->Reset();

		s_Frame.Recording = SUCCEEDED(s_Frame.CommandList->Reset(s_Frame.CommandAllocator, nullptr));

		if (s_Frame.Recording)
			s_Frame.CommandList->SetPredication(nullptr, 0, D3D12_PREDICATION_OP_EQUAL_ZERO);

		if (!s_Frame.Recording)
		{
			Logger::Warn("Could not reset command list of frame {}.", s_BackBufferIndex);
			return;
		}
	}

	{
		D3D12_RESOURCE_BARRIER s_Barrier = {};
		s_Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		s_Barrier.Transition.pResource = s_Frame.BackBuffer;
		s_Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		s_Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		s_Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		s_Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		s_Frame.CommandList->ResourceBarrier(1, &s_Barrier);
	}

	// Render
	s_Frame.CommandList->OMSetRenderTargets(1, &s_Frame.DescriptorHandle, false, nullptr);
	s_Frame.CommandList->SetDescriptorHeaps(1, &m_SrvDescriptorHeap);

	Draw();

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), s_Frame.CommandList);

	{
		D3D12_RESOURCE_BARRIER s_Barrier = {};
		s_Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		s_Barrier.Transition.pResource = s_Frame.BackBuffer;
		s_Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		s_Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		s_Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		s_Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		s_Frame.CommandList->ResourceBarrier(1, &s_Barrier);
	}

	ExecuteCmdList(&s_Frame);

	const auto s_SyncValue = s_Frame.FenceValue + 1;

	if (SUCCEEDED(Globals::RenderManager->m_pDevice->m_pCommandQueue->Signal(s_Frame.Fence, s_SyncValue)))
		s_Frame.FenceValue = s_SyncValue;
}

bool ImGuiRenderer::SetupRenderer(IDXGISwapChain3* p_SwapChain)
{
	if (m_RendererSetup)
		return true;

	Logger::Debug("Setting up ImGui renderer.");

	m_SwapChain = p_SwapChain;

	ScopedD3DRef<ID3D12Device> s_Device;

	if (p_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK)
		return false;

	DXGI_SWAP_CHAIN_DESC1 s_SwapChainDesc;

	if (p_SwapChain->GetDesc1(&s_SwapChainDesc) != S_OK)
		return false;

	m_BufferCount = s_SwapChainDesc.BufferCount;
	m_FrameContext = new FrameContext[m_BufferCount];

	{
		D3D12_DESCRIPTOR_HEAP_DESC s_Desc = {};
		s_Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		s_Desc.NumDescriptors = m_BufferCount;
		s_Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		s_Desc.NodeMask = 0;

		if (s_Device->CreateDescriptorHeap(&s_Desc, IID_PPV_ARGS(&m_RtvDescriptorHeap)) != S_OK)
			return false;

		D3D_SET_OBJECT_NAME_A(m_RtvDescriptorHeap, "ZHMModSDK ImGui Rtv Descriptor Heap");
	}

	{
		D3D12_DESCRIPTOR_HEAP_DESC s_Desc = {};
		s_Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		s_Desc.NumDescriptors = 1;
		s_Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		s_Desc.NodeMask = 0;

		if (s_Device->CreateDescriptorHeap(&s_Desc, IID_PPV_ARGS(&m_SrvDescriptorHeap)) != S_OK)
			return false;

		D3D_SET_OBJECT_NAME_A(m_SrvDescriptorHeap, "ZHMModSDK ImGui Srv Descriptor Heap");
	}

	const auto s_RtvDescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < m_BufferCount; ++i)
	{
		auto& s_Frame = m_FrameContext[i];

		s_Frame.Index = i;

		if (s_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&s_Frame.Fence)) != S_OK)
			return false;

		char s_FenceDebugName[128];
		sprintf_s(s_FenceDebugName, sizeof(s_FenceDebugName), "ZHMModSDK ImGui Fence #%u", i);
		D3D_SET_OBJECT_NAME_A(s_Frame.Fence, s_FenceDebugName);

		if (s_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s_Frame.CommandAllocator)) != S_OK)
			return false;

		char s_CmdAllocDebugName[128];
		sprintf_s(s_CmdAllocDebugName, sizeof(s_CmdAllocDebugName), "ZHMModSDK ImGui Command Allocator #%u", i);
		D3D_SET_OBJECT_NAME_A(s_Frame.CommandAllocator, s_CmdAllocDebugName);

		if (s_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, s_Frame.CommandAllocator, nullptr, IID_PPV_ARGS(&s_Frame.CommandList)) != S_OK ||
			s_Frame.CommandList->Close() != S_OK)
			return false;

		char s_CmdListDebugName[128];
		sprintf_s(s_CmdListDebugName, sizeof(s_CmdListDebugName), "ZHMModSDK ImGui Command List #%u", i);
		D3D_SET_OBJECT_NAME_A(s_Frame.CommandList, s_CmdListDebugName);

		if (p_SwapChain->GetBuffer(i, IID_PPV_ARGS(&s_Frame.BackBuffer)) != S_OK)
			return false;

		s_Frame.DescriptorHandle.ptr = s_RtvHandle.ptr + (i * s_RtvDescriptorSize);

		s_Device->CreateRenderTargetView(s_Frame.BackBuffer, nullptr, s_Frame.DescriptorHandle);

		s_Frame.FenceValue = 0;
		s_Frame.FenceEvent = CreateEventA(nullptr, false, false, nullptr);

		s_Frame.Recording = false;
	}

	if (p_SwapChain->GetHwnd(&m_Hwnd) != S_OK)
		return false;

	if (!ImGui_ImplDX12_Init(s_Device, m_BufferCount, DXGI_FORMAT_R8G8B8A8_UNORM, m_SrvDescriptorHeap, m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()))
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
	ScopedExclusiveGuard s_Guard(&m_Lock);

	Logger::Debug("Resetting ImGui renderer.");

	if (m_RendererSetup)
	{
		ImGui_ImplDX12_InvalidateDeviceObjects();
		ImGui_ImplDX12_Shutdown();
	}

	m_RendererSetup = false;

	for (UINT i = 0; i < m_BufferCount; i++)
	{
		auto& s_Frame = m_FrameContext[i];

		if (m_SwapChain && m_SwapChain->GetCurrentBackBufferIndex() != s_Frame.Index)
			s_Frame.Recording = false;

		WaitForGpu(&s_Frame);

		if (s_Frame.CommandList)
		{
			s_Frame.CommandList->Release();
			s_Frame.CommandList = nullptr;
		}

		if (s_Frame.CommandAllocator)
		{
			s_Frame.CommandAllocator->Release();
			s_Frame.CommandAllocator = nullptr;
		}

		if (s_Frame.BackBuffer)
		{
			s_Frame.BackBuffer->Release();
			s_Frame.BackBuffer = nullptr;
		}

		if (s_Frame.Fence)
		{
			s_Frame.Fence->Release();
			s_Frame.Fence = nullptr;
		}

		if (s_Frame.FenceEvent)
		{
			CloseHandle(s_Frame.FenceEvent);
			s_Frame.FenceEvent = nullptr;
		}

		s_Frame.FenceValue = 0;
		s_Frame.Recording = false;
		s_Frame.DescriptorHandle = { 0 };
	}

	delete[] m_FrameContext;
	m_FrameContext = nullptr;

	//m_CommandQueue = nullptr;

	if (m_RtvDescriptorHeap)
	{
		m_RtvDescriptorHeap->Release();
		m_RtvDescriptorHeap = nullptr;
	}

	if (m_SrvDescriptorHeap)
	{
		m_SrvDescriptorHeap->Release();
		m_SrvDescriptorHeap = nullptr;
	}

	m_SwapChain = nullptr;
	m_BufferCount = 0;
	m_Hwnd = nullptr;
}

void ImGuiRenderer::SetCommandQueue(ID3D12CommandQueue* p_CommandQueue)
{
	/*if (m_Shutdown)
		return;

	if (!m_RendererSetup || m_CommandQueue || !m_SwapChain)
		return;
	
	m_CommandQueue = Globals::RenderManager->m_pDevice->m_pCommandQueue;*/
}

void ImGuiRenderer::WaitForGpu(FrameContext* p_Frame)
{
	if (p_Frame->Recording)
		ExecuteCmdList(p_Frame);

	if (!p_Frame->FenceEvent || !Globals::RenderManager->m_pDevice->m_pCommandQueue)
		return;

	const auto s_SyncValue = p_Frame->FenceValue + 1;

	if (FAILED(Globals::RenderManager->m_pDevice->m_pCommandQueue->Signal(p_Frame->Fence, s_SyncValue)))
		return;

	if (SUCCEEDED(p_Frame->Fence->SetEventOnCompletion(s_SyncValue, p_Frame->FenceEvent)))
		WaitForSingleObject(p_Frame->FenceEvent, INFINITE);

	p_Frame->FenceValue = s_SyncValue;
}

void ImGuiRenderer::ExecuteCmdList(FrameContext* p_Frame)
{
	if (FAILED(p_Frame->CommandList->Close()))
		return;

	p_Frame->Recording = false;

	ID3D12CommandList* const s_CommandLists[] = {
		p_Frame->CommandList,
	};

	Globals::RenderManager->m_pDevice->m_pCommandQueue->ExecuteCommandLists(1, s_CommandLists);
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
