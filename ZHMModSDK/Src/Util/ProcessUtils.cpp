#include "ProcessUtils.h"

#include <TlHelp32.h>
#include <Windows.h>
#include <unordered_set>

#include "Logging.h"

using namespace Util;

uintptr_t ProcessUtils::SearchPattern(uintptr_t p_BaseAddress, size_t p_ScanSize, const uint8_t* p_Pattern, const char* p_Mask)
{
	for (uintptr_t s_SearchAddr = p_BaseAddress; s_SearchAddr < (p_BaseAddress + p_ScanSize); ++s_SearchAddr)
	{
		const uint8_t* s_MemoryPtr = reinterpret_cast<uint8_t*>(s_SearchAddr);

		if (s_MemoryPtr[0] != p_Pattern[0])
			continue;

		const uint8_t* s_PatternPtr = p_Pattern;
		const uint8_t* s_MaskPtr = reinterpret_cast<const uint8_t*>(p_Mask);

		bool s_Found = true;

		for (; s_MaskPtr[0] && (reinterpret_cast<uintptr_t>(s_MemoryPtr) < (p_BaseAddress + p_ScanSize)); ++s_MaskPtr, ++s_PatternPtr, ++s_MemoryPtr)
		{
			if (s_MaskPtr[0] != 'x')
				continue;

			if (s_MemoryPtr[0] != s_PatternPtr[0])
			{
				s_Found = false;
				break;
			}
		}

		if (s_Found)
			return s_SearchAddr;
	}

	return 0;
}

uint32_t ProcessUtils::GetSizeOfCode(HMODULE p_Module)
{
	PIMAGE_DOS_HEADER s_DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(p_Module);
	PIMAGE_NT_HEADERS s_NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(p_Module) + s_DOSHeader->e_lfanew);

	if (!s_NTHeader)
		return 0;

	return static_cast<uint32_t>(s_NTHeader->OptionalHeader.SizeOfCode);
}

uintptr_t ProcessUtils::GetBaseOfCode(HMODULE p_Module)
{
	PIMAGE_DOS_HEADER s_DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(p_Module);
	PIMAGE_NT_HEADERS s_NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(p_Module) + s_DOSHeader->e_lfanew);

	if (!s_NTHeader)
		return 0;

	return static_cast<uintptr_t>(s_NTHeader->OptionalHeader.BaseOfCode);
}

uint32_t ProcessUtils::GetSizeOfImage(HMODULE p_Module)
{
	PIMAGE_DOS_HEADER s_DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(p_Module);
	PIMAGE_NT_HEADERS s_NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(p_Module) + s_DOSHeader->e_lfanew);

	if (!s_NTHeader)
		return 0;

	return static_cast<uint32_t>(s_NTHeader->OptionalHeader.SizeOfImage);
}

std::tuple<uintptr_t, uintptr_t> ProcessUtils::GetSectionStartAndEnd(HMODULE p_Module, const std::string& p_SectionName)
{
	PIMAGE_DOS_HEADER s_DOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(p_Module);
	PIMAGE_NT_HEADERS s_NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(p_Module) + s_DOSHeader->e_lfanew);

	if (!s_NTHeader)
		return std::make_tuple<uintptr_t, uintptr_t>(0, 0);

	PIMAGE_SECTION_HEADER s_Section = IMAGE_FIRST_SECTION(s_NTHeader);

	for (int i = 0; i < s_NTHeader->FileHeader.NumberOfSections; ++i)
	{
		if (strcmp(reinterpret_cast<const char*>(s_Section->Name), p_SectionName.c_str()) == 0)
		{
			uintptr_t s_RDataSectionStart = s_Section->VirtualAddress;
			s_RDataSectionStart += reinterpret_cast<uintptr_t>(p_Module);

			uintptr_t s_RDataSectionEnd = s_RDataSectionStart + s_Section->SizeOfRawData;

			return std::make_tuple(s_RDataSectionStart, s_RDataSectionEnd);
		}

		++s_Section;
	}

	return std::make_tuple<uintptr_t, uintptr_t>(0, 0);
}

uintptr_t ProcessUtils::GetRelativeAddr(uintptr_t p_Base, int32_t p_Offset)
{
	uintptr_t s_RelAddrPtr = p_Base + p_Offset;
	int32_t s_RelAddr = *reinterpret_cast<int32_t*>(s_RelAddrPtr);

	return s_RelAddrPtr + s_RelAddr + sizeof(int32_t);
}

static std::unordered_set<HANDLE>* g_SuspendedThreads = nullptr;

void ProcessUtils::SuspendAllThreadsButCurrent()
{
	if (g_SuspendedThreads != nullptr)
		return;
	
	g_SuspendedThreads = new std::unordered_set<HANDLE>();
	
	HANDLE s_Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	if (s_Snapshot == INVALID_HANDLE_VALUE)
		return;

	auto s_CurrentThread = GetCurrentThreadId();
	
	THREADENTRY32 s_ThreadEntry{};
	s_ThreadEntry.dwSize = sizeof(s_ThreadEntry);
	
	if (Thread32First(s_Snapshot, &s_ThreadEntry))
	{
		do 
		{
			if (s_ThreadEntry.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(s_ThreadEntry.th32OwnerProcessID) && 
				s_ThreadEntry.th32ThreadID != s_CurrentThread && 
				s_ThreadEntry.th32OwnerProcessID == GetCurrentProcessId())
			{
				HANDLE s_Thread = OpenThread(THREAD_ALL_ACCESS, false, s_ThreadEntry.th32ThreadID);

				if (s_Thread != nullptr)
				{
					Logger::Trace("Adding thread {} to the list of threads to suspend.", s_ThreadEntry.th32ThreadID);
					g_SuspendedThreads->insert(s_Thread);
				}
				else
				{
					Logger::Trace("Could not open thread {} to suspend. Error: {}.", s_ThreadEntry.th32ThreadID, GetLastError());
				}
			}
			
			s_ThreadEntry.dwSize = sizeof(s_ThreadEntry);
		}
		while (Thread32Next(s_Snapshot, &s_ThreadEntry));
	}
	
	CloseHandle(s_Snapshot);

	Logger::Trace("Suspending {} threads.", g_SuspendedThreads->size());

	for (auto* s_Thread : *g_SuspendedThreads)
		SuspendThread(s_Thread);
}

void ProcessUtils::ResumeSuspendedThreads()
{
	if (g_SuspendedThreads == nullptr)
		return;

	Logger::Trace("Resuming {} suspended threads.", g_SuspendedThreads->size());

	for (auto* s_Thread : *g_SuspendedThreads)
	{
		ResumeThread(s_Thread);
		CloseHandle(s_Thread);
	}

	delete g_SuspendedThreads;
	g_SuspendedThreads = nullptr;
}