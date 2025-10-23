#pragma once

#include <cstddef>

template <size_t T>
class alignas(T) TAlignedType {
public:
    char dummy;
};