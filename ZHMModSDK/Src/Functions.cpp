#include "Functions.h"
#include "EngineFunctionImpl.h"

PATTERN_FUNCTION(
	"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x00\x48\x8B\xD9\x48\x8D\x15",
	"xxxxxxxxx?xxxxxx",
	ZActor_OnOutfitChanged,
	void(ZActor*)
);

PATTERN_FUNCTION(
	"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x00\x48\x8B\x81\xD8\x02\x00\x00", 
	"xxxxxxxxx?xxxxxxx",
	ZActor_ReviveActor,
	void(ZActor*)
);

PATTERN_FUNCTION(
	"\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x00\x48\x8B\xF1\x0F\x57\xC0",
	"xxxxxxxxxxxxxx?xxxxxx",
	ZDynamicObject_ToString,
	void(ZDynamicObject*, ZString*)
);