#pragma once

#include "EventDispatcher.h"
#include "Common.h"

#include <vector>
#include <string>

class ZHMSDK_API Events
{
public:
    static EventDispatcher<std::vector<std::string>>* OnConsoleCommand;
};
