#pragma once

#include "Common.h"
#include <cstdint>

class ZCollisionManager;
class ZResourceContainer;
class ZResourceManager;
class ZGameStatsManager;
class ZEntityManager;
class ZMemoryManager;
class ZGameLoopManager;
class ZTypeRegistry;
class ZGameTimeManager;
class ZHitman5Module;
class ZGameContext;
class ZActorManager;
class ZRenderManager;
class ZApplicationEngineWin32;
class ZGameUIManager;
class ZCameraManager;
class ZPlayerRegistry;
class ZHM5InputManager;
class ZContentKitManager;
class ZHM5ActionManager;

class ZHMSDK_API Globals
{
public:
	static ZGameLoopManager* GameLoopManager;
	static ZTypeRegistry** TypeRegistry;
	static ZGameTimeManager** GameTimeManager;
	static ZHitman5Module* Hitman5Module;
	static ZGameContext* GameContext;
	static ZActorManager* ActorManager;
	static uint16_t* NextActorId;
	static ZMemoryManager** MemoryManager;
	static ZRenderManager* RenderManager;
	static ZApplicationEngineWin32** ApplicationEngineWin32;
	static ZGameUIManager* GameUIManager;
	static ZEntityManager* EntityManager;
	static ZGameStatsManager* GameStatsManager;
	static ZCameraManager* CameraManager;
	static ZPlayerRegistry* PlayerRegistry;
	static ZHM5InputManager* InputManager;
	static ZResourceContainer** ResourceContainer;
	static ZResourceManager* ResourceManager;
	static ZCollisionManager** CollisionManager;
	static ZContentKitManager* ContentKitManager;
	static ZHM5ActionManager* HM5ActionManager;
};
