#pragma once

#include <memory>

#include "IModSDK.h"

class ModLoader;
class DebugConsole;

#define DECLARE_SDK_GLOBAL(GlobalType, GlobalName) GlobalType GlobalName() override;

class ModSDK : public IModSDK
{
private:
	static ModSDK* g_Instance;

public:
	static ModSDK* GetInstance();
	static void DestroyInstance();

public:
	ModSDK();
	~ModSDK();

	bool Startup();

	ModLoader* GetModLoader() const { return m_ModLoader; }

#if _DEBUG
	DebugConsole* GetDebugConsole() const { return m_DebugConsole; }
#endif

	uintptr_t GetModuleBase() const { return m_ModuleBase; }
	uint32_t GetSizeOfCode() const { return m_SizeOfCode; }
	uint32_t GetImageSize() const { return m_ImageSize; }

private:
	ModLoader* m_ModLoader;

#if _DEBUG
	DebugConsole* m_DebugConsole;
#endif

	uintptr_t m_ModuleBase;
	uint32_t m_SizeOfCode;
	uint32_t m_ImageSize;
};
