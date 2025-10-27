#pragma once

#include "Hash.h"
#include "ZString.h"
#include "TPair.h"
#include "TIterator.h"
#include "ZResourceID.h"
#include "ZMemory.h"
#include "Globals.h"

template <class T>
class THashMapNode {
public:
    int32_t m_nNextIndex = 0;
    T m_value;
};

template <class T>
struct SHashMapInfo {
    uint32_t m_nBucketCount = 0;
    uint32_t* m_pBuckets = nullptr;
    THashMapNode<T>* m_pNodes = nullptr;
};

template <class T>
struct TDefaultHashMapPolicy;

template <>
struct TDefaultHashMapPolicy<ZString> {
    uint64_t operator()(const ZString& p_Value) const {
        return Hash::Fnv1a64(p_Value.c_str(), p_Value.size());
    }
};

template <>
struct TDefaultHashMapPolicy<ZRepositoryID> {
    uint64_t operator()(const ZRepositoryID& p_Value) const {
        return p_Value.GetHashCode();
    }
};

template <>
struct TDefaultHashMapPolicy<ZRuntimeResourceID> {
    uint64_t operator()(const ZRuntimeResourceID& p_Value) const {
        return p_Value.GetHashCode();
    }
};

template <class T>
class THashMapIterator : public TIterator<THashMapNode<T>> {
protected:
    THashMapIterator(const SHashMapInfo<T>* p_MapInfo, uint32_t p_Bucket, THashMapNode<T>* p_Current) :
        TIterator<THashMapNode<T>>(p_Current),
        m_pMapInfo(p_MapInfo),
        m_nBucket(p_Bucket) {}

    THashMapIterator(const SHashMapInfo<T>* p_MapInfo) :
        TIterator<THashMapNode<T>>(nullptr),
        m_pMapInfo(p_MapInfo),
        m_nBucket(UINT32_MAX) {}

public:
    THashMapIterator<T>& operator++() {
        uint32_t s_NextIndex = this->m_pCurrent->m_nNextIndex;

        if (s_NextIndex != UINT32_MAX) {
            this->m_pCurrent = &m_pMapInfo->m_pNodes[s_NextIndex];
            return *this;
        }

        ++m_nBucket;

        if (m_nBucket >= m_pMapInfo->m_nBucketCount) {
            m_nBucket = UINT32_MAX;
            this->m_pCurrent = nullptr;
            return *this;
        }

        while (m_pMapInfo->m_pBuckets[m_nBucket] == UINT32_MAX) {
            ++m_nBucket;

            if (m_nBucket >= m_pMapInfo->m_nBucketCount) {
                m_nBucket = UINT32_MAX;
                this->m_pCurrent = nullptr;
                return *this;
            }
        }

        this->m_pCurrent = &m_pMapInfo->m_pNodes[m_pMapInfo->m_pBuckets[m_nBucket]];

        return *this;
    }

    T& operator*() {
        return this->m_pCurrent->m_value;
    }

    T* operator->() {
        return &this->m_pCurrent->m_value;
    }

    T& operator*() const {
        return this->m_pCurrent->m_value;
    }

    T* operator->() const {
        return &this->m_pCurrent->m_value;
    }

    bool operator==(const THashMapIterator<T>& p_Other) const {
        return p_Other.m_pCurrent == this->m_pCurrent &&
                p_Other.m_nBucket == m_nBucket &&
                p_Other.m_pMapInfo == m_pMapInfo;
    }

    bool operator!=(const THashMapIterator<T>& p_Other) const {
        return !(*this == p_Other);
    }

public:
    const SHashMapInfo<T>* m_pMapInfo = nullptr;
    uint32_t m_nBucket = 0;

    template <class K, class V, class H>
    friend class THashMap;
};

template <class TKeyType, class TValueType, class THashingPolicy = TDefaultHashMapPolicy<TKeyType>>
class THashMap {
public:
    using value_type = TPair<TKeyType, TValueType>;
    using node_type = THashMapNode<value_type>;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = THashMapIterator<value_type>;
    using const_iterator = THashMapIterator<const value_type>;
    using const_map_info_type = const SHashMapInfo<const value_type>*;

public:
    THashMap() {
        init(4);
    }

    explicit THashMap(uint32_t p_BucketCount) {
        init(p_BucketCount);
    }

