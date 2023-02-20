#pragma once

#include "EngineFunction.h"
#include "ModSDK.h"
#include "Logging.h"
#include "Util/ProcessUtils.h"

template <class T>
class PatternEngineFunction;

template <class ReturnType, class... Args>
class PatternEngineFunction<ReturnType(Args...)> final : public EngineFunction<ReturnType(Args...)>
{
public:
    PatternEngineFunction(const char* p_FunctionName, const char* p_Pattern, const char* p_Mask) :
        EngineFunction<ReturnType(Args...)>(GetTarget(p_Pattern, p_Mask))
    {
        if (this->m_Address == nullptr)
        {
            Logger::Error("Could not locate address for function '{}'. This probably means that the game was updated and the SDK requires changes.", p_FunctionName);
            return;
        }

        Logger::Debug("Successfully located function '{}' at address '{}'.", p_FunctionName, fmt::ptr(this->m_Address));
    }

private:
    void* GetTarget(const char* p_Pattern, const char* p_Mask) const
    {
        const auto* s_Pattern = reinterpret_cast<const uint8_t*>(p_Pattern);
        return reinterpret_cast<void*>(Util::ProcessUtils::SearchPattern(ModSDK::GetInstance()->GetModuleBase(), ModSDK::GetInstance()->GetSizeOfCode(), s_Pattern, p_Mask));
    }
};

template <class T>
class PatternRelativeEngineFunction;

template <class ReturnType, class... Args>
class PatternRelativeEngineFunction<ReturnType(Args...)> final : public EngineFunction<ReturnType(Args...)>
{
public:
    PatternRelativeEngineFunction(const char* p_FunctionName, const char* p_Pattern, const char* p_Mask) :
        EngineFunction<ReturnType(Args...)>(GetTarget(p_FunctionName, p_Pattern, p_Mask))
    {
        if (this->m_Address == nullptr)
        {
            Logger::Error("Could not locate address for function '{}'. This probably means that the game was updated and the SDK requires changes.", p_FunctionName);
            return;
        }

        Logger::Debug("Successfully located function '{}' at address '{}'.", p_FunctionName, fmt::ptr(this->m_Address));
    }

private:
    void* GetTarget(const char* p_FunctionName, const char* p_Pattern, const char* p_Mask) const
    {
        const auto* s_Pattern = reinterpret_cast<const uint8_t*>(p_Pattern);
        auto s_Target = Util::ProcessUtils::SearchPattern(ModSDK::GetInstance()->GetModuleBase(), ModSDK::GetInstance()->GetSizeOfCode(), s_Pattern, p_Mask);

        // We expect this to be a CALL (0xE8) instruction.
        if (s_Target != 0 && *reinterpret_cast<uint8_t*>(s_Target) != 0xE8)
        {
            Logger::Error("Expected a call instruction for function '{}' at address {} but instead got 0x{:02X}.", p_FunctionName, fmt::ptr(reinterpret_cast<void*>(s_Target)), *reinterpret_cast<uint8_t*>(s_Target));
            return nullptr;
        }

        if (s_Target == 0)
            return nullptr;

        const uintptr_t s_OriginalFunction = s_Target + 5 + *reinterpret_cast<int32_t*>(s_Target + 1);
        return reinterpret_cast<void*>(s_OriginalFunction);
    }
};


#define PATTERN_FUNCTION(Pattern, Mask, FunctionName, FunctionType) \
    EngineFunction<FunctionType>* Functions::FunctionName = new PatternEngineFunction<FunctionType>(#FunctionName, Pattern, Mask);

#define PATTERN_RELATIVE_FUNCTION(Pattern, Mask, FunctionName, FunctionType) \
    EngineFunction<FunctionType>* Functions::FunctionName = new PatternRelativeEngineFunction<FunctionType>(#FunctionName, Pattern, Mask);
