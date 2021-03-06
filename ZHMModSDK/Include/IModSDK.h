#pragma once

#include "Glacier/ZPrimitives.h"
#include "Common.h"
#include "imgui.h"

class IModSDK
{
public:
	virtual void RequestUIFocus() = 0;
	virtual void ReleaseUIFocus() = 0;
	virtual ImGuiContext* GetImGuiContext() = 0;
	virtual ImGuiMemAllocFunc GetImGuiAlloc() = 0;
	virtual ImGuiMemFreeFunc GetImGuiFree() = 0;
	virtual void* GetImGuiAllocatorUserData() = 0;
	virtual ImFont* GetImGuiLightFont() = 0;
	virtual ImFont* GetImGuiRegularFont() = 0;
	virtual ImFont* GetImGuiMediumFont() = 0;
	virtual ImFont* GetImGuiBoldFont() = 0;
	virtual ImFont* GetImGuiBlackFont() = 0;
	virtual bool GetPinName(int32_t p_PinId, ZString& p_Name) = 0;
};

ZHMSDK_API IModSDK* SDK();
