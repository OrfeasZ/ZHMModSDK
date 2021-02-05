#pragma once

#include "Common.h"
#include <cstdint>

class ZGameLoopManager;
class ZTypeRegistry;
class ZGameTimeManager;
class ZHitman5Module;
class ZGameContext;
class ZActorManager;

class ZHMSDK_API Globals
{
	static ZGameLoopManager** GameLoopManager;
	static ZTypeRegistry** TypeRegistry;
	static ZGameTimeManager** GameTimeManager;
	static ZHitman5Module** Hitman5Module;
	static ZGameContext* GameContext;
	static ZActorManager* ActorManager;
	static uint16_t* NextActorId;
};
