#include "DebugCheckKeyEntityEnabler.h"

#include <Logging.h>
#include <IconsMaterialDesign.h>
#include <Globals.h>

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZScene.h>

DebugCheckKeyEntityEnabler::~DebugCheckKeyEntityEnabler()
{
	uint8_t code = 1;
	uint8_t code2 = 0;

	if (!SDK()->PatchCode("\x83\x3D\x00\x00\x00\x00\x00\x0F\x94\xC1\x40\x84\xFF", "xx?????xxxxxx", &code, sizeof(code), 6))
	{
		Logger::Error("Could not patch ZDebugCheckKeyEntity::FrameUpdate.");
	}

	if (!SDK()->PatchCode("\x80\x7B\x19\x00\x0F\x84\x00\x00\x00\x00\x84\xC9", "xxx?xx????xx", &code, sizeof(code2), 3))
	{
		Logger::Error("Could not patch ZDebugCheckKeyEntity::FrameUpdate.");
	}
}

void DebugCheckKeyEntityEnabler::Init()
{
	uint8_t code = 0;
	uint8_t code2 = 1;

	if (!SDK()->PatchCode("\x83\x3D\x00\x00\x00\x00\x00\x0F\x94\xC1\x40\x84\xFF", "xx?????xxxxxx", &code, sizeof(code), 6))
	{
		Logger::Error("Could not patch ZDebugCheckKeyEntity::FrameUpdate.");
	}

	if (!SDK()->PatchCode("\x80\x7B\x19\x00\x0F\x84\x00\x00\x00\x00\x84\xC9", "xxx?xx????xx", &code2, sizeof(code2), 3))
	{
		Logger::Error("Could not patch ZDebugCheckKeyEntity::FrameUpdate.");
	}
}

DECLARE_ZHM_PLUGIN(DebugCheckKeyEntityEnabler);
