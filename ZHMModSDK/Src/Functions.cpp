#include "Functions.h"
#include "EngineFunctionImpl.h"

PATTERN_FUNCTION(
	"\x48\x83\xEC\x00\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x81\x20\x04\x00\x00",
	"xxx?xxx????xxxxxxx",
	ZActor_OnOutfitChanged,
	void(ZActor*)
);

PATTERN_FUNCTION(
	"\x48\x89\x5C\x24\x18\x55\x57\x41\x57\x48\x8D\x6C\x24\xB9\x48\x81\xEC\x00\x00\x00\x00\x48\x8D\x99\xD8\x02\x00\x00",
	"xxxxxxxxxxxxxxxxx????xxxxxxx",
	ZActor_ReviveActor,
	void(ZActor*)
);

PATTERN_FUNCTION(
	"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x00\x48\x8B\xFA\x48\x8B\xD9\x0F\x57\xC0",
	"xxxxxxxxx?xxxxxxxxx",
	ZDynamicObject_ToString,
	void(ZDynamicObject*, ZString*)
);

PATTERN_FUNCTION(
	"\x40\x55\x57\x41\x54\x48\x8D\x6C\x24\xB9\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x01",
	"xxxxxxxxxxxxx????xxx",
	ZHM5BaseCharacter_ActivateRagdoll,
	void(ZHM5BaseCharacter*, bool)
);

PATTERN_FUNCTION(
	"\x48\x89\x5C\x24\x10\x57\x48\x83\xEC\x00\x48\x8B\x1D\x00\x00\x00\x00\x48\x85\xDB",
	"xxxxxxxxx?xxx????xxx",
	GetCurrentCamera,
	ZCameraEntity* ()
);

PATTERN_FUNCTION(
	"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x00\xF7\x41\x6C\x00\x00\x00\x00\x48\x8B\xFA\x48\x8B\xD9\x74\x00\xE8\x00\x00\x00\x00\x0F\x10\x53\x20",
	"xxxxxxxxx?xxx????xxxxxxx?x????xxxx",
	ZSpatialEntity_WorldTransform,
	void(ZSpatialEntity* th, SMatrix* out)
);

PATTERN_FUNCTION(
	"\x48\x8B\xC4\x55\x48\x8D\xA8\xD8\xFE\xFF\xFF",
	"xxxxxxxxxxx",
	ZEngineAppCommon_CreateFreeCamera,
	void(ZEngineAppCommon* th)
);

PATTERN_FUNCTION(
	"\x48\x89\x5C\x24\x18\x48\x89\x74\x24\x20\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8D\x79\x08",
	"xxxxxxxxxxxxxxxxxxxxxx?xxxx",
	ZCameraManager_GetActiveRenderDestinationEntity,
	TEntityRef<IRenderDestinationEntity>* (ZCameraManager* th, TEntityRef<IRenderDestinationEntity>* result)
);

PATTERN_FUNCTION(
	"\x40\x53\x48\x83\xEC\x00\x80\x79\x2C\x00\x48\x8B\xD9\x74",
	"xxxxx?xxx?xxxx",
	ZInputAction_Analog,
	double(ZInputAction* th, int a2)
);

PATTERN_FUNCTION(
	"\x40\x53\x41\x56\x48\x83\xEC\x00\x8B\xDA",
	"xxxxxxx?xx",
	ZInputAction_Digital,
	bool(ZInputAction* th, int a2)
);

PATTERN_RELATIVE_FUNCTION(
	"\xE8\x00\x00\x00\x00\x48\x8B\x44\x24\x48\x48\x8B\xB0\x40\x11\x00\x00",
	"x????xxxxxxxxxxxx",
	ZPlayerRegistry_GetLocalPlayer,
	void(ZPlayerRegistry* th, TEntityRef<ZHitman5>* out)
);

PATTERN_FUNCTION(
	"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x00\x48\x8B\x15\x00\x00\x00\x00\x4C\x8D\x15",
	"xxxxxxxxx?xxx????xxx",
	ZHM5InputManager_GetInputControlForLocalPlayer,
	ZHM5InputControl* (ZHM5InputManager* th)
);
