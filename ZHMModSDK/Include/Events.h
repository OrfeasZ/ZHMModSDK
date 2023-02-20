#pragma once

#include "EventDispatcher.h"
#include "Common.h"

class ZHMSDK_API Events
{
public:
    static EventDispatcher<void>* OnConsoleCommand;
};
