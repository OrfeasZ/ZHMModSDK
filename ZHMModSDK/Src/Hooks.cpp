#include "Hooks.h"
#include "HookImpl.h"

#include <Glacier/ZEntity.h>

std::unordered_set<HookBase*>* HookRegistry::g_Hooks = nullptr;

DetourTrampoline* Trampolines::g_Trampolines = nullptr;
size_t Trampolines::g_TrampolineCount = 0;

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x00\x48\x8B\xD9\xE8\x00\x00\x00\x00\x33\xED\x48\x8D\x05\x00\x00\x00\x00\x48\x89\xAB\x08\x03\x00\x00",
	"xxxxxxxxxxxxxxxxxxx?xxxx????xxxxx????xxxxxxx",
	ZActor_ZActor, void(ZActor* th, ZComponentCreateInfo* createInfo)
);

PATTERN_HOOK(
	"\x40\x55\x56\x57\x41\x56\x48\x8D\x6C\x24\xC1\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xFA",
	"xxxxxxxxxxxxxx????xxx",
	ZEntitySceneContext_LoadScene, void(ZEntitySceneContext* th, ZSceneData& sceneData)
);

PATTERN_RELATIVE_CALL_HOOK(
	"\xE8\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x5C\x24\x48\x48\x89\x44\x24\x40",
	"x????xxx????xxxxxxxxxx",
	ZGameLoopManager_RegisterFrameUpdate, void(ZGameLoopManager* th, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)
);

PATTERN_RELATIVE_CALL_HOOK(
	"\xE8\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x7C\x24\x38\x48\x89\x44\x24\x30\x44\x8D\x47\x41",
	"x????xxx????xxxxxxxxxxxxxx",
	ZGameLoopManager_UnregisterFrameUpdate, void(ZGameLoopManager* th, const ZDelegate<void(const SGameUpdateEvent&)>&, int, EUpdateMode)
);

PATTERN_HOOK(
	"\x40\x55\x56\x41\x54\x41\x56\x41\x57\x48\x8D\x6C\x24\xD0",
	"xxxxxxxxxxxxxx",
	Engine_Init, bool(void*, void*)
);

PATTERN_HOOK(
	"\x40\x55\x57\x41\x56\x48\x83\xEC\x00\x4D\x8B\xF0",
	"xxxxxxxx?xxx",
	ZKnowledge_SetGameTension, void(ZKnowledge*, EGameTension)
);

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x00\x48\x8B\xD9\x49\x8B\xF9\x48\x8B\x89\x10\x07\x01\x00",
	"xxxxxxxxxxxxxxxxxxx?xxxxxxxxxxxxx",
	ZActorManager_SetHitmanSharedEvent, void(ZActorManager*, EAISharedEventType, bool)
);

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x00\x0F\xB6\xFA\x48\x8B\xD9\xE8\x00\x00\x00\x00\x48\x8B\xC8",
	"xxxxxxxxx?xxxxxxx????xxx",
	GetApplicationOptionBool, bool(const ZString&, bool)
);

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x18\x48\x89\x74\x24\x20\x57\x48\x83\xEC\x00\x33\xF6\x0F\xB6\xEA",
	"xxxxxxxxxxxxxxxxxxx?xxxxx",
	ZApplicationEngineWin32_OnMainWindowActivated, void(ZApplicationEngineWin32*, bool)
);

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x48\x89\x7C\x24\x20\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C\x24\xD1",
	"xxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	ZApplicationEngineWin32_MainWindowProc, LRESULT(ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM)
);

PATTERN_HOOK(
	"\x40\x55\x56\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8B\x29",
	"xxxxxxxxxxxx?xxx",
	SetPropertyValue, bool(ZEntityRef, uint32_t, const ZObjectRef&, bool)
);

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x10\x57\x48\x83\xEC\x00\x49\x8B\xF8\x48\xC7\x44\x24\x30",
	"xxxxxxxxx?xxxxxxxx",
	GetPropertyValue, bool(ZEntityRef, uint32_t, void*)
);

PATTERN_HOOK(
	"\x48\x89\x6C\x24\x20\x56\x57\x41\x56\x48\x83\xEC\x00\x48\x8B\x31",
	"xxxxxxxxxxxx?xxx",
	SignalOutputPin, bool(ZEntityRef, uint32_t, const ZObjectRef&)
);

