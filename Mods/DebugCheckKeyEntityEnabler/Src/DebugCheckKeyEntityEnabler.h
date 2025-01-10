#pragma once

#include <IPluginInterface.h>

class DebugCheckKeyEntityEnabler : public IPluginInterface {
public:
    ~DebugCheckKeyEntityEnabler() override;
    void Init() override;
};

DEFINE_ZHM_PLUGIN(DebugCheckKeyEntityEnabler)
