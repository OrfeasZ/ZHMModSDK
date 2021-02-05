#pragma once

#include <cassert>

#include "Common.h"

class IPluginInterface;

class HookBase : public IDestructible
{
public:
	~HookBase() override = default;
	
protected:
	struct Detour
	{
		IPluginInterface* Plugin;
		void* DetourFunc;
	};
	
	virtual void AddDetourInternal(IPluginInterface* p_Plugin, void* p_Detour) = 0;
	virtual void RemoveDetourInternal(void* p_Detour) = 0;
	virtual Detour** GetDetours() = 0;
	virtual void RemovePluginDetours(IPluginInterface* p_Plugin) = 0;

	friend class HookRegistry;
};

namespace HookAction
{
	struct Return {};
	struct Continue {};
}

template <class T>
class HookResult
{
public:
	HookResult(HookAction::Return, T p_ReturnVal) :
		m_ReturnVal(p_ReturnVal),
		m_HasReturnVal(true)
	{
	}

	HookResult(HookAction::Continue) :
		m_HasReturnVal(false)
	{
	}

public:
	T m_ReturnVal;
	bool m_HasReturnVal;
};

template <>
class HookResult<void>
{	
public:
	HookResult(HookAction::Return) : m_HasReturnVal(true) {}
	HookResult(HookAction::Continue) : m_HasReturnVal(false) {}

public:
	bool m_HasReturnVal;
};

template <class ReturnType, class... Args>
class Hook : public HookBase
{
public:
	typedef ReturnType(__fastcall* OriginalFunc_t)(Args...);
	typedef HookResult<ReturnType>(*DetourFunc_t)(IPluginInterface*, Hook<ReturnType, Args...>*, Args...);

	void AddDetour(IPluginInterface* p_Plugin, DetourFunc_t p_Detour)
	{
		AddDetourInternal(p_Plugin, p_Detour);
	}

	void RemoveDetour(DetourFunc_t p_Detour)
	{
		RemoveDetourInternal(p_Detour);
	}

	ReturnType Call(Args... p_Args)
	{
		auto s_Detours = GetDetours();

		auto s_Detour = *s_Detours;

		while (s_Detour != nullptr)
		{
			auto s_DetourFunc = static_cast<DetourFunc_t>(s_Detour->DetourFunc);
			auto s_Result = s_DetourFunc(s_Detour->Plugin, this, p_Args...);

			// Detour returned a value. Stop execution and return it.
			if (s_Result.m_HasReturnVal)
				return s_Result.m_ReturnVal;

			s_Detour = *++s_Detours;
		}

		// None of the detours returned a value. Call the original function.
		return CallOriginal(p_Args...);
	}

	ReturnType CallOriginal(Args... p_Args)
	{
		assert(m_OriginalFunc != nullptr);
		return m_OriginalFunc(p_Args...);
	}

protected:
	OriginalFunc_t m_OriginalFunc = nullptr;
};

template <class... Args>
class Hook<void, Args...> : public HookBase
{
public:
	typedef void (__fastcall* OriginalFunc_t)(Args...);
	typedef HookResult<void> (*DetourFunc_t)(IPluginInterface*, Hook<void, Args...>*, Args...);

	void AddDetour(IPluginInterface* p_Plugin, DetourFunc_t p_Detour)
	{
		AddDetourInternal(p_Plugin, p_Detour);
	}

	void RemoveDetour(DetourFunc_t p_Detour)
	{
		RemoveDetourInternal(p_Detour);
	}

	void Call(Args... p_Args)
	{
		auto s_Detours = GetDetours();

		auto s_Detour = *s_Detours;

		while (s_Detour != nullptr)
		{
			auto s_DetourFunc = static_cast<DetourFunc_t>(s_Detour->DetourFunc);
			auto s_Result = s_DetourFunc(s_Detour->Plugin, this, p_Args...);

			// Detour returned a value. Stop execution and return it.
			if (s_Result.m_HasReturnVal)
				return;

			s_Detour = *++s_Detours;
		}

		// None of the detours returned a value. Call the original function.
		CallOriginal(p_Args...);
	}

	void CallOriginal(Args... p_Args)
	{
		assert(m_OriginalFunc != nullptr);
		m_OriginalFunc(p_Args...);
	}

protected:
	OriginalFunc_t m_OriginalFunc = nullptr;
};