    ~THashMap() {
        clear();

        IAllocator* p_Allocator = (*Globals::MemoryManager)->m_pNormalAllocator;

        if (m_Info.m_pBuckets) {
            p_Allocator->Free(m_Info.m_pBuckets);
        }

        if (m_Info.m_pNodes) {
            p_Allocator->Free(m_Info.m_pNodes);
        }
    }

    THashMap(const THashMap& p_Other) {
        init(p_Other.m_Info.m_nBucketCount);

        for (const auto& pair : p_Other) {
            insert(pair.first, pair.second);
        }
    }

    THashMap& operator=(const THashMap& p_Other) {
        if (this == &p_Other) {
            return *this;
        }

        clear();

        init(p_Other.m_Info.m_nBucketCount);

        for (const auto& pair : p_Other) {
            insert(pair.first, pair.second);
        }

        return *this;
    }

    THashMap(THashMap&& p_Other) {
        m_Info = p_Other.m_Info;
        m_nSize = p_Other.m_nSize;
        m_nFreeSlots = p_Other.m_nFreeSlots;

        p_Other.m_Info.m_pBuckets = nullptr;
        p_Other.m_Info.m_pNodes = nullptr;
        p_Other.m_Info.m_nBucketCount = 0;
        p_Other.m_nSize = 0;
        p_Other.m_nFreeSlots = UINT32_MAX;
    }

    THashMap& operator=(THashMap&& p_Other) {
        if (this == &p_Other) {
            return *this;
        }

        IAllocator* p_Allocator = (*Globals::MemoryManager)->m_pNormalAllocator;

        if (m_Info.m_pBuckets) {
            p_Allocator->Free(m_Info.m_pBuckets);
        }

        if (m_Info.m_pNodes) {
            p_Allocator->Free(m_Info.m_pNodes);
        }

        m_Info = p_Other.m_Info;
        m_nSize = p_Other.m_nSize;
        m_nFreeSlots = p_Other.m_nFreeSlots;

        p_Other.m_Info.m_pBuckets = nullptr;
        p_Other.m_Info.m_pNodes = nullptr;
        p_Other.m_Info.m_nBucketCount = 0;
        p_Other.m_nSize = 0;
        p_Other.m_nFreeSlots = UINT32_MAX;

        return *this;
    }

    void init(uint32_t p_BucketCount) {
        if (p_BucketCount < 4) {
            p_BucketCount = 4;
        }

        IAllocator* p_Allocator = (*Globals::MemoryManager)->m_pNormalAllocator;

        m_Info.m_nBucketCount = p_BucketCount;
        m_Info.m_pBuckets = static_cast<uint32_t*>(p_Allocator->Allocate(sizeof(uint32_t) * p_BucketCount));
        m_Info.m_pNodes = static_cast<node_type*>(
            p_Allocator->AllocateAligned(sizeof(node_type) * p_BucketCount,
            alignof(node_type)
        ));

        std::fill_n(m_Info.m_pBuckets, p_BucketCount, UINT32_MAX);

        m_nSize = 0;
        m_nFreeSlots = UINT32_MAX;
    }

    void ensureCapacity(uint32_t p_ElementCount) {
        if (m_Info.m_nBucketCount < p_ElementCount) {
            uint32_t p_NewBucketCount = static_cast<uint32_t>(m_Info.m_nBucketCount * 1.5f);

            if (p_NewBucketCount == 0) {
                p_NewBucketCount = 4;
            }

            rehash(p_NewBucketCount);
        }
    }

    const_iterator begin() const {
        if (m_Info.m_nBucketCount == 0) {
            return const_iterator(reinterpret_cast<const_map_info_type>(&m_Info));
        }

        for (uint32_t i = 0; i < m_Info.m_nBucketCount; ++i) {
            const uint32_t s_NodeIndex = m_Info.m_pBuckets[i];

            if (s_NodeIndex != UINT32_MAX) {
                return const_iterator(
                    reinterpret_cast<const_map_info_type>(&m_Info),
                    i,
                    reinterpret_cast<THashMapNode<const value_type>*>(&m_Info.m_pNodes[s_NodeIndex])
                );
            }
        }

        return const_iterator(reinterpret_cast<const_map_info_type>(&m_Info));
    }

