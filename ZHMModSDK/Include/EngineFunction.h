#pragma once

#include <type_traits>

template <class T>
class EngineFunction;

template <class ReturnType, class... Args>
class EngineFunction<ReturnType(Args...)>
{
protected:
    EngineFunction(void* p_Address) :
        m_Address(p_Address)
    {
    }

public:
    ReturnType Call(Args... p_Args)
    {
        if (m_Address == nullptr)
        {
            if constexpr (std::is_pointer<ReturnType>::value)
                return nullptr;
            else
                return ReturnType();
        }

        return reinterpret_cast<ReturnType(*)(Args...)>(m_Address)(p_Args...);
    }

protected:
    void* m_Address;
};

template <class... Args>
class EngineFunction<void(Args...)>
{
protected:
    EngineFunction(void* p_Address) :
        m_Address(p_Address)
    {
    }

public:
    void Call(Args... p_Args)
    {
        if (m_Address == nullptr)
            return;

        reinterpret_cast<void(*)(Args...)>(m_Address)(p_Args...);
    }

protected:
    void* m_Address;
};
