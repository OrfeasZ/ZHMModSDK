#pragma once

template <typename T>
class alignas(16) ZDelegate;

template <class ReturnType, typename... Args>
class alignas(16) ZDelegate<ReturnType(Args...)>
{
public:
	ReturnType(*m_mfp)(void*, Args...);
	ReturnType(*pStaticFunc)(Args...);
	void* pInst;
};