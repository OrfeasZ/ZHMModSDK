#pragma once

#include "EventDispatcher.h"

#include <Windows.h>
#include <vector>
#include <algorithm>
#include <unordered_set>

class IPluginInterface;

class EventDispatcherRegistry
{
private:
    static std::unordered_set<EventDispatcherBase*>* g_Dispatchers;

public:
    static void RegisterDispatcher(EventDispatcherBase* p_Dispatcher)
    {
        if (g_Dispatchers == nullptr)
            g_Dispatchers = new std::unordered_set<EventDispatcherBase*>();

        g_Dispatchers->insert(p_Dispatcher);
    }

    static void RemoveDispatcher(EventDispatcherBase* p_Dispatcher)
    {
        if (g_Dispatchers == nullptr)
            return;

        g_Dispatchers->erase(p_Dispatcher);
    }

    static void ClearPluginListeners(IPluginInterface* p_Plugin)
    {
        if (g_Dispatchers == nullptr)
            return;

        for (auto s_Dispatcher : *g_Dispatchers)
            s_Dispatcher->RemoveListenersWithContext(p_Plugin);
    }
};

template <class... Args>
class EventDispatcherImpl : public EventDispatcher<Args...>
{
public:
    EventDispatcherImpl()
    {
        InitializeSRWLock(&m_Lock);

        // We push null here because that's what's used by the caller
        // implementation to determine when we've ran out of listeners.
        m_Listeners.push_back(nullptr);

        EventDispatcherRegistry::RegisterDispatcher(this);
    }

    ~EventDispatcherImpl() override
    {
        AcquireSRWLockExclusive(&m_Lock);
        m_Listeners.clear();
        ReleaseSRWLockExclusive(&m_Lock);

        EventDispatcherRegistry::RemoveDispatcher(this);
    }

    void RemoveListenersWithContext(void* p_Context) override
    {
        AcquireSRWLockExclusive(&m_Lock);

        for (auto it = m_Listeners.begin(); it != m_Listeners.end();)
        {
            if (*it == nullptr)
            {
                ++it;
                continue;
            }

            if ((*it)->Context == p_Context)
            {
                delete* it;
                it = m_Listeners.erase(it);
            }
            else
            {
                ++it;
            }
        }

        ReleaseSRWLockExclusive(&m_Lock);
    }

protected:
    void AddListenerInternal(void* p_Context, void* p_Listener) override
    {
        // We remove it first to make sure we only have unique listeners
        // in our list. We could use a set to make this easier but iteration
        // performance wouldn't be very great.
        RemoveListenerInternal(p_Listener);

        AcquireSRWLockExclusive(&m_Lock);

        auto* s_Registration = new EventDispatcherBase::EventListenerRegistration();
        s_Registration->Listener = p_Listener;
        s_Registration->Context = p_Context;

        m_Listeners.insert(m_Listeners.end() - 1, s_Registration);

        ReleaseSRWLockExclusive(&m_Lock);
    }

    void RemoveListenerInternal(void* p_Listener) override
    {
        AcquireSRWLockExclusive(&m_Lock);

        for (auto it = m_Listeners.begin(); it != m_Listeners.end();)
        {
            if (*it == nullptr)
            {
                ++it;
                continue;
            }

            if ((*it)->Listener == p_Listener)
            {
                delete* it;
                it = m_Listeners.erase(it);
            }
            else
            {
                ++it;
            }
        }

        ReleaseSRWLockExclusive(&m_Lock);
    }

    EventDispatcherBase::EventListenerRegistration** GetRegistrations() override
    {
        return m_Listeners.data();
    }

    void LockForCall() override
    {
        AcquireSRWLockShared(&m_Lock);
    }

    void UnlockForCall() override
    {
        ReleaseSRWLockShared(&m_Lock);
    }

private:
    SRWLOCK m_Lock;
    std::vector<EventDispatcherBase::EventListenerRegistration*> m_Listeners;
};

#define DEFINE_EVENT(EventName, ...) \
    EventDispatcher<__VA_ARGS__>* Events::EventName = new EventDispatcherImpl<__VA_ARGS__>();\
    \
    static ScopedDestructible g_ ## EventName ## _Destructible = ScopedDestructible(reinterpret_cast<IDestructible**>(&Events::EventName));
