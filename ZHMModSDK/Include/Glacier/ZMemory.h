#pragma once

#include "IComponentInterface.h"

#include "Globals.h"

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

class ZVirtualMemory {
public:
    enum class EVirtualMemoryPageSize : uint32_t {
        Unknown0 = 0,
        Unknown1 = 1,
    };

    static uint64_t PageSize(EVirtualMemoryPageSize p_PageSize) {
        return GetAllocator(p_PageSize)->GetPageSize();
    }

    static void Commit(void* p_Address, uint32_t p_Offset, uint32_t p_Size, EVirtualMemoryPageSize p_PageSize) {
        auto* s_Allocator = GetAllocator(p_PageSize);
        const uint64_t s_PageSize = s_Allocator->GetPageSize();

        auto* s_Address = static_cast<uint8_t*>(p_Address) + p_Offset;
        const auto* s_EndAddress = s_Address + p_Size;

        while (s_Address < s_EndAddress) {
            s_Allocator->Commit(
                s_Address,
                s_PageSize,
                IPageAllocator::EMemType::EMEMTYPE_MAIN
            );

            s_Address += s_PageSize;
        }
    }

    static void Release(void* p_Address, uint32_t p_Offset, uint32_t p_Size, EVirtualMemoryPageSize p_PageSize) {
        GetAllocator(p_PageSize)->Decommit(
            static_cast<uint8_t*>(p_Address) + p_Offset,
            p_Size
        );
    }

private:
    static IPageAllocator* GetAllocator(EVirtualMemoryPageSize p_PageSize) {
        auto* s_MemoryManager = *Globals::MemoryManager;

        return p_PageSize == EVirtualMemoryPageSize::Unknown0
            ? s_MemoryManager->m_pPageAllocator2
            : s_MemoryManager->m_pPageAllocator1;
    }
};