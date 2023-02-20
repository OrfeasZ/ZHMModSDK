#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#if LOADER_EXPORTS
#	define ZHMSDK_API __declspec(dllexport)
#else
#	define ZHMSDK_API __declspec(dllimport)
#endif

#define ALIGN_TO(address, alignment) static_cast<uintptr_t>(address + alignment - 1) & ~(static_cast<ptrdiff_t>(alignment) - 1)

#ifndef CONCAT_IMPL
#define CONCAT_IMPL(x, y) x##y
#endif

#ifndef MACRO_CONCAT
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#endif

#ifndef PAD
#define PAD(SIZE) unsigned char MACRO_CONCAT(_pad, __COUNTER__)[SIZE];
#endif

class IDestructible
{
public:
    virtual ~IDestructible() = default;
};

class ScopedDestructible
{
public:
    ScopedDestructible(IDestructible** p_Destructible) :
        m_Destructible(p_Destructible)
    {
    }

    ~ScopedDestructible()
    {
        if (*m_Destructible)
            delete* m_Destructible;
    }

private:
    IDestructible** m_Destructible;
};

class ScopedSharedGuard
{
public:
    ScopedSharedGuard(SRWLOCK* p_Lock) : m_Lock(p_Lock)
    {
        AcquireSRWLockShared(m_Lock);
    }

    ~ScopedSharedGuard()
    {
        ReleaseSRWLockShared(m_Lock);
    }

private:
    SRWLOCK* m_Lock;
};

class ScopedExclusiveGuard
{
public:
    ScopedExclusiveGuard(SRWLOCK* p_Lock) : m_Lock(p_Lock)
    {
        AcquireSRWLockExclusive(m_Lock);
    }

    ~ScopedExclusiveGuard()
    {
        ReleaseSRWLockExclusive(m_Lock);
    }

private:
    SRWLOCK* m_Lock;
};