    iterator begin() {
        if (m_Info.m_nBucketCount == 0) {
            return iterator(&m_Info);
        }

        for (uint32_t i = 0; i < m_Info.m_nBucketCount; ++i) {
            const uint32_t s_NodeIndex = m_Info.m_pBuckets[i];

            if (s_NodeIndex != UINT32_MAX) {
                return iterator(&m_Info, i, &m_Info.m_pNodes[s_NodeIndex]);
            }
        }

        return iterator(&m_Info);
    }

    const_iterator end() const {
        return const_iterator(reinterpret_cast<const_map_info_type>(&m_Info));
    }

    iterator end() {
        return iterator(&m_Info);
    }

    size_t size() const {
        return m_nSize;
    }

    bool empty() const {
        return m_nSize == 0;
    }

    const TValueType& at(const TKeyType& p_Key) const {
        const_iterator s_Iterator = find(p_Key);

        if (s_Iterator != end()) {
            return s_Iterator->second;
        }

        throw std::out_of_range("Key not found in the hash map!");
    }

    TValueType& at(const TKeyType& p_Key) {
        iterator s_Iterator = find(p_Key);

        if (s_Iterator != end()) {
            return s_Iterator->second;
        }

        throw std::out_of_range("Key not found in the hash map!");
    }

    TValueType& operator[](const TKeyType& p_Key) {
        iterator s_Iterator = find(p_Key);

        if (s_Iterator != end()) {
            return s_Iterator->second;
        }

        return insert(p_Key, TValueType{})->second;
    }

    bool contains(const TKeyType& p_Key) const {
        return find(p_Key) != end();
    }

    bool contains(const TKeyType& p_Key) {
        return find(p_Key) != end();
    }

    const_iterator find(const TKeyType& p_Key) const {
        if (auto* s_Node = findNode(p_Key)) {
            return const_iterator(
                reinterpret_cast<const_map_info_type>(&m_Info),
                THashingPolicy()(p_Key) % m_Info.m_nBucketCount,
                reinterpret_cast<THashMapNode<const value_type>*>(s_Node)
            );
        }

        return end();
    }

    iterator find(const TKeyType& p_Key) {
        if (auto* s_Node = findNode(p_Key)) {
            return iterator(
                &m_Info,
                THashingPolicy::GetHashCode(p_Key) % m_Info.m_nBucketCount,
                s_Node
            );
        }

        return end();
    }

    iterator insert(const TKeyType& p_Key, const TValueType& p_Value) {
        if (!m_Info.m_pBuckets) {
            return iterator(&m_Info);
        }

        ensureCapacity(m_nSize + 1);

        const uint64_t s_Hash = THashingPolicy()(p_Key);
        const uint32_t s_BucketIndex = static_cast<uint32_t>(s_Hash % m_Info.m_nBucketCount);
        uint32_t s_NodeIndex = m_Info.m_pBuckets[s_BucketIndex];

        while (s_NodeIndex != UINT32_MAX) {
            node_type& s_Node = m_Info.m_pNodes[s_NodeIndex];

            if (s_Node.m_value.first == p_Key) {
                return iterator(&m_Info, s_BucketIndex, &s_Node);
            }

            s_NodeIndex = s_Node.m_nNextIndex;
        }

        uint32_t s_NewNodeIndex;

        if (m_nFreeSlots == UINT32_MAX) {
            s_NewNodeIndex = m_nSize++;
        }
        else {
            s_NewNodeIndex = m_nFreeSlots;
            m_nFreeSlots = m_Info.m_pNodes[s_NewNodeIndex].m_nNextIndex;
        }

        node_type& s_NewNode = m_Info.m_pNodes[s_NewNodeIndex];
        s_NewNode.m_value = TPair<TKeyType, TValueType>(p_Key, p_Value);
        s_NewNode.m_nNextIndex = m_Info.m_pBuckets[s_BucketIndex];

        m_Info.m_pBuckets[s_BucketIndex] = s_NewNodeIndex;

        return iterator(&m_Info, s_BucketIndex, &s_NewNode);
    }

