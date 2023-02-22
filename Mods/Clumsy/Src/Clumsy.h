#pragma once

#include <random>
#include <unordered_map>

#include "IPluginInterface.h"
#include "Glacier/ZEntity.h"
#include "Glacier/ZInput.h"

#include <Audio.h>

class Clumsy : public IPluginInterface
{
public:
    Clumsy();
    ~Clumsy() override;

    void OnEngineInitialized() override;
    void Init() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    bool GetEntities();
    DEFINE_PLUGIN_DETOUR(Clumsy, void, OnClearScene, ZEntitySceneContext* th, bool fullyClear);

private:
    std::unique_ptr<DirectX::AudioEngine> m_AudioEngine;
    std::unique_ptr<DirectX::SoundEffect> m_Music;
    std::unique_ptr<DirectX::SoundEffectInstance> m_MusicLoop;

    static constexpr size_t VelocitiesToAverage = 120;
    
    float4 m_SampledVelocitySum;
    std::array<float4, VelocitiesToAverage> m_SampledVelocities;
    size_t m_VelocitySamples;

    bool m_Ragdolling = false;
    float m_RagdollTimer = 0.f;
    bool m_DeactivateRagdollQueued = false;
    bool m_ShowBrickWarning = false;
    float4 m_LastPos;
    ZEntityRef m_GetUpAnimation;
    ZEntityRef m_ShakeEntity;
    ZEntityRef m_MusicEntity;
    ZEntityRef m_MusicEmitter;
};

DEFINE_ZHM_PLUGIN(Clumsy)
