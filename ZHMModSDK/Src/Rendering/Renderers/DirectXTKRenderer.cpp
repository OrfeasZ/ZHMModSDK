#include <directx/d3dx12.h>

#include "DirectXTKRenderer.h"

#include <d3dcompiler.h>
#include <dxgi1_4.h>

#include "Hooks.h"
#include "Logging.h"

#include <Glacier/ZApplicationEngineWin32.h>
#include <Glacier/ZDelegate.h>

#include <UI/Console.h>

#include "CommonStates.h"
#include "ResourceUploadBatch.h"
#include "DirectXHelpers.h"
#include "Functions.h"
#include "Globals.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZActor.h"
#include "Glacier/ZRender.h"
#include "Glacier/ZCameraEntity.h"
#include "Rendering/D3DUtils.h"
#include "Fonts.h"
#include "ModSDK.h"
#include "Glacier/ZGameLoopManager.h"

using namespace Rendering::Renderers;

DirectXTKRenderer::DirectXTKRenderer()
{
}

DirectXTKRenderer::~DirectXTKRenderer()
{
	const ZMemberDelegate<DirectXTKRenderer, void(const SGameUpdateEvent&)> s_Delegate(this, &DirectXTKRenderer::OnFrameUpdate);
	Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);

	OnReset();

	if (m_CommandQueue)
	{
		m_CommandQueue->Release();
		m_CommandQueue = nullptr;
	}
}

void DirectXTKRenderer::OnEngineInit()
{
	const ZMemberDelegate<DirectXTKRenderer, void(const SGameUpdateEvent&)> s_Delegate(this, &DirectXTKRenderer::OnFrameUpdate);
	Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, INT_MAX, EUpdateMode::eUpdateAlways);
}

void DirectXTKRenderer::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
}

void DirectXTKRenderer::Draw()
{
	ID3D12DescriptorHeap* s_Heaps[] = { m_ResourceDescriptors->Heap() };
	m_CommandList->SetDescriptorHeaps(static_cast<UINT>(std::size(s_Heaps)), s_Heaps);

	m_SpriteBatch->Begin(m_CommandList);

	m_LineEffect->Apply(m_CommandList);

	m_LineBatch->Begin(m_CommandList);

	ModSDK::GetInstance()->OnDraw3D();

	m_LineBatch->End();

	m_SpriteBatch->End();
}

