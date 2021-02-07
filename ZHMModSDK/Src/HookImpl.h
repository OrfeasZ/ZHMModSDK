#pragma once

#include <cassert>
#include <unordered_set>
#include <vector>

#include "ModSDK.h"

#include "Hook.h"
#include "MinHook.h"
#include "Util/ProcessUtils.h"
#include "Logging.h"

#define MAX_TRAMPOLINES 2048

// TODO: Suspend all threads but the current one when installing or removing hooks.
// This should fix various crashes when hot-reloading.

class IPluginInterface;

class HookRegistry
{
private:
	static std::unordered_set<HookBase*>* g_Hooks;

public:
	static void RegisterHook(HookBase* p_Hook)
	{
		if (g_Hooks == nullptr)
			g_Hooks = new std::unordered_set<HookBase*>();

		g_Hooks->insert(p_Hook);
	}

	static void RemoveHook(HookBase* p_Hook)
	{
		if (g_Hooks == nullptr)
			return;

		g_Hooks->erase(p_Hook);
	}

	static void ClearPluginDetours(IPluginInterface* p_Plugin)
	{
		if (g_Hooks == nullptr)
			return;

		for (auto s_Hook : *g_Hooks)
			s_Hook->RemoveDetoursWithContext(p_Plugin);
	}
};

template <class T>
class HookImpl;

template <class ReturnType, class... Args>
class HookImpl<ReturnType(Args...)> : public Hook<ReturnType(Args...)>
{
protected:
	HookImpl(const char* p_HookName, void* p_Target, typename Hook<ReturnType(Args...)>::OriginalFunc_t p_Detour) :
		m_Target(p_Target)
	{
		Util::ProcessUtils::SuspendAllThreadsButCurrent();
		
		InitializeSRWLock(&m_Lock);
		
		HookRegistry::RegisterHook(this);
		
		// We push null here because that's what's used by the caller
		// implementation to determine when we've ran out of detours.
		m_Detours.push_back(nullptr);
		
		if (p_Target == nullptr)
		{
			Logger::Error("Could not find address for hook '{}'. This probably means that the game was updated and the SDK requires changes.", p_HookName);
			return;
		}

		// Make sure MinHook is initialized.
		MH_Initialize();
		
		// Install the detour.
		void* s_Original = nullptr;		
		auto s_Result = MH_CreateHook(m_Target, p_Detour, &s_Original);

		if (s_Result != MH_OK)
		{
			Logger::Error("Could not create hook '{}' at address {}. Error code: {}.", p_HookName, fmt::ptr(p_Target), s_Result);
			return;
		}

		s_Result = MH_EnableHook(m_Target);

		if (s_Result != MH_OK)
		{
			Logger::Error("Could install detour for hook '{}' at address {}. Error code: {}.", p_HookName, fmt::ptr(p_Target), s_Result);
			return;
		}

		this->m_OriginalFunc = reinterpret_cast<typename Hook<ReturnType(Args...)>::OriginalFunc_t>(s_Original);

		Logger::Debug("Successfully installed detour for hook '{}' at address {}.", p_HookName, fmt::ptr(p_Target));
	}

	HookImpl(const char* p_HookName, typename Hook<ReturnType(Args...)>::OriginalFunc_t p_Original) :
		m_Target(nullptr)
	{
		Util::ProcessUtils::SuspendAllThreadsButCurrent();

		InitializeSRWLock(&m_Lock);
		
		HookRegistry::RegisterHook(this);

		if (p_Original == nullptr)
		{
			Logger::Error("Could not find address for hook '{}'. This probably means that the game was updated and the SDK requires changes.", p_HookName);
			return;
		}

		this->m_OriginalFunc = reinterpret_cast<typename Hook<ReturnType(Args...)>::OriginalFunc_t>(p_Original);

		Logger::Debug("Successfully installed detour for hook '{}'.", p_HookName);
	}

public:
	~HookImpl() override
	{
		AcquireSRWLockExclusive(&m_Lock);

		m_Detours.clear();
		
		if (m_Target != nullptr)
		{
			MH_DisableHook(m_Target);
			MH_RemoveHook(m_Target);
		}

		ReleaseSRWLockExclusive(&m_Lock);
		
		HookRegistry::RemoveHook(this);
	}

