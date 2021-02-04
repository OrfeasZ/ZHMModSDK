#pragma once

#include "Common.h"

class EventDispatcherBase : public IDestructible
{
public:
	~EventDispatcherBase() override = default;
	
protected:	
	virtual void AddListenerInternal(void* p_Listener) = 0;
	virtual void RemoveListenerInternal(void* p_Listener) = 0;
	virtual void** GetListeners() = 0;
	virtual void LockForCall() = 0;
	virtual void UnlockForCall() = 0;
};

/**
 * A thread-safe event listener registry.
 * NOTE: Registering a listener from a listener callback will result in a deadlock.
 */
template <class... Args>
class EventDispatcher : public EventDispatcherBase
{
public:
	typedef void (*EventListener_t)(Args...);

	EventListener_t AddListener(EventListener_t p_Listener)
	{
		AddListenerInternal(p_Listener);
		return p_Listener;
	}

	void RemoveListener(EventListener_t p_Listener)
	{
		RemoveListenerInternal(p_Listener);
	}

	void Call(Args... p_Args)
	{
		LockForCall();

		auto s_Listeners = static_cast<EventListener_t*>(GetListeners());

		auto s_Listener = *s_Listeners;

		while (s_Listener != nullptr)
		{
			s_Listener(p_Args...);
			s_Listener = *++s_Listeners;
		}

		UnlockForCall();
	}
};

template <>
class EventDispatcher<void> : public EventDispatcherBase
{
public:
	typedef void (*EventListener_t)();

	EventListener_t AddListener(EventListener_t p_Listener)
	{
		AddListenerInternal(p_Listener);
		return p_Listener;
	}

	void RemoveListener(EventListener_t p_Listener)
	{
		RemoveListenerInternal(p_Listener);
	}

	void Call()
	{
		LockForCall();
		
		auto s_Listeners = reinterpret_cast<EventListener_t*>(GetListeners());

		auto s_Listener = *s_Listeners;

		while (s_Listener != nullptr)
		{
			s_Listener();
			s_Listener = *++s_Listeners;
		}

		UnlockForCall();
	}
};
