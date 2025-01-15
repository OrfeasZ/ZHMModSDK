#include "Globals.h"
#include "GlobalsImpl.h"

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\x4C\x89\x74\x24\x48\xE8",
    "xxx????xxxxxx",
    3,
    ZGameLoopManager*, GameLoopManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x4C\x39\x25\x00\x00\x00\x00\x75\x00\xE8\x00\x00\x00\x00\x48\x8B\x57\x50",
    "xxx????x?x????xxxx",
    3,
    ZTypeRegistry**, TypeRegistry
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x89\x05\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x0F\x57\xC9",
    "xxx????xxx????xxx",
    3,
    ZGameTimeManager*, GameTimeManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x89\x05\x00\x00\x00\x00\xF3\x0F\x11\x05\x00\x00\x00\x00\x48\x89\x0D",
    "xxx????xxxx????xxx",
    3,
    ZHitman5Module*, Hitman5Module
);

// Look for ??_7ZGameContext@@6B@
PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\x48\x89\x7C\x24\x78\xE8",
    "xxx????xxxxxx",
    3,
    ZGameContext*, GameContext
);

PATTERN_RELATIVE_GLOBAL(
    "\x4C\x8D\x15\x00\x00\x00\x00\x48\x89\xB4\x24\x40\x06\x00\x00",
    "xxx????xxxxxxxx",
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
    "\x48\x89\x1D\x00\x00\x00\x00\x48\x83\xC4\x00\x5B\xC3\x48\x8D\x0D",
    "xxx????xxx?xxxxx",
    3,
    ZMemoryManager**, MemoryManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x41\x80\xBE\x89\x00\x00\x00",
    "xxx????x????xxxxxxx",
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
    "\x48\x8D\x0D\x00\x00\x00\x00\xF3\x0F\x59\xC6",
    "xxx????xxxx",
    3,
    ZGameUIManager*, GameUIManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\x4D\x8B\xCE\x4C\x89\x44\x24\x20",
    "xxx????xxxxxxxx",
    3,
    ZEntityManager*, EntityManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x4D\xDF",
    "xxx????x????xxxx",
    3,
    ZGameStatsManager*, GameStatsManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x89\x46\x18",
    "xxx????x????xxxx",
    3,
    ZCameraManager*, CameraManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\x49\x89\x73\xE8\xE8\x00\x00\x00\x00\x48\x8B\x0D",
    "xxx????xxxxx????xxx",
    3,
    ZPlayerRegistry*, PlayerRegistry
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8B\xC8",
    "xxx????x????xxxx?xxx",
    3,
    ZHM5InputManager*, InputManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\xFF\x90\x98\x01\x00\x00",
    "xxx????xxxxxx",
    3,
    ZResourceManager*, ResourceManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8B\x05\x00\x00\x00\x00\x41\x8B\xC8\x48\xC1\xE1",
    "xxx????xxxxxx",
    3,
    ZResourceContainer**, ResourceContainer
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8B\x0D\x00\x00\x00\x00\xB2\x00\x48\x8B\x01\xFF\x50\x28",
    "xxx????x?xxxxxx",
    3,
    ZCollisionManager**, CollisionManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x55\x87",
    "xxx????xxxx",
    3,
    ZContentKitManager*, ContentKitManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\x03\x48\x8B\xCB\xFF\x90\xB0\x01\x00\x00",
    "xxx????x????xxxxxxxxxxxx",
    3,
    ZHM5ActionManager*, HM5ActionManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x0F\x1F\x00",
    "xxx????xxx????xxx????xxx",
    3,
    ZBehaviorService*, BehaviorService
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\x4A\x8B\x0C\xC1",
    "xxx????xxxx",
    3,
    SPrimitiveBufferData*, PrimitiveBufferData
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8B\x0D\x00\x00\x00\x00\xC7\x44\x24\x30\x00\x00\x00\x00\x4C\x89\x74\x24\x38",
    "xxx????xxxx????xxxxx",
    3,
    IGameMode**, GameMode
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8B\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\x0D",
    "xxx????xxx????x????xxx",
    3,
    IEngineMode**, EngineMode
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x05\x00\x00\x00\x00\x4C\x89\x46\x30",
    "xxx????xxxx",
    3,
    void*, ZTemplateEntityBlueprintFactory_vtbl
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\x0F\x10\x00\x0F\x11\x85\xA8\x02\x00\x00",
    "xxx????xxxxxxxxxx",
    3,
    ZInputActionManager*, InputActionManager
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x89\x05\x00\x00\x00\x00\xF2\x0F\x11\x85\xE8\x02\x00\x00",
    "xxx????xxxxxxxx",
    3,
    int*, InputActionManager_BindMem
);

PATTERN_RELATIVE_GLOBAL(
    "\xFF\x05\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x05",
    "xx????x????xxx",
    2,
    int*, InputActionManager_Seq
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x15\x00\x00\x00\x00\x0F\xB6\xC1\xEB",
    "xxx????xxxx",
    3,
    TArray<TEntityRef<ZSelectionForFreeCameraEditorStyleEntity>>*, Selections
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8B\x0D\x00\x00\x00\x00\x4C\x8D\xB5\x88\x05\x00\x00",
    "xxx????xxxxxxx",
    3,
    SD3D12ObjectPools**, D3D12ObjectPools
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xCE\x44\x89\x35",
    "xxx????x????xxxxxx",
    3,
    ZProfileServerPageProxyBaseMap*, ZProfileServerPageProxyBase_m_aRouteMap
);

PATTERN_RELATIVE_GLOBAL(
    "\x48\x89\x3D\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x89\x1D",
    "xxx????xxx????xxx",
    3,
    ZObjectPool*, ZVariantPool1
);

PATTERN_RELATIVE_GLOBAL(
    "\xF0\x48\x0F\xB1\x0D\x00\x00\x00\x00\x75\x00\xEB\x00\xE8\x00\x00\x00\x00\x49\x8B\xD7",
    "xxxxx????x?x?x????xxx",
    5,
    ZObjectPool*, ZVariantPool2
);
