#include "Hooks.h"
#include "HookImpl.h"

DetourTrampoline* Trampolines::g_Trampolines = nullptr;
size_t Trampolines::g_TrampolineCount = 0;

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x00\x48\x8B\xD9\xE8\x00\x00\x00\x00\x33\xED\x48\x8D\x05\x00\x00\x00\x00\x48\x89\xAB\x08\x03\x00\x00",
	"xxxxxxxxxxxxxxxxxxx?xxxx????xxxxx????xxxxxxx",
	void, ZActor_ZActor, ZActor*, ZComponentCreateInfo*
);

PATTERN_CALL_HOOK(
	"\xE8\x00\x00\x00\x00\x48\x8B\x4C\x24\x70\x8B\xF8",
	"x????xxxxxxx",
	void, Test
);
