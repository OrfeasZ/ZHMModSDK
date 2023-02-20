#pragma once

#include <Windows.h>
#include <tuple>
#include <string>
#include <cstdint>

namespace Util
{
    class ProcessUtils
    {
    public:
        static uintptr_t GetBaseOfCode(HMODULE p_Module);
        static uint32_t GetSizeOfCode(HMODULE p_Module);
        static uint32_t GetSizeOfImage(HMODULE p_Module);
        static uintptr_t SearchPattern(uintptr_t p_BaseAddress, size_t p_ScanSize, const uint8_t* p_Pattern, const char* p_Mask);
        static std::tuple<uintptr_t, uintptr_t> GetSectionStartAndEnd(HMODULE p_Module, const std::string& p_SectionName);
        static uintptr_t GetRelativeAddr(uintptr_t p_Base, int32_t p_Offset);
    };
}
