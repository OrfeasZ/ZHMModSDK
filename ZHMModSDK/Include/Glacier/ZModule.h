#pragma once

#include "Reflection.h"
#include "ZScene.h"
#include "ZString.h"

class ZEntitySceneContext;
class ZUserFeedbackManager;

class IModule :
    public IComponentInterface
{
};

// size: 0x68
class ZSimpleModuleBase :
    public IModule
{
public:
    PAD(0x60); // 0x08
};

class ZConfiguration
{
public:
    PAD(0x10);
};

class ZHitman5Module :
    public ZSimpleModuleBase,
    public ZConfiguration
{
public:
    PAD(0x10); // 0x78
    ZEntitySceneContext* m_pEntitySceneContext; // 0x88
    ZUserFeedbackManager* m_pUserFeedbackManager; // 0x90
    PAD(0x50);

public:
    [[nodiscard]]
    bool IsEngineInitialized() const
    {
        if (!m_pEntitySceneContext)
            return false;

        return m_pEntitySceneContext->m_sceneData.m_sceneName.size() != 0;
    }
};
