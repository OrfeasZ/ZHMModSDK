#pragma once

#include "EventDispatcher.h"

#include <Windows.h>
#include <vector>
#include <algorithm>

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
	}

	~EventDispatcherImpl() override
	{
		AcquireSRWLockExclusive(&m_Lock);
		m_Listeners.clear();
		ReleaseSRWLockExclusive(&m_Lock);
	}

protected:
	void AddListenerInternal(void* p_Listener) override
	{
		// We remove it first to make sure we only have unique listeners
		// in our list. We could use a set to make this easier but iteration
		// performance wouldn't be very great.		
		RemoveListenerInternal(p_Listener);

		AcquireSRWLockExclusive(&m_Lock);
		m_Listeners.insert(m_Listeners.end() - 1, p_Listener);
		ReleaseSRWLockExclusive(&m_Lock);
	}

	void RemoveListenerInternal(void* p_Listener) override
	{
		AcquireSRWLockExclusive(&m_Lock);
		m_Listeners.erase(std::remove(m_Listeners.begin(), m_Listeners.end(), p_Listener), m_Listeners.end());
		ReleaseSRWLockExclusive(&m_Lock);
	}

	void** GetListeners() override
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
	std::vector<void*> m_Listeners;
};

#define DEFINE_EVENT(EventName, ...) \
	EventDispatcher<__VA_ARGS__>* Events::EventName = new EventDispatcherImpl<__VA_ARGS__>();\
	\
	static ScopedDestructible g_ ## EventName ## _Destructible = ScopedDestructible(reinterpret_cast<IDestructible**>(&Events::EventName));