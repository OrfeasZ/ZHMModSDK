#include "HashingUtils.h"

using namespace Util;

uint32_t HashingUtils::FNV1a(const char* p_String)
{
    uint32_t hash = 0x811C9DC5;
    while (*p_String) {
        hash = 0x1000193 * (hash ^ *p_String++);
    }
    return hash;
}