	void RemoveDetoursWithContext(void* p_Context) override
	{
		AcquireSRWLockExclusive(&m_Lock);

		for (auto it = m_Detours.begin(); it != m_Detours.end();)
		{
			if (*it == nullptr)
			{
				++it;
				continue;
			}

			if ((*it)->Context == p_Context)
			{
				delete* it;
				it = m_Detours.erase(it);
			}
			else
			{
				++it;
			}
		}

		ReleaseSRWLockExclusive(&m_Lock);
	}

protected:
	void AddDetourInternal(void* p_Context, void* p_Detour) override
	{
		// TODO: Do we need any sort of locking here?

		// We remove it first to make sure we only have unique detours
		// in our list. We could use a set to make this easier but iteration
		// performance wouldn't be very great.		
		RemoveDetourInternal(p_Detour);

		auto s_Detour = new HookBase::Detour();
		s_Detour->DetourFunc = p_Detour;
		s_Detour->Context = p_Context;

		AcquireSRWLockExclusive(&m_Lock);

		m_Detours.insert(m_Detours.end() - 1, s_Detour);

		ReleaseSRWLockExclusive(&m_Lock);
	}

	void RemoveDetourInternal(void* p_Detour) override
	{
		AcquireSRWLockExclusive(&m_Lock);

		for (auto it = m_Detours.begin(); it != m_Detours.end();)
		{
			if (*it == nullptr)
			{
				++it;
				continue;
			}

			if ((*it)->DetourFunc == p_Detour)
			{
				delete* it;
				it = m_Detours.erase(it);
			}
			else
			{
				++it;
			}
		}

		ReleaseSRWLockExclusive(&m_Lock);
	}

	HookBase::Detour** GetDetours() override
	{
		return m_Detours.data();
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
	std::vector<HookBase::Detour*> m_Detours;
	void* m_Target;
	SRWLOCK m_Lock;
};

template <class T>
class PatternHook;

template <class ReturnType, class... Args>
class PatternHook<ReturnType(Args...)> final : public HookImpl<ReturnType(Args...)>
{
public:
	PatternHook(const char* p_HookName, const char* p_Pattern, const char* p_Mask, typename Hook<ReturnType(Args...)>::OriginalFunc_t p_Detour) :
		HookImpl<ReturnType(Args...)>(p_HookName, GetTarget(p_Pattern, p_Mask), p_Detour)
	{		
	}

private:
	void* GetTarget(const char* p_Pattern, const char* p_Mask) const
	{
		const auto* s_Pattern = reinterpret_cast<const uint8_t*>(p_Pattern);
		return reinterpret_cast<void*>(Util::ProcessUtils::SearchPattern(ModSDK::GetInstance()->GetModuleBase(), ModSDK::GetInstance()->GetSizeOfCode(), s_Pattern, p_Mask));
	}
};

#pragma pack(push, 1)
struct DetourTrampoline
{
	DetourTrampoline(uintptr_t p_Address) :
		FunctionAddress(p_Address)
	{
		// 0x48 0xB8 => movabs rax 
		Mov = 0xB848;

		// 0xFF 0xE0 => jmp rax
		Jmp = 0xE0FF;

		Pad = 0xCCCCCCCC;
	}
	
