#pragma once

#include "ZPrimitives.h"
#include "Glacier/ZMemory.h"
#include "Globals.h"

#include <vector>

template <class T>
class TIterator {
protected:
    TIterator(T* p_Current) :
        m_pCurrent(p_Current) {}

public:
    T* m_pCurrent;
};

template <class T>
class TArray {
public:
    TArray() :
        m_pBegin(nullptr),
        m_pEnd(nullptr),
        m_pAllocationEnd(nullptr) {}

    TArray(const std::vector<T>& p_Other) :
        m_pBegin(nullptr),
        m_pEnd(nullptr),
        m_pAllocationEnd(nullptr) {
        if (p_Other.size() == 0) {
            return;
        }

        for (const auto& s_Value : p_Other) {
            push_back(s_Value);
        }
    }

    TArray(const TArray<T>& p_Other) :
        m_pBegin(nullptr),
        m_pEnd(nullptr),
        m_pAllocationEnd(nullptr) {
        if (p_Other.size() == 0) {
            return;
        }

        for (const auto& s_Value : p_Other) {
            push_back(s_Value);
        }
    }

    TArray(TArray<T>&& p_Other) {
        m_pBegin = p_Other.m_pBegin;
        m_pEnd = p_Other.m_pEnd;
        m_pAllocationEnd = p_Other.m_pAllocationEnd;

        p_Other.m_pBegin = nullptr;
        p_Other.m_pEnd = nullptr;
        p_Other.m_pAllocationEnd = nullptr;
    }

    ~TArray() {
        for (T* s_Item = begin(); s_Item != end(); ++s_Item)
            s_Item->~T();

        if (!hasInlineFlag()) {
            (*Globals::MemoryManager)->m_pNormalAllocator->Free(m_pBegin);
        }
    }

    TArray<T>& operator=(const TArray<T>& p_Other) {
        if (this == &p_Other) {
            return *this;
        }

        clear();

        for (const auto& s_Value : p_Other) {
            push_back(s_Value);
        }

        return *this;
    }

    TArray& operator=(TArray<T>&& p_Other) {
        clear();

        m_pBegin = p_Other.m_pBegin;
        m_pEnd = p_Other.m_pEnd;
        m_pAllocationEnd = p_Other.m_pAllocationEnd;

        p_Other.m_pBegin = nullptr;
        p_Other.m_pEnd = nullptr;
        p_Other.m_pAllocationEnd = nullptr;

        return *this;
    }

    void push_back(const T& p_Value) {
        // If we have the inline flag, we need to copy everything into
        // a temporary dynamically-allocated array, and then swap it with
        // the current one.
        if (hasInlineFlag()) {
            TArray<T> s_DynamicCopy(*this);
            *this = s_DynamicCopy;
        }

        if (m_pEnd < m_pAllocationEnd) {
            new(m_pEnd++) T(p_Value);
        }
        else {
            // Out of space, need to allocate a new array and move everything over.
            // We will allocate double the existing capacity.
            const size_t s_NewCapacity = capacity() == 0 ? 1 : capacity() * 2;
            const size_t s_CurrentSize = size();
            const auto s_NewBegin = static_cast<T*>((*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
                sizeof(T) * s_NewCapacity, alignof(T)
            ));

            // Copy the old data over to the new array.
            // After copying, we destroy them and free the old array.
            auto s_NewItemMem = s_NewBegin;

            for (T* s_Item = begin(); s_Item != end(); ++s_Item) {
                new(s_NewItemMem++) T(*s_Item);
                s_Item->~T();
            }

            (*Globals::MemoryManager)->m_pNormalAllocator->Free(m_pBegin);

            // Update the pointers.
            m_pBegin = s_NewBegin;
            m_pEnd = m_pBegin + s_CurrentSize;
            m_pAllocationEnd = m_pBegin + s_NewCapacity;

            // Write the new value.
            new(m_pEnd++) T(p_Value);
        }
    }

