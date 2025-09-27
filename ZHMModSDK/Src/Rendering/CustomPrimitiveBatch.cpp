//#include "CustomPrimitiveBatch.h"
//
//#include <stdexcept>
//
//CustomPrimitiveBatch::CustomPrimitiveBatch(
//    ID3D12Device* p_Device,
//    FlushBatchCallback p_FlushBatchCallback,
//    size_t p_MaxIndices,
//    size_t p_MaxVertices,
//    size_t p_VertexSize
//)
//    : m_Device(p_Device),
//    m_FlushBatchCallback(p_FlushBatchCallback),
//    m_CommandList(nullptr),
//    m_MaxIndices(p_MaxIndices),
//    m_MaxVertices(p_MaxVertices),
//    m_VertexSize(p_VertexSize),
//    m_VertexPageSize(p_MaxVertices* p_VertexSize),
//    m_IndexPageSize(p_MaxIndices * sizeof(uint16_t)),
//    m_CurrentTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED),
//    m_InBeginEndPair(false),
//    m_CurrentlyIndexed(false),
//    m_IndexCount(0),
//    m_VertexCount(0),
//    m_BaseIndex(0),
//    m_BaseVertex(0) {
//    if (!p_Device) {
//        throw std::invalid_argument("Direct3D device is null");
//    }
//
//    if (!p_MaxVertices) {
//        throw std::invalid_argument("p_MaxVertices must be greater than 0");
//    }
//
//    if (p_VertexSize > D3D12_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES) {
//        throw std::invalid_argument("Vertex size is too large for DirectX 12");
//    }
//
//    if ((uint64_t(p_MaxIndices) * sizeof(uint16_t)) > uint64_t(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u)) {
//        throw std::invalid_argument("IB too large for DirectX 12");
//    }
//
//    if ((uint64_t(p_MaxVertices) * uint64_t(p_VertexSize)) > uint64_t(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u)) {
//        throw std::invalid_argument("VB too large for DirectX 12");
//    }
//}
//
//void CustomPrimitiveBatch::Begin(ID3D12GraphicsCommandList* p_CommandList) {
//    if (m_InBeginEndPair) {
//        throw std::logic_error("Cannot nest Begin calls");
//    }
//
//    m_CommandList = p_CommandList;
//    m_InBeginEndPair = true;
//}
//
//void CustomPrimitiveBatch::End() {
//    if (!m_InBeginEndPair) {
//        throw std::logic_error("Begin must be called before End");
//    }
//
//    FlushBatch();
//
//    m_IndexSegment.Reset();
//    m_VertexSegment.Reset();
//    m_CommandList.Reset();
//    m_InBeginEndPair = false;
//}
//
//void CustomPrimitiveBatch::Draw(
//    D3D_PRIMITIVE_TOPOLOGY p_Topology,
//    bool p_IsIndexed,
//    uint16_t const* p_Indices,
//    size_t p_IndexCount,
//    size_t p_VertexCount,
//    void** p_MappedVertices
//) {
//    if (p_IsIndexed && !p_Indices) {
//        throw std::invalid_argument("Indices cannot be null");
//    }
//
//    if (p_IndexCount >= m_MaxIndices) {
//        throw std::invalid_argument("Too many indices");
//    }
//
//    if (p_VertexCount >= m_MaxVertices) {
//        throw std::invalid_argument("Too many vertices");
//    }
//
//    if (!m_InBeginEndPair) {
//        throw std::logic_error("Begin must be called before Draw");
//    }
//
//    assert(p_MappedVertices != nullptr);
//
//    const bool s_WrapIndexBuffer = m_IndexCount + p_IndexCount > m_MaxIndices;
//    const bool s_WrapVertexBuffer = m_VertexCount + p_VertexCount > m_MaxVertices;
//
//    if (p_Topology != m_CurrentTopology ||
//        p_IsIndexed != m_CurrentlyIndexed ||
//        s_WrapIndexBuffer || s_WrapVertexBuffer) {
//        FlushBatch();
//    }
//
//    if (m_CurrentTopology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED) {
//        m_IndexCount = 0;
//        m_VertexCount = 0;
//        m_BaseIndex = 0;
//        m_BaseVertex = 0;
//        m_CurrentTopology = p_Topology;
//        m_CurrentlyIndexed = p_IsIndexed;
//
//        if (p_IsIndexed) {
//            m_IndexSegment = GraphicsMemory::Get(mDevice.Get()).Allocate(m_IndexPageSize, 16, GraphicsMemory::TAG_INDEX);
//        }
//
//        m_VertexSegment = GraphicsMemory::Get(mDevice.Get()).Allocate(m_VertexPageSize, 16, GraphicsMemory::TAG_VERTEX);
//    }
//
//    if (isIndexed) {
//        auto outputIndices = static_cast<uint16_t*>(m_IndexSegment.Memory()) + m_IndexCount;
//
//        for (size_t i = 0; i < indexCount; i++) {
//            outputIndices[i] = static_cast<uint16_t>(indices[i] + m_VertexCount - m_BaseIndex);
//        }
//
//        m_IndexCount += p_IndexCount;
//    }
//
//    *p_MappedVertices = static_cast<uint8_t*>(m_VertexSegment.Memory()) + m_VertexSize * m_VertexCount;
//
//    m_VertexCount += p_VertexCount;
//}
//
//void CustomPrimitiveBatch::FlushBatch() {
//    if (m_CurrentTopology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED) {
//        return;
//    }
//
//    m_FlushBatchCallback();
//
//    m_CommandList->IASetPrimitiveTopology(m_CurrentTopology);
//
//    D3D12_VERTEX_BUFFER_VIEW s_VertexBufferView;
//    s_VertexBufferView.BufferLocation = m_VertexSegment.GpuAddress();
//    s_VertexBufferView.SizeInBytes = static_cast<UINT>(m_VertexSize * (m_VertexCount - m_BaseVertex));
//    s_VertexBufferView.StrideInBytes = static_cast<UINT>(m_VertexSize);
//
//    m_CommandList->IASetVertexBuffers(0, 1, &s_VertexBufferView);
//
//    if (m_CurrentlyIndexed) {
//        D3D12_INDEX_BUFFER_VIEW s_IndexBufferView;
//        s_IndexBufferView.BufferLocation = m_IndexSegment.GpuAddress();
//        s_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
//        s_IndexBufferView.SizeInBytes = static_cast<UINT>(m_IndexCount - m_BaseIndex) * sizeof(uint16_t);
//
//        m_CommandList->IASetIndexBuffer(&s_IndexBufferView);
//
//        m_CommandList->DrawIndexedInstanced(static_cast<UINT>(m_IndexCount - m_BaseIndex), 1, 0, 0, 0);
//    }
//    else
//    {
//        m_CommandList->DrawInstanced(static_cast<UINT>(m_VertexCount - m_BaseVertex), 1, 0, 0);
//    }
//
//    m_CurrentTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
//}
