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
#include "D3DUtils.h"
#include "Fonts.h"
#include "ModSDK.h"
#include "Glacier/ZGameLoopManager.h"
#include "Glacier/MDF_FONT.h"

#undef min
#undef max

using namespace Rendering::Renderers;

DirectXTKRenderer::DirectXTKRenderer() {}

DirectXTKRenderer::~DirectXTKRenderer() {
    const ZMemberDelegate<DirectXTKRenderer, void(const SGameUpdateEvent&)> s_Delegate(
        this, &DirectXTKRenderer::OnFrameUpdate
    );
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdateAlways);

    OnReset();

    if (m_CommandQueue) {
        m_CommandQueue->Release();
        m_CommandQueue = nullptr;
    }
}

void DirectXTKRenderer::OnEngineInit() {
    const ZMemberDelegate<DirectXTKRenderer, void(const SGameUpdateEvent&)> s_Delegate(
        this, &DirectXTKRenderer::OnFrameUpdate
    );
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, INT_MAX, EUpdateMode::eUpdateAlways);
}

void DirectXTKRenderer::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {}

void DirectXTKRenderer::Draw() {
    const auto s_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
    const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    const CD3DX12_CPU_DESCRIPTOR_HANDLE s_RtvDescriptor(s_RtvHandle, s_BackBufferIndex, m_RtvDescriptorSize);

    m_CommandList->OMSetRenderTargets(1, &s_RtvDescriptor, false, nullptr);

    m_TriangleBatch->Begin(m_CommandList);
    m_LineBatch->Begin(m_CommandList);
    m_TextBatch->Begin(m_CommandList);

    ModSDK::GetInstance()->OnDraw3D();

    m_TriangleBatch->End();
    m_LineBatch->End();
    m_TextBatch->End();

    ID3D12DescriptorHeap* s_Heaps[] = {m_ResourceDescriptors->Heap()};

    m_CommandList->SetDescriptorHeaps(static_cast<UINT>(std::size(s_Heaps)), s_Heaps);

    m_SpriteBatch->Begin(m_CommandList);

    for (const auto& s_2DText : m_Text2DBuffer) {
        DrawText2D(s_2DText);
    }

    m_SpriteBatch->End();

    m_Text2DBuffer.clear();
}

void DirectXTKRenderer::DepthDraw() {
    const auto s_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    const CD3DX12_CPU_DESCRIPTOR_HANDLE s_RtvDescriptor(s_RtvHandle, s_BackBufferIndex, m_RtvDescriptorSize);

    const auto s_DsvHandle = Globals::RenderManager->m_pDevice->m_pDescriptorHeapDSV->
                                                     GetCPUDescriptorHandleForHeapStart();
    const CD3DX12_CPU_DESCRIPTOR_HANDLE s_DsvDescriptor(s_DsvHandle, *m_DsvIndex, m_DsvDescriptorSize);

    m_CommandList->OMSetRenderTargets(1, &s_RtvDescriptor, false, &s_DsvDescriptor);

    m_TriangleBatch->Begin(m_CommandList);
    m_LineBatch->Begin(m_CommandList);
    m_TextBatch->Begin(m_CommandList);

    ModSDK::GetInstance()->OnDepthDraw3D();

    m_TriangleBatch->End();
    m_LineBatch->End();
    m_TextBatch->End();
}

void DirectXTKRenderer::OnPresent(IDXGISwapChain3* p_SwapChain) {
    if (!m_CommandQueue)
        return;

    if (!SetupRenderer(p_SwapChain)) {
        Logger::Error("Failed to set up DirectXTK renderer.");
        OnReset();
        return;
    }

    // Get context of next frame to render.
    auto& s_FrameCtx = m_FrameContext[++m_FrameCounter % m_FrameContext.size()];

    // If this context is still being rendered, we should wait for it.
    if (s_FrameCtx.FenceValue != 0 && s_FrameCtx.FenceValue > m_Fence->GetCompletedValue()) {
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

    // Update camera matrices.
    const auto s_CameraRight = Globals::RenderManager->m_pDevice->m_Constants.cameraRight;
    const auto s_CameraUp = Globals::RenderManager->m_pDevice->m_Constants.cameraUp;
    const auto s_CameraFwd = Globals::RenderManager->m_pDevice->m_Constants.cameraFwd;
    const auto s_CameraPos = Globals::RenderManager->m_pDevice->m_Constants.cameraPos;

    const auto s_CameraView = SMatrix {
        {s_CameraRight.x, s_CameraRight.y, s_CameraRight.z, 0.f},
        {s_CameraUp.x, s_CameraUp.y, s_CameraUp.z, 0.f},
        {-s_CameraFwd.x, -s_CameraFwd.y, -s_CameraFwd.z, 0.f},
        {s_CameraPos.x, s_CameraPos.y, s_CameraPos.z, 1.f}
    }.Inverse();

    m_View = *reinterpret_cast<DirectX::FXMMATRIX*>(&s_CameraView);
    m_Projection = *reinterpret_cast<DirectX::FXMMATRIX*>(&Globals::RenderManager->m_pDevice->m_Constants.
        cameraViewToClip);

    m_ViewProjection = m_View * m_Projection;
    m_ProjectionViewInverse = (m_Projection * m_View).Invert();

    m_ViewFrustum.UpdateClipPlanes(
        s_CameraView, Globals::RenderManager->m_pDevice->m_Constants.
                                              cameraViewToClip
    );

    if (m_RendererSetup) {
        m_TriangleEffect->SetView(m_View);
        m_TriangleEffect->SetProjection(m_Projection);

        m_LineEffect->SetView(m_View);
        m_LineEffect->SetProjection(m_Projection);

        m_TextEffect->SetView(m_View);
        m_TextEffect->SetProjection(m_Projection);

        m_DebugEffect->SetView(m_View);
        m_DebugEffect->SetProjection(m_Projection);
    }

    // Set up the viewport.
    D3D12_VIEWPORT s_Viewport = {0.0f, 0.0f, m_WindowWidth, m_WindowHeight, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH};
    m_CommandList->RSSetViewports(1, &s_Viewport);

    D3D12_RECT s_ScissorRect = {0, 0, static_cast<LONG>(m_WindowWidth), static_cast<LONG>(m_WindowHeight)};
    m_CommandList->RSSetScissorRects(1, &s_ScissorRect);

    m_CommandList->BeginEvent(0, L"DebugRender", sizeof(L"DebugRender"));

    if (m_DsvIndex.has_value()) {
        DepthDraw();
    }

    Draw();

    m_CommandList->EndEvent();

    const D3D12_RESOURCE_BARRIER s_PresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_BackBuffers[s_BackBufferIndex],
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );

    m_CommandList->ResourceBarrier(1, &s_PresentBarrier);
    BreakIfFailed(m_CommandList->Close());

    m_CommandQueue->ExecuteCommandLists(1, CommandListCast(&m_CommandList.Ref));
}