	uint16_t Mov;
	uintptr_t FunctionAddress;
	uint16_t Jmp;
	uint32_t Pad;
};
#pragma pack(pop)

class Trampolines
{
private:
	static DetourTrampoline* g_Trampolines;
	static size_t g_TrampolineCount;

public:
	static uintptr_t CreateTrampoline(uintptr_t p_TargetAddress)
	{
		if (g_Trampolines == nullptr)
		{
			// Get system information. We use this later to find the first usable free memory region.
			SYSTEM_INFO s_SysInfo{};
			GetSystemInfo(&s_SysInfo);

			auto s_AllocAddress = ALIGN_TO(reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)) + ModSDK::GetInstance()->GetImageSize(), s_SysInfo.dwAllocationGranularity);
			uintptr_t s_AllocEnd = s_AllocAddress + sizeof(DetourTrampoline) * MAX_TRAMPOLINES;

			// Iterate through memory regions until we find one that's free and fits our data.
			MEMORY_BASIC_INFORMATION s_MemInfo{};
			auto s_QueryResult = VirtualQuery(reinterpret_cast<void*>(s_AllocAddress), &s_MemInfo, sizeof(s_MemInfo));

			while (s_QueryResult != 0 && (s_MemInfo.State != MEM_FREE || reinterpret_cast<uintptr_t>(s_MemInfo.BaseAddress) + s_MemInfo.RegionSize < s_AllocEnd))
			{
				s_AllocAddress = ALIGN_TO(reinterpret_cast<uintptr_t>(s_MemInfo.BaseAddress) + s_MemInfo.RegionSize, s_SysInfo.dwAllocationGranularity);
				s_AllocEnd = s_AllocAddress + sizeof(DetourTrampoline) * MAX_TRAMPOLINES;

				Logger::Trace("Memory segment at {} is not suitable. Trying next at {}.", fmt::ptr(s_MemInfo.BaseAddress), fmt::ptr(reinterpret_cast<void*>(s_AllocAddress)));

				s_QueryResult = VirtualQuery(reinterpret_cast<void*>(s_AllocAddress), &s_MemInfo, sizeof(s_MemInfo));
			}

			if (s_QueryResult == 0)
			{
				// We didn't find a free memory region. Just allocate wherever and pray for the best.
				Logger::Warn("Could not find a free memory region for trampoline storage. Allocating anywhere and praying.");
				s_AllocAddress = 0;				
			}

			Logger::Trace("Attempting to allocate trampoline storage at {}.", fmt::ptr(reinterpret_cast<void*>(s_AllocAddress)));
			
			g_Trampolines = static_cast<DetourTrampoline*>(VirtualAlloc(reinterpret_cast<void*>(s_AllocAddress), sizeof(DetourTrampoline) * MAX_TRAMPOLINES, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

			if (g_Trampolines == nullptr)
			{
				Logger::Error("Trampoline storage allocation failed with error {}. Requested address was {}.", GetLastError(), fmt::ptr(reinterpret_cast<void*>(s_AllocAddress)));
				return 0;
			}

			g_TrampolineCount = 0;
			
			Logger::Trace("Allocated trampoline storage at {} (requested address: {}).", fmt::ptr(g_Trampolines), fmt::ptr(reinterpret_cast<void*>(s_AllocAddress)));
		}

		if (g_TrampolineCount >= MAX_TRAMPOLINES)
		{
			Logger::Error("Could not create trampoline because we have reached the max number of trampolines allowed ({}). You can raise this limit by modifying MAX_TRAMPOLINES in HookImpl.h.", MAX_TRAMPOLINES);
			return 0;
		}
		
		auto* s_Trampoline = g_Trampolines + g_TrampolineCount;
		*s_Trampoline = DetourTrampoline(p_TargetAddress);

		++g_TrampolineCount;

		return reinterpret_cast<uintptr_t>(s_Trampoline);
	}

	static void ClearTrampolines()
	{
		if (g_Trampolines == nullptr)
			return;

		VirtualFree(g_Trampolines, 0, MEM_RELEASE);
	}
};

template <class T>
class PatternCallHook;

template <class ReturnType, class... Args>
class PatternCallHook<ReturnType(Args...)> final : public HookImpl<ReturnType(Args...)>
{
public:
	PatternCallHook(const char* p_HookName, const char* p_Pattern, const char* p_Mask, typename Hook<ReturnType(Args...)>::OriginalFunc_t p_Detour) :
		HookImpl<ReturnType(Args...)>(p_HookName, InstallDetourAndGetOriginal(p_HookName, p_Pattern, p_Mask, p_Detour))
	{		
	}