    iterator insert_or_assign(const TKeyType& p_Key, const TValueType& p_Value) {
        if (!m_Info.m_pBuckets) {
            return iterator(&m_Info);
        }

        ensureCapacity(m_nSize + 1);

        const uint64_t s_Hash = THashingPolicy()(p_Key);
        const uint32_t s_BucketIndex = static_cast<uint32_t>(s_Hash % m_Info.m_nBucketCount);
        uint32_t s_NodeIndex = m_Info.m_pBuckets[s_BucketIndex];

        while (s_NodeIndex != UINT32_MAX) {
            node_type& s_Node = m_Info.m_pNodes[s_NodeIndex];

            if (s_Node.m_value.first == p_Key) {
                s_Node.m_value.second = p_Value;

                return iterator(&m_Info, s_BucketIndex, &s_Node);
            }

            s_NodeIndex = s_Node.m_nNextIndex;
        }

        uint32_t s_NewNodeIndex;

        if (m_nFreeSlots == UINT32_MAX) {
            s_NewNodeIndex = m_nSize++;
        }
        else {
            s_NewNodeIndex = m_nFreeSlots;
            m_nFreeSlots = m_Info.m_pNodes[s_NewNodeIndex].m_nNextIndex;
        }

        node_type& s_NewNode = m_Info.m_pNodes[s_NewNodeIndex];
        s_NewNode.m_value = TPair<TKeyType, TValueType>(p_Key, p_Value);
        s_NewNode.m_nNextIndex = m_Info.m_pBuckets[s_BucketIndex];

        m_Info.m_pBuckets[s_BucketIndex] = s_NewNodeIndex;

        return iterator(&m_Info, s_BucketIndex, &s_NewNode);
    }

    void rehash(uint32_t p_NewBucketCount) {
        if (p_NewBucketCount < 4) {
            p_NewBucketCount = 4;
        }

        IAllocator* p_Allocator = (*Globals::MemoryManager)->m_pNormalAllocator;
        uint32_t* s_NewBuckets = static_cast<uint32_t*>(p_Allocator->Allocate(sizeof(uint32_t) * p_NewBucketCount));
        node_type* s_NewNodes = static_cast<node_type*>(
            p_Allocator->AllocateAligned(sizeof(node_type) * p_NewBucketCount,
            alignof(node_type)
        ));

        std::fill_n(s_NewBuckets, p_NewBucketCount, UINT32_MAX);

        uint32_t s_NewSize = 0;

        if (m_Info.m_nBucketCount > 0 && m_Info.m_pBuckets && m_Info.m_pNodes) {
            for (uint32_t i = 0; i < m_Info.m_nBucketCount; ++i) {
                uint32_t s_NodeIndex = m_Info.m_pBuckets[i];

                while (s_NodeIndex != UINT32_MAX) {
                    node_type& s_OldNode = m_Info.m_pNodes[s_NodeIndex];
                    const uint64_t s_Hash = THashingPolicy()(s_OldNode.m_value.first);
                    const uint32_t s_NewBucketIndex = static_cast<uint32_t>(s_Hash % p_NewBucketCount);

                    node_type& s_NewNode = s_NewNodes[s_NewSize];
                    s_NewNode.m_value = s_OldNode.m_value;
                    s_NewNode.m_nNextIndex = s_NewBuckets[s_NewBucketIndex];
                    s_NewBuckets[s_NewBucketIndex] = s_NewSize++;

                    s_NodeIndex = s_OldNode.m_nNextIndex;
                }
            }

            for (uint32_t i = 0; i < m_Info.m_nBucketCount; ++i) {
                uint32_t s_NodeIndex = m_Info.m_pBuckets[i];

                while (s_NodeIndex != UINT32_MAX) {
                    node_type& s_Node = m_Info.m_pNodes[s_NodeIndex];

                    s_Node.m_value.second.~TValueType();
                    s_Node.m_value.first.~TKeyType();

                    s_NodeIndex = s_Node.m_nNextIndex;
                }
            }
        }

        if (m_Info.m_pBuckets) {
            p_Allocator->Free(m_Info.m_pBuckets);
        }

        if (m_Info.m_pNodes) {
            p_Allocator->Free(m_Info.m_pNodes);
        }

        m_Info.m_nBucketCount = p_NewBucketCount;
        m_Info.m_pBuckets = s_NewBuckets;
        m_Info.m_pNodes = s_NewNodes;
        m_nSize = s_NewSize;
        m_nFreeSlots = UINT32_MAX;
    }

