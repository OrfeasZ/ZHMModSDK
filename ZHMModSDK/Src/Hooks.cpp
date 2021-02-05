#include "Hooks.h"
#include "HookImpl.h"

std::unordered_set<HookBase*>* HookRegistry::g_Hooks = nullptr;

DetourTrampoline* Trampolines::g_Trampolines = nullptr;
size_t Trampolines::g_TrampolineCount = 0;

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x00\x48\x8B\xD9\xE8\x00\x00\x00\x00\x33\xED\x48\x8D\x05\x00\x00\x00\x00\x48\x89\xAB\x08\x03\x00\x00",
	"xxxxxxxxxxxxxxxxxxx?xxxx????xxxxx????xxxxxxx",
	ZActor_ZActor, void(ZActor* th, ZComponentCreateInfo* createInfo)
);

PATTERN_HOOK(
	"\x40\x55\x56\x57\x41\x56\x48\x8D\x6C\x24\xC1",
	"xxxxxxxxxxx",
	ZEntitySceneContext_LoadScene, void(ZEntitySceneContext* th, const ZSceneData& sceneData)
);

PATTERN_RELATIVE_CALL_HOOK(
	"\xE8\x00\x00\x00\x00\x48\x8B\x5C\x24\x58\x48\x8B\xC7",
	"x????xxxxxxxx",
	ZGameLoopManager_RegisterFrameUpdate, void(ZGameLoopManager* th, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)
);