	~PatternCallHook() override
	{
		// Restore the original call.
		const ptrdiff_t s_Distance = reinterpret_cast<uintptr_t>(this->m_OriginalFunc) - (m_Target + 5);

		// We don't need to check if this is within INT32 bounds here because it should always be.
		// Cast down to int and rewrite the call offset.
		DWORD s_OldProtect;
		VirtualProtect(reinterpret_cast<void*>(m_Target), 5, PAGE_EXECUTE_READWRITE, &s_OldProtect);

		*reinterpret_cast<int32_t*>(m_Target + 1) = static_cast<int32_t>(s_Distance);

		VirtualProtect(reinterpret_cast<void*>(m_Target), 5, s_OldProtect, nullptr);
	}

private:
	typename Hook<ReturnType(Args...)>::OriginalFunc_t InstallDetourAndGetOriginal(const char* p_HookName, const char* p_Pattern, const char* p_Mask, typename Hook<ReturnType(Args...)>::OriginalFunc_t p_Detour)
	{
		Util::ProcessUtils::SuspendAllThreadsButCurrent();

		const auto* s_Pattern = reinterpret_cast<const uint8_t*>(p_Pattern);
		m_Target = Util::ProcessUtils::SearchPattern(ModSDK::GetInstance()->GetModuleBase(), ModSDK::GetInstance()->GetSizeOfCode(), s_Pattern, p_Mask);

		// We expect this to be a CALL (0xE8) instruction.
		if (m_Target != 0 && *reinterpret_cast<uint8_t*>(m_Target) != 0xE8)
		{
			Logger::Error("Expected a call instruction for hook '{}' at address {} but instead got 0x{:02X}.", p_HookName, fmt::ptr(reinterpret_cast<void*>(m_Target)), *reinterpret_cast<uint8_t*>(m_Target));
			return nullptr;
		}

		if (m_Target == 0)
			return nullptr;

		// Real function location will be at instruction + 5 + offset.
		// Offset is a 32-bit signed integer at instruction + 1.
		const uintptr_t s_OriginalFunction = m_Target + 5 + *reinterpret_cast<int32_t*>(m_Target + 1);

		// Get the distance between this call and our detour.
		ptrdiff_t s_Distance = reinterpret_cast<uintptr_t>(p_Detour) - (m_Target + 5);

		// If the distance is out of the int32 range we must create a trampoline.
		if (s_Distance >= INT32_MAX || s_Distance <= INT32_MIN)
		{
			Logger::Trace("Detour for hook '{}' is too far from the original call ({} - {} = {}). Creating trampoline.", p_HookName, fmt::ptr(p_Detour), fmt::ptr(reinterpret_cast<void*>(m_Target + 5)), s_Distance);

			const auto s_TrampolineAddress = Trampolines::CreateTrampoline(reinterpret_cast<uintptr_t>(p_Detour));

			// Failed to create trampoline.
			if (s_TrampolineAddress == 0)
				return nullptr;

			s_Distance = s_TrampolineAddress - (m_Target + 5);

			// Sanity check again.
			if (s_Distance >= INT32_MAX || s_Distance <= INT32_MIN)
			{
				Logger::Error("Trampoline for hook '{}' is too far from the original call ({} - {} = {}).", p_HookName, fmt::ptr(reinterpret_cast<void*>(s_TrampolineAddress)), fmt::ptr(reinterpret_cast<void*>(m_Target + 5)), s_Distance);
				return nullptr;
			}
		}

		// Cast down to int and rewrite the call offset.
		DWORD s_OldProtect;
		VirtualProtect(reinterpret_cast<void*>(m_Target), 5, PAGE_EXECUTE_READWRITE, &s_OldProtect);
		
		*reinterpret_cast<int32_t*>(m_Target + 1) = static_cast<int32_t>(s_Distance);
		
		VirtualProtect(reinterpret_cast<void*>(m_Target), 5, s_OldProtect, nullptr);
		
		return reinterpret_cast<typename Hook<ReturnType(Args...)>::OriginalFunc_t>(s_OriginalFunction);
	}

