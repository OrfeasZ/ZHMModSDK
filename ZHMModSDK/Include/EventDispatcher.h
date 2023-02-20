#pragma once

#include "Common.h"

class EventDispatcherBase : public IDestructible
{
public:
    ~EventDispatcherBase() override = default;
    virtual void RemoveListenersWithContext(void* p_Context) = 0;

protected:
    struct EventListenerRegistration
    {
        void* Context;
        void* Listener;
    };

protected:
    virtual void AddListenerInternal(void* p_Context, void* p_Listener) = 0;
    virtual void RemoveListenerInternal(void* p_Listener) = 0;
    virtual EventListenerRegistration** GetRegistrations() = 0;
    virtual void LockForCall() = 0;
    virtual void UnlockForCall() = 0;

    friend class EventDispatcherRegistry;
};

/**
 * A thread-safe event listener registry.
 * NOTE: Registering a listener from a listener callback will result in a deadlock.
 */
template <class... Args>
class EventDispatcher : public EventDispatcherBase
{
public:
    typedef void (*EventListener_t)(void*, Args...);

    EventListener_t AddListener(void* p_Context, EventListener_t p_Listener)
    {
        AddListenerInternal(p_Context, p_Listener);
        return p_Listener;
    }

    void RemoveListener(EventListener_t p_Listener)
    {
        RemoveListenerInternal(p_Listener);
    }

    void Call(Args... p_Args)
    {
        LockForCall();

        const auto* s_Registrations = GetRegistrations();

        auto* s_Registration = *s_Registrations;

        while (s_Registration != nullptr)
        {
            const auto s_Listener = static_cast<EventListener_t>(s_Registration->Listener);
            s_Listener(s_Registration->Context);
            s_Registration = *++s_Registrations;
        }

        UnlockForCall();
    }
};

template <>
class EventDispatcher<void> : public EventDispatcherBase
{
public:
    typedef void (*EventListener_t)(void*);

    EventListener_t AddListener(void* p_Context, EventListener_t p_Listener)
    {
        AddListenerInternal(p_Context, p_Listener);
        return p_Listener;
    }

    void RemoveListener(EventListener_t p_Listener)
    {
        RemoveListenerInternal(p_Listener);
    }

    void Call()
    {
        LockForCall();

        const auto* s_Registrations = GetRegistrations();

        auto* s_Registration = *s_Registrations;

        while (s_Registration != nullptr)
        {
            const auto s_Listener = static_cast<EventListener_t>(s_Registration->Listener);
            s_Listener(s_Registration->Context);
            s_Registration = *++s_Registrations;
        }

        UnlockForCall();
    }
};
