#pragma once

#include "ZMemory.h"
#include "Globals.h"

class ZBuffer {
public:
    static ZBuffer FromData(const std::string& p_Data) {
        const auto s_Size = static_cast<uint32_t>(p_Data.size());

        const auto s_DataBuffer = (*Globals::MemoryManager)->m_pNormalAllocator->Allocate(sizeof(ZBuffer) + s_Size);

        const auto s_Data = new(s_DataBuffer) SBufferData();
        s_Data->m_nRefCount = 1;
        s_Data->m_nSize = s_Size;

        ZBuffer s_Buffer(s_Data);
        memcpy(s_Buffer.Get(), p_Data.data(), s_Size);

        return s_Buffer;
    }

private:
    struct SBufferData {
        int32_t m_nRefCount;
        uint32_t m_nSize;
    };

private:
    ZBuffer(SBufferData* p_Data) :
        m_pData(p_Data) {
    }

public:
    ZBuffer(const ZBuffer& p_Other) :
        m_pData(p_Other.m_pData) {
        IncReference();
    }

    ~ZBuffer() {
        Clear();
    }

    ZBuffer& operator=(const ZBuffer& p_Other) {
        Clear();

        m_pData = p_Other.m_pData;

        IncReference();

        return *this;
    }

    const void* Get() const {
        if (!m_pData) {
            return nullptr;
        }

        const auto s_DataOffset = reinterpret_cast<uintptr_t>(m_pData) + sizeof(ZBuffer);

        return reinterpret_cast<void*>(s_DataOffset);
    }

    void* Get() {
        if (!m_pData) {
            return nullptr;
        }

        const auto s_DataOffset = reinterpret_cast<uintptr_t>(m_pData) + sizeof(ZBuffer);

        return reinterpret_cast<void*>(s_DataOffset);
    }

    uint32_t Size() const {
        if (!m_pData) {
            return 0;
        }

        return m_pData->m_nSize;
    }

private:
    void IncReference() {
        if (!m_pData) {
            return;
        }

        ++m_pData->m_nRefCount;
    }

    void DecReference() {
        if (!m_pData) {
            return;
        }

        --m_pData->m_nRefCount;
    }

    void Clear() {
        if (!m_pData) {
            return;
        }

        if (m_pData->m_nRefCount > 1) {
            --m_pData->m_nRefCount;
            m_pData = nullptr;
            return;
        }

        auto* s_MemoryManager = *Globals::MemoryManager;

        IAllocator* s_Allocator = nullptr;

        if (s_MemoryManager->m_pPageAllocator) {
            s_Allocator = s_MemoryManager->m_pPageAllocator->GetAllocator(m_pData);
        }

        if (!s_Allocator) {
            s_Allocator = s_MemoryManager->m_pNormalAllocator;
        }

        s_Allocator->Free(m_pData);
        m_pData = nullptr;
    }

private:
    SBufferData* m_pData;
};