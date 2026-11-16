#pragma once

#include <optional>

#include "D3DUtils.h"
#include "IRenderer.h"

#include <DirectXTex.h>

// Render thread only.
class TextureReadback {
public:
    ~TextureReadback();

    bool IsBusy() const;

    // Copies the first subresource on the game's command queue. p_Texture must be in p_State.
    bool Start(ID3D12Resource* p_Texture, D3D12_RESOURCE_STATES p_State);

    std::optional<DirectX::ScratchImage> Finish();

private:
    bool CreateCommandObjects(ID3D12Device* p_Device);

    ScopedD3DRef<ID3D12CommandAllocator> m_CommandAllocator;
    ScopedD3DRef<ID3D12GraphicsCommandList> m_CommandList;
    ScopedD3DRef<ID3D12Fence> m_Fence;
    uint64_t m_FenceValue = 0;

    ScopedD3DRef<ID3D12Resource> m_Buffer;
    uint64_t m_BufferSize = 0;

    bool m_IsPending = false;
    D3D12_RESOURCE_DESC m_TextureDesc {};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT m_Footprint {};
    uint32_t m_RowCount = 0;
    uint64_t m_RowSize = 0;
};
