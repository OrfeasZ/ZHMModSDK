#pragma once

#include "ZPrimitives.h"

class ZMutex {
public:
  uint64_t m_impl[5];
  uint32_t m_nUniqueID;
};

class ZInfiniteBuffer {
public:
  void* m_pData;
  uint32_t m_nSize;
  uint32_t m_nActualSize;
  uint32_t m_nMaxSize;
};

class ZObjectPool {
public:
  uint64_t m_nFreeListStart;
  uint32_t* m_pData;
  uint32_t m_nObjectSize;
  uint32_t m_nMaxObjectCount;
  uint32_t m_nGrowCount;
  uint32_t m_nObjectDelta;
  uint32_t m_nSize;
  ZInfiniteBuffer m_Buffer;
  ZMutex m_Mutex;
};

template <class T>
class TObjectPool {
public:
  ZObjectPool m_Pool;
  T* m_pStart;
  T* m_pEnd;

  size_t IndexOf(T* p_Object) {
    return (reinterpret_cast<uintptr_t>(p_Object) - reinterpret_cast<uintptr_t>(m_pStart)) / sizeof(T);
  }
};
