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
            auto s_PageAllocator = (*Globals::MemoryManager)->m_pPageAllocator;

            if (s_PageAllocator) {
                auto s_Allocator = s_PageAllocator->GetAllocator(this);

                if (s_Allocator) {
                    s_Allocator->Free(this);
                }
            }
            else {
                (*Globals::MemoryManager)->m_pNormalAllocator->Free(this);
            }
        }
    }

    int32_t GetRefCount() override {
        return m_iRefCount;
    }

public:
    volatile LONG m_iRefCount = 0;
};
