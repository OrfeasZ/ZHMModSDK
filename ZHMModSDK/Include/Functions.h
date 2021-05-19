#pragma once

#include "Common.h"
#include "EngineFunction.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZPrimitives.h"

class ZActor;
class ZDynamicObject;
class ZString;
class ZHM5BaseCharacter;
class ZCameraEntity;
class ZSpatialEntity;
class ZEngineAppCommon;
class IRenderDestinationEntity;
class ZInputAction;
struct SMatrix;

class ZHMSDK_API Functions
{
public:
	static EngineFunction<void(ZActor* th)>* ZActor_OnOutfitChanged;
	static EngineFunction<void(ZActor* th)>* ZActor_ReviveActor;
	static EngineFunction<void(ZDynamicObject* th, ZString* a2)>* ZDynamicObject_ToString;
	static EngineFunction<void(ZHM5BaseCharacter* th, bool activate)>* ZHM5BaseCharacter_ActivateRagdoll;
	static EngineFunction<ZCameraEntity*()>* GetCurrentCamera;
	static EngineFunction<void(ZSpatialEntity* th, SMatrix* out)>* ZSpatialEntity_WorldTransform;
	static EngineFunction<void(ZEngineAppCommon* th)>* ZEngineAppCommon_CreateFreeCamera;
	static EngineFunction<TEntityRef<IRenderDestinationEntity>* (ZCameraManager* th, TEntityRef<IRenderDestinationEntity>* result)>* ZCameraManager_GetActiveRenderDestinationEntity;
	static EngineFunction<double(ZInputAction* th, int a2)>* ZInputAction_Analog;
	static EngineFunction<bool(ZInputAction* th, int a2)>* ZInputAction_Digital;
};
