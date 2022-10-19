#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <memory>

#include <GraphicsMemory.h>

#include "DescriptorHeap.h"
#include "Effects.h"
#include "PrimitiveBatch.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "VertexTypes.h"
#include "Hooks.h"
#include "IRenderer.h"
#include "SpriteFont.h"

class SGameUpdateEvent;

namespace Rendering::Renderers
{
	class DirectXTKRenderer : public IRenderer
	{
	private:
		struct FrameContext
		{
			size_t Index = 0;

			ID3D12CommandAllocator* CommandAllocator = nullptr;
			ID3D12GraphicsCommandList* CommandList = nullptr;

			ID3D12Resource* BackBuffer = nullptr;
			D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = { 0 };

			ID3D12Fence* Fence = nullptr;
			HANDLE FenceEvent = nullptr;
			volatile uint64_t FenceValue = 0;

			volatile bool Recording = false;
		};

		enum class Descriptors : int
		{
			FontRegular,
			FontBold,
			Count
		};

	public:
		DirectXTKRenderer();
		~DirectXTKRenderer();

	public:
		void OnEngineInit();
		void Shutdown();

	public:
		void OnPresent(IDXGISwapChain3* p_SwapChain);
		void PostPresent(IDXGISwapChain3* p_SwapChain);
		void SetCommandQueue(ID3D12CommandQueue* p_CommandQueue);
		void OnReset();

	private:
		void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
		bool SetupRenderer(IDXGISwapChain3* p_SwapChain);
		void WaitForGpu(FrameContext* p_Frame);
		void ExecuteCmdList(FrameContext* p_Frame);
		void Draw(FrameContext* p_Frame);

	public:
		void DrawLine3D(const SVector3& p_From, const SVector3& p_To, const SVector4& p_FromColor, const SVector4& p_ToColor) override;
		void DrawText2D(const ZString& p_Text, const SVector2& p_Pos, const SVector4& p_Color, float p_Rotation = 0.f, float p_Scale = 1.f, TextAlignment p_Alignment = TextAlignment::Center) override;
		bool WorldToScreen(const SVector3& p_WorldPos, SVector2& p_Out) override;
		bool ScreenToWorld(const SVector2& p_ScreenPos, SVector3& p_WorldPosOut, SVector3& p_DirectionOut) override;
		void DrawBox3D(const SVector3& p_Min, const SVector3& p_Max, const SVector4& p_Color) override;
		void DrawOBB3D(const SVector3& p_Min, const SVector3& p_Max, const SMatrix& p_Transform, const SVector4& p_Color) override;
		
	private:
		bool m_RendererSetup = false;
		UINT m_BufferCount = 0;
		ID3D12DescriptorHeap* m_RtvDescriptorHeap = nullptr;
		//ID3D12CommandQueue* m_CommandQueue = nullptr;
		FrameContext* m_FrameContext = nullptr;
		IDXGISwapChain3* m_SwapChain = nullptr;
		HWND m_Hwnd = nullptr;
		float m_WindowWidth = 1;
		float m_WindowHeight = 1;

	private:
		std::unique_ptr<DirectX::GraphicsMemory> m_GraphicsMemory {};
		std::unique_ptr<DirectX::BasicEffect> m_LineEffect {};
		std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_LineBatch {};
		
		DirectX::SimpleMath::Matrix m_World {};
		DirectX::SimpleMath::Matrix m_View {};
		DirectX::SimpleMath::Matrix m_Projection {};
		DirectX::SimpleMath::Matrix m_ViewProjection {};
		DirectX::SimpleMath::Matrix m_ProjectionViewInverse {};

		std::unique_ptr<DirectX::DescriptorHeap> m_ResourceDescriptors {};
		std::unique_ptr<DirectX::SpriteFont> m_Font {};
		std::unique_ptr<DirectX::SpriteBatch> m_SpriteBatch {};
		
		SRWLOCK m_Lock {};
	};
}