    bool erase(const TKeyType& p_Key) {
        if (!m_Info.m_pBuckets) {
            return false;
        }

        const uint64_t s_Hash = THashingPolicy()(p_Key);
        const uint32_t s_BucketIndex = static_cast<uint32_t>(s_Hash % m_Info.m_nBucketCount);
        uint32_t s_NodeIndex = m_Info.m_pBuckets[s_BucketIndex];

        while (s_NodeIndex != UINT32_MAX) {
            node_type* s_pCurrentNode = &m_Info.m_pNodes[s_NodeIndex];

            if (s_pCurrentNode->m_value.first == p_Key) {
                eraseNode(s_BucketIndex, s_pCurrentNode);

                return true;
            }

            s_NodeIndex = s_pCurrentNode->m_nNextIndex;
        }

        return false;
    }

    iterator erase(iterator p_Where) {
        if (!m_Info.m_pBuckets || !p_Where.m_pCurrent)
            return end();

        iterator s_Next = p_Where;
        ++s_Next;

        const TKeyType& s_Key = p_Where->first;
        const uint64_t s_Hash = THashingPolicy()(s_Key);
        const uint32_t s_BucketIndex = static_cast<uint32_t>(s_Hash % m_Info.m_nBucketCount);

        eraseNode(s_BucketIndex, p_Where.m_pCurrent);

        return s_Next;
    }

    void clear() {
        if (!m_Info.m_pBuckets || !m_Info.m_pNodes) {
            return;
        }

        for (uint32_t i = 0; i < m_Info.m_nBucketCount; ++i) {
            uint32_t s_NodeIndex = m_Info.m_pBuckets[i];

            while (s_NodeIndex != UINT32_MAX) {
                node_type& s_Node = m_Info.m_pNodes[s_NodeIndex];

                s_Node.m_value.second.~TValueType();
                s_Node.m_value.first.~TKeyType();

                s_NodeIndex = s_Node.m_nNextIndex;
            }
        }

        std::fill_n(m_Info.m_pBuckets, m_Info.m_nBucketCount, UINT32_MAX);

        for (uint32_t i = 0; i < m_Info.m_nBucketCount; ++i) {
            m_Info.m_pNodes[i].m_nNextIndex = UINT32_MAX;
        }

        m_nSize = 0;
        m_nFreeSlots = UINT32_MAX;
    }

private:
    node_type* findNode(const TKeyType& p_Key) const {
        if (!m_Info.m_pBuckets) {
            return nullptr;
        }

        const uint64_t s_Hash = THashingPolicy()(p_Key);
        const uint32_t s_BucketIndex = static_cast<uint32_t>(s_Hash % m_Info.m_nBucketCount);
        uint32_t s_NodeIndex = m_Info.m_pBuckets[s_BucketIndex];

        while (s_NodeIndex != UINT32_MAX) {
            node_type& s_Node = m_Info.m_pNodes[s_NodeIndex];

            if (s_Node.m_value.first == p_Key) {
                return &s_Node;
            }

            s_NodeIndex = s_Node.m_nNextIndex;
        }

        return nullptr;
    }

    void eraseNode(uint32_t p_BucketIndex, node_type* p_TargetNode) {
        if (!m_Info.m_pBuckets || !p_TargetNode) {
            return;
        }

        uint32_t* s_pBucket = &m_Info.m_pBuckets[p_BucketIndex];
        uint32_t s_NodeIndex = *s_pBucket;
        node_type* s_pPrevNode = nullptr;

        while (s_NodeIndex != UINT32_MAX) {
            node_type* s_pCurrentNode = &m_Info.m_pNodes[s_NodeIndex];

            if (s_pCurrentNode == p_TargetNode) {
                if (s_pPrevNode) {
                    s_pPrevNode->m_nNextIndex = s_pCurrentNode->m_nNextIndex;
                }
                else {
                    *s_pBucket = s_pCurrentNode->m_nNextIndex;
                }

                s_pCurrentNode->m_value.second.~TValueType();
                s_pCurrentNode->m_value.first.~TKeyType();

                s_pCurrentNode->m_nNextIndex = m_nFreeSlots;
                m_nFreeSlots = s_NodeIndex;
                --m_nSize;

                return;
            }

            s_pPrevNode = s_pCurrentNode;
            s_NodeIndex = s_pCurrentNode->m_nNextIndex;
        }
    }

public:
    uint32_t m_nSize = 0;
    uint32_t m_nFreeSlots = UINT32_MAX;
    SHashMapInfo<value_type> m_Info;
};