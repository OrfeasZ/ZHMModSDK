#pragma once


class ISharedPointerTarget {
public:
    virtual ~ISharedPointerTarget() {}
    virtual void addRef() = 0;
    virtual void release() = 0;
    virtual int32_t getRefCount() = 0;
};

class ZSharedPointerTarget : public ISharedPointerTarget {
public:
    long volatile m_iRefCount = 0;

public:
    void addRef() override {
        InterlockedIncrement(&m_iRefCount);
    }

    void release() override {
        if (InterlockedDecrement(&m_iRefCount) == 0) {
            (*Globals::MemoryManager)->m_pPageAllocator->GetAllocator(this)->Free(this);
        }
    }

    int32_t getRefCount() override {
        return m_iRefCount;
    }
};
