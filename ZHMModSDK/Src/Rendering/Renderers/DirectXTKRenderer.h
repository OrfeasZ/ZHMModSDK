#pragma once

#include <directx/d3d12.h>
#include <dxgi1_4.h>
#include <memory>

#include <GraphicsMemory.h>

#include "DescriptorHeap.h"
#include "Effects.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "VertexTypes.h"
#include "Hooks.h"
#include "IRenderer.h"
#include "SpriteFont.h"
#include "D3DUtils.h"
#include "CommonStates.h"

#include "../DebugEffect.h"
#include "Rendering/ViewFrustum.h"
#include "../CustomPrimitiveBatch.h"

class SGameUpdateEvent;

namespace Rendering::Renderers {
    class DirectXTKRenderer : public IRenderer {
    private:
        struct FrameContext {
            ScopedD3DRef<ID3D12CommandAllocator> CommandAllocator;
            volatile uint64_t FenceValue = 0;
        };

        enum class Descriptors : int {
            FontRegular,
            FontBold,
            Count
        };

        enum RootParameterIndex {
            ConstantBuffer,
            TextureSRV,
            TextureSampler,
            RootParameterCount
        };

        struct Text2D {
            std::string m_Text;
            SVector2 m_Position;
            SVector4 m_Color;
            float m_Rotation = 0.f;
            float m_Scale = 1.f;
            TextAlignment m_HorizontalAlignment = TextAlignment::Center;
            TextAlignment m_VerticalAlignment = TextAlignment::Center;
        };

    public:
        DirectXTKRenderer();
        ~DirectXTKRenderer();

    public:
        void OnEngineInit();

    public:
        void OnPresent(IDXGISwapChain3* p_SwapChain);
        void PostPresent(IDXGISwapChain3* p_SwapChain, HRESULT p_PresentResult);
        void SetCommandQueue(ID3D12CommandQueue* p_CommandQueue);
        void OnReset();
        void PostReset();
        void SetDsvIndex(size_t p_Index) { m_DsvIndex = p_Index; }
        void ClearDsvIndex() { m_DsvIndex = std::nullopt; }

    private:
        void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
        bool SetupRenderer(IDXGISwapChain3* p_SwapChain);
        void Draw();
        void DepthDraw();
        void WaitForCurrentFrameToFinish() const;

        bool CompileShaderFromString(
            const std::string& p_ShaderCode, const std::string& p_EntryPoint, const std::string& p_ShaderModel,
            ID3DBlob** p_ShaderBlob
        );
        bool CreateFontDistanceFieldTexture();

        void DrawText2D(const Text2D& p_Text2D);

    public:
        bool WorldToScreen(const SVector3& p_WorldPos, SVector2& p_Out) override;
        bool ScreenToWorld(const SVector2& p_ScreenPos, SVector3& p_WorldPosOut, SVector3& p_DirectionOut) override;

        void DrawLine3D(
            const SVector3& p_From, const SVector3& p_To, const SVector4& p_FromColor, const SVector4& p_ToColor
        ) override;
        
        void DrawBox3D(const SVector3& p_Min, const SVector3& p_Max, const SVector4& p_Color) override;
        void DrawBox3D(
            const SVector3& p_Center, const SVector3& p_HalfSize, const SMatrix& p_Transform, const SVector4& p_Color
        ) override;
        void DrawBoxWire3D(
            const SVector3& p_Center, const SVector3& p_HalfSize, const SMatrix& p_Transform, const SVector4& p_Color
        ) override;

        void DrawOBB3D(
            const SVector3& p_Min, const SVector3& p_Max, const SMatrix& p_Transform, const SVector4& p_Color
        ) override;

        void DrawBoundingQuads3D(
            const SVector3& p_Min, const SVector3& p_Max, const SMatrix& p_Transform, const SVector4& p_Color
        ) override;

        void DrawTriangle3D(
            const SVector3& p_V1, const SVector4& p_Color1,
            const SVector3& p_V2, const SVector4& p_Color2,
            const SVector3& p_V3, const SVector4& p_Color3
        ) override;

        void DrawTriangle3D(
            const SVector3& p_V1, const SVector4& p_Color1, const SVector2& p_TextureCoordinates1,
            const SVector3& p_V2, const SVector4& p_Color2, const SVector2& p_TextureCoordinates2,
            const SVector3& p_V3, const SVector4& p_Color3, const SVector2& p_TextureCoordinates3
        ) override;

        void DrawQuad3D(
            const SVector3& p_V1, const SVector4& p_Color1, const SVector3& p_V2, const SVector4& p_Color2,
            const SVector3& p_V3, const SVector4& p_Color3, const SVector3& p_V4, const SVector4& p_Color4
        ) override;

        void DrawText2D(
            const ZString& p_Text, const SVector2& p_Pos, const SVector4& p_Color, float p_Rotation = 0.f,
            float p_Scale = 1.f, TextAlignment p_HorizontalAlignment = TextAlignment::Center,
            TextAlignment p_VerticalAlignment = TextAlignment::Top
        ) override;

