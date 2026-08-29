#pragma once

#include "ZMemory.h"

class ISharedPointerTarget {
public:
    virtual ~ISharedPointerTarget() {}
    virtual void AddReference() = 0;
    virtual void RemoveReference() = 0;
    virtual int32_t GetRefCount() = 0;
};

class ZSharedPointerTarget : public ISharedPointerTarget {
public:
    void AddReference() override {
        InterlockedIncrement(&m_iRefCount);
    }

    void RemoveReference() override {
        if (InterlockedDecrement(&m_iRefCount) == 0) {
            auto* s_MemoryManager = *Globals::MemoryManager;

            IAllocator* s_Allocator = nullptr;

            if (s_MemoryManager->m_pPageAllocator1) {
                s_Allocator = s_MemoryManager->m_pPageAllocator1->GetAllocator(this);
            }

            if (!s_Allocator) {
                s_Allocator = s_MemoryManager->m_pNormalAllocator;
            }

            s_Allocator->Free(this);
        }
    }

    int32_t GetRefCount() override {
        return m_iRefCount;
    }

public:
    volatile LONG m_iRefCount = 0;
};