	uintptr_t m_Target;
};

template <class T>
class PatternRelativeCallHook;

template <class ReturnType, class... Args>
class PatternRelativeCallHook<ReturnType(Args...)> final : public HookImpl<ReturnType(Args...)>
{
public:
	PatternRelativeCallHook(const char* p_HookName, const char* p_Pattern, const char* p_Mask, typename Hook<ReturnType(Args...)>::OriginalFunc_t p_Detour) :
		HookImpl<ReturnType(Args...)>(p_HookName, GetTarget(p_HookName, p_Pattern, p_Mask), p_Detour)
	{
	}

private:
	void* GetTarget(const char* p_HookName, const char* p_Pattern, const char* p_Mask) const
	{
		const auto* s_Pattern = reinterpret_cast<const uint8_t*>(p_Pattern);
		auto s_Target = Util::ProcessUtils::SearchPattern(ModSDK::GetInstance()->GetModuleBase(), ModSDK::GetInstance()->GetSizeOfCode(), s_Pattern, p_Mask);

		// We expect this to be a CALL (0xE8) instruction.
		if (s_Target != 0 && *reinterpret_cast<uint8_t*>(s_Target) != 0xE8)
		{
			Logger::Error("Expected a call instruction for hook '{}' at address {} but instead got 0x{:02X}.", p_HookName, fmt::ptr(reinterpret_cast<void*>(s_Target)), *reinterpret_cast<uint8_t*>(s_Target));
			return nullptr;
		}

		if (s_Target == 0)
			return nullptr;
		
		const uintptr_t s_OriginalFunction = s_Target + 5 + *reinterpret_cast<int32_t*>(s_Target + 1);
		return reinterpret_cast<void*>(s_OriginalFunction);
	}
};


#define PATTERN_HOOK(Pattern, Mask, HookName, HookType) \
	Hook<HookType>* Hooks::HookName = new PatternHook<HookType>(\
		#HookName, \
		Pattern, Mask, \
		(typename Hook<HookType>::OriginalFunc_t) []<class... Args>(Args... p_Args) { return Hooks::HookName->Call(p_Args...); }\
	);\
	\
	static ScopedDestructible g_ ## HookName ## _Destructible = ScopedDestructible(reinterpret_cast<IDestructible**>(&Hooks::HookName));

#define PATTERN_CALL_HOOK(Pattern, Mask, HookName, HookType) \
	Hook<HookType>* Hooks::HookName = new PatternCallHook<HookType>(\
		#HookName, \
		Pattern, Mask, \
		(typename Hook<HookType>::OriginalFunc_t) []<class... Args>(Args... p_Args) { return Hooks::HookName->Call(p_Args...); }\
	);\
	\
	static ScopedDestructible g_ ## HookName ## _Destructible = ScopedDestructible(reinterpret_cast<IDestructible**>(&Hooks::HookName));

#define PATTERN_RELATIVE_CALL_HOOK(Pattern, Mask, HookName, HookType) \
	Hook<HookType>* Hooks::HookName = new PatternRelativeCallHook<HookType>(\
		#HookName, \
		Pattern, Mask, \
		(typename Hook<HookType>::OriginalFunc_t) []<class... Args>(Args... p_Args) { return Hooks::HookName->Call(p_Args...); }\
	);\
	\
	static ScopedDestructible g_ ## HookName ## _Destructible = ScopedDestructible(reinterpret_cast<IDestructible**>(&Hooks::HookName));