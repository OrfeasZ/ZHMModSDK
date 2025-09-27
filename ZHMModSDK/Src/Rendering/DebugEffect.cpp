#include <directx/d3dx12.h>
#include <d3dcompiler.h>

#include "DirectXHelpers.h"
#include "CommonStates.h"
#include "ResourceUploadBatch.h"
#include "DebugEffect.h"
#include "Logging.h"

DebugEffect::DebugEffect(
    ScopedD3DRef<ID3D12Device> p_Device, ScopedD3DRef<ID3D12GraphicsCommandList> p_CommandList,
    ScopedD3DRef<ID3D12CommandQueue> p_CommandQueue, const D3D12_INPUT_LAYOUT_DESC* p_InputLayoutDesc
)
{
    CreateRootSignature(p_Device);

    const std::string s_DebugVertexShader = R"(
		struct DebugEffectConstants
        {
            row_major float4x4 world;
            row_major float4x4 view;
            row_major float4x4 projection;

            float4 positionScale;
            float4 positionBias;
            float4 textureScaleBias;

            float4 materialColor;
        };

        cbuffer DebugConstants : register(b0)
        {
            DebugEffectConstants gConstants;
        };

        struct VSInput
        {
            float4 position : POSITION0;
            float4 normal   : NORMAL0;
            float4 tangent  : TANGENT0;
            float4 binormal : BINORMAL0;
            float2 texcoord : TEXCOORD0;
            float4 color    : COLOR0;
        };

        struct VSOutput
        {
            float4 position : SV_POSITION;
            float4 color    : COLOR0;
            float2 texcoord : TEXCOORD0;
        };

        VSOutput main(VSInput input)
        {
            VSOutput output;

            float4 localPos = float4(input.position.xyz, 1.0f);
            localPos.xyz = localPos.xyz * gConstants.positionScale.xyz + gConstants.positionBias.xyz;

            float4 worldPos = mul(localPos, gConstants.world);
            float4 viewPos  = mul(worldPos, gConstants.view);
            output.position = mul(viewPos, gConstants.projection);

            output.color = input.color;

            float2 scaledUV = input.texcoord * gConstants.textureScaleBias.xy + gConstants.textureScaleBias.zw;
            output.texcoord = scaledUV;

            return output;
        }
	)";

    const std::string s_DebugPixelShader = R"(
        struct DebugEffectConstants
        {
            row_major float4x4 world;
            row_major float4x4 view;
            row_major float4x4 projection;

            float4 positionScale;
            float4 positionBias;
            float4 textureScaleBias;

            float4 materialColor;
        };

        cbuffer DebugConstants : register(b0)
        {
            DebugEffectConstants gConstants;
        };

        SamplerState samplerPointClampNode_s : register(s0);
        Texture2D<float4> mapDebug2D : register(t0);

        struct PSInput
        {
            float4 position : SV_POSITION;
            float4 color    : COLOR;
            float2 texcoord : TEXCOORD;
        };

        float4 main(PSInput input) : SV_TARGET
        {
            float4 texColor = mapDebug2D.Sample(samplerPointClampNode_s, input.texcoord);
            return gConstants.materialColor * texColor * input.color;
        }
    )";

    ScopedD3DRef<ID3DBlob> s_VertexShaderBlob;
    ScopedD3DRef<ID3DBlob> s_PixelShaderBlob;

    if (!CompileShaderFromString(s_DebugVertexShader, "main", "vs_5_0", &s_VertexShaderBlob.Ref))
    {
        return;
    }

    if (!CompileShaderFromString(s_DebugPixelShader, "main", "ps_5_0", &s_PixelShaderBlob.Ref))
    {
        return;
    }

    D3D12_SHADER_BYTECODE s_VertexShader;
    s_VertexShader.pShaderBytecode = reinterpret_cast<UINT8*>(s_VertexShaderBlob->GetBufferPointer());
    s_VertexShader.BytecodeLength = s_VertexShaderBlob->GetBufferSize();

    D3D12_SHADER_BYTECODE s_PixelShader;
    s_PixelShader.pShaderBytecode = reinterpret_cast<UINT8*>(s_PixelShaderBlob->GetBufferPointer());
    s_PixelShader.BytecodeLength = s_PixelShaderBlob->GetBufferSize();

    DirectX::RenderTargetState s_RenderTargetState = DirectX::RenderTargetState(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
    DirectX::EffectPipelineStateDescription s_EffectPipelineStateDescription(
        p_InputLayoutDesc,
        DirectX::CommonStates::Opaque,
        DirectX::CommonStates::DepthReverseZ,
        DirectX::CommonStates::CullNone,
        s_RenderTargetState);

    s_EffectPipelineStateDescription.CreatePipelineState(p_Device, m_pRootSignature, s_VertexShader, s_PixelShader, &m_pPipelineState);

    CreateTexture(p_Device, p_CommandList, p_CommandQueue);
    CreateSampler(p_Device);
}