void DirectXTKRenderer::OnPresent(IDXGISwapChain3* p_SwapChain)
{
	if (!m_CommandQueue)
		return;

	if (!SetupRenderer(p_SwapChain))
	{
		Logger::Error("Failed to set up DirectXTK renderer.");
		OnReset();
		return;
	}
	
	if (const auto s_CurrentCamera = Functions::GetCurrentCamera->Call())
	{
		const auto s_ViewMatrix = s_CurrentCamera->GetViewMatrix();
		const SMatrix s_ProjectionMatrix = *s_CurrentCamera->GetProjectionMatrix();

		m_View = *reinterpret_cast<DirectX::FXMMATRIX*>(&s_ViewMatrix);
		m_Projection = *reinterpret_cast<DirectX::FXMMATRIX*>(&s_ProjectionMatrix);

		m_ViewProjection = m_View * m_Projection;
		m_ProjectionViewInverse = (m_Projection * m_View).Invert();

		if (m_RendererSetup)
		{
			m_LineEffect->SetView(m_View);
			m_LineEffect->SetProjection(m_Projection);
		}
	}

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

	D3D12_VIEWPORT s_Viewport = { 0.0f, 0.0f, m_WindowWidth, m_WindowHeight, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	m_CommandList->RSSetViewports(1, &s_Viewport);

	D3D12_RECT s_ScissorRect = { 0, 0, static_cast<LONG>(m_WindowWidth), static_cast<LONG>(m_WindowHeight) };
	m_CommandList->RSSetScissorRects(1, &s_ScissorRect);

	const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	const CD3DX12_CPU_DESCRIPTOR_HANDLE s_RtvDescriptor(s_RtvHandle, s_BackBufferIndex, m_RtvDescriptorSize);

	m_CommandList->OMSetRenderTargets(1, &s_RtvDescriptor, false, nullptr);

	Draw();

	const D3D12_RESOURCE_BARRIER s_PresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_BackBuffers[s_BackBufferIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	m_CommandList->ResourceBarrier(1, &s_PresentBarrier);
	BreakIfFailed(m_CommandList->Close());

	m_CommandQueue->ExecuteCommandLists(1, CommandListCast(&m_CommandList.Ref));
}

void DirectXTKRenderer::PostPresent(IDXGISwapChain3* p_SwapChain, HRESULT p_PresentResult)
{
	if (!m_RendererSetup || !m_CommandQueue)
		return;

	if (p_PresentResult == DXGI_ERROR_DEVICE_REMOVED || p_PresentResult == DXGI_ERROR_DEVICE_RESET)
	{
		Logger::Error("Device lost after present.");
		abort();
	}
	else
	{
		m_GraphicsMemory->Commit(m_CommandQueue);

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

void DirectXTKRenderer::WaitForCurrentFrameToFinish() const
{
	if (m_FenceValue != 0 && m_FenceValue > m_Fence->GetCompletedValue())
	{
		BreakIfFailed(m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent.Handle));
		WaitForSingleObject(m_FenceEvent.Handle, INFINITE);
	}
}

bool DirectXTKRenderer::SetupRenderer(IDXGISwapChain3* p_SwapChain)
{
	if (m_RendererSetup)
		return true;

	Logger::Debug("Setting up DirectXTK renderer.");
	
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

		D3D_SET_OBJECT_NAME_A(m_RtvDescriptorHeap, "ZHMModSDK DirectXTK Rtv Descriptor Heap");
	}

	m_FrameContext.clear();

	for (UINT i = 0; i < MaxRenderedFrames; ++i)
	{
		FrameContext s_Frame {};

		if (s_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(s_Frame.CommandAllocator.ReleaseAndGetPtr())) != S_OK)
			return false;

		char s_CmdAllocDebugName[128];
		sprintf_s(s_CmdAllocDebugName, sizeof(s_CmdAllocDebugName), "ZHMModSDK DirectXTK Command Allocator #%u", i);
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
	sprintf_s(s_CmdListDebugName, sizeof(s_CmdListDebugName), "ZHMModSDK DirectXTK Command List");
	D3D_SET_OBJECT_NAME_A(m_CommandList, s_CmdListDebugName);

	if (s_Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.ReleaseAndGetPtr())) != S_OK)
		return false;

	char s_FenceDebugName[128];
	sprintf_s(s_FenceDebugName, sizeof(s_FenceDebugName), "ZHMModSDK DirectXTK Fence");
	D3D_SET_OBJECT_NAME_A(m_Fence, s_FenceDebugName);

	m_FenceEvent = CreateEventW(nullptr, false, false, nullptr);

	if (!m_FenceEvent)
		return false;

	if (p_SwapChain->GetHwnd(&m_Hwnd) != S_OK)
		return false;

	RECT s_Rect = { 0, 0, 0, 0 };
	GetClientRect(m_Hwnd, &s_Rect);

	m_WindowWidth = static_cast<float>(s_Rect.right - s_Rect.left);
	m_WindowHeight = static_cast<float>(s_Rect.bottom - s_Rect.top);

	m_GraphicsMemory = std::make_unique<DirectX::GraphicsMemory>(s_Device);

	m_LineBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(s_Device);

	{
		const DirectX::RenderTargetState s_RtState(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT);

		DirectX::EffectPipelineStateDescription s_Desc(
			&DirectX::VertexPositionColor::InputLayout,
			DirectX::CommonStates::Opaque,
			DirectX::CommonStates::DepthDefault,
			DirectX::CommonStates::CullNone,
			s_RtState,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

		m_LineEffect = std::make_unique<DirectX::BasicEffect>(s_Device, DirectX::EffectFlags::VertexColor, s_Desc);
	}

	{
		m_ResourceDescriptors = std::make_unique<DirectX::DescriptorHeap>(s_Device, static_cast<int>(Descriptors::Count));

		DirectX::ResourceUploadBatch s_ResourceUpload(s_Device);

		s_ResourceUpload.Begin();

		m_Font = std::make_unique<DirectX::SpriteFont>(
			s_Device,
			s_ResourceUpload,
			RobotoRegularSpritefont_data,
			RobotoRegularSpritefont_size,
			m_ResourceDescriptors->GetCpuHandle(static_cast<int>(Descriptors::FontRegular)),
			m_ResourceDescriptors->GetGpuHandle(static_cast<int>(Descriptors::FontRegular))
		);

		DirectX::RenderTargetState s_RtState(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT);

		DirectX::SpriteBatchPipelineStateDescription s_Desc(s_RtState);
		m_SpriteBatch = std::make_unique<DirectX::SpriteBatch>(s_Device, s_ResourceUpload, s_Desc);

		s_ResourceUpload.End(m_CommandQueue).wait();
		
		const D3D12_VIEWPORT s_Viewport = { 0.0f, 0.0f, m_WindowWidth, m_WindowHeight, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
		m_SpriteBatch->SetViewport(s_Viewport);
	}

	m_LineEffect->SetWorld(m_World);
	m_LineEffect->SetView(m_View);
	m_LineEffect->SetProjection(m_Projection);

	m_RendererSetup = true;

	Logger::Debug("DirectXTK renderer successfully set up.");

	return true;
}

void DirectXTKRenderer::OnReset()
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
}

void DirectXTKRenderer::PostReset()
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

	RECT s_Rect = { 0, 0, 0, 0 };
	GetClientRect(m_Hwnd, &s_Rect);

	m_WindowWidth = static_cast<float>(s_Rect.right - s_Rect.left);
	m_WindowHeight = static_cast<float>(s_Rect.bottom - s_Rect.top);

	const D3D12_VIEWPORT s_Viewport = { 0.0f, 0.0f, m_WindowWidth, m_WindowHeight, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	m_SpriteBatch->SetViewport(s_Viewport);
}

void DirectXTKRenderer::SetCommandQueue(ID3D12CommandQueue* p_CommandQueue)
{
	if (m_CommandQueue == p_CommandQueue)
		return;

	if (m_CommandQueue)
	{
		m_CommandQueue->Release();
		m_CommandQueue = nullptr;
	}

	Logger::Debug("Setting up DirectXTK12 command queue.");
	m_CommandQueue = p_CommandQueue;
	m_CommandQueue->AddRef();
}

void DirectXTKRenderer::DrawLine3D(const SVector3& p_From, const SVector3& p_To, const SVector4& p_FromColor, const SVector4& p_ToColor)
{
	if (!m_RendererSetup)
		return;

	DirectX::VertexPositionColor s_From(
		DirectX::SimpleMath::Vector3(p_From.x, p_From.y, p_From.z),
		DirectX::SimpleMath::Vector4(p_FromColor.x, p_FromColor.y, p_FromColor.z, p_FromColor.w)
	);

	DirectX::VertexPositionColor s_To(
		DirectX::SimpleMath::Vector3(p_To.x, p_To.y, p_To.z),
		DirectX::SimpleMath::Vector4(p_ToColor.x, p_ToColor.y, p_ToColor.z, p_ToColor.w)
	);

	m_LineBatch->DrawLine(s_From, s_To);
}

void DirectXTKRenderer::DrawText2D(const ZString& p_Text, const SVector2& p_Pos, const SVector4& p_Color, float p_Rotation/* = 0.f*/, float p_Scale/* = 1.f*/, TextAlignment p_Alignment/* = TextAlignment::Center*/)
{
	if (!m_RendererSetup)
		return;

	const std::string s_Text(p_Text.c_str(), p_Text.size());
	const DirectX::SimpleMath::Vector2 s_StringSize = m_Font->MeasureString(s_Text.c_str());

	DirectX::SimpleMath::Vector2 s_Origin(0.f, 0.f);

	if (p_Alignment == TextAlignment::Center)
		s_Origin.x = s_StringSize.x / 2.f;
	else if (p_Alignment == TextAlignment::Right)
		s_Origin.x = s_StringSize.x;

	m_Font->DrawString(
		m_SpriteBatch.get(),
		s_Text.c_str(),
		DirectX::SimpleMath::Vector2(p_Pos.x, p_Pos.y),
		DirectX::SimpleMath::Vector4(p_Color.x, p_Color.y, p_Color.z, p_Color.w),
		p_Rotation,
		s_Origin,
		p_Scale
	);
}

bool DirectXTKRenderer::WorldToScreen(const SVector3& p_WorldPos, SVector2& p_Out)
{
	if (!m_RendererSetup)
		return false;

	const DirectX::SimpleMath::Vector4 s_World(p_WorldPos.x, p_WorldPos.y, p_WorldPos.z, 1.f);
	const DirectX::SimpleMath::Vector4 s_Projected = DirectX::XMVector4Transform(s_World, m_ViewProjection);

	if (s_Projected.w <= 0.000001f)
		return false;

	const float s_InvertedZ = 1.f / s_Projected.w;
	const DirectX::SimpleMath::Vector3 s_FinalProjected(s_Projected.x * s_InvertedZ, s_Projected.y * s_InvertedZ, s_Projected.z * s_InvertedZ);

	p_Out.x = (1.f + s_FinalProjected.x) * 0.5f * m_WindowWidth;
	p_Out.y = (1.f - s_FinalProjected.y) * 0.5f * m_WindowHeight;

	return true;
}

bool DirectXTKRenderer::ScreenToWorld(const SVector2& p_ScreenPos, SVector3& p_WorldPosOut, SVector3& p_DirectionOut)
{
	if (!m_RendererSetup)
		return false;

	const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

	if (!s_CurrentCamera)
		return false;

	auto s_CameraTrans = s_CurrentCamera->GetWorldMatrix();

	auto s_ScreenPos = DirectX::SimpleMath::Vector3((2.0f * p_ScreenPos.x) / m_WindowWidth - 1.0f, 1.0f - (2.0f * p_ScreenPos.y) / m_WindowHeight, 1.f);
	auto s_RayClip = DirectX::SimpleMath::Vector4(s_ScreenPos.x, s_ScreenPos.y, 0.f, 1.f);

	DirectX::SimpleMath::Vector4 s_RayEye = DirectX::XMVector4Transform(s_RayClip, m_Projection.Invert());
	s_RayEye.z = -1.f;
	s_RayEye.w = 0.f;

	DirectX::SimpleMath::Vector4 s_RayWorld = DirectX::XMVector4Transform(s_RayEye, m_View.Invert());
	s_RayWorld.Normalize();

	p_WorldPosOut.x = s_CameraTrans.Trans.x + s_RayWorld.x;
	p_WorldPosOut.y = s_CameraTrans.Trans.y + s_RayWorld.y;
	p_WorldPosOut.z = s_CameraTrans.Trans.z + s_RayWorld.z;

	p_DirectionOut.x = s_RayWorld.x;
	p_DirectionOut.y = s_RayWorld.y;
	p_DirectionOut.z = s_RayWorld.z;

	return true;
}

void DirectXTKRenderer::DrawBox3D(const SVector3& p_Min, const SVector3& p_Max, const SVector4& p_Color)
{
	SVector3 s_Corners[] = {
		SVector3(p_Min.x, p_Min.y, p_Min.z),
		SVector3(p_Min.x, p_Max.y, p_Min.z),
		SVector3(p_Max.x, p_Max.y, p_Min.z),
		SVector3(p_Max.x, p_Min.y, p_Min.z),
		SVector3(p_Max.x, p_Max.y, p_Max.z),
		SVector3(p_Min.x, p_Max.y, p_Max.z),
		SVector3(p_Min.x, p_Min.y, p_Max.z),
		SVector3(p_Max.x, p_Min.y, p_Max.z),
	};

	DrawLine3D(s_Corners[0], s_Corners[1], p_Color, p_Color);
	DrawLine3D(s_Corners[1], s_Corners[2], p_Color, p_Color);
	DrawLine3D(s_Corners[2], s_Corners[3], p_Color, p_Color);
	DrawLine3D(s_Corners[3], s_Corners[0], p_Color, p_Color);

	DrawLine3D(s_Corners[4], s_Corners[5], p_Color, p_Color);
	DrawLine3D(s_Corners[5], s_Corners[6], p_Color, p_Color);
	DrawLine3D(s_Corners[6], s_Corners[7], p_Color, p_Color);
	DrawLine3D(s_Corners[7], s_Corners[4], p_Color, p_Color);

	DrawLine3D(s_Corners[1], s_Corners[5], p_Color, p_Color);
	DrawLine3D(s_Corners[0], s_Corners[6], p_Color, p_Color);

	DrawLine3D(s_Corners[2], s_Corners[4], p_Color, p_Color);
	DrawLine3D(s_Corners[3], s_Corners[7], p_Color, p_Color);
}

inline SVector3 XMVecToSVec3(const DirectX::XMVECTOR& p_Vec)
{
	return SVector3(DirectX::XMVectorGetX(p_Vec), DirectX::XMVectorGetY(p_Vec), DirectX::XMVectorGetZ(p_Vec));
}

void DirectXTKRenderer::DrawOBB3D(const SVector3& p_Min, const SVector3& p_Max, const SMatrix& p_Transform, const SVector4& p_Color)
{
	const auto s_Transform = *reinterpret_cast<DirectX::FXMMATRIX*>(&p_Transform);

	DirectX::XMVECTOR s_Corners[] = {
		DirectX::XMVector3Transform(DirectX::SimpleMath::Vector3(p_Min.x, p_Min.y, p_Min.z), s_Transform),
		DirectX::XMVector3Transform(DirectX::SimpleMath::Vector3(p_Min.x, p_Max.y, p_Min.z), s_Transform),
		DirectX::XMVector3Transform(DirectX::SimpleMath::Vector3(p_Max.x, p_Max.y, p_Min.z), s_Transform),
		DirectX::XMVector3Transform(DirectX::SimpleMath::Vector3(p_Max.x, p_Min.y, p_Min.z), s_Transform),
		DirectX::XMVector3Transform(DirectX::SimpleMath::Vector3(p_Max.x, p_Max.y, p_Max.z), s_Transform),
		DirectX::XMVector3Transform(DirectX::SimpleMath::Vector3(p_Min.x, p_Max.y, p_Max.z), s_Transform),
		DirectX::XMVector3Transform(DirectX::SimpleMath::Vector3(p_Min.x, p_Min.y, p_Max.z), s_Transform),
		DirectX::XMVector3Transform(DirectX::SimpleMath::Vector3(p_Max.x, p_Min.y, p_Max.z), s_Transform),
	};

	DrawLine3D(XMVecToSVec3(s_Corners[0]), XMVecToSVec3(s_Corners[1]), p_Color, p_Color);
	DrawLine3D(XMVecToSVec3(s_Corners[1]), XMVecToSVec3(s_Corners[2]), p_Color, p_Color);
	DrawLine3D(XMVecToSVec3(s_Corners[2]), XMVecToSVec3(s_Corners[3]), p_Color, p_Color);
	DrawLine3D(XMVecToSVec3(s_Corners[3]), XMVecToSVec3(s_Corners[0]), p_Color, p_Color);

	DrawLine3D(XMVecToSVec3(s_Corners[4]), XMVecToSVec3(s_Corners[5]), p_Color, p_Color);
	DrawLine3D(XMVecToSVec3(s_Corners[5]), XMVecToSVec3(s_Corners[6]), p_Color, p_Color);
	DrawLine3D(XMVecToSVec3(s_Corners[6]), XMVecToSVec3(s_Corners[7]), p_Color, p_Color);
	DrawLine3D(XMVecToSVec3(s_Corners[7]), XMVecToSVec3(s_Corners[4]), p_Color, p_Color);

	DrawLine3D(XMVecToSVec3(s_Corners[1]), XMVecToSVec3(s_Corners[5]), p_Color, p_Color);
	DrawLine3D(XMVecToSVec3(s_Corners[0]), XMVecToSVec3(s_Corners[6]), p_Color, p_Color);

	DrawLine3D(XMVecToSVec3(s_Corners[2]), XMVecToSVec3(s_Corners[4]), p_Color, p_Color);
	DrawLine3D(XMVecToSVec3(s_Corners[3]), XMVecToSVec3(s_Corners[7]), p_Color, p_Color);
}
