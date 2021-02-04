#pragma once

#if LOADER_EXPORTS
#	define ZHMSDK_API __declspec(dllexport)
#else
#	define ZHMSDK_API __declspec(dllimport)
#endif

#define ALIGN_TO(address, alignment) static_cast<uintptr_t>(address + alignment - 1) & ~(static_cast<ptrdiff_t>(alignment) - 1)

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
			delete *m_Destructible;
	}

private:
	IDestructible** m_Destructible;
};