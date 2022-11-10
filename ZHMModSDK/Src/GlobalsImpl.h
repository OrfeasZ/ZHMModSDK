#pragma once

#include <cstdint>

#include "ModSDK.h"
#include "Util/ProcessUtils.h"
#include "Logging.h"

template <class T>
T PatternGlobalRelative(const char* p_GlobalName, const char* p_Pattern, const char* p_Mask, ptrdiff_t p_Offset)
{
	static_assert(std::is_pointer<T>::value, "Global type is not a pointer type.");

	const auto* s_Pattern = reinterpret_cast<const uint8_t*>(p_Pattern);
	auto s_Target = Util::ProcessUtils::SearchPattern(ModSDK::GetInstance()->GetModuleBase(), ModSDK::GetInstance()->GetSizeOfCode(), s_Pattern, p_Mask);

	if (s_Target == 0)
	{
		Logger::Error("Could not find address for global '{}'. This probably means that the game was updated and the SDK requires changes.", p_GlobalName);
		return nullptr;
	}

	uintptr_t s_RelAddrPtr = s_Target + p_Offset;
	int32_t s_RelAddr = *reinterpret_cast<int32_t*>(s_RelAddrPtr);

	uintptr_t s_FinalAddr = s_RelAddrPtr + s_RelAddr + sizeof(int32_t);

	Logger::Debug("Successfully located global '{}' at address {}.", p_GlobalName, fmt::ptr(reinterpret_cast<void*>(s_FinalAddr)));

	return reinterpret_cast<T>(s_FinalAddr);
}

#define PATTERN_RELATIVE_GLOBAL(Pattern, Mask, Offset, GlobalType, GlobalName) \
	GlobalType Globals::GlobalName = PatternGlobalRelative<GlobalType>(#GlobalName, Pattern, Mask, Offset);
