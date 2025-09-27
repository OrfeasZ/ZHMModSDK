#pragma once

#include <directx/d3d12.h>
#include <cstdint>
#include <functional>

#include "GraphicsMemory.h"

#include "D3DUtils.h"

template <typename TVertex>
class CustomPrimitiveBatch
{
private:
    using FlushBatchCallback = std::function<void()>;

public:
    CustomPrimitiveBatch(
        ID3D12Device* p_Device,
        FlushBatchCallback p_FlushBatchCallback,
        size_t p_MaxIndices = m_DefaultBatchSize * 3,
        size_t p_MaxVertices = m_DefaultBatchSize,
        size_t p_VertexSize = sizeof(TVertex)
    )
        : m_Device(p_Device),
        m_FlushBatchCallback(p_FlushBatchCallback),
        m_CommandList(nullptr),
        m_MaxIndices(p_MaxIndices),
        m_MaxVertices(p_MaxVertices),
        m_VertexSize(p_VertexSize),
        m_VertexPageSize(p_MaxVertices* p_VertexSize),
        m_IndexPageSize(p_MaxIndices * sizeof(uint16_t)),
        m_CurrentTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED),
        m_InBeginEndPair(false),
        m_CurrentlyIndexed(false),
        m_IndexCount(0),
        m_VertexCount(0),
        m_BaseIndex(0),
        m_BaseVertex(0)
    {
        if (!p_Device)
        {
            throw std::invalid_argument("Direct3D device is null");
        }

        if (!p_MaxVertices)
        {
            throw std::invalid_argument("p_MaxVertices must be greater than 0");
        }

        if (p_VertexSize > D3D12_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES)
        {
            throw std::invalid_argument("Vertex size is too large for DirectX 12");
        }

        if ((uint64_t(p_MaxIndices) * sizeof(uint16_t)) > uint64_t(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u))
        {
            throw std::invalid_argument("IB too large for DirectX 12");
        }

        if ((uint64_t(p_MaxVertices) * uint64_t(p_VertexSize)) > uint64_t(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u))
        {
            throw std::invalid_argument("VB too large for DirectX 12");
        }
    }

    CustomPrimitiveBatch(CustomPrimitiveBatch&&) = default;
    CustomPrimitiveBatch& operator= (CustomPrimitiveBatch&&) = default;

    CustomPrimitiveBatch(CustomPrimitiveBatch const&) = delete;
    CustomPrimitiveBatch& operator= (CustomPrimitiveBatch const&) = delete;

    void Begin(ID3D12GraphicsCommandList* p_CommandList)
    {
        if (m_InBeginEndPair)
        {
            throw std::logic_error("Cannot nest Begin calls");
        }

        m_CommandList = p_CommandList;
        m_InBeginEndPair = true;
    }
    void End()
    {
        if (!m_InBeginEndPair)
        {
            throw std::logic_error("Begin must be called before End");
        }

        FlushBatch();

        m_IndexSegment.Reset();
        m_VertexSegment.Reset();
        m_CommandList.Reset();
        m_InBeginEndPair = false;
    }

    void Draw(
        D3D_PRIMITIVE_TOPOLOGY p_Topology,
        bool p_IsIndexed,
        uint16_t const* p_Indices,
        size_t p_IndexCount,
        size_t p_VertexCount,
        void** p_MappedVertices
    )
    {
        if (p_IsIndexed && !p_Indices)
        {
            throw std::invalid_argument("Indices cannot be null");
        }

        if (p_IndexCount >= m_MaxIndices)
        {
            throw std::invalid_argument("Too many indices");
        }

        if (p_VertexCount >= m_MaxVertices)
        {
            throw std::invalid_argument("Too many vertices");
        }

        if (!m_InBeginEndPair)
        {
            throw std::logic_error("Begin must be called before Draw");
        }

        assert(p_MappedVertices != nullptr);

        const bool s_WrapIndexBuffer = m_IndexCount + p_IndexCount > m_MaxIndices;
        const bool s_WrapVertexBuffer = m_VertexCount + p_VertexCount > m_MaxVertices;

        if (p_Topology != m_CurrentTopology ||
            p_IsIndexed != m_CurrentlyIndexed ||
            s_WrapIndexBuffer || s_WrapVertexBuffer)
        {
            FlushBatch();
        }

        if (m_CurrentTopology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
        {
            m_IndexCount = 0;
            m_VertexCount = 0;
            m_BaseIndex = 0;
            m_BaseVertex = 0;
            m_CurrentTopology = p_Topology;
            m_CurrentlyIndexed = p_IsIndexed;

            if (p_IsIndexed)
            {
                m_IndexSegment = DirectX::GraphicsMemory::Get(m_Device).Allocate(m_IndexPageSize, 16, DirectX::GraphicsMemory::TAG_INDEX);
            }

            m_VertexSegment = DirectX::GraphicsMemory::Get(m_Device).Allocate(m_VertexPageSize, 16, DirectX::GraphicsMemory::TAG_VERTEX);
        }

        if (p_IsIndexed)
        {
            auto outputIndices = static_cast<uint16_t*>(m_IndexSegment.Memory()) + m_IndexCount;

            for (size_t i = 0; i < p_IndexCount; i++)
            {
                outputIndices[i] = static_cast<uint16_t>(p_Indices[i] + m_VertexCount - m_BaseIndex);
            }

            m_IndexCount += p_IndexCount;
        }

        *p_MappedVertices = static_cast<uint8_t*>(m_VertexSegment.Memory()) + m_VertexSize * m_VertexCount;

        m_VertexCount += p_VertexCount;
    }

    void Draw(
        D3D_PRIMITIVE_TOPOLOGY p_Topology,
        TVertex const* p_Vertices,
        size_t p_VertexCount
    )
    {
        void* s_MappedVertices;

        Draw(p_Topology, false, nullptr, 0, p_VertexCount, &s_MappedVertices);

        memcpy(s_MappedVertices, p_Vertices, p_VertexCount * sizeof(TVertex));
    }

    void DrawIndexed(
        D3D_PRIMITIVE_TOPOLOGY p_Topology,
        uint16_t const* indices, size_t p_IndexCount,
        TVertex const* vertices, size_t p_VertexCount)
    {
        void* s_MappedVertices;

        Draw(p_Topology, true, indices, p_IndexCount, p_VertexCount, &s_MappedVertices);

        memcpy(s_MappedVertices, vertices, p_VertexCount * sizeof(TVertex));
    }

    void DrawLine(
        TVertex const& v1,
        TVertex const& v2)
    {
        TVertex* s_MappedVertices;

        Draw(D3D_PRIMITIVE_TOPOLOGY_LINELIST, false, nullptr, 0, 2, reinterpret_cast<void**>(&s_MappedVertices));

        s_MappedVertices[0] = v1;
        s_MappedVertices[1] = v2;
    }

    void DrawTriangle(
        TVertex const& p_V1,
        TVertex const& p_V2,
        TVertex const& p_V3)
    {
        TVertex* s_MappedVertices;

        Draw(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, false, nullptr, 0, 3, reinterpret_cast<void**>(&s_MappedVertices));

        s_MappedVertices[0] = p_V1;
        s_MappedVertices[1] = p_V2;
        s_MappedVertices[2] = p_V3;
    }

    void DrawQuad(
        TVertex const& p_V1,
        TVertex const& p_V2,
        TVertex const& p_V3,
        TVertex const& p_V4)
    {
        static const uint16_t s_QuadIndices[] = { 0, 1, 2, 0, 2, 3 };
        TVertex* s_MappedVertices;

        Draw(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, true, s_QuadIndices, 6, 4, reinterpret_cast<void**>(&s_MappedVertices));

        s_MappedVertices[0] = p_V1;
        s_MappedVertices[1] = p_V2;
        s_MappedVertices[2] = p_V3;
        s_MappedVertices[3] = p_V4;
    }

private:
    void FlushBatch()
    {
        if (m_CurrentTopology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
        {
            return;
        }

        m_FlushBatchCallback();

        m_CommandList->IASetPrimitiveTopology(m_CurrentTopology);

        D3D12_VERTEX_BUFFER_VIEW s_VertexBufferView;
        s_VertexBufferView.BufferLocation = m_VertexSegment.GpuAddress();
        s_VertexBufferView.SizeInBytes = static_cast<UINT>(m_VertexSize * (m_VertexCount - m_BaseVertex));
        s_VertexBufferView.StrideInBytes = static_cast<UINT>(m_VertexSize);

        m_CommandList->IASetVertexBuffers(0, 1, &s_VertexBufferView);

        if (m_CurrentlyIndexed)
        {
            D3D12_INDEX_BUFFER_VIEW s_IndexBufferView;
            s_IndexBufferView.BufferLocation = m_IndexSegment.GpuAddress();
            s_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
            s_IndexBufferView.SizeInBytes = static_cast<UINT>(m_IndexCount - m_BaseIndex) * sizeof(uint16_t);

            m_CommandList->IASetIndexBuffer(&s_IndexBufferView);

            m_CommandList->DrawIndexedInstanced(static_cast<UINT>(m_IndexCount - m_BaseIndex), 1, 0, 0, 0);
        }
        else
        {
            m_CommandList->DrawInstanced(static_cast<UINT>(m_VertexCount - m_BaseVertex), 1, 0, 0);
        }

        m_CurrentTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }

    FlushBatchCallback m_FlushBatchCallback;

    static constexpr size_t m_DefaultBatchSize = 4096;

    DirectX::GraphicsResource m_VertexSegment;
    DirectX::GraphicsResource m_IndexSegment;

    ScopedD3DRef<ID3D12Device> m_Device;
    ScopedD3DRef<ID3D12GraphicsCommandList> m_CommandList;

    size_t m_MaxIndices;
    size_t m_MaxVertices;
    size_t m_VertexSize;
    size_t m_VertexPageSize;
    size_t m_IndexPageSize;

    D3D_PRIMITIVE_TOPOLOGY m_CurrentTopology;
    bool m_InBeginEndPair;
    bool m_CurrentlyIndexed;

    size_t m_IndexCount;
    size_t m_VertexCount;

    size_t m_BaseIndex;
    size_t m_BaseVertex;
};