        void DrawText3D(
            const std::string& p_Text, const SMatrix& p_Transform,
            const SVector4& p_Color, float p_Scale = 1.f,
            TextAlignment p_HorizontalAlignment = TextAlignment::Left,
            TextAlignment p_VerticalAlignment = TextAlignment::Top,
            const bool p_IsCameraTransform = true
        );

        void DrawText3D(
            const char* p_Text, const SMatrix& p_Transform,
            const SVector4& p_Color, float p_Scale = 1.f,
            TextAlignment p_HorizontalAlignment = TextAlignment::Left,
            TextAlignment p_VerticalAlignment = TextAlignment::Top,
            const bool p_IsCameraTransform = true
        );

        void DrawMesh(
            const std::vector<SVector3>& p_Vertices, const std::vector<unsigned short>& p_Indices,
            const SVector4& p_VertexColor
        ) override;

        void DrawMesh(
            ZRenderPrimitiveResource* s_pRenderPrimitiveResource,
            ZRenderVertexBuffer** p_VertexBuffers, const uint32_t p_VertexBufferCount,
            ZRenderIndexBuffer* p_IndexBuffer,
            const SMatrix& p_Transform, const float4& p_PositionScale, const float4& p_PositionBias,
            const float4& p_TextureScaleBias,
            const SVector4& p_MaterialColor
        ) override;

        bool IsPointInsideViewFrustum(const SVector3& p_Point) const override;
        bool IsAABBInsideViewFrustum(
            const SVector3& p_Min, const SVector3& p_Max, const SMatrix& p_Transform
        ) const override;
        bool IsOBBInsideViewFrustum(
            const float4& p_Center, const float4& p_HalfSize, const SMatrix& p_Transform
        ) const override;

        void SetFrustumCullingEnabled(const bool p_Enabled) override;
        bool IsFrustumCullingEnabled() const override;

        void SetDistanceCullingEnabled(const bool p_Enabled) override;
        bool IsDistanceCullingEnabled() const override;

        void SetMaxDrawDistance(const float p_MaxDrawDistance) override;
        float GetMaxDrawDistance() const override;

        static AABB TransformAABB(const DirectX::SimpleMath::Matrix& p_Transform, const AABB& p_AABB);

    private:
        bool m_RendererSetup = false;

        ScopedD3DRef<ID3D12CommandQueue> m_CommandQueue;
        ScopedD3DRef<IDXGISwapChain3> m_SwapChain;
        HWND m_Hwnd = nullptr;

        uint32_t m_RtvDescriptorSize = 0;
        uint32_t m_DsvDescriptorSize = 0;
        ScopedD3DRef<ID3D12DescriptorHeap> m_RtvDescriptorHeap;

        /** The maximum number of frames that can be buffered for render. */
        inline constexpr static size_t MaxRenderedFrames = 4;
        std::vector<FrameContext> m_FrameContext;

        std::vector<ScopedD3DRef<ID3D12Resource>> m_BackBuffers;

        ScopedD3DRef<ID3D12GraphicsCommandList> m_CommandList;

        ScopedD3DRef<ID3D12Fence> m_Fence;
        SafeHandle m_FenceEvent;

        volatile uint32_t m_FrameCounter = 0;
        volatile uint64_t m_FenceValue = 0;

        float m_WindowWidth = 1;
        float m_WindowHeight = 1;

        std::unique_ptr<DirectX::GraphicsMemory> m_GraphicsMemory {};
        std::unique_ptr<DirectX::BasicEffect> m_TriangleEffect {};
        std::unique_ptr<DirectX::BasicEffect> m_LineEffect {};
        std::unique_ptr<DirectX::BasicEffect> m_TextEffect {};
        std::unique_ptr<DebugEffect> m_DebugEffect {};
        std::unique_ptr<CustomPrimitiveBatch<DirectX::VertexPositionColor>> m_TriangleBatch {};
        std::unique_ptr<CustomPrimitiveBatch<DirectX::VertexPositionColor>> m_LineBatch {};
        std::unique_ptr<CustomPrimitiveBatch<DirectX::VertexPositionColorTexture>> m_TextBatch {};
        std::vector<Text2D> m_Text2DBuffer;

        DirectX::SimpleMath::Matrix m_World {};
        DirectX::SimpleMath::Matrix m_View {};
        DirectX::SimpleMath::Matrix m_Projection {};
        DirectX::SimpleMath::Matrix m_ViewProjection {};
        DirectX::SimpleMath::Matrix m_ProjectionViewInverse {};

        std::unique_ptr<DirectX::DescriptorHeap> m_ResourceDescriptors {};
        std::unique_ptr<DirectX::SpriteFont> m_Font {};
        std::unique_ptr<DirectX::SpriteBatch> m_SpriteBatch {};

        std::optional<size_t> m_DsvIndex = std::nullopt;

        std::unique_ptr<DirectX::CommonStates> m_CommonStates;
        ScopedD3DRef<ID3D12Resource> m_FontDistanceFieldTexture;
        ScopedD3DRef<ID3D12DescriptorHeap> m_fontSRVDescriptorHeap;
        ScopedD3DRef<ID3D12PipelineState> m_PipelineState;

        ViewFrustum m_ViewFrustum;
        bool m_IsFrustumCullingEnabled = true;
    };
}