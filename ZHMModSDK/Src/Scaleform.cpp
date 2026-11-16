#include "Scaleform.h"

#include "Functions.h"
#include "Globals.h"

Scaleform::File* Scaleform::MemoryFile::Create(const char* p_Url, const std::vector<uint8_t>& p_Data) {
    auto* s_Heap = *Globals::ScaleformGlobalHeap;

    using AllocFn = void*(__fastcall*)(Scaleform::MemoryHeap* th, size_t size, const void* info);
    const auto s_Alloc = (*reinterpret_cast<AllocFn**>(s_Heap))[10];

    auto* s_File = static_cast<MemoryFile*>(s_Alloc(s_Heap, sizeof(MemoryFile), nullptr));

    if (!s_File) {
        return nullptr;
    }

    *reinterpret_cast<void**>(s_File) = Globals::ScaleformMemoryFile_vtbl;
    s_File->m_nRefCount = 1;
    Functions::Scaleform_String_String->Call(&s_File->m_sFilePath, p_Url);
    s_File->m_pFileData = p_Data.data();
    s_File->m_nFileSize = static_cast<int32_t>(p_Data.size());
    s_File->m_nFileIndex = 0;
    s_File->m_bValid = !p_Data.empty();

    return s_File;
}