DebugEffect::~DebugEffect()
{
    if (m_pPipelineState)
    {
        m_pPipelineState->Release();
    }

    if (m_pRootSignature)
    {
        m_pRootSignature->Release();
    }

    if (m_pTextureResource)
    {
        m_pTextureResource->Release();
    }

    if (m_pSRVHeap)
    {
        m_pSRVHeap->Release();
    }

    if (m_pSamplerHeap)
    {
        m_pSamplerHeap->Release();
    }
}

void DebugEffect::CreateRootSignature(ScopedD3DRef<ID3D12Device> p_Device)
{
    constexpr D3D12_ROOT_SIGNATURE_FLAGS s_RootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_PARAMETER s_RootParameters[RootParameterIndex::RootParameterCount] = {};
    s_RootParameters[RootParameterIndex::ConstantBuffer].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_ROOT_SIGNATURE_DESC s_RootSignatureDesc = {};
    const CD3DX12_DESCRIPTOR_RANGE textureSRV = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    const CD3DX12_DESCRIPTOR_RANGE textureSampler = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

    s_RootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(1, &textureSRV, D3D12_SHADER_VISIBILITY_PIXEL);
    s_RootParameters[RootParameterIndex::TextureSampler].InitAsDescriptorTable(1, &textureSampler, D3D12_SHADER_VISIBILITY_PIXEL);

    s_RootSignatureDesc.Init(static_cast<UINT>(std::size(s_RootParameters)), s_RootParameters, 0, nullptr, s_RootSignatureFlags);

    HRESULT result = DirectX::CreateRootSignature(p_Device, &s_RootSignatureDesc, &m_pRootSignature);

    if (FAILED(result))
    {
        Logger::Error("Failed to create root signature!");
    }
}

void DebugEffect::CreateTexture(
    ScopedD3DRef<ID3D12Device> p_Device, ScopedD3DRef<ID3D12GraphicsCommandList> p_CommandList,
    ScopedD3DRef<ID3D12CommandQueue> p_CommandQueue
)
{
    DirectX::ResourceUploadBatch s_Upload(p_Device);

    s_Upload.Begin();

    const UINT s_nTextureWidth = 1;
    const UINT s_nTextureHeight = 1;
    const UINT s_BytesPerPixel = 4;
    const unsigned char s_WhiteTextureData[s_BytesPerPixel] = { 255, 255, 255, 255 };

    auto s_TextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        s_nTextureWidth,
        s_nTextureHeight,
        1, 1);

    CD3DX12_HEAP_PROPERTIES s_DefaultHeap(D3D12_HEAP_TYPE_DEFAULT);

    HRESULT s_Result = p_Device->CreateCommittedResource(
        &s_DefaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &s_TextureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_pTextureResource));

    if (FAILED(s_Result))
    {
        Logger::Error("Failed to create committed resource for DebugEffect texture!");

        return;
    }

    D3D12_SUBRESOURCE_DATA s_InitData = {};
    s_InitData.pData = s_WhiteTextureData;
    s_InitData.RowPitch = s_nTextureWidth * s_BytesPerPixel;
    s_InitData.SlicePitch = s_InitData.RowPitch * s_nTextureWidth;

    s_Upload.Upload(m_pTextureResource, 0, &s_InitData, 1);

    s_Upload.Transition(m_pTextureResource,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    auto s_Finish = s_Upload.End(p_CommandQueue);

    s_Finish.wait();

    CreateShaderResourceView(p_Device, s_TextureDesc, m_pTextureResource);
}

