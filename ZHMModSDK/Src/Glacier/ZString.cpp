#include <Glacier/ZString.h>

#include "Globals.h"
#include <Glacier/ZMemory.h>

ZString::~ZString() {
    if (IsAllocated()) {
        (*Globals::MemoryManager)->m_pNormalAllocator->Free(const_cast<char*>(m_pChars));
    }
}

void ZString::Allocate(const char* str, size_t size) {
    m_nLength = static_cast<uint32_t>(size);
    m_pChars = reinterpret_cast<char*>((*Globals::MemoryManager)->m_pNormalAllocator->Allocate(size + 1));
    memcpy(const_cast<char*>(m_pChars), str, size);
    const_cast<char*>(m_pChars)[size] = '\0';
}