void DirectXTKRenderer::PostPresent(IDXGISwapChain3* p_SwapChain, HRESULT p_PresentResult) {
    if (!m_RendererSetup || !m_CommandQueue)
        return;

    if (p_PresentResult == DXGI_ERROR_DEVICE_REMOVED || p_PresentResult == DXGI_ERROR_DEVICE_RESET) {
        Logger::Error("Device lost after present.");
        abort();
    }
    else {
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

void DirectXTKRenderer::WaitForCurrentFrameToFinish() const {
    if (m_FenceValue != 0 && m_FenceValue > m_Fence->GetCompletedValue()) {
        BreakIfFailed(m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent.Handle));
        WaitForSingleObject(m_FenceEvent.Handle, INFINITE);
    }
}

bool DirectXTKRenderer::SetupRenderer(IDXGISwapChain3* p_SwapChain) {
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

    for (UINT i = 0; i < MaxRenderedFrames; ++i) {
        FrameContext s_Frame {};

        if (s_Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(s_Frame.CommandAllocator.ReleaseAndGetPtr())
        ) != S_OK)
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
    m_DsvDescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < s_BufferCount; ++i) {
        if (p_SwapChain->GetBuffer(i, IID_PPV_ARGS(m_BackBuffers[i].ReleaseAndGetPtr())) != S_OK)
            return false;

        const CD3DX12_CPU_DESCRIPTOR_HANDLE s_RtvDescriptor(s_RtvHandle, i, m_RtvDescriptorSize);
        s_Device->CreateRenderTargetView(m_BackBuffers[i], nullptr, s_RtvDescriptor);
    }

    if (s_Device->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_FrameContext[0].CommandAllocator, nullptr,
            IID_PPV_ARGS(m_CommandList.ReleaseAndGetPtr())
        ) != S_OK ||
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

    RECT s_Rect = {0, 0, 0, 0};
    GetClientRect(m_Hwnd, &s_Rect);

    m_WindowWidth = static_cast<float>(s_Rect.right - s_Rect.left);
    m_WindowHeight = static_cast<float>(s_Rect.bottom - s_Rect.top);

    m_GraphicsMemory = std::make_unique<DirectX::GraphicsMemory>(s_Device);

    DirectX::RenderTargetState s_RtState(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT_S8X24_UINT);

    {
        D3D12_BLEND_DESC s_AlphaBlend = {};

        s_AlphaBlend.AlphaToCoverageEnable = FALSE;
        s_AlphaBlend.IndependentBlendEnable = FALSE;
        s_AlphaBlend.RenderTarget[0].BlendEnable = TRUE;
        s_AlphaBlend.RenderTarget[0].LogicOpEnable = FALSE;
        s_AlphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        s_AlphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        s_AlphaBlend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        s_AlphaBlend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        s_AlphaBlend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        s_AlphaBlend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        s_AlphaBlend.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        s_AlphaBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_RED |
                D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE;

        DirectX::EffectPipelineStateDescription s_Desc(
            &DirectX::VertexPositionColor::InputLayout,
            s_AlphaBlend,
            DirectX::CommonStates::DepthReadReverseZ,
            DirectX::CommonStates::CullNone,
            s_RtState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
        );

        m_TriangleEffect = std::make_unique<DirectX::BasicEffect>(s_Device, DirectX::EffectFlags::VertexColor, s_Desc);

        s_Desc.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

        m_LineEffect = std::make_unique<DirectX::BasicEffect>(s_Device, DirectX::EffectFlags::VertexColor, s_Desc);

        s_Desc.inputLayout = DirectX::VertexPositionColorTexture::InputLayout;
        s_Desc.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        m_TextEffect = std::make_unique<DirectX::BasicEffect>(
            s_Device, DirectX::EffectFlags::VertexColor | DirectX::EffectFlags::Texture, s_Desc
        );

        m_TriangleBatch = std::make_unique<CustomPrimitiveBatch<DirectX::VertexPositionColor>>(
            s_Device, [&]() {
                m_TriangleEffect->Apply(m_CommandList);
            }
        );

        m_LineBatch = std::make_unique<CustomPrimitiveBatch<DirectX::VertexPositionColor>>(
            s_Device, [&]() {
                m_LineEffect->Apply(m_CommandList);
            }
        );

        m_TextBatch = std::make_unique<CustomPrimitiveBatch<DirectX::VertexPositionColorTexture>>(
            s_Device, [&]() {
                ID3D12DescriptorHeap* s_Heaps[] = {m_fontSRVDescriptorHeap.Ref, m_CommonStates->Heap()};

                m_CommandList->SetDescriptorHeaps(static_cast<UINT>(std::size(s_Heaps)), s_Heaps);

                m_TextEffect->Apply(m_CommandList);

                m_CommandList->SetPipelineState(m_PipelineState);
            }
        );

        m_Text2DBuffer.reserve(1024);

        if (!CreateFontDistanceFieldTexture()) {
            return false;
        }

        m_CommonStates = std::make_unique<DirectX::CommonStates>(s_Device.Ref);

        m_TextEffect->SetTexture(
            m_fontSRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), m_CommonStates->LinearClamp()
        );

        const std::string s_DebugRenderDistanceFieldFontVertexShader = R"(
			cbuffer Parameters : register(b0)
			{
				float4 DiffuseColor             : packoffset(c0);
				float3 EmissiveColor            : packoffset(c1);
				float3 SpecularColor            : packoffset(c2);
				float  SpecularPower            : packoffset(c2.w);

				float3 LightDirection[3]        : packoffset(c3);
				float3 LightDiffuseColor[3]     : packoffset(c6);
				float3 LightSpecularColor[3]    : packoffset(c9);

				float3 EyePosition              : packoffset(c12);

				float3 FogColor                 : packoffset(c13);
				float4 FogVector                : packoffset(c14);

				float4x4 World                  : packoffset(c15);
				float3x3 WorldInverseTranspose  : packoffset(c19);
				float4x4 WorldViewProj          : packoffset(c22);
			};

			struct VSInputTxVc
			{
				float4 Position : SV_Position;
				float2 TexCoord : TEXCOORD0;
				float4 Color    : COLOR;
			};

			struct VSOutputTxNoFog
			{
				float4 Diffuse    : COLOR0;
				float2 TexCoord   : TEXCOORD0;
				float4 PositionPS : SV_Position;
			};

			struct CommonVSOutput
			{
				float4 Pos_ps;
				float4 Diffuse;
				float3 Specular;
				float  FogFactor;
			};

			float ComputeFogFactor(float4 position)
			{
				return saturate(dot(position, FogVector));
			}

			CommonVSOutput ComputeCommonVSOutput(float4 position)
			{
				CommonVSOutput vout;

				vout.Pos_ps = mul(position, WorldViewProj);
				vout.Diffuse = DiffuseColor;
				vout.Specular = 0;
				vout.FogFactor = ComputeFogFactor(position);

				return vout;
			}

			#define SetCommonVSOutputParamsNoFog \
				vout.PositionPS = cout.Pos_ps; \
				vout.Diffuse = cout.Diffuse;

		    VSOutputTxNoFog VSBasicTxVcNoFog(VSInputTxVc vin)
			{
				VSOutputTxNoFog vout;

				CommonVSOutput cout = ComputeCommonVSOutput(vin.Position);
				SetCommonVSOutputParamsNoFog;

				vout.TexCoord = vin.TexCoord;
				vout.Diffuse *= vin.Color;

				return vout;
			}
		)";

        const std::string s_DebugRenderDistanceFieldFontPixelShader = R"(
		    Texture2D<float4> mapDebug2DLinear : register(t0);
			sampler samplerLinearClamp : register(s0);

			struct PSInput
			{
				float4 color : COLOR0;
				float2 uv : TEXCOORD0;
			};

			float4 mainPS(PSInput input) : SV_Target0
			{
				// Sample the font distance field texture
				float distance = mapDebug2DLinear.Sample(samplerLinearClamp, input.uv).x;

				// Shift and scale the distance to control edge sharpness
				const float edgeThreshold = -0.4f; // Shift distance for smoothing
				const float scaleFactor = 5.0f; // Scale to sharpen the edge
				distance = saturate((distance + edgeThreshold) * scaleFactor);

				// Further control the smoothness of the transition
				float smoothedAlpha = distance * distance * (-2.0f) + 3.0f * distance;

				// Output the final color with computed alpha
				float alpha = smoothedAlpha * distance;
    
				return float4(input.color.xyz, alpha);
			}
		)";

        ScopedD3DRef<ID3DBlob> s_VertexShaderBlob;
        ScopedD3DRef<ID3DBlob> s_PixelShaderBlob;

        if (!CompileShaderFromString(
            s_DebugRenderDistanceFieldFontVertexShader, "VSBasicTxVcNoFog", "vs_5_0", &s_VertexShaderBlob.Ref
        )) {
            return false;
        }

        if (!CompileShaderFromString(
            s_DebugRenderDistanceFieldFontPixelShader, "mainPS", "ps_5_0", &s_PixelShaderBlob.Ref
        )) {
            return false;
        }

        D3D12_ROOT_SIGNATURE_FLAGS s_RootSignatureFlags =
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

        CD3DX12_ROOT_PARAMETER s_RootParameters[RootParameterIndex::RootParameterCount] = {};
        s_RootParameters[RootParameterIndex::ConstantBuffer].
                InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

        CD3DX12_ROOT_SIGNATURE_DESC s_RootSignatureDesc = {};

        const CD3DX12_DESCRIPTOR_RANGE s_TextureSRV(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        const CD3DX12_DESCRIPTOR_RANGE s_TextureSampler(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

        s_RootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(
            1, &s_TextureSRV, D3D12_SHADER_VISIBILITY_PIXEL
        );
        s_RootParameters[RootParameterIndex::TextureSampler].InitAsDescriptorTable(
            1, &s_TextureSampler, D3D12_SHADER_VISIBILITY_PIXEL
        );

        s_RootSignatureDesc.Init(
            static_cast<UINT>(std::size(s_RootParameters)), s_RootParameters, 0, nullptr, s_RootSignatureFlags
        );

        ScopedD3DRef<ID3D12RootSignature> s_RootSignature;

        DirectX::CreateRootSignature(s_Device, &s_RootSignatureDesc, &s_RootSignature.Ref);

        D3D12_SHADER_BYTECODE s_VertexShader;
        D3D12_SHADER_BYTECODE s_PixelShader;

        s_VertexShader.pShaderBytecode = s_VertexShaderBlob->GetBufferPointer();
        s_VertexShader.BytecodeLength = s_VertexShaderBlob->GetBufferSize();

        s_PixelShader.pShaderBytecode = s_PixelShaderBlob->GetBufferPointer();
        s_PixelShader.BytecodeLength = s_PixelShaderBlob->GetBufferSize();

        s_Desc.CreatePipelineState(s_Device, s_RootSignature.Ref, s_VertexShader, s_PixelShader, &m_PipelineState.Ref);

        const D3D12_INPUT_ELEMENT_DESC s_InputElementDescs[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R16G16B16A16_SNORM, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"BINORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R16G16_SNORM, 1, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        D3D12_INPUT_LAYOUT_DESC s_InputLayoutDesc = {};
        s_InputLayoutDesc.pInputElementDescs = s_InputElementDescs;
        s_InputLayoutDesc.NumElements = _countof(s_InputElementDescs);

        m_DebugEffect = std::make_unique<DebugEffect>(s_Device, m_CommandList, m_CommandQueue, &s_InputLayoutDesc);
    }

    {
        m_ResourceDescriptors = std::make_unique<DirectX::DescriptorHeap>(
            s_Device, static_cast<int>(Descriptors::Count)
        );

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

        DirectX::SpriteBatchPipelineStateDescription s_Desc(s_RtState);
        m_SpriteBatch = std::make_unique<DirectX::SpriteBatch>(s_Device, s_ResourceUpload, s_Desc);

        s_ResourceUpload.End(m_CommandQueue).wait();

        const D3D12_VIEWPORT s_Viewport = {0.0f, 0.0f, m_WindowWidth, m_WindowHeight, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH};
        m_SpriteBatch->SetViewport(s_Viewport);
    }

    m_TriangleEffect->SetWorld(m_World);
    m_TriangleEffect->SetView(m_View);
    m_TriangleEffect->SetProjection(m_Projection);

    m_LineEffect->SetWorld(m_World);
    m_LineEffect->SetView(m_View);
    m_LineEffect->SetProjection(m_Projection);

    m_TextEffect->SetWorld(m_World);
    m_TextEffect->SetView(m_View);
    m_TextEffect->SetProjection(m_Projection);

    m_DebugEffect->SetWorld(m_World);
    m_DebugEffect->SetView(m_View);
    m_DebugEffect->SetProjection(m_Projection);

    m_RendererSetup = true;

    Logger::Debug("DirectXTK renderer successfully set up.");

    return true;
}

void DirectXTKRenderer::OnReset() {
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

void DirectXTKRenderer::PostReset() {
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
    m_DsvDescriptorSize = s_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    const auto s_RtvHandle = m_RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < m_BackBuffers.size(); ++i) {
        if (m_SwapChain->GetBuffer(i, IID_PPV_ARGS(m_BackBuffers[i].ReleaseAndGetPtr())) != S_OK)
            return;

        const CD3DX12_CPU_DESCRIPTOR_HANDLE s_RtvDescriptor(s_RtvHandle, i, m_RtvDescriptorSize);
        s_Device->CreateRenderTargetView(m_BackBuffers[i], nullptr, s_RtvDescriptor);
    }

    RECT s_Rect = {0, 0, 0, 0};
    GetClientRect(m_Hwnd, &s_Rect);

    m_WindowWidth = static_cast<float>(s_Rect.right - s_Rect.left);
    m_WindowHeight = static_cast<float>(s_Rect.bottom - s_Rect.top);

    const D3D12_VIEWPORT s_Viewport = {0.0f, 0.0f, m_WindowWidth, m_WindowHeight, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH};
    m_SpriteBatch->SetViewport(s_Viewport);
}

void DirectXTKRenderer::SetCommandQueue(ID3D12CommandQueue* p_CommandQueue) {
    if (m_CommandQueue == p_CommandQueue)
        return;

    if (m_CommandQueue) {
        m_CommandQueue->Release();
        m_CommandQueue = nullptr;
    }

    Logger::Debug("Setting up DirectXTK12 command queue.");
    m_CommandQueue = p_CommandQueue;
}

bool DirectXTKRenderer::CompileShaderFromString(
    const std::string& p_ShaderCode,
    const std::string& p_EntryPoint,
    const std::string& p_ShaderModel,
    ID3DBlob** p_ShaderBlob
) {
    UINT s_CompileFlags = 0;

    #if defined(_DEBUG)
    s_CompileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #endif

    ScopedD3DRef<ID3DBlob> s_ErrorBlob;

    const HRESULT s_Result = D3DCompile(
        p_ShaderCode.c_str(),
        p_ShaderCode.size(),
        nullptr,
        nullptr,
        nullptr,
        p_EntryPoint.c_str(),
        p_ShaderModel.c_str(),
        s_CompileFlags,
        0,
        p_ShaderBlob,
        &s_ErrorBlob.Ref
    );

    if (FAILED(s_Result)) {
        if (s_ErrorBlob) {
            Logger::Error("{}", static_cast<const char*>(s_ErrorBlob->GetBufferPointer()));
        }

        return false;
    }

    return true;
}

bool DirectXTKRenderer::CreateFontDistanceFieldTexture() {
    ScopedD3DRef<ID3D12Device> s_Device;

    if (m_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK) {
        return false;
    }

    DirectX::ResourceUploadBatch s_Upload(s_Device);

    s_Upload.Begin();

    auto s_TextureDesc = CD3DX12_RESOURCE_DESC(
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        0,
        1024,
        384,
        1,
        1,
        DXGI_FORMAT_R8_UNORM,
        1, 0,
        D3D12_TEXTURE_LAYOUT_UNKNOWN,
        D3D12_RESOURCE_FLAG_NONE
    );

    CD3DX12_HEAP_PROPERTIES s_DefaultHeap(D3D12_HEAP_TYPE_DEFAULT);

    HRESULT s_Result = s_Device->CreateCommittedResource(
        &s_DefaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &s_TextureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_FontDistanceFieldTexture.Ref)
    );

    if (FAILED(s_Result)) {
        Logger::Error("Failed to create committed resource for FontDistanceFieldTexture!");
        return false;
    }

    D3D12_SUBRESOURCE_DATA s_InitData = {};
    s_InitData.pData = MDF_FONT::g_DistanceField;
    s_InitData.RowPitch = 1024;
    s_InitData.SlicePitch = 1024 * 384;

    s_Upload.Upload(m_FontDistanceFieldTexture.Ref, 0, &s_InitData, 1);

    s_Upload.Transition(
        m_FontDistanceFieldTexture.Ref,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    auto s_Finish = s_Upload.End(m_CommandQueue);

    s_Finish.wait();

    D3D12_DESCRIPTOR_HEAP_DESC s_SRVHeapDesc = {};

    s_SRVHeapDesc.NumDescriptors = 1;
    s_SRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    s_SRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    s_Result = s_Device->CreateDescriptorHeap(&s_SRVHeapDesc, IID_PPV_ARGS(&m_fontSRVDescriptorHeap.Ref));

    if (FAILED(s_Result)) {
        Logger::Error("Unable to create SRV descriptor heap!");
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC s_SRVDesc = {};
    s_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    s_SRVDesc.Format = s_TextureDesc.Format;
    s_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    s_SRVDesc.Texture2D.MipLevels = 1;
    s_SRVDesc.Texture2D.MostDetailedMip = 0;
    s_SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    s_Device->CreateShaderResourceView(
        m_FontDistanceFieldTexture.Ref, &s_SRVDesc, m_fontSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
    );

    return true;
}

void DirectXTKRenderer::DrawLine3D(
    const SVector3& p_From, const SVector3& p_To, const SVector4& p_FromColor, const SVector4& p_ToColor
) {
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

void DirectXTKRenderer::DrawText2D(
    const ZString& p_Text, const SVector2& p_Pos, const SVector4& p_Color, float p_Rotation/* = 0.f*/,
    float p_Scale/* = 1.f*/, TextAlignment p_Alignment /* = TextAlignment::Center*/
) {
    if (!m_RendererSetup)
        return;

    m_Text2DBuffer.push_back(Text2D {p_Text.c_str(), p_Pos, p_Color, p_Rotation, p_Scale, p_Alignment});
}

bool DirectXTKRenderer::WorldToScreen(const SVector3& p_WorldPos, SVector2& p_Out) {
    if (!m_RendererSetup)
        return false;

    const DirectX::SimpleMath::Vector4 s_World(p_WorldPos.x, p_WorldPos.y, p_WorldPos.z, 1.f);
    const DirectX::SimpleMath::Vector4 s_Projected = DirectX::XMVector4Transform(s_World, m_ViewProjection);

    if (s_Projected.w <= 0.000001f)
        return false;

    const float s_InvertedZ = 1.f / s_Projected.w;
    const DirectX::SimpleMath::Vector3 s_FinalProjected(
        s_Projected.x * s_InvertedZ, s_Projected.y * s_InvertedZ, s_Projected.z * s_InvertedZ
    );

    p_Out.x = (1.f + s_FinalProjected.x) * 0.5f * m_WindowWidth;
    p_Out.y = (1.f - s_FinalProjected.y) * 0.5f * m_WindowHeight;

    return true;
}

bool DirectXTKRenderer::ScreenToWorld(const SVector2& p_ScreenPos, SVector3& p_WorldPosOut, SVector3& p_DirectionOut) {
    if (!m_RendererSetup)
        return false;

    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();

    if (!s_CurrentCamera)
        return false;

    auto s_CameraTrans = s_CurrentCamera->GetWorldMatrix();

    auto s_ScreenPos = DirectX::SimpleMath::Vector3(
        (2.0f * p_ScreenPos.x) / m_WindowWidth - 1.0f, 1.0f - (2.0f * p_ScreenPos.y) / m_WindowHeight, 1.f
    );
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

void DirectXTKRenderer::DrawBox3D(const SVector3& p_Min, const SVector3& p_Max, const SVector4& p_Color) {
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

inline SVector3 XMVecToSVec3(const DirectX::XMVECTOR& p_Vec) {
    return SVector3(DirectX::XMVectorGetX(p_Vec), DirectX::XMVectorGetY(p_Vec), DirectX::XMVectorGetZ(p_Vec));
}

void DirectXTKRenderer::DrawOBB3D(
    const SVector3& p_Min, const SVector3& p_Max, const SMatrix& p_Transform, const SVector4& p_Color
) {
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

void DirectXTKRenderer::DrawBoundingQuads(
    const SVector3& p_Min, const SVector3& p_Max, const SMatrix& p_Transform, const SVector4& p_Color
) {
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

    auto drawQuad = [&](int i0, int i1, int i2, int i3) {
        SVector3 v0(s_Corners[i0].m128_f32[0], s_Corners[i0].m128_f32[1], s_Corners[i0].m128_f32[2]);
        SVector3 v1(s_Corners[i1].m128_f32[0], s_Corners[i1].m128_f32[1], s_Corners[i1].m128_f32[2]);
        SVector3 v2(s_Corners[i2].m128_f32[0], s_Corners[i2].m128_f32[1], s_Corners[i2].m128_f32[2]);
        SVector3 v3(s_Corners[i3].m128_f32[0], s_Corners[i3].m128_f32[1], s_Corners[i3].m128_f32[2]);

        DrawTriangle3D(v0, p_Color, v1, p_Color, v2, p_Color);
        DrawTriangle3D(v0, p_Color, v2, p_Color, v3, p_Color);
    };

    drawQuad(0, 1, 2, 3); // Front face
    drawQuad(4, 5, 6, 7); // Back face
    drawQuad(0, 1, 5, 6); // Left face
    drawQuad(2, 3, 7, 4); // Right face
    drawQuad(1, 2, 4, 5); // Top face
    drawQuad(0, 3, 7, 6); // Bottom face
}

void DirectXTKRenderer::DrawTriangle3D(
    const SVector3& p_V1, const SVector4& p_Color1,
    const SVector3& p_V2, const SVector4& p_Color2,
    const SVector3& p_V3, const SVector4& p_Color3
) {
    m_TriangleBatch->DrawTriangle(
        DirectX::VertexPositionColor(
            DirectX::SimpleMath::Vector3(p_V1.x, p_V1.y, p_V1.z),
            DirectX::SimpleMath::Vector4(p_Color1.x, p_Color1.y, p_Color1.z, p_Color1.w)
        ),
        DirectX::VertexPositionColor(
            DirectX::SimpleMath::Vector3(p_V2.x, p_V2.y, p_V2.z),
            DirectX::SimpleMath::Vector4(p_Color2.x, p_Color2.y, p_Color2.z, p_Color2.w)
        ),
        DirectX::VertexPositionColor(
            DirectX::SimpleMath::Vector3(p_V3.x, p_V3.y, p_V3.z),
            DirectX::SimpleMath::Vector4(p_Color3.x, p_Color3.y, p_Color3.z, p_Color3.w)
        )
    );
}

void DirectXTKRenderer::DrawTriangle3D(
    const SVector3& p_V1, const SVector4& p_Color1, const SVector2& p_TextureCoordinates1,
    const SVector3& p_V2, const SVector4& p_Color2, const SVector2& p_TextureCoordinates2,
    const SVector3& p_V3, const SVector4& p_Color3, const SVector2& p_TextureCoordinates3
) {
    m_TextBatch->DrawTriangle(
        DirectX::VertexPositionColorTexture(
            DirectX::SimpleMath::Vector3(p_V1.x, p_V1.y, p_V1.z),
            DirectX::SimpleMath::Vector4(p_Color1.x, p_Color1.y, p_Color1.z, p_Color1.w),
            DirectX::SimpleMath::Vector2(p_TextureCoordinates1.x, p_TextureCoordinates1.y)
        ),
        DirectX::VertexPositionColorTexture(
            DirectX::SimpleMath::Vector3(p_V2.x, p_V2.y, p_V2.z),
            DirectX::SimpleMath::Vector4(p_Color2.x, p_Color2.y, p_Color2.z, p_Color2.w),
            DirectX::SimpleMath::Vector2(p_TextureCoordinates2.x, p_TextureCoordinates2.y)
        ),
        DirectX::VertexPositionColorTexture(
            DirectX::SimpleMath::Vector3(p_V3.x, p_V3.y, p_V3.z),
            DirectX::SimpleMath::Vector4(p_Color3.x, p_Color3.y, p_Color3.z, p_Color3.w),
            DirectX::SimpleMath::Vector2(p_TextureCoordinates3.x, p_TextureCoordinates3.y)
        )
    );
}

void DirectXTKRenderer::DrawQuad3D(
    const SVector3& p_V1,
    const SVector4& p_Color1,
    const SVector3& p_V2,
    const SVector4& p_Color2,
    const SVector3& p_V3,
    const SVector4& p_Color3,
    const SVector3& p_V4,
    const SVector4& p_Color4
) {
    m_TriangleBatch->DrawQuad(
        DirectX::VertexPositionColor(
            DirectX::SimpleMath::Vector3(p_V1.x, p_V1.y, p_V1.z),
            DirectX::SimpleMath::Vector4(p_Color1.x, p_Color1.y, p_Color1.z, p_Color1.w)
        ),
        DirectX::VertexPositionColor(
            DirectX::SimpleMath::Vector3(p_V2.x, p_V2.y, p_V2.z),
            DirectX::SimpleMath::Vector4(p_Color2.x, p_Color2.y, p_Color2.z, p_Color2.w)
        ),
        DirectX::VertexPositionColor(
            DirectX::SimpleMath::Vector3(p_V3.x, p_V3.y, p_V3.z),
            DirectX::SimpleMath::Vector4(p_Color3.x, p_Color3.y, p_Color3.z, p_Color3.w)
        ),
        DirectX::VertexPositionColor(
            DirectX::SimpleMath::Vector3(p_V4.x, p_V4.y, p_V4.z),
            DirectX::SimpleMath::Vector4(p_Color4.x, p_Color4.y, p_Color4.z, p_Color4.w)
        )
    );
}

void DirectXTKRenderer::DrawText3D(
    const std::string& p_Text, const SMatrix& p_World, const bool p_IsCameraTransform,
    const SVector4& p_Color, float p_Scale,
    TextAlignment p_HorizontalAlignment, TextAlignment p_VerticalAlignment
) {
    DrawText3D(p_Text.c_str(), p_World, p_IsCameraTransform, p_Color, p_Scale, p_HorizontalAlignment, p_VerticalAlignment);
}

void DirectXTKRenderer::DrawText3D(
    const char* p_Text, const SMatrix& p_World, const bool p_IsCameraTransform,
    const SVector4& p_Color, float p_Scale,
    TextAlignment p_HorizontalAlignment, TextAlignment p_VerticalAlignment
) {
    int s_TextLength = -1;

    do {
        ++s_TextLength;
    }
    while (p_Text[s_TextLength] != '\0');

    if (s_TextLength > 255) {
        s_TextLength = 255;
    }

    const char* s_BaseText = p_Text;
    int s_BaseTextLength = s_TextLength;
    int s_PrintableCharacterCount = 0;

    while (s_TextLength) {
        if (static_cast<unsigned char>(*p_Text) >= 33 && static_cast<unsigned char>(*p_Text) <= 126) {
            ++s_PrintableCharacterCount;
        }

        ++p_Text;
        --s_TextLength;
    }

    MDF_FONT::STextBoundingBox s_TextBoundingBox;

    MDF_FONT::CalcBoundingBox(s_TextBoundingBox, s_BaseText);

    float s_OffsetX = 0.f;
    float s_OffsetY = 0.f;

    if (p_HorizontalAlignment == TextAlignment::Center) {
        s_OffsetX = (s_TextBoundingBox.m_fMaxX - s_TextBoundingBox.m_fMinX) * -0.5f;
    }
    else if (p_HorizontalAlignment == TextAlignment::Right) {
        s_OffsetX = (s_TextBoundingBox.m_fMaxX - s_TextBoundingBox.m_fMinX) * -1.f;
    }

    if (p_VerticalAlignment == TextAlignment::Middle) {
        s_OffsetY = (s_TextBoundingBox.m_fMaxY - s_TextBoundingBox.m_fMinY) * -0.5f;
    }
    else if (p_VerticalAlignment == TextAlignment::Bottom) {
        s_OffsetY = (s_TextBoundingBox.m_fMaxY - s_TextBoundingBox.m_fMinY) * -1.f;
    }

    const float4 s_Translate = float4(s_OffsetX * p_Scale, 0.f, s_OffsetY * p_Scale, 1.f);
    const float4 s_Scale2 = float4(p_Scale, p_Scale, p_Scale, 1.f);
    const SMatrix s_OffsetMatrix = SMatrix::ScaleTranslate(s_Scale2, s_Translate);

    SMatrix s_World = p_World;

    if (p_IsCameraTransform) {
        std::swap(s_World.YAxis, s_World.ZAxis);
    }

    const SMatrix s_WorldMatrix = s_World.AffineMultiply(s_OffsetMatrix);

    const unsigned int s_VertexCount = 2 * s_PrintableCharacterCount;
    std::vector<Triangle> s_Triangles;

    s_Triangles.reserve(s_VertexCount);

    static const float s_LineHeight =
    (MDF_FONT::ComputeLineHeightFromMetrics() /
        static_cast<float>(MDF_FONT::g_FontHeader.m_anTexRes[1]));
    float s_PenX = 0.f;
    float s_PenY = 0.f;

    while (s_BaseTextLength) {
        if (*s_BaseText == '\n') {
            s_PenX = 0.f;
            s_PenY -= s_LineHeight;
            ++s_BaseText;
            --s_BaseTextLength;

            continue;
        }

        if (*s_BaseText == ' ') {
            s_PenX += MDF_FONT::GetAdvanceWidth(*s_BaseText);
            ++s_BaseText;
            --s_BaseTextLength;

            continue;
        }

        if (static_cast<unsigned char>(*s_BaseText) >= 33 && static_cast<unsigned char>(*s_BaseText) <= 126) {
            static const float s_Scale = 1.f;
            float s_Vertices[8];
            float s_TextureCoordinates[8];

            MDF_FONT::RenderQuad(
                static_cast<unsigned int>(*s_BaseText), s_Scale, s_PenX, s_PenY, s_Vertices, s_TextureCoordinates
            );

            float4 s_BottomLeft = float4(s_Vertices[0], 0.f, s_Vertices[1], 1.f);
            float4 s_BottomRight = float4(s_Vertices[2], 0.f, s_Vertices[3], 1.f);
            float4 s_TopRight = float4(s_Vertices[4], 0.f, s_Vertices[5], 1.f);
            float4 s_TopLeft = float4(s_Vertices[6], 0.f, s_Vertices[7], 1.f);

            s_BottomLeft = s_WorldMatrix.WVectorTransform(s_BottomLeft);
            s_BottomRight = s_WorldMatrix.WVectorTransform(s_BottomRight);
            s_TopRight = s_WorldMatrix.WVectorTransform(s_TopRight);
            s_TopLeft = s_WorldMatrix.WVectorTransform(s_TopLeft);

            Triangle& s_Triangle1 = s_Triangles.emplace_back();
            Triangle& s_Triangle2 = s_Triangles.emplace_back();

            s_Triangle1.vertexPosition1 = SVector3 {s_BottomLeft.x, s_BottomLeft.y, s_BottomLeft.z};
            s_Triangle1.vertexPosition2 = SVector3 {s_BottomRight.x, s_BottomRight.y, s_BottomRight.z};
            s_Triangle1.vertexPosition3 = SVector3 {s_TopLeft.x, s_TopLeft.y, s_TopLeft.z};

            s_Triangle2.vertexPosition1 = SVector3 {s_BottomRight.x, s_BottomRight.y, s_BottomRight.z};
            s_Triangle2.vertexPosition2 = SVector3 {s_TopRight.x, s_TopRight.y, s_TopRight.z};
            s_Triangle2.vertexPosition3 = SVector3 {s_TopLeft.x, s_TopLeft.y, s_TopLeft.z};

            s_Triangle1.vertexColor1 = p_Color;
            s_Triangle1.vertexColor2 = p_Color;
            s_Triangle1.vertexColor3 = p_Color;

            s_Triangle2.vertexColor1 = p_Color;
            s_Triangle2.vertexColor2 = p_Color;
            s_Triangle2.vertexColor3 = p_Color;

            s_Triangle1.textureCoordinates1 = SVector2 {s_TextureCoordinates[0], s_TextureCoordinates[1]};
            s_Triangle1.textureCoordinates2 = SVector2 {s_TextureCoordinates[2], s_TextureCoordinates[3]};
            s_Triangle1.textureCoordinates3 = SVector2 {s_TextureCoordinates[6], s_TextureCoordinates[7]};

            s_Triangle2.textureCoordinates1 = SVector2 {s_TextureCoordinates[2], s_TextureCoordinates[3]};
            s_Triangle2.textureCoordinates2 = SVector2 {s_TextureCoordinates[4], s_TextureCoordinates[5]};
            s_Triangle2.textureCoordinates3 = SVector2 {s_TextureCoordinates[6], s_TextureCoordinates[7]};
        }

        ++s_BaseText;
        --s_BaseTextLength;
    }

    for (size_t i = 0; i < s_Triangles.size(); ++i) {
        DrawTriangle3D(
            s_Triangles[i].vertexPosition1, s_Triangles[i].vertexColor1, s_Triangles[i].textureCoordinates1,
            s_Triangles[i].vertexPosition2, s_Triangles[i].vertexColor2, s_Triangles[i].textureCoordinates2,
            s_Triangles[i].vertexPosition3, s_Triangles[i].vertexColor3, s_Triangles[i].textureCoordinates3
        );
    }
}

void DirectXTKRenderer::DrawMesh(
    const std::vector<SVector3>& p_Vertices, const std::vector<unsigned short>& p_Indices, const SVector4& p_VertexColor
) {
    std::vector<DirectX::VertexPositionColor> s_Vertices2;

    s_Vertices2.reserve(p_Vertices.size());

    for (size_t i = 0; i < p_Vertices.size(); ++i) {
        const SVector3& s_Vertex = p_Vertices[i];

        s_Vertices2.push_back(
            DirectX::VertexPositionColor(
                DirectX::SimpleMath::Vector3(s_Vertex.x, s_Vertex.y, s_Vertex.z),
                DirectX::SimpleMath::Vector4(p_VertexColor.x, p_VertexColor.y, p_VertexColor.z, p_VertexColor.w)
            )
        );
    }

    m_TriangleBatch->DrawIndexed(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, p_Indices.data(), p_Indices.size(), s_Vertices2.data(), s_Vertices2.size()
    );
}

void DirectXTKRenderer::DrawMesh(
    ZRenderVertexBuffer** p_VertexBuffers, const uint32_t p_VertexBufferCount, ZRenderIndexBuffer* p_IndexBuffer,
    const SMatrix& p_World, const float4& p_PositionScale, const float4& p_PositionBias,
    const float4& p_TextureScaleBias,
    const SVector4& p_MaterialColor
) {
    ScopedD3DRef<ID3D12Device> s_Device;

    if (m_SwapChain->GetDevice(REF_IID_PPV_ARGS(s_Device)) != S_OK) {
        return;
    }

    std::vector<D3D12_VERTEX_BUFFER_VIEW> s_VertexBufferViews(p_VertexBufferCount);

    for (uint32_t i = 0; i < p_VertexBufferCount; ++i) {
        s_VertexBufferViews[i].BufferLocation = p_VertexBuffers[i]->m_pResource->GetGPUVirtualAddress() +
                p_VertexBuffers[i]->m_nOffset;
        s_VertexBufferViews[i].StrideInBytes = p_VertexBuffers[i]->m_nStride;
        s_VertexBufferViews[i].SizeInBytes = p_VertexBuffers[i]->m_nSize;
    }

    D3D12_INDEX_BUFFER_VIEW s_IndexBufferView;
    s_IndexBufferView.BufferLocation = p_IndexBuffer->m_pResource->GetGPUVirtualAddress() + p_IndexBuffer->m_nOffset;
    s_IndexBufferView.SizeInBytes = p_IndexBuffer->m_nSize;
    s_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;

    const uint32_t s_IndexCount = p_IndexBuffer->m_nSize / p_IndexBuffer->m_nStride;

    m_DebugEffect->SetWorld(*reinterpret_cast<const DirectX::XMFLOAT4X4*>(&p_World));
    m_DebugEffect->SetPositionScale(*reinterpret_cast<const DirectX::XMFLOAT4*>(&p_PositionScale));
    m_DebugEffect->SetPositionBias(*reinterpret_cast<const DirectX::XMFLOAT4*>(&p_PositionBias));
    m_DebugEffect->SetTextureScaleBias(*reinterpret_cast<const DirectX::XMFLOAT4*>(&p_TextureScaleBias));
    m_DebugEffect->SetMaterialColor(*reinterpret_cast<const DirectX::XMFLOAT4*>(&p_MaterialColor));
    m_DebugEffect->Apply(s_Device, m_CommandList);

    m_CommandList->IASetVertexBuffers(0, p_VertexBufferCount, s_VertexBufferViews.data());
    m_CommandList->IASetIndexBuffer(&s_IndexBufferView);
    m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_CommandList->DrawIndexedInstanced(s_IndexCount, 1, 0, 0, 0);
}

void DirectXTKRenderer::DrawText2D(const Text2D& text2D) {
    const std::string s_Text(text2D.m_Text.c_str(), text2D.m_Text.size());
    const DirectX::SimpleMath::Vector2 s_StringSize = m_Font->MeasureString(s_Text.c_str());

    DirectX::SimpleMath::Vector2 s_Origin(0.f, 0.f);

    if (text2D.m_Alignment == TextAlignment::Center)
        s_Origin.x = s_StringSize.x / 2.f;
    else if (text2D.m_Alignment == TextAlignment::Right)
        s_Origin.x = s_StringSize.x;

    m_Font->DrawString(
        m_SpriteBatch.get(),
        s_Text.c_str(),
        DirectX::SimpleMath::Vector2(text2D.m_Position.x, text2D.m_Position.y),
        DirectX::SimpleMath::Vector4(text2D.m_Color.x, text2D.m_Color.y, text2D.m_Color.z, text2D.m_Color.w),
        text2D.m_Rotation,
        s_Origin,
        text2D.m_Scale
    );
}

bool DirectXTKRenderer::IsInsideViewFrustum(const SVector3& p_Point) const {
    return m_ViewFrustum.ContainsPoint(p_Point);
}

bool DirectXTKRenderer::IsInsideViewFrustum(
    const SVector3& p_Min, const SVector3& p_Max, const SMatrix& p_Transform
) const {
    const DirectX::SimpleMath::Matrix s_World = *reinterpret_cast<const DirectX::SimpleMath::Matrix*>(&p_Transform);

    AABB s_AABB;
    s_AABB.min = p_Min;
    s_AABB.max = p_Max;

    const AABB s_TransformedAABB = TransformAABB(s_World, s_AABB);

    return m_ViewFrustum.ContainsAABB(s_TransformedAABB);
}

bool DirectXTKRenderer::IsInsideViewFrustum(
    const SMatrix& p_Transform, const float4& p_Center, const float4& p_HalfSize
) const {
    return m_ViewFrustum.ContainsOBB(p_Transform, p_Center, p_HalfSize);
}

AABB DirectXTKRenderer::TransformAABB(const DirectX::SimpleMath::Matrix& p_Transform, const AABB& p_AABB) {
    const DirectX::SimpleMath::Vector3 s_Corners[8] = {
        {p_AABB.min.x, p_AABB.min.y, p_AABB.min.z},
        {p_AABB.min.x, p_AABB.min.y, p_AABB.max.z},
        {p_AABB.min.x, p_AABB.max.y, p_AABB.min.z},
        {p_AABB.min.x, p_AABB.max.y, p_AABB.max.z},
        {p_AABB.max.x, p_AABB.min.y, p_AABB.min.z},
        {p_AABB.max.x, p_AABB.min.y, p_AABB.max.z},
        {p_AABB.max.x, p_AABB.max.y, p_AABB.min.z},
        {p_AABB.max.x, p_AABB.max.y, p_AABB.max.z}
    };

    AABB s_TransformedAABB;
    s_TransformedAABB.min = {FLT_MAX, FLT_MAX, FLT_MAX};
    s_TransformedAABB.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (int i = 0; i < 8; ++i) {
        const DirectX::SimpleMath::Vector3 s_TransformedCorner = DirectX::SimpleMath::Vector3::Transform(
            s_Corners[i], p_Transform
        );

        s_TransformedAABB.min.x = std::min(s_TransformedAABB.min.x, s_TransformedCorner.x);
        s_TransformedAABB.min.y = std::min(s_TransformedAABB.min.y, s_TransformedCorner.y);
        s_TransformedAABB.min.z = std::min(s_TransformedAABB.min.z, s_TransformedCorner.z);

        s_TransformedAABB.max.x = std::max(s_TransformedAABB.max.x, s_TransformedCorner.x);
        s_TransformedAABB.max.y = std::max(s_TransformedAABB.max.y, s_TransformedCorner.y);
        s_TransformedAABB.max.z = std::max(s_TransformedAABB.max.z, s_TransformedCorner.z);
    }

    return s_TransformedAABB;
}