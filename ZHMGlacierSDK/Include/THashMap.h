#pragma once

#include "ZString.h"
#include "TPair.h"

template <typename T>
class THashMapNode
{
public:
	uint32_t m_nNextIndex;
	T m_value;
};

template <typename T, typename Z>
class THashMap
{
public:
	uint32_t m_nSize;
	uint32_t m_unk00;
	uint32_t m_unk01;
	void* m_unk02;
	THashMapNode<TPair<T, Z>>* m_pNodes;
};