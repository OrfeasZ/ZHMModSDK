#pragma once

#include "TArray.h"
#include "ZDelegate.h"
#include "TList.h"

class ZEventNull;

class ZEvent0 {
public:
    struct SInvocationData {
        SInvocationData* pNullOrDelegateAddedIndicator;
        uint32_t nRemoved;
        TList<TPair<int32_t, ZDelegate<void()>>> added;
    };

    TArray<TPair<int32_t, ZDelegate<void()>>> m_delegates;
    SInvocationData* m_pInvocation;
};

template <typename TArg1>
class ZEvent1 {
public:
    struct SInvocationData {
        SInvocationData* pNullOrDelegateAddedIndicator;
        uint32_t nRemoved;
        TList<TPair<int32_t, ZDelegate<void(TArg1)>>> added;
    };

    TArray<TPair<int32_t, ZDelegate<void(TArg1)>>> m_delegates;
    SInvocationData* m_pInvocation;
};

template <typename TArg1, typename TArg2>
struct ZEvent2 {
public:
    struct SInvocationData {
        SInvocationData* pNullOrDelegateAddedIndicator;
        uint32 nRemoved;
        TList<TPair<int32_t, ZDelegate<void(TArg1, TArg2)>>> added;
    };

    TArray<TPair<int32_t, ZDelegate<void(TArg1, TArg2)>>> m_delegates;
    SInvocationData* m_pInvocation;
};

template <typename TArg1, typename TArg2, typename TArg3>
struct ZEvent3 {
public:
    struct SInvocationData {
        SInvocationData* pNullOrDelegateAddedIndicator;
        uint32 nRemoved;
        TList<TPair<int32_t, ZDelegate<void(TArg1, TArg2, TArg3)>>> added;
    };

    TArray<TPair<int32_t, ZDelegate<void(TArg1, TArg2, TArg3)>>> m_delegates;
    SInvocationData* m_pInvocation;
};

template <typename TArg1, typename TArg2, typename TArg3, typename TArg4>
struct ZEvent4 {
public:
    struct SInvocationData {
        SInvocationData* pNullOrDelegateAddedIndicator;
        uint32 nRemoved;
        TList<TPair<int32_t, ZDelegate<void(TArg1, TArg2, TArg3, TArg4)>>> added;
    };

    TArray<TPair<int32_t, ZDelegate<void(TArg1, TArg2, TArg3, TArg4)>>> m_delegates;
    SInvocationData* m_pInvocation;
};

template <
    typename TArg1 = ZEventNull,
    typename TArg2 = ZEventNull,
    typename TArg3 = ZEventNull,
    typename TArg4 = ZEventNull,
    typename TArg5 = ZEventNull
>
class ZEvent;

template <>
class ZEvent<ZEventNull, ZEventNull, ZEventNull, ZEventNull, ZEventNull>
    : public ZEvent0 {
};

template <typename TArg1>
class ZEvent<TArg1, ZEventNull, ZEventNull, ZEventNull, ZEventNull>
    : public ZEvent1<TArg1> {
};

template <typename TArg1, typename TArg2>
class ZEvent<TArg1, TArg2, ZEventNull, ZEventNull, ZEventNull>
    : public ZEvent2<TArg1, TArg2> {
};

template <typename TArg1, typename TArg2, typename TArg3>
class ZEvent<TArg1, TArg2, TArg3, ZEventNull, ZEventNull>
    : public ZEvent3<TArg1, TArg2, TArg3> {
};

template <typename TArg1, typename TArg2, typename TArg3, typename TArg4>
class ZEvent<TArg1, TArg2, TArg3, TArg4, ZEventNull>
    : public ZEvent4<TArg1, TArg2, TArg3, TArg4> {
};