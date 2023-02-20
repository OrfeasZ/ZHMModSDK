#include "ProcessUtils.h"

#include <TlHelp32.h>
#include <Windows.h>
#include <unordered_set>

#include "Logging.h"

using namespace Util;

uintptr_t ProcessUtils::SearchPattern(uintptr_t p_BaseAddress, size_t p_ScanSize, const uint8_t* p_Pattern, const char* p_Mask)
{
    for (uintptr_t s_SearchAddr = p_BaseAddress; s_SearchAddr < (p_BaseAddress + p_ScanSize); ++s_SearchAddr)
    {
        const uint8_t* s_MemoryPtr = reinterpret_cast<uint8_t*>(s_SearchAddr);

        if (s_MemoryPtr[0] != p_Pattern[0])
            continue;

        const uint8_t* s_PatternPtr = p_Pattern;
        const uint8_t* s_MaskPtr = reinterpret_cast<const uint8_t*>(p_Mask);

        bool s_Found = true;

        for (; s_MaskPtr[0] && (reinterpret_cast<uintptr_t>(s_MemoryPtr) < (p_BaseAddress + p_ScanSize)); ++s_MaskPtr, ++s_PatternPtr, ++s_MemoryPtr)
        {
            if (s_MaskPtr[0] != 'x')
                continue;

            if (s_MemoryPtr[0] != s_PatternPtr[0])
            {
                s_Found = false;
                break;
            }
        }

        if (s_Found)
            return s_SearchAddr;
    }

    return 0;
}

uint32_t ProcessUtils::GetSizeOfCode(HMODULE p_Module)
{
    PIMAGE_DOS_HEADER s_DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(p_Module);
    PIMAGE_NT_HEADERS s_NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(p_Module) + s_DOSHeader->e_lfanew);

    if (!s_NTHeader)
        return 0;

    return static_cast<uint32_t>(s_NTHeader->OptionalHeader.SizeOfCode);
}

uintptr_t ProcessUtils::GetBaseOfCode(HMODULE p_Module)
{
    PIMAGE_DOS_HEADER s_DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(p_Module);
    PIMAGE_NT_HEADERS s_NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(p_Module) + s_DOSHeader->e_lfanew);

    if (!s_NTHeader)
        return 0;

    return static_cast<uintptr_t>(s_NTHeader->OptionalHeader.BaseOfCode);
}

uint32_t ProcessUtils::GetSizeOfImage(HMODULE p_Module)
{
    PIMAGE_DOS_HEADER s_DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(p_Module);
    PIMAGE_NT_HEADERS s_NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(p_Module) + s_DOSHeader->e_lfanew);

    if (!s_NTHeader)
        return 0;

    return static_cast<uint32_t>(s_NTHeader->OptionalHeader.SizeOfImage);
}

std::tuple<uintptr_t, uintptr_t> ProcessUtils::GetSectionStartAndEnd(HMODULE p_Module, const std::string& p_SectionName)
{
    PIMAGE_DOS_HEADER s_DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(p_Module);
    PIMAGE_NT_HEADERS s_NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(p_Module) + s_DOSHeader->e_lfanew);

    if (!s_NTHeader)
        return std::make_tuple<uintptr_t, uintptr_t>(0, 0);

    PIMAGE_SECTION_HEADER s_Section = IMAGE_FIRST_SECTION(s_NTHeader);

    for (int i = 0; i < s_NTHeader->FileHeader.NumberOfSections; ++i)
    {
        if (strcmp(reinterpret_cast<const char*>(s_Section->Name), p_SectionName.c_str()) == 0)
        {
            uintptr_t s_RDataSectionStart = s_Section->VirtualAddress;
            s_RDataSectionStart += reinterpret_cast<uintptr_t>(p_Module);

            uintptr_t s_RDataSectionEnd = s_RDataSectionStart + s_Section->SizeOfRawData;

            return std::make_tuple(s_RDataSectionStart, s_RDataSectionEnd);
        }

        ++s_Section;
    }

    return std::make_tuple<uintptr_t, uintptr_t>(0, 0);
}

uintptr_t ProcessUtils::GetRelativeAddr(uintptr_t p_Base, int32_t p_Offset)
{
    uintptr_t s_RelAddrPtr = p_Base + p_Offset;
    int32_t s_RelAddr = *reinterpret_cast<int32_t*>(s_RelAddrPtr);

    return s_RelAddrPtr + s_RelAddr + sizeof(int32_t);
}