PATTERN_HOOK(
	"\x48\x89\x6C\x24\x20\x56\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8B\x29",
	"xxxxxxxxxxxxx?xxx",
	SignalInputPin, bool(ZEntityRef, uint32_t, const ZObjectRef&)
);

PATTERN_HOOK(
	"\x48\x89\x4C\x24\x08\x55\x53\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x38\xE9\xFF\xFF",
	"xxxxxxxxxxxxxxxxxxxxxxxxx",
	ZRenderDevice_ZRenderDevice, ZRenderDevice* (ZRenderDevice* th)
);

MODULE_HOOK(
	"d3d12.dll", "D3D12CreateDevice",
	D3D12CreateDevice, HRESULT(IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice)
);

PATTERN_HOOK(
	"\x44\x88\x44\x24\x18\x57",
	"xxxxxx",
	ZRenderSwapChain_Resize, void(ZRenderSwapChain* th, void* a2, bool a3)
);

PATTERN_HOOK(
	"\x48\x89\x54\x24\x10\x55\x53\x57\x48\x8D\xAC\x24\x40\xFF\xFF\xFF",
	"xxxxxxxxxxxxxxxx",
	Check_SSL_Cert, bool(void*, void*)
);

PATTERN_HOOK(
	"\x40\x53\x48\x83\xEC\x00\x41\xF7\x00",
	"xxxxx?xxx",
	ZApplicationEngineWin32_OnDebugInfo, void(ZApplicationEngineWin32* th, const ZString& info, const ZString& details)
);

PATTERN_HOOK(
	"\x40\x53\x41\x55\x48\x83\xEC\x00\x48\x8B\xD9",
	"xxxxxxx?xxx",
	ZKeyboardWindows_Update, void(ZKeyboardWindows* th, bool a2)
);

PATTERN_CALL_HOOK(
	"\xE8\x00\x00\x00\x00\x41\x0F\xB6\x85\xA0\x00\x00\x00\x88\x45\x81",
	"x????xxxxxxxxxxx",
	ZRenderContext_Unknown01, void(ZRenderContext* th)
);

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x00\x89\x11",
	"xxxxxxxxxxxxxxxxxxx?xx",
	ZPackageManagerPackage_ZPackageManagerPackage, void*(ZPackageManagerPackage* th, void* a2, const ZString& a3, int a4, int patchLevel)
);

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x10\x57\x48\x83\xEC\x00\x48\x8B\xF9\x48\x8B\xDA\x48\x8B\x0D",
	"xxxxxxxxx?xxxxxxxxx",
	ZGameLoopManager_RequestPause, void(ZGameLoopManager* th, const ZString& a2)
);

PATTERN_HOOK(
	"\x48\x89\x6C\x24\x18\x57\x48\x83\xEC\x00\x48\x8D\xB9\x20\x01\x00\x00",
	"xxxxxxxxx?xxxxxxx",
	ZGameLoopManager_ReleasePause, void(ZGameLoopManager* th, const ZString& a2)
);

PATTERN_HOOK(
	"\x48\x89\x7C\x24\x20\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8D\x79\x18",
	"xxxxxxxxxxxxxx?xxxx",
	ZGameUIManagerEntity_TryOpenMenu, bool(ZGameUIManagerEntity* th, EGameUIMenu menu, bool force)
);

PATTERN_HOOK(
	"\x40\x53\x48\x83\xEC\x00\xF3\x48\x0F\x2C\x05\x00\x00\x00\x00\x48\x8B\xD9",
	"xxxxx?xxxxx????xxx",
	ZGameStatsManager_SendAISignals, void(ZGameStatsManager* th)
);

PATTERN_HOOK(
	"\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x55\x41\x56\x41\x57\x48\x8D\x6C\x24\xB9\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xFF",
	"xxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxx",
	ZAchievementManagerSimple_OnEventReceived, void(ZAchievementManagerSimple* th, const SOnlineEvent& event)
);

PATTERN_HOOK(
	"\x40\x55\x53\x56\x57\x41\x54\x48\x8D\x6C\x24\xC9\x48\x81\xEC\x00\x00\x00\x00\x33\xFF",
	"xxxxxxxxxxxxxxx????xx",
	ZAchievementManagerSimple_OnEventSent, void(ZAchievementManagerSimple* th, uint32_t eventIndex, const ZDynamicObject& event)
);
