#pragma once

#include <cstdint>

template <typename T>
class TListNode {
public:
    TListNode* m_pNext;
    TListNode* m_pPrevious;
    T m_data;
};

template <typename T>
class TList {
public:
    class ZFakeListNode {
    public:
        TListNode<T>* m_pFirst;
        TListNode<T>* m_pLast;
    };

    uint32_t m_nSize;
    ZFakeListNode m_list;
};