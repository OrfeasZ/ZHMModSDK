#pragma once

#include "Hash.h"
#include "ZString.h"
#include "TPair.h"
#include "TArray.h"

template <class T>
class THashMapNode
{
public:
	int32_t m_nNextIndex;
	T m_value;
};

template <class T>
struct SHashMapInfo
{
	uint32_t m_nBucketCount;
	uint32_t* m_pBuckets;
	THashMapNode<T>* m_pNodes;
};

template <class T>
struct TDefaultHashMapPolicy;

template <>
struct TDefaultHashMapPolicy<ZString>
{
	uint64_t operator()(const ZString& p_Value) const
	{
		return Hash::Fnv1a64(p_Value.c_str(), p_Value.size());
	}
};

template <class T>
class THashMapIterator : public TIterator<THashMapNode<T>>
{
protected:
	THashMapIterator(SHashMapInfo<T>* p_MapInfo, uint32_t p_Bucket, THashMapNode<T>* p_Current) :
		TIterator<THashMapNode<T>>(p_Current),
		m_pMapInfo(p_MapInfo),
		m_nBucket(p_Bucket)
	{
	}
	
	THashMapIterator(SHashMapInfo<T>* p_MapInfo) :
		TIterator<THashMapNode<T>>(nullptr),
		m_pMapInfo(p_MapInfo),
		m_nBucket(UINT32_MAX)
	{
	}
	
public:
	THashMapIterator<T>& operator++()
	{
		uint32_t s_NextIndex = this->m_pCurrent->m_nNextIndex;
		
		if (s_NextIndex != UINT32_MAX)
		{
			this->m_pCurrent = &m_pMapInfo->m_pNodes[s_NextIndex];
			return *this;
		}

		++m_nBucket;

		if (m_nBucket >= m_pMapInfo->m_nBucketCount)
		{
			m_nBucket = UINT32_MAX;
			this->m_pCurrent = nullptr;
			return *this;
		}
		
		while (m_pMapInfo->m_pBuckets[m_nBucket] == UINT32_MAX)
		{
			++m_nBucket;

			if (m_nBucket >= m_pMapInfo->m_nBucketCount)
			{
				m_nBucket = UINT32_MAX;
				this->m_pCurrent = nullptr;
				return *this;
			}
		}

		this->m_pCurrent = &m_pMapInfo->m_pNodes[m_pMapInfo->m_pBuckets[m_nBucket]];

		return *this;
	}

	T& operator*()
	{
		return this->m_pCurrent->m_value;
	}

	T* operator->()
	{
		return &this->m_pCurrent->m_value;
	}

	bool operator==(const THashMapIterator<T>& p_Other) const
	{
		return p_Other.m_pCurrent == this->m_pCurrent &&
			p_Other.m_nBucket == m_nBucket &&
			p_Other.m_pMapInfo == m_pMapInfo;
	}
	
public:
	SHashMapInfo<T>* m_pMapInfo;
	int32_t m_nBucket;

	template <class K, class V, class H>
	friend class THashMap;
};

template <class TKeyType, class TValueType, class THashingPolicy = TDefaultHashMapPolicy<TKeyType>>
class THashMap
{
public:
    using value_type = TPair<const TKeyType, TValueType>;
	using node_type = THashMapNode<value_type>;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = THashMapIterator<value_type>;
    using const_iterator = const THashMapIterator<value_type>;
	
public:
	iterator find(const TKeyType& p_Key)
	{
		if (!m_Info.m_pBuckets)
			return iterator(&m_Info);

		const auto s_Hash = THashingPolicy()(p_Key);

		uint32_t s_Bucket = s_Hash % m_Info.m_nBucketCount;
		auto s_NodeIndex = m_Info.m_pBuckets[s_Bucket];

		if (s_NodeIndex == UINT32_MAX)
			return iterator(&m_Info);

		node_type* s_Node = nullptr;
		
		while (true)
		{
			s_Node = &m_Info.m_pNodes[s_NodeIndex];

			if (s_Node->m_value.first == p_Key)
				break;

			if (s_Node->m_nNextIndex == UINT32_MAX)
				return iterator(&m_Info);

			s_NodeIndex = s_Node->m_nNextIndex;
		}

		return iterator(&m_Info, s_Bucket, s_Node);
	}

	iterator begin()
	{
		if (m_Info.m_nBucketCount == 0 || m_Info.m_pBuckets[0] == UINT32_MAX)
			return iterator(&m_Info);

		auto s_FirstNode = m_Info.m_pNodes[m_Info.m_pBuckets[0]];
		return iterator(&m_Info, 0, &s_FirstNode);
	}

	iterator end()
	{
		return iterator(&m_Info);
	}

	size_t size() const
	{
		return m_nSize;
	}
	
public:
	uint32_t m_nSize;
	uint32_t m_nFreeSlots;
	SHashMapInfo<value_type> m_Info;
};