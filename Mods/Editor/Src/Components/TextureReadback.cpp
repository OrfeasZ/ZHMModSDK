#include "TextureReadback.h"

#include <cstring>

#include "Logging.h"

#include <Globals.h>
#include <Glacier/ZRender.h>

TextureReadback::~TextureReadback() {
    // The GPU may still be writing to the buffer.
    if (m_IsPending) {
        m_Fence->SetEventOnCompletion(m_FenceValue, nullptr);
    }
}

bool TextureReadback::IsBusy() const {
    return m_IsPending;
}

bool TextureReadback::CreateCommandObjects(ID3D12Device* p_Device) {
    if (FAILED(p_Device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_CommandAllocator.ReleaseAndGetPtr())
    ))) {
        return false;
    }

    if (FAILED(p_Device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator, nullptr, IID_PPV_ARGS(m_CommandList.ReleaseAndGetPtr())
    ))) {
        return false;
    }

    // Start() resets the list before recording, which needs it closed.
    m_CommandList->Close();

    return SUCCEEDED(p_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.ReleaseAndGetPtr())));
}

bool TextureReadback::Start(ID3D12Resource* p_Texture, D3D12_RESOURCE_STATES p_State) {
    const auto s_RenderDevice = Globals::RenderManager->m_pDevice;
    ID3D12Device* s_Device = s_RenderDevice->m_pDevice;

    if (m_IsPending) {
        return false;
    }

    const D3D12_RESOURCE_DESC s_Desc = p_Texture->GetDesc();

    if (s_Desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || s_Desc.SampleDesc.Count > 1) {
        Logger::Error(
            "[Editor] Can't read back a texture of dimension {} with {} sample(s).",
            static_cast<int>(s_Desc.Dimension),
            s_Desc.SampleDesc.Count
        );
        return false;
    }

    if (!m_Fence && !CreateCommandObjects(s_Device)) {
        Logger::Error("[Editor] Failed to create the command objects for texture readback.");
        return false;
    }

    uint64_t s_BufferSize = 0;
    s_Device->GetCopyableFootprints(&s_Desc, 0, 1, 0, &m_Footprint, &m_RowCount, &m_RowSize, &s_BufferSize);

    if (s_BufferSize > m_BufferSize) {
        const D3D12_HEAP_PROPERTIES s_HeapProperties {.Type = D3D12_HEAP_TYPE_READBACK};

        const D3D12_RESOURCE_DESC s_BufferDesc {
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Width = s_BufferSize,
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_UNKNOWN,
            .SampleDesc = {.Count = 1},
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        };

        if (FAILED(s_Device->CreateCommittedResource(
            &s_HeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &s_BufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(m_Buffer.ReleaseAndGetPtr())
        ))) {
            Logger::Error("[Editor] Failed to create a texture readback buffer of {} bytes.", s_BufferSize);
            m_BufferSize = 0;
            return false;
        }

        m_BufferSize = s_BufferSize;
    }

    m_CommandAllocator->Reset();
    m_CommandList->Reset(m_CommandAllocator, nullptr);

    D3D12_RESOURCE_BARRIER s_Barrier {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition = {
            .pResource = p_Texture,
            .Subresource = 0,
            .StateBefore = p_State,
            .StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE,
        },
    };

    m_CommandList->ResourceBarrier(1, &s_Barrier);

    const D3D12_TEXTURE_COPY_LOCATION s_Destination {
        .pResource = m_Buffer,
        .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
        .PlacedFootprint = m_Footprint,
    };

    const D3D12_TEXTURE_COPY_LOCATION s_Source {
        .pResource = p_Texture,
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = 0,
    };

    m_CommandList->CopyTextureRegion(&s_Destination, 0, 0, 0, &s_Source, nullptr);

    std::swap(s_Barrier.Transition.StateBefore, s_Barrier.Transition.StateAfter);
    m_CommandList->ResourceBarrier(1, &s_Barrier);

    if (FAILED(m_CommandList->Close())) {
        Logger::Error("[Editor] Failed to record the texture readback.");
        return false;
    }

    ID3D12CommandList* s_CommandLists[] = {m_CommandList};
    s_RenderDevice->m_pCommandQueue->ExecuteCommandLists(1, s_CommandLists);
    s_RenderDevice->m_pCommandQueue->Signal(m_Fence, ++m_FenceValue);

    m_TextureDesc = s_Desc;
    m_IsPending = true;

    return true;
}

std::optional<DirectX::ScratchImage> TextureReadback::Finish() {
    if (!m_IsPending || m_Fence->GetCompletedValue() < m_FenceValue) {
        return std::nullopt;
    }

    m_IsPending = false;

    DirectX::ScratchImage s_Image;

    if (FAILED(s_Image.Initialize2D(
        m_TextureDesc.Format, static_cast<size_t>(m_TextureDesc.Width), m_TextureDesc.Height, 1, 1
    ))) {
        Logger::Error("[Editor] Failed to create an image for a texture readback (format {}).", static_cast<int>(m_TextureDesc.Format));
        return std::nullopt;
    }

    const D3D12_RANGE s_ReadRange {0, m_BufferSize};
    void* s_Data = nullptr;

    if (FAILED(m_Buffer->Map(0, &s_ReadRange, &s_Data))) {
        Logger::Error("[Editor] Failed to map the texture readback buffer.");
        return std::nullopt;
    }

    const DirectX::Image* s_Destination = s_Image.GetImage(0, 0, 0);
    const auto* s_Source = static_cast<const uint8_t*>(s_Data) + m_Footprint.Offset;
    const size_t s_RowSize = std::min<size_t>(m_RowSize, s_Destination->rowPitch);

    for (uint32_t i = 0; i < m_RowCount; ++i) {
        std::memcpy(
            s_Destination->pixels + i * s_Destination->rowPitch,
            s_Source + i * static_cast<size_t>(m_Footprint.Footprint.RowPitch),
            s_RowSize
        );
    }

    const D3D12_RANGE s_WrittenRange {0, 0};
    m_Buffer->Unmap(0, &s_WrittenRange);

    return s_Image;
}
