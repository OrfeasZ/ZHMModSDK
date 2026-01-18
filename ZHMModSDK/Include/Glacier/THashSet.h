#pragma once

#include <cstdint>
#include <cstddef>

#include "Common.h"
#include "Hash.h"

template <typename T>
class TDefaultHashSetPolicy {
public:
    static uint64_t GetHashCode(const T& p_Key) {
        if constexpr (requires { p_Key.GetHashCode(); }) {
            return static_cast<uint64_t>(p_Key.GetHashCode());
        }
        else {
            return Hash::Fnv1a(reinterpret_cast<const char*>(&p_Key), sizeof(T));
        }
    }
};

template <typename T>
class TDefaultHashSetPolicy<T*> {
public:
    static uint64_t GetHashCode(const T* p_Key) {
        return Hash::Pointer(p_Key);
    }
};

template <typename T>
class THashSetNode {
public:
    uint32_t m_iNext;
    T m_data;
};

template <typename T>
struct SHashSetInfo {
    bool m_bFixed;
    uint32_t m_nBuckets;
    uint32_t* m_pBuckets;
    THashSetNode<T>* m_pNodes;
};

template <class T>
class THashSetIterator : public TIterator<THashSetNode<T>> {
protected:
    THashSetIterator(const SHashSetInfo<T>* p_Info, uint32_t p_Bucket, THashSetNode<T>* p_Current) :
        TIterator<THashSetNode<T>>(p_Current),
        m_pInfo(p_Info),
        m_nBucket(p_Bucket) {
    }

    THashSetIterator(const SHashSetInfo<T>* p_Info) :
        TIterator<THashSetNode<T>>(nullptr),
        m_pInfo(p_Info),
        m_nBucket(UINT32_MAX) {
    }

public:
    THashSetIterator<T>& operator++() {
        uint32_t s_NextIndex = this->m_pCurrent->m_iNext;

        if (s_NextIndex != UINT32_MAX) {
            this->m_pCurrent = &m_pInfo->m_pNodes[s_NextIndex];
            return *this;
        }

        ++m_nBucket;

        if (m_nBucket >= m_pInfo->m_nBuckets) {
            m_nBucket = UINT32_MAX;
            this->m_pCurrent = nullptr;
            return *this;
        }

        while (m_pInfo->m_pBuckets[m_nBucket] == UINT32_MAX) {
            ++m_nBucket;

            if (m_nBucket >= m_pInfo->m_nBuckets) {
                m_nBucket = UINT32_MAX;
                this->m_pCurrent = nullptr;
                return *this;
            }
        }

        this->m_pCurrent = &m_pInfo->m_pNodes[m_pInfo->m_pBuckets[m_nBucket]];

        return *this;
    }

    T& operator*() {
        return this->m_pCurrent->m_data;
    }

    T* operator->() {
        return &this->m_pCurrent->m_data;
    }

    T& operator*() const {
        return this->m_pCurrent->m_data;
    }

    T* operator->() const {
        return &this->m_pCurrent->m_data;
    }

    bool operator==(const THashSetIterator<T>& p_Other) const {
        return p_Other.m_pCurrent == this->m_pCurrent &&
            p_Other.m_nBucket == m_nBucket &&
            p_Other.m_pInfo == m_pInfo;
    }

    bool operator!=(const THashSetIterator<T>& p_Other) const {
        return !(*this == p_Other);
    }

public:
    const SHashSetInfo<T>* m_pInfo = nullptr;
    uint32_t m_nBucket = 0;

    template <class T, class H>
    friend class THashSet;
};

template <class T, class THashingPolicy = TDefaultHashSetPolicy<T>>
class THashSet {
public:
    using node_type = THashSetNode<T>;
    using iterator = THashSetIterator<T>;
    using const_iterator = THashSetIterator<const T>;
    using const_set_info_type = const SHashSetInfo<const T>*;

    const_iterator begin() const {
        if (m_info.m_nBuckets == 0) {
            return const_iterator(reinterpret_cast<const_set_info_type>(&m_info));
        }

        for (uint32_t i = 0; i < m_info.m_nBuckets; ++i) {
            const uint32_t s_NodeIndex = m_info.m_pBuckets[i];

            if (s_NodeIndex != UINT32_MAX) {
                return const_iterator(
                    reinterpret_cast<const_set_info_type>(&m_info),
                    i,
                    reinterpret_cast<THashSetNode<const T>*>(&m_info.m_pNodes[s_NodeIndex])
                );
            }
        }

        return const_iterator(reinterpret_cast<const_set_info_type>(&m_info));
    }

    iterator begin() {
        if (m_info.m_nBuckets == 0) {
            return iterator(&m_info);
        }

        for (uint32_t i = 0; i < m_info.m_nBuckets; ++i) {
            const uint32_t s_NodeIndex = m_info.m_pBuckets[i];

            if (s_NodeIndex != UINT32_MAX) {
                return iterator(&m_info, i, &m_info.m_pNodes[s_NodeIndex]);
            }
        }

        return iterator(&m_info);
    }

    const_iterator end() const {
        return const_iterator(reinterpret_cast<const_set_info_type>(&m_info));
    }

    iterator end() {
        return iterator(&m_info);
    }

    size_t size() const {
        return m_nSize;
    }

    bool empty() const {
        return m_nSize == 0;
    }

    bool contains(const T& p_Key) const {
        return find(p_Key) != end();
    }

    bool contains(const T& p_Key) {
        return find(p_Key) != end();
    }

    const_iterator find(const T& p_Key) const {
        if (auto* s_Node = findNode(p_Key)) {
            return const_iterator(
                reinterpret_cast<const_set_info_type>(&m_info),
                THashingPolicy::GetHashCode(p_Key) % m_info.m_nBuckets,
                reinterpret_cast<THashSetNode<const T>*>(s_Node)
            );
        }

        return end();
    }

    iterator find(const T& p_Key) {
        if (auto* s_Node = findNode(p_Key)) {
            return iterator(
                &m_info,
                THashingPolicy::GetHashCode(p_Key) % m_info.m_nBuckets,
                s_Node
            );
        }

        return end();
    }

private:
    node_type* findNode(const T& p_Key) const {
        if (!m_info.m_pBuckets) {
            return nullptr;
        }

        const uint64_t s_Hash = THashingPolicy::GetHashCode(p_Key);
        const uint32_t s_BucketIndex = static_cast<uint32_t>(s_Hash % m_info.m_nBuckets);
        uint32_t s_NodeIndex = m_info.m_pBuckets[s_BucketIndex];

        while (s_NodeIndex != UINT32_MAX) {
            node_type& s_Node = m_info.m_pNodes[s_NodeIndex];

            if (s_Node.m_data == p_Key) {
                return &s_Node;
            }

            s_NodeIndex = s_Node.m_iNext;
        }

        return nullptr;
    }

public:
    uint32_t m_nSize;
    uint32_t m_iFree;
    SHashSetInfo<T> m_info;
};

static_assert(sizeof(THashSet<void*>) == 0x20);