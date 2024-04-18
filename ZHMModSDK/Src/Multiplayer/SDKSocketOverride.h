#pragma once

#include <Glacier/ZRakNet.h>
#include <PluginInterface2.h>

class SDKSocketOverride : public NetworkSocketOverride, public RakNet::PluginInterface2 {
    
};