void DebugEffect::CreateShaderResourceView(ScopedD3DRef<ID3D12Device> p_Device, const CD3DX12_RESOURCE_DESC& p_TextureDesc, ID3D12Resource* p_TextureResource)
{
    D3D12_DESCRIPTOR_HEAP_DESC s_SRVHeapDesc = {};
    s_SRVHeapDesc.NumDescriptors = 1;
    s_SRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    s_SRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT s_Result = p_Device->CreateDescriptorHeap(&s_SRVHeapDesc, IID_PPV_ARGS(&m_pSRVHeap));

    if (FAILED(s_Result))
    {
        Logger::Error("Failed to create descriptor heap!");

        return;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC s_SRVDesc = {};
    s_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    s_SRVDesc.Format = p_TextureDesc.Format;
    s_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    s_SRVDesc.Texture2D.MipLevels = 1;

    p_Device->CreateShaderResourceView(p_TextureResource, &s_SRVDesc, m_pSRVHeap->GetCPUDescriptorHandleForHeapStart());
}

void DebugEffect::CreateSampler(ScopedD3DRef<ID3D12Device> p_Device)
{
    D3D12_SAMPLER_DESC s_SamplerDesc = {};
    s_SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    s_SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    s_SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    s_SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    s_SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    s_SamplerDesc.MinLOD = 0;
    s_SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    D3D12_DESCRIPTOR_HEAP_DESC s_SamplerHeapDesc = {};
    s_SamplerHeapDesc.NumDescriptors = 1;
    s_SamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    s_SamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT s_Result = p_Device->CreateDescriptorHeap(&s_SamplerHeapDesc, IID_PPV_ARGS(&m_pSamplerHeap));

    if (FAILED(s_Result))
    {
        Logger::Error("Failed to create descriptor heap!");

        return;
    }

    p_Device->CreateSampler(&s_SamplerDesc, m_pSamplerHeap->GetCPUDescriptorHandleForHeapStart());
}

void DebugEffect::Apply(ScopedD3DRef<ID3D12Device> p_Device, ScopedD3DRef<ID3D12GraphicsCommandList> p_CommandList)
{
    m_constantBufferResource = DirectX::GraphicsMemory::Get(p_Device).AllocateConstant(m_Constants);

    p_CommandList->SetGraphicsRootSignature(m_pRootSignature);

    ID3D12DescriptorHeap* s_Heaps[] = { m_pSRVHeap, m_pSamplerHeap };

    p_CommandList->SetDescriptorHeaps(_countof(s_Heaps), s_Heaps);

    p_CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSRV, m_pSRVHeap->GetGPUDescriptorHandleForHeapStart());
    p_CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSampler, m_pSamplerHeap->GetGPUDescriptorHandleForHeapStart());

    p_CommandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, m_constantBufferResource.GpuAddress());

    p_CommandList->SetPipelineState(m_pPipelineState);
}

void DebugEffect::SetWorld(const DirectX::XMFLOAT4X4& p_World)
{
    m_Constants.world = p_World;
}

void DebugEffect::SetView(const DirectX::XMFLOAT4X4& p_View)
{
    m_Constants.view = p_View;
}

void DebugEffect::SetProjection(const DirectX::XMFLOAT4X4& p_Projection)
{
    m_Constants.projection = p_Projection;
}

void DebugEffect::SetPositionScale(const DirectX::XMFLOAT4& p_PositionScale)
{
    m_Constants.positionScale = p_PositionScale;
}

void DebugEffect::SetPositionBias(const DirectX::XMFLOAT4& p_PositionBias)
{
    m_Constants.positionBias = p_PositionBias;
}

void DebugEffect::SetTextureScaleBias(const DirectX::XMFLOAT4& p_TextureScaleBias)
{
    m_Constants.textureScaleBias = p_TextureScaleBias;
}

void DebugEffect::SetMaterialColor(const DirectX::XMFLOAT4& p_MaterialColor)
{
    m_Constants.materialColor = p_MaterialColor;
}

bool DebugEffect::CompileShaderFromString(
    const std::string& p_ShaderCode,
    const std::string& p_EntryPoint,
    const std::string& p_ShaderModel,
    ID3DBlob** p_ShaderBlob
)
{
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
        &s_ErrorBlob.Ref);

    if (FAILED(s_Result))
    {
        if (s_ErrorBlob)
        {
            Logger::Error("{}", static_cast<const char*>(s_ErrorBlob->GetBufferPointer()));
        }

        return false;
    }

    return true;
}
