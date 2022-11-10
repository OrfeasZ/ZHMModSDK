#pragma once

#include "ZPrimitives.h"

extern void* g_EmptyDelegatePtr;

template <class T>
class ZDelegate;

template <class ReturnType, class... Args>
class ZDelegate<ReturnType(Args...)>
{
public:
	ZDelegate() :
		m_mfp(nullptr),
		m_pad(nullptr),
		pStaticFunc(nullptr),
		pInst(nullptr)
	{
	}

	ReturnType operator()(Args... p_Args)
	{
		// We're only targeting x64 msvc, so calling convention for both static
		// and member functions is the same (__fastcall).
		if (m_mfp)
			return reinterpret_cast<ReturnType(*)(void*, Args...)>(m_mfp)(pInst, p_Args...);

		return pStaticFunc(p_Args...);
	}

protected:
	typedef ReturnType(* MemberCallback_t)(void*, Args...);
	typedef ReturnType(* StaticCallback_t)(Args...);

	ZDelegate(MemberCallback_t p_MemberCb, StaticCallback_t p_StaticCb, void* p_MemberInstance) :
		m_mfp(p_MemberCb),
		m_pad(nullptr),
		pStaticFunc(p_StaticCb),
		pInst(p_MemberInstance)
	{
	}

public:
	MemberCallback_t m_mfp;
	void* m_pad;
	StaticCallback_t pStaticFunc;
	void* pInst;
};

template <class InstanceType, class T>
class ZMemberDelegate;

template <class InstanceType, class ReturnType, class... Args>
class ZMemberDelegate<InstanceType, ReturnType(Args...)> : public ZDelegate<ReturnType(Args...)>
{
private:
	typedef ReturnType(InstanceType::* MemberFunction_t)(Args...);

	union MemberFunctionCaster
	{
		MemberFunction_t MemberFunc;
		typename ZDelegate<ReturnType(Args...)>::MemberCallback_t MemberCallback;
	};

public:
	ZMemberDelegate(InstanceType* p_Instance, MemberFunction_t p_MemberFunction) :
		ZDelegate<ReturnType(Args...)>(GetMemberCb(p_MemberFunction), nullptr, p_Instance)
	{
	}

private:
	typename ZDelegate<ReturnType(Args...)>::MemberCallback_t GetMemberCb(MemberFunction_t p_MemberFunction)
	{
		MemberFunctionCaster s_Caster;
		s_Caster.MemberFunc = p_MemberFunction;

		return s_Caster.MemberCallback;
	}
};

template <class T>
class ZStaticDelegate;

template <class ReturnType, class... Args>
class ZStaticDelegate<ReturnType(Args...)> : public ZDelegate<ReturnType(Args...)>
{
public:
	ZStaticDelegate(typename ZDelegate<ReturnType(Args...)>::StaticCallback_t p_Callback) :
		ZDelegate<ReturnType(Args...)>(nullptr, p_Callback, nullptr)
	{
	}
};
