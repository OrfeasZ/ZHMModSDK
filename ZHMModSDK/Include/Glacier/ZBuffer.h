#pragma once

#include "ZMemory.h"
#include "Globals.h"

class ZBuffer
{
public:
    static ZBuffer FromData(const std::string& p_Data)
    {
        const auto s_Size = static_cast<uint32_t>(p_Data.size());

        const auto s_DataBuffer = (*Globals::MemoryManager)->m_pNormalAllocator->Allocate(sizeof(ZBuffer) + s_Size);

        const auto s_Data = new (s_DataBuffer) SBufferData();
        s_Data->m_nRefCount = 1;
        s_Data->m_nSize = s_Size;

        ZBuffer s_Buffer(s_Data);
        memcpy(s_Buffer.data(), p_Data.data(), s_Size);
        return s_Buffer;
    }

private:
    struct SBufferData
    {
        int32_t m_nRefCount;
        uint32_t m_nSize;
    };

private:
    ZBuffer(SBufferData* p_Data) : m_pData(p_Data)
    {
    }

public:
    ZBuffer(const ZBuffer& p_Other) : m_pData(p_Other.m_pData)
    {
        addRef();
    }

    ~ZBuffer()
    {
        release();
    }

    ZBuffer& operator=(const ZBuffer& p_Other)
    {
        release();
        m_pData = p_Other.m_pData;
        addRef();

        return *this;
    }

    void* data() const
    {
        if (!m_pData)
            return nullptr;

        const auto s_DataOffset = reinterpret_cast<uintptr_t>(m_pData) + sizeof(ZBuffer);
        return reinterpret_cast<void*>(s_DataOffset);
    }

    uint32_t size() const
    {
        if (!m_pData)
            return 0;

        return m_pData->m_nSize;
    }

private:
    void addRef() const
    {
        if (!m_pData)
            return;

        ++m_pData->m_nRefCount;
    }

    void release()
    {
        if (!m_pData)
            return;

        const auto s_NewRefCount = m_pData->m_nRefCount;

        if (s_NewRefCount != 0)
            return;

        (*Globals::MemoryManager)->m_pPageAllocator->GetAllocator(m_pData)->Free(m_pData);
        m_pData = nullptr;
    }

private:
    SBufferData* m_pData;
};
