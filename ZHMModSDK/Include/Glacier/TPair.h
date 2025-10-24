#pragma once

#include <type_traits>

template <typename TKey, typename TValue>
class TPair {
public:
    TPair(const TKey& key, const TValue& value) :
        first(key), second(value) {}

    TKey first;
    TValue second;
};
