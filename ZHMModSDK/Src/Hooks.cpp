#include "Hooks.h"
#include "HookImpl.h"

std::unordered_set<HookBase*>* HookRegistry::g_Hooks = nullptr;

DetourTrampoline* Trampolines::g_Trampolines = nullptr;
size_t Trampolines::g_TrampolineCount = 0;

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x00\x48\x8B\xD9\xE8\x00\x00\x00\x00\x33\xED\x48\x8D\x05\x00\x00\x00\x00\x48\x89\xAB\x08\x03\x00\x00",
	"xxxxxxxxxxxxxxxxxxx?xxxx????xxxxx????xxxxxxx",
	ZActor_ZActor, void(ZActor* th, ZComponentCreateInfo* createInfo)
)

PATTERN_HOOK(
	"\x40\x55\x56\x57\x41\x56\x48\x8D\x6C\x24\xC1",
	"xxxxxxxxxxx",
	ZEntitySceneContext_LoadScene, void(ZEntitySceneContext* th, ZSceneData& sceneData)
)

PATTERN_RELATIVE_CALL_HOOK(
	"\xE8\x00\x00\x00\x00\x48\x8B\x5C\x24\x58\x48\x8B\xC7",
	"x????xxxxxxxx",
	ZGameLoopManager_RegisterFrameUpdate, void(ZGameLoopManager* th, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)
)

PATTERN_RELATIVE_CALL_HOOK(
	"\xE8\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x89\x5D\xB8",
	"x????xxx????xxx",
	ZGameLoopManager_UnregisterFrameUpdate, void(ZGameLoopManager* th, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)
)

PATTERN_HOOK(
	"\x40\x55\x56\x41\x55",
	"xxxxx",
	Engine_Init, bool(void*, void*)
)

PATTERN_HOOK(
	"\x40\x55\x57\x41\x56\x48\x83\xEC\x00\x4D\x8B\xF0",
	"xxxxxxxx?xxx",
	ZKnowledge_SetGameTension, void(ZKnowledge*, EGameTension)
)

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x00\x48\x8B\xD9\x49\x8B\xF9",
	"xxxxxxxxxxxxxxxxxxx?xxxxxx",
	ZActorManager_SetHitmanSharedEvent, void(ZActorManager*, EAISharedEventType, bool)
)

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x00\x0F\xB6\xFA",
	"xxxxxxxxx?xxx",
	GetApplicationOptionBool, bool(const ZString&, bool)
)

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x18\x48\x89\x74\x24\x20\x57\x48\x83\xEC",
	"xxxxxxxxxxxxxxxxxxx",
	ZApplicationEngineWin32_OnMainWindowActivated, void(ZApplicationEngineWin32*, bool)
)

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x48\x89\x7C\x24\x20",
	"xxxxxxxxxxxxxxx",
	ZApplicationEngineWin32_MainWindowProc, LRESULT(ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM)
)

PATTERN_HOOK(
	"\x40\x55\x56\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8B\x29",
	"xxxxxxxxxxxx?xxx",
	SetPropertyValue, bool(ZEntityRef, uint32_t, const ZObjectRef&, bool)
)

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x10\x57\x48\x83\xEC\x00\x49\x8B\xF8\x48\xC7\x44\x24\x30",
	"xxxxxxxxx?xxxxxxxx",
	GetPropertyValue, bool(ZEntityRef, uint32_t, void*)
)

PATTERN_HOOK(
	"\x48\x89\x6C\x24\x20\x56\x57\x41\x56\x48\x83\xEC\x00\x48\x8B\x31",
	"xxxxxxxxxxxx?xxx",
	SignalOutputPin, bool(ZEntityRef, uint32_t, const ZObjectRef&)
)

PATTERN_HOOK(
	"\x48\x89\x6C\x24\x20\x56\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8B\x29",
	"xxxxxxxxxxxxx?xxx",
	SignalInputPin, bool(ZEntityRef, uint32_t, const ZObjectRef&)
)