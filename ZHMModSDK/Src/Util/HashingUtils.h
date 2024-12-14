#pragma once

#include <cstdint>
#include <string>

namespace Util
{
    class HashingUtils
    {
    public:
        static uint32_t FNV1a(const char* p_String);
    };
}