    void insert(size_t p_Index, const T& p_Value) {
        // If we have the inline flag, we need to copy everything into
        // a temporary dynamically-allocated array, and then swap it with
        // the current one.
        if (hasInlineFlag()) {
            TArray<T> s_DynamicCopy(*this);
            *this = s_DynamicCopy;
        }

        // If we're pushing after the end, just push.
        if (p_Index >= size()) {
            push_back(p_Value);
            return;
        }

        // TODO: Improve this.
        // For now we're re-allocating everything, since it's easier.
        const size_t s_NewCapacity = capacity() == 0 ? 1 : capacity() * 2;
        const size_t s_CurrentSize = size();
        const auto s_NewBegin = static_cast<T*>((*Globals::MemoryManager)->m_pNormalAllocator->AllocateAligned(
            sizeof(T) * s_NewCapacity, alignof(T)
        ));

        // Copy the old data over to the new array.
        // After copying, we destroy them and free the old array.
        auto s_NewItemMem = s_NewBegin;

        // We first copy the items before the index.
        for (size_t i = 0; i < p_Index; ++i) {
            new(s_NewItemMem++) T(operator[](i));
        }

        // Then we copy the item we're inserting.
        new(s_NewItemMem++) T(p_Value);

        // And then we copy the rest of the items (after the index).
        for (size_t i = p_Index; i < s_CurrentSize; ++i) {
            new(s_NewItemMem++) T(operator[](i));
        }

        // Destroy everything.
        for (T* s_Item = begin(); s_Item != end(); ++s_Item) {
            s_Item->~T();
        }

        (*Globals::MemoryManager)->m_pNormalAllocator->Free(m_pBegin);

        m_pBegin = s_NewBegin;
        m_pEnd = m_pBegin + s_CurrentSize + 1;
        m_pAllocationEnd = m_pBegin + s_NewCapacity;
    }

    void clear() {
        for (T* s_Item = begin(); s_Item != end(); ++s_Item) {
            s_Item->~T();
        }

        if (hasInlineFlag()) {
            // If data was stored inline, just clear everything (including the inline flag).
            m_pBegin = m_pEnd = m_pAllocationEnd = nullptr;
        }
        else {
            // We're not freeing anything here since the allocated memory can be re-used.
            m_pBegin = m_pEnd;
        }
    }

    size_t size() const {
        if (fitsInline() && hasInlineFlag()) {
            return m_nInlineCount;
        }

        return (reinterpret_cast<uintptr_t>(m_pEnd) - reinterpret_cast<uintptr_t>(m_pBegin)) / sizeof(T);
    }

    size_t capacity() const {
        if (fitsInline() && hasInlineFlag()) {
            return m_nInlineCapacity;
        }

        return (reinterpret_cast<uintptr_t>(m_pAllocationEnd) - reinterpret_cast<uintptr_t>(m_pBegin)) / sizeof(T);
    }

    T& operator[](size_t p_Index) {
        return begin()[p_Index];
    }

    const T& operator[](size_t p_Index) const {
        return begin()[p_Index];
    }

    T& at(size_t p_Index) {
        return begin()[p_Index];
    }

    const T& at(size_t p_Index) const {
        return begin()[p_Index];
    }

    T* begin() {
        if (fitsInline() && hasInlineFlag())
            return reinterpret_cast<T*>(&m_pBegin);

        return m_pBegin;
    }

    T* end() {
        if (fitsInline() && hasInlineFlag())
            return begin() + m_nInlineCount;

        return m_pEnd;
    }

    const T* begin() const {
        if (fitsInline() && hasInlineFlag())
            return reinterpret_cast<const T*>(&m_pBegin);

        return m_pBegin;
    }

    const T* end() const {
        if (fitsInline() && hasInlineFlag())
            return begin() + m_nInlineCount;

        return m_pEnd;
    }

    T* find(const T& p_Value) const {
        T* s_Current = begin();

        while (s_Current != end()) {
            if (*s_Current == p_Value)
                return s_Current;

            ++s_Current;
        }

        return m_pEnd;
    }

    [[nodiscard]] bool fitsInline() const {
        return sizeof(T) <= sizeof(T*) * 2;
    }

    [[nodiscard]] bool hasInlineFlag() const {
        return (m_nFlags >> 62) & 1;
    }

public:
    T* m_pBegin;
    T* m_pEnd;

    union {
        T* m_pAllocationEnd;
        int64_t m_nFlags;

        struct {
            uint8_t m_nInlineCount;
            uint8_t m_nInlineCapacity;
        };
    };
};

template <typename T>
class TFixedArray {
public:
    inline size_t size() const {
        return (reinterpret_cast<uintptr_t>(m_pEnd) - reinterpret_cast<uintptr_t>(m_pBegin)) / sizeof(T);
    }

    inline size_t capacity() const {
        return (reinterpret_cast<uintptr_t>(m_pEnd) - reinterpret_cast<uintptr_t>(m_pBegin)) / sizeof(T);
    }

    inline T& operator[](size_t p_Index) const {
        return m_pBegin[p_Index];
    }

    inline T* begin() {
        return m_pBegin;
    }

    inline T* end() {
        return m_pEnd;
    }

    inline T* begin() const {
        return m_pBegin;
    }

    inline T* end() const {
        return m_pEnd;
    }

    inline T* find(const T& p_Value) const {
        T* s_Current = m_pBegin;

        while (s_Current != m_pEnd) {
            if (*s_Current == p_Value)
                return s_Current;

            ++s_Current;
        }

        return m_pEnd;
    }

public:
    T* m_pBegin;
    T* m_pEnd;
};
