#pragma once

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