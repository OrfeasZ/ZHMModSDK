#include "Globals.h"
#include "GlobalsImpl.h"

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\x0F\x11\x44\x24\x20\xE8\x00\x00\x00\x00\x48\x89\x5C\x24\x40",
    "xxx????xxxxxx????xxxxx",
    3,
    ZGameLoopManager*, GameLoopManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8B\x1D\x00\x00\x00\x00\x48\x85\xDB\x75\x00\xE8\x00\x00\x00\x00\x48\x8B\x1D\x00\x00\x00\x00\x48\x8D\x15",
    "xxx????xxxx?x????xxx????xxx",
    3,
    ZTypeRegistry**, TypeRegistry
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x89\x05\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x0F\x57\xC9",
    "xxx????xxx????xxx",
    3,
    ZGameTimeManager**, GameTimeManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x89\x05\x00\x00\x00\x00\x0F\x57\xC0\xF3\x0F\x11\x05",
    "xxx????xxxxxxx",
    3,
    ZHitman5Module*, Hitman5Module
);

// Look for ??_7ZGameContext@@6B@
PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xE9\x00\x00\x00\x00\x33\xD2",
    "xxx????x????x????xx",
    3,
    ZGameContext*, GameContext
);

PATTERN_RELATIVE_GLOBAL(
    "\x4C\x8D\x2D\x00\x00\x00\x00\x75",
    "xxx????x",
    3,
    ZActorManager*, ActorManager
);

PATTERN_RELATIVE_GLOBAL(
    "\xFF\x05\x00\x00\x00\x00\x48\x83\xC1",
    "xx????xxx",
    2,
    uint16_t*, NextActorId
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8B\x1D\x00\x00\x00\x00\x44\x0F\xB6\xD8",
    "xxx????xxxx",
    3,
    ZMemoryManager**, MemoryManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x0F\xB6\x57\x0C",
    "xxx????x????xxxx",
    3,
    ZRenderManager*, RenderManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x89\x1D\x00\x00\x00\x00\xC7\x45\xE0",
    "xxx????xxx",
    3,
    ZApplicationEngineWin32**, ApplicationEngineWin32
);

PATTERN_RELATIVE_GLOBAL(
    "\x33\xC0\x48\x8D\x51\x20",
    "xxxxxx",
    12,
    ZGameUIManager*, GameUIManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\x49\x89\x6B\x10\x49\x8D\x53\x08",
    "xxx????xxxxxxxx",
    3,
    ZEntityManager*, EntityManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x40\x53\x48\x83\xEC\x00\x8B\x51\x18\x48\x8B\xD9\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x80\x3D",
    "xxxxx?xxxxxxxxx????x????xx",
    15,
    ZGameStatsManager*, GameStatsManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x40\x57\x48\x83\xEC\x00\x48\x8B\x51\x58",
    "xxxxx?xxxx",
    21,
    ZCameraManager*, CameraManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\x0F\x29\x78\xA8\x4C\x8B\xF2",
    "xxx????xxxxxxx",
    3,
    ZPlayerRegistry*, PlayerRegistry
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xEB\x00\x48\x8B\xC6",
    "xxx????x????x?xxx",
    3,
    ZHM5InputManager*, InputManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\x48\x89\x43\x30",
    "xxx????xxxx",
    3,
    ZResourceManager*, ResourceManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8B\x05\x00\x00\x00\x00\x44\x8B\xFE\x49\xC1\xE7",
    "xxx????xxxxxx",
    3,
    ZResourceContainer**, ResourceContainer
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01\x48\x3B\xC7\x0F\x85\x00\x00\x00\x00\xC7\x81\x08\x03\x00\x00",
    "xxx????xxxxxxxx????xxxxxx",
    3,
    ZCollisionManager**, CollisionManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x39\x75\x9F",
    "xxx????x????xxxx",
    3,
    ZContentKitManager*, ContentKitManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\x48\x83\xC4\x00\x5B\xE9\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\xB9\x91\x01\x00\x00",
    "xxx????xxx?xx???????????xxxxxx",
    3,
    ZHM5ActionManager*, HM5ActionManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x1D\x00\x00\x00\x00\x4C\x8D\x35",
    "xxx????xxx????xxx",
    3,
    ZBehaviorService*, BehaviorService
);

PATTERN_RELATIVE_GLOBAL(
    "\x4C\x8D\x2D\x00\x00\x00\x00\x4C\x89\xB4\x24\xD0\x00\x00\x00\x0F\x29\xB4\x24\xC0\x00\x00\x00",
    "xxx????xxxxxxxxxxxxxxxx",
    3,
    SPrimitiveBufferData*, PrimitiveBufferData
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8B\x0D\x00\x00\x00\x00\xC7\x44\x24\x30\x00\x00\x00\x00\x4C\x89\x64\x24\x38",
    "xxx????xxxx????xxxxx",
    3,
    IGameMode**, GameMode
);

PATTERN_RELATIVE_GLOBAL(
	"\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x74\x00\x48\x8B\x01\x48\x8B\x50\x40",
	"xxx????xxxx?xxxxxxx",
	3,
	IEngineMode**, EngineMode
);

PATTERN_RELATIVE_GLOBAL(
	"\x48\x8D\x05\x00\x00\x00\x00\x4C\x89\x41\x18\x4C\x89\x41\x30",
	"xxx????xxxxxxxx",
	3,
	void*, ZTemplateEntityBlueprintFactory_vtbl
);

PATTERN_RELATIVE_GLOBAL(
	"\x48\x89\x05\x00\x00\x00\x00\x48\x89\x2D\x00\x00\x00\x00\x40\x88\x2D",
	"xxx????xxx????xxx",
	3,
	ZInputActionManager*, InputActionManager
);
