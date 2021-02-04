#pragma once

#include "Common.h"
#include "Hook.h"

class ZActor;
class ZComponentCreateInfo;

class ZHMSDK_API Hooks
{
public:
	static Hook<void, ZActor*, ZComponentCreateInfo*>* ZActor_ZActor;
	static Hook<void>* Test;
};
