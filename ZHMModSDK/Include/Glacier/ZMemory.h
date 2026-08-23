#pragma once

#include "IComponentInterface.h"

class IAllocator : public IComponentInterface {
public:
    virtual ~IAllocator() {}
    virtual uint64_t DefaultAlignment() = 0;
    virtual bool SupportsAlignment() = 0;
    virtual void* Allocate(uint64_t nSize) = 0;
    virtual void* AllocateAligned(uint64_t nSize, uint64_t nAlignment) = 0;
    virtual void Free(void* pData) = 0;
    virtual uint64_t GetAllocationSize(void* pData) = 0;
};

class IPageAllocator {
public:
    enum class EMemType {
        EMEMTYPE_MAIN = 0,
        EMEMTYPE_GPU = 1,
        EMEMTYPE_GPU_WC = 2,
        EMEMTYPE_FLEXIBLE = 3,
        EMEMTYPE_GEOMETRY = 4
    };

    virtual ~IPageAllocator() = default;
    virtual uint64_t GetPageSize() = 0;
    virtual void IPageAllocator_unk2() = 0;
    virtual void IPageAllocator_unk3() = 0;
    virtual bool Commit(void* pAddress, size_t nSize, EMemType memType) = 0;
    virtual bool Decommit(void* pAddress, size_t nSize) = 0;
    virtual void IPageAllocator_unk6() = 0;
    virtual void IPageAllocator_unk7() = 0;
    virtual void IPageAllocator_unk8() = 0;
    virtual IAllocator* GetAllocator(void* pAddress) = 0;
};

class ZMemoryManager {
public:
    IPageAllocator* m_pPageAllocator1;
    IPageAllocator* m_pPageAllocator2;
    IAllocator* m_pNormalAllocator;
};