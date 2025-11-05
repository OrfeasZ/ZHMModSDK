#include "Editor.h"

#include "Glacier/ZModule.h"
#include "Glacier/Enums.h"
#include "Glacier/SColorRGB.h"

#include "Logging.h"

#include <SimpleMath.h>

class ZGuideLadder;
class ZGuideWindow;
class ZPFBoxEntity;
class ZPFObstacleEntity;
class ZPFSeedPoint;
class ZDebugGizmoEntity;
class ZPureWaveModifierEntity;
class ZLightEntity;
class ZDarkLightEntity;
class ZBoxReflectionEntity;
class ZCubemapProbeEntity;
class ZVolumeLightEntity;
class ZParticleEmitterBoxEntity;
class ZParticleEmitterEmitterEntity;
class ZParticleEmitterMeshEntity;
class ZParticleEmitterPointEntity;
class ZParticleGlobalAttractorEntity;
class ZParticleKillVolumeEntity;
class ZGateEntity;
class ZOccluderEntity;
class ZDecalsSpawnEntity;
class ZStaticDecalEntity;
class ZLiquidTrailEntity;
class ZCrowdActorGroupEntity;
class ZCrowdActorGroupFocalPointEntity;
class ZCrowdEntity;
class ZManualActorEntity;
class ZSplineCrowdFlowEntity;
class ZBoxShapeAspect;
class ZCapsuleShapeAspect;
class ZSphereShapeAspect;
class ZWindEntity;
class ZAISoundConnector;
class ZAISoundConnectorTarget;
class ZAISoundModifierVolume;
class ZAIVisionBlockerPlane;
class ZAgitatedGuardPointEntity;
class ZAgitatedWaypointEntity;
class ZCombatActEntity;
class ZLookAtEntity;
class ZOutfitProviderEntity;
class ZPointOfInterestEntity;
class ZActBehaviorEntity;
class ZSpawnPointEntity;
class ZBystanderPointEntity;
class ZWaypointEntity;
class ZPatrolBehaviorEntity;
class ZPostfilterAreaBoxEntity;
class ZFogBoxEntity;
class ZBoxVolumeEntity;
class ZSphereVolumeEntity;
class ZVolumeShapeEntity;
class ZCameraEntity;
class ZOrientationEntity;
class ZScatterContainerEntity;
class ZTrailShapeEntity;
class ZSplineEntity;
class ZSplineControlPointEntity;
class ZAudioEmitterSpatialAspect;
class ZAudioEmitterVolumetricAspect;
class ZClothWireEntity;
class ZChildNetworkActEntity;
class ZCompositeEntity;

void Editor::InitializeDebugChannels() {
    m_DebugChannels.push_back(std::make_pair("Guides - Cover", DEBUGCHANNEL_GUIDES_COVER));
    m_DebugChannels.push_back(std::make_pair("Guides - Ladders", DEBUGCHANNEL_GUIDES_LADDERS));
    m_DebugChannels.push_back(std::make_pair("Guides - Windows", DEBUGCHANNEL_GUIDES_WINDOWS));
    m_DebugChannels.push_back(std::make_pair("Guides - Path Finder", DEBUGCHANNEL_GUIDES_PATHFINDER));
    m_DebugChannels.push_back(std::make_pair("Default", DEBUGCHANNEL_DEFAULT));
    m_DebugChannels.push_back(std::make_pair("Light", DEBUGCHANNEL_LIGHT));
    m_DebugChannels.push_back(std::make_pair("Particles", DEBUGCHANNEL_PARTICLES));
    m_DebugChannels.push_back(std::make_pair("Partitioning", DEBUGCHANNEL_PARTITIONING));
    m_DebugChannels.push_back(std::make_pair("Decals", DEBUGCHANNEL_DECALS));
    m_DebugChannels.push_back(std::make_pair("Crowd", DEBUGCHANNEL_CROWD));
    m_DebugChannels.push_back(std::make_pair("Physics", DEBUGCHANNEL_PHYSICS));
    m_DebugChannels.push_back(std::make_pair("Hero", DEBUGCHANNEL_HERO));
    m_DebugChannels.push_back(std::make_pair("AI", DEBUGCHANNEL_AI));
    m_DebugChannels.push_back(std::make_pair("AI Situation", DEBUGCHANNEL_AI_SITUATION));
    m_DebugChannels.push_back(std::make_pair("AI Area", DEBUGCHANNEL_AI_AREA));
    m_DebugChannels.push_back(std::make_pair("NPC Locomotion", DEBUGCHANNEL_NPC_LOCOMOTION));
    m_DebugChannels.push_back(std::make_pair("Game", DEBUGCHANNEL_GAME));
    m_DebugChannels.push_back(std::make_pair("Alignment", DEBUGCHANNEL_ALIGNMENT));
    m_DebugChannels.push_back(std::make_pair("Engine", DEBUGCHANNEL_ENGINE));
    m_DebugChannels.push_back(std::make_pair("Sound", DEBUGCHANNEL_SOUND));
    m_DebugChannels.push_back(std::make_pair("Animation", DEBUGCHANNEL_ANIMATION));
    m_DebugChannels.push_back(std::make_pair("Cloth", DEBUGCHANNEL_CLOTH));
    m_DebugChannels.push_back(std::make_pair("Sound Paritioning", DEBUGCHANNEL_SOUND_PARTITIONING));
    m_DebugChannels.push_back(std::make_pair("UI", DEBUGCHANNEL_UI));

    m_DebugChannelNameToTypeNames["Guides - Cover"].push_back("ZCoverPlane");

    m_DebugChannelNameToTypeNames["Guides - Ladders"].push_back("ZGuideLadder");

    m_DebugChannelNameToTypeNames["Guides - Windows"].push_back("ZGuideWindow");

    m_DebugChannelNameToTypeNames["Guides - Path Finder"].push_back("ZPFBoxEntity");
    m_DebugChannelNameToTypeNames["Guides - Path Finder"].push_back("ZPFObstacleEntity");
    m_DebugChannelNameToTypeNames["Guides - Path Finder"].push_back("ZPFSeedPoint");

    m_DebugChannelNameToTypeNames["Default"].push_back("ZDebugGizmoEntity");
    m_DebugChannelNameToTypeNames["Default"].push_back("ZPureWaveModifierEntity");

    m_DebugChannelNameToTypeNames["Light"].push_back("ZDebugGizmoEntity");
    m_DebugChannelNameToTypeNames["Light"].push_back("ZLightEntity");
    m_DebugChannelNameToTypeNames["Light"].push_back("ZDarkLightEntity");
    m_DebugChannelNameToTypeNames["Light"].push_back("ZBoxReflectionEntity");
    m_DebugChannelNameToTypeNames["Light"].push_back("ZCubemapProbeEntity");
    m_DebugChannelNameToTypeNames["Light"].push_back("ZVolumeLightEntity");

    m_DebugChannelNameToTypeNames["Particles"].push_back("ZDebugGizmoEntity");
    m_DebugChannelNameToTypeNames["Particles"].push_back("ZParticleEmitterBoxEntity");
    m_DebugChannelNameToTypeNames["Particles"].push_back("ZParticleEmitterEmitterEntity");
    m_DebugChannelNameToTypeNames["Particles"].push_back("ZParticleEmitterMeshEntity");
    m_DebugChannelNameToTypeNames["Particles"].push_back("ZParticleEmitterPointEntity");
    m_DebugChannelNameToTypeNames["Particles"].push_back("ZParticleGlobalAttractorEntity");
    m_DebugChannelNameToTypeNames["Particles"].push_back("ZParticleKillVolumeEntity");

    m_DebugChannelNameToTypeNames["Partitioning"].push_back("ZDebugGizmoEntity");
    m_DebugChannelNameToTypeNames["Partitioning"].push_back("ZGateEntity");
    m_DebugChannelNameToTypeNames["Partitioning"].push_back("ZOccluderEntity");

    m_DebugChannelNameToTypeNames["Decals"].push_back("ZDebugGizmoEntity");
    m_DebugChannelNameToTypeNames["Decals"].push_back("ZDecalsSpawnEntity");
    m_DebugChannelNameToTypeNames["Decals"].push_back("ZStaticDecalEntity");
    m_DebugChannelNameToTypeNames["Decals"].push_back("ZLiquidTrailEntity");

    m_DebugChannelNameToTypeNames["Crowd"].push_back("ZDebugGizmoEntity");
    m_DebugChannelNameToTypeNames["Crowd"].push_back("ZCrowdActorGroupEntity");
    m_DebugChannelNameToTypeNames["Crowd"].push_back("ZCrowdActorGroupFocalPointEntity");
    m_DebugChannelNameToTypeNames["Crowd"].push_back("ZCrowdEntity");
    m_DebugChannelNameToTypeNames["Crowd"].push_back("ZManualActorEntity");
    m_DebugChannelNameToTypeNames["Crowd"].push_back("ZSplineCrowdFlowEntity");

    m_DebugChannelNameToTypeNames["Physics"].push_back("ZDebugGizmoEntity");
    m_DebugChannelNameToTypeNames["Physics"].push_back("ZBoxShapeAspect");
    m_DebugChannelNameToTypeNames["Physics"].push_back("ZCapsuleShapeAspect");
    m_DebugChannelNameToTypeNames["Physics"].push_back("ZSphereShapeAspect");
    m_DebugChannelNameToTypeNames["Physics"].push_back("ZWindEntity");

    m_DebugChannelNameToTypeNames["Hero"].push_back("ZDebugGizmoEntity");

    m_DebugChannelNameToTypeNames["AI"].push_back("ZDebugGizmoEntity");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZAISoundConnector");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZAISoundConnectorTarget");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZAISoundModifierVolume");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZAIVisionBlockerPlane");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZAgitatedGuardPointEntity");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZAgitatedWaypointEntity");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZCombatActEntity");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZLookAtEntity");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZOutfitProviderEntity");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZPointOfInterestEntity");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZActBehaviorEntity");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZSpawnPointEntity");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZBystanderPointEntity");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZWaypointEntity");
    m_DebugChannelNameToTypeNames["AI"].push_back("ZPatrolBehaviorEntity");

    m_DebugChannelNameToTypeNames["AI Situation"].push_back("ZDebugGizmoEntity");

    m_DebugChannelNameToTypeNames["AI Area"].push_back("ZDebugGizmoEntity");

    m_DebugChannelNameToTypeNames["Game"].push_back("ZDebugGizmoEntity");
    m_DebugChannelNameToTypeNames["Game"].push_back("ZPostfilterAreaBoxEntity");
    m_DebugChannelNameToTypeNames["Game"].push_back("ZFogBoxEntity");

    m_DebugChannelNameToTypeNames["Alignment"].push_back("ZDebugGizmoEntity");

    m_DebugChannelNameToTypeNames["Engine"].push_back("ZDebugGizmoEntity");
    m_DebugChannelNameToTypeNames["Engine"].push_back("ZBoxVolumeEntity");
    m_DebugChannelNameToTypeNames["Engine"].push_back("ZSphereVolumeEntity");
    m_DebugChannelNameToTypeNames["Engine"].push_back("ZVolumeShapeEntity");
    m_DebugChannelNameToTypeNames["Engine"].push_back("ZCameraEntity");
    m_DebugChannelNameToTypeNames["Engine"].push_back("ZOrientationEntity");
    m_DebugChannelNameToTypeNames["Engine"].push_back("ZScatterContainerEntity");
    m_DebugChannelNameToTypeNames["Engine"].push_back("ZTrailShapeEntity");
    m_DebugChannelNameToTypeNames["Engine"].push_back("ZSplineEntity");
    m_DebugChannelNameToTypeNames["Engine"].push_back("ZSplineControlPointEntity");

    m_DebugChannelNameToTypeNames["Sound"].push_back("ZDebugGizmoEntity");
    m_DebugChannelNameToTypeNames["Sound"].push_back("ZAudioEmitterSpatialAspect");
    m_DebugChannelNameToTypeNames["Sound"].push_back("ZAudioEmitterVolumetricAspect");

    m_DebugChannelNameToTypeNames["Animation"].push_back("ZDebugGizmoEntity");

    m_DebugChannelNameToTypeNames["Cloth"].push_back("ZDebugGizmoEntity");
    m_DebugChannelNameToTypeNames["Cloth"].push_back("ZClothWireEntity");

    m_DebugChannelNameToTypeNames["Sound Paritioning"].push_back("ZDebugGizmoEntity");

    m_DebugChannelNameToTypeNames["UI"].push_back("ZDebugGizmoEntity");

    for (const auto& [s_DebugChannelName, s_DebugChannelEnum] : m_DebugChannels) {
        for (const auto& s_TypeName : m_DebugChannelNameToTypeNames[s_DebugChannelName]) {
            m_DebugChannelToTypeNameToVisibility[s_DebugChannelEnum][s_TypeName] = true;
        }
    }
}

void Editor::InitializeDebugEntityTypeIDs() {
    ZTypeRegistry* s_TypeRegistry = (*Globals::TypeRegistry);

    m_DebugEntityTypeIds.resize(DebugEntityTypeName::Count);

    m_DebugEntityTypeIds[DebugEntityTypeName::CoverPlane] = s_TypeRegistry->GetTypeID("ZCoverPlane");
    m_DebugEntityTypeIds[DebugEntityTypeName::GuideLadder] = s_TypeRegistry->GetTypeID("ZGuideLadder");
    m_DebugEntityTypeIds[DebugEntityTypeName::GuideWindow] = s_TypeRegistry->GetTypeID("ZGuideWindow");
    m_DebugEntityTypeIds[DebugEntityTypeName::PFBoxEntity] = s_TypeRegistry->GetTypeID("ZPFBoxEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::PFObstacleEntity] = s_TypeRegistry->GetTypeID("ZPFObstacleEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::PFSeedPoint] = s_TypeRegistry->GetTypeID("ZPFSeedPoint");
    m_DebugEntityTypeIds[DebugEntityTypeName::DebugGizmoEntity] = s_TypeRegistry->GetTypeID("ZDebugGizmoEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::PureWaveModifierEntity] = s_TypeRegistry->GetTypeID("ZPureWaveModifierEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::LightEntity] = s_TypeRegistry->GetTypeID("ZLightEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::DarkLightEntity] = s_TypeRegistry->GetTypeID("ZDarkLightEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::BoxReflectionEntity] = s_TypeRegistry->GetTypeID("ZBoxReflectionEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::CubemapProbeEntity] = s_TypeRegistry->GetTypeID("ZCubemapProbeEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::VolumeLightEntity] = s_TypeRegistry->GetTypeID("ZVolumeLightEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::ParticleEmitterBoxEntity] = s_TypeRegistry->GetTypeID("ZParticleEmitterBoxEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::ParticleEmitterEmitterEntity] = s_TypeRegistry->GetTypeID("ZParticleEmitterEmitterEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::ParticleEmitterMeshEntity] = s_TypeRegistry->GetTypeID("ZParticleEmitterMeshEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::ParticleEmitterPointEntity] = s_TypeRegistry->GetTypeID("ZParticleEmitterPointEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::ParticleGlobalAttractorEntity] = s_TypeRegistry->GetTypeID("ZParticleGlobalAttractorEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::ParticleKillVolumeEntity] = s_TypeRegistry->GetTypeID("ZParticleKillVolumeEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::GateEntity] = s_TypeRegistry->GetTypeID("ZGateEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::OccluderEntity] = s_TypeRegistry->GetTypeID("ZOccluderEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::DecalsSpawnEntity] = s_TypeRegistry->GetTypeID("ZDecalsSpawnEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::StaticDecalEntity] = s_TypeRegistry->GetTypeID("ZStaticDecalEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::LiquidTrailEntity] = s_TypeRegistry->GetTypeID("ZLiquidTrailEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::CrowdActorGroupEntity] = s_TypeRegistry->GetTypeID("ZCrowdActorGroupEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::CrowdActorGroupFocalPointEntity] = s_TypeRegistry->GetTypeID("ZCrowdActorGroupFocalPointEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::CrowdEntity] = s_TypeRegistry->GetTypeID("ZCrowdEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::ManualActorEntity] = s_TypeRegistry->GetTypeID("ZManualActorEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::SplineCrowdFlowEntity] = s_TypeRegistry->GetTypeID("ZSplineCrowdFlowEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::BoxShapeAspect] = s_TypeRegistry->GetTypeID("ZBoxShapeAspect");
    m_DebugEntityTypeIds[DebugEntityTypeName::CapsuleShapeAspect] = s_TypeRegistry->GetTypeID("ZCapsuleShapeAspect");
    m_DebugEntityTypeIds[DebugEntityTypeName::SphereShapeAspect] = s_TypeRegistry->GetTypeID("ZSphereShapeAspect");
    m_DebugEntityTypeIds[DebugEntityTypeName::WindEntity] = s_TypeRegistry->GetTypeID("ZWindEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::AISoundConnector] = s_TypeRegistry->GetTypeID("ZAISoundConnector");
    m_DebugEntityTypeIds[DebugEntityTypeName::AISoundConnectorTarget] = s_TypeRegistry->GetTypeID("ZAISoundConnectorTarget");
    m_DebugEntityTypeIds[DebugEntityTypeName::AISoundModifierVolume] = s_TypeRegistry->GetTypeID("ZAISoundModifierVolume");
    m_DebugEntityTypeIds[DebugEntityTypeName::AIVisionBlockerPlane] = s_TypeRegistry->GetTypeID("ZAIVisionBlockerPlane");
    m_DebugEntityTypeIds[DebugEntityTypeName::AgitatedGuardPointEntity] = s_TypeRegistry->GetTypeID("ZAgitatedGuardPointEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::AgitatedWaypointEntity] = s_TypeRegistry->GetTypeID("ZAgitatedWaypointEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::CombatActEntity] = s_TypeRegistry->GetTypeID("ZCombatActEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::LookAtEntity] = s_TypeRegistry->GetTypeID("ZLookAtEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::OutfitProviderEntity] = s_TypeRegistry->GetTypeID("ZOutfitProviderEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::PointOfInterestEntity] = s_TypeRegistry->GetTypeID("ZPointOfInterestEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::ActBehaviorEntity] = s_TypeRegistry->GetTypeID("ZActBehaviorEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::SpawnPointEntity] = s_TypeRegistry->GetTypeID("ZSpawnPointEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::BystanderPointEntity] = s_TypeRegistry->GetTypeID("ZBystanderPointEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::WaypointEntity] = s_TypeRegistry->GetTypeID("ZWaypointEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::PatrolBehaviorEntity] = s_TypeRegistry->GetTypeID("ZPatrolBehaviorEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::PostfilterAreaBoxEntity] = s_TypeRegistry->GetTypeID("ZPostfilterAreaBoxEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::FogBoxEntity] = s_TypeRegistry->GetTypeID("ZFogBoxEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::BoxVolumeEntity] = s_TypeRegistry->GetTypeID("ZBoxVolumeEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::SphereVolumeEntity] = s_TypeRegistry->GetTypeID("ZSphereVolumeEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::VolumeShapeEntity] = s_TypeRegistry->GetTypeID("ZVolumeShapeEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::CameraEntity] = s_TypeRegistry->GetTypeID("ZCameraEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::OrientationEntity] = s_TypeRegistry->GetTypeID("ZOrientationEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::ScatterContainerEntity] = s_TypeRegistry->GetTypeID("ZScatterContainerEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::TrailShapeEntity] = s_TypeRegistry->GetTypeID("ZTrailShapeEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::SplineEntity] = s_TypeRegistry->GetTypeID("ZSplineEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::SplineControlPointEntity] = s_TypeRegistry->GetTypeID("ZSplineControlPointEntity");
    m_DebugEntityTypeIds[DebugEntityTypeName::AudioEmitterSpatialAspect] = s_TypeRegistry->GetTypeID("ZAudioEmitterSpatialAspect");
    m_DebugEntityTypeIds[DebugEntityTypeName::AudioEmitterVolumetricAspect] = s_TypeRegistry->GetTypeID("ZAudioEmitterVolumetricAspect");
    m_DebugEntityTypeIds[DebugEntityTypeName::ClothWireEntity] = s_TypeRegistry->GetTypeID("ZClothWireEntity");
}

void Editor::DrawDebugChannels(bool p_HasFocus) {
    if (!p_HasFocus || !m_DebugChannelsMenuActive) {
        return;
    }

    ImGui::PushFont(SDK()->GetImGuiBlackFont());
    const auto s_Showing = ImGui::Begin("DEBUG CHANNELS", &m_DebugChannelsMenuActive);
    ImGui::PushFont(SDK()->GetImGuiRegularFont());

    if (s_Showing) {
        if (m_DebugChannels.empty()) {
            InitializeDebugChannels();
            InitializeDebugEntityTypeIDs();
        }

        if (!m_DebugEntities.empty()) {
            ImGui::Text("Debug Entity Count: %zu", m_DebugEntities.size());

            ImGui::Separator();

            ImGui::Checkbox("Draw Gizmos", &m_DrawGizmos);
            
            if (ImGui::Checkbox("Draw All Gizmos", &m_DrawAllGizmos)) {
                for (auto& [s_DebugChannel, s_IsVisible] : m_DebugChannelToVisibility) {
                    s_IsVisible = m_DrawAllGizmos;
                }
            }

            ImGui::Separator();

            ImGui::Checkbox("Draw Shapes", &m_DrawShapes);

            for (const auto& pair : m_DebugChannels) {
                const std::string s_Header = fmt::format(
                    "{} ({})",
                    pair.first,
                    m_DebugChannelToDebugEntityCount[pair.second]
                );

                if (ImGui::CollapsingHeader(s_Header.c_str())) {
                    bool& s_DrawGizmos = m_DebugChannelToVisibility[pair.second];

                    ImGui::Checkbox(fmt::format("Draw Gizmos##{}", pair.first).c_str(), &s_DrawGizmos);

                    ImGui::Separator();

                    const auto& s_TypeNameToGizmoCount = m_DebugChannelToTypeNameToDebugEntityCount[pair.second];
                    auto& s_TypeNameToVisibility = m_DebugChannelToTypeNameToVisibility[pair.second];

                    for (const auto& s_Pair : s_TypeNameToGizmoCount) {
                        bool& s_DrawGizmos2 = s_TypeNameToVisibility[s_Pair.first];
                        const std::string s_Label = fmt::format("{} ({})##{}{}",
                            s_Pair.first,
                            s_Pair.second,
                            pair.first,
                            s_Pair.first
                        );

                        ImGui::Checkbox(s_Label.c_str(), &s_DrawGizmos2);
                    }
                }
            }
        }
        else {
            if (ImGui::Button("Get Debug Entities")) {
                if (!m_CachedEntityTree || !m_CachedEntityTree->Entity) {
                    UpdateEntities();
                }

                GetDebugEntities(m_CachedEntityTree);
            }
        }
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopFont();
}

void Editor::DrawDebugEntities(IRenderer* p_Renderer) {
    p_Renderer->SetDistanceCullingEnabled(true);

    if (!m_DrawGizmos && !m_DrawShapes) {
        return;
    }

    for (const auto& s_DebugEntity : m_DebugEntities) {
        if (s_DebugEntity->m_HasGizmo) {
            DrawGizmo(static_cast<GizmoEntity&>(*s_DebugEntity), p_Renderer);
        }
        else {
            DrawShapes(*s_DebugEntity, p_Renderer);
        }
    }

    p_Renderer->SetDistanceCullingEnabled(false);
}

void Editor::DrawGizmo(GizmoEntity& p_GizmoEntity, IRenderer* p_Renderer) {
    if (!m_DrawGizmos) {
        return;
    }

    if (!m_DebugChannelToVisibility[p_GizmoEntity.m_DebugChannel]) {
        return;
    }

    if (!m_DebugChannelToTypeNameToVisibility[p_GizmoEntity.m_DebugChannel][p_GizmoEntity.m_TypeName]) {
        return;
    }

    if (p_GizmoEntity.m_DebugChannel == EDebugChannel::DEBUGCHANNEL_PARTITIONING &&
        p_GizmoEntity.m_TypeName == "ZGateEntity") {
        const bool s_IsOpen = p_GizmoEntity.m_EntityRef.GetProperty<bool>("m_bIsOpen").Get();

        if (s_IsOpen && p_GizmoEntity.m_RuntimeResourceID.GetID() !=
            ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_gate_01.prim].pc_prim">
        ) {
            return;
        }

        if (!s_IsOpen && p_GizmoEntity.m_RuntimeResourceID.GetID() ==
            ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_gate_01.prim].pc_prim">
        ) {
            return;
        }
    }
    else if (p_GizmoEntity.m_DebugChannel == EDebugChannel::DEBUGCHANNEL_DEFAULT &&
        p_GizmoEntity.m_TypeName == "ZPureWaveModifierEntity" &&
        p_GizmoEntity.m_RuntimeResourceID.GetID() ==
        ResId<"[assembly:/geometry/g2/gizmos.wl2?/unit_circle.prim].pc_prim">
    ) {
        const float s_Radius = p_GizmoEntity.m_EntityRef.GetProperty<float>("m_fRadius").Get();

        if (p_GizmoEntity.m_Transform.XAxis.x != s_Radius) {
            const float4 s_RadiusGizmoScale(s_Radius, s_Radius, s_Radius, 0.f);
            const float4 s_RadiusGizmoTranslate(0.f, 0.f, 0.f, 0.f);

            p_GizmoEntity.m_Transform = SMatrix::ScaleTranslate(s_RadiusGizmoScale, s_RadiusGizmoTranslate);
        }
    }

    SMatrix s_Transform;

    if (p_GizmoEntity.m_DebugChannel == EDebugChannel::DEBUGCHANNEL_AI &&
        p_GizmoEntity.m_TypeName == "ZActBehaviorEntity") {
        const TEntityRef<ZSpatialEntity> s_MoveToTransform = p_GizmoEntity.m_EntityRef.GetProperty<TEntityRef<ZSpatialEntity>>("m_rMoveToTransform").Get();

        s_Transform = s_MoveToTransform.m_pInterfaceRef->GetWorldMatrix() * p_GizmoEntity.m_Transform;
    }
    else {
        static STypeID* s_SpatialEntityTypeID = (*Globals::TypeRegistry)->GetTypeID("ZSpatialEntity");
        auto s_SpatialEntity = p_GizmoEntity.m_EntityRef.QueryInterface<ZSpatialEntity>(s_SpatialEntityTypeID);

        s_Transform = s_SpatialEntity->GetWorldMatrix() * p_GizmoEntity.m_Transform;
    }

    ZRenderPrimitiveResource* s_pRenderPrimitiveResource = static_cast<ZRenderPrimitiveResource*>(p_GizmoEntity.m_PrimResourcePtr.GetResourceData());

    if (!s_pRenderPrimitiveResource) {
        Logger::Error("PRIM of {:016x} gizmo isn't installed!", p_GizmoEntity.m_RuntimeResourceID.GetID());
        return;
    }

    for (size_t j = 0; j < s_pRenderPrimitiveResource->m_Primitives.size(); ++j) {
        ZRenderPrimitiveMesh* s_pRenderPrimitive = static_cast<ZRenderPrimitiveMesh*>(s_pRenderPrimitiveResource->m_Primitives[j].m_pObject);
        SPrimitiveBufferData* s_PrimitiveBufferData = &Globals::PrimitiveBufferData[s_pRenderPrimitive->m_BufferDataIndex];

        p_Renderer->DrawMesh(
            s_pRenderPrimitiveResource,
            s_PrimitiveBufferData->m_pVertexBuffers,
            3,
            s_PrimitiveBufferData->m_pIndexBuffer,
            s_Transform,
            s_PrimitiveBufferData->vPositionScale,
            s_PrimitiveBufferData->vPositionBias,
            s_PrimitiveBufferData->vTextureScaleBias,
            p_GizmoEntity.m_Color
        );
    }
}

void Editor::DrawShapes(const DebugEntity& p_DebugEntity, IRenderer* p_Renderer) {
    if (!m_DrawShapes) {
        return;
    }

    if (!m_DebugChannelToVisibility[p_DebugEntity.m_DebugChannel]) {
        return;
    }

    if (!m_DebugChannelToTypeNameToVisibility[p_DebugEntity.m_DebugChannel][p_DebugEntity.m_TypeName]) {
        return;
    }
}

void Editor::GetDebugEntities(const std::shared_ptr<EntityTreeNode>& p_EntityTreeNode) {
    /*
    * Entity id of queried interface has to be compared with entity id of current node
    * because if current entity is composed entity or if it has exposed interfaces
    * it will have interfaces of it's children.
    *
    * EntityIDMatches method can't be used for ZBoxShapeAspect, ZCapsuleShapeAspect, ZSphereShapeAspect,
    * ZAudioEmitterSpatialAspect, ZAudioEmitterVolumetricAspect and ZClothWireEntity since they are
    * in aspect entity and they don't have entity type.
    */
    
    if (!p_EntityTreeNode) {
        return;
    }

    static const SVector4 s_Color = SVector4(1.f, 1.f, 1.f, 1.f);
    
    static STypeID* s_CompositeEntityTypeID = (*Globals::TypeRegistry)->GetTypeID("ZCompositeEntity");

    auto s_LightEntity = p_EntityTreeNode->Entity.QueryInterface<ZLightEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::LightEntity]
    );
    auto s_DarkLightEntity = p_EntityTreeNode->Entity.QueryInterface<ZDarkLightEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::DarkLightEntity]
    );

    /*if (auto s_CoverPlane = p_EntityTreeNode->Entity.QueryInterface<ZCoverPlane>(
        m_DebugEntityTypeIds[DebugEntityTypeName::CoverPlane]
    )) {
        if (EntityIDMatches(s_CoverPlane, p_EntityTreeNode->EntityId)) {
            AddDebugEntity(p_EntityTreeNode->Entity, "ZCoverPlane", DEBUGCHANNEL_GUIDES_COVER);
        }
    }*/
    if (auto s_GuideLadder = p_EntityTreeNode->Entity.QueryInterface<ZGuideLadder>(
        m_DebugEntityTypeIds[DebugEntityTypeName::GuideLadder]
    )) {
        if (EntityIDMatches(s_GuideLadder, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZGuideLadder", DEBUGCHANNEL_GUIDES_LADDERS, "m_pHelper");
        }
    }
    else if (auto s_GuideWindow = p_EntityTreeNode->Entity.QueryInterface<ZGuideWindow>(
        m_DebugEntityTypeIds[DebugEntityTypeName::GuideWindow]
    )) {
        if (EntityIDMatches(s_GuideWindow, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZGuideWindow", DEBUGCHANNEL_GUIDES_WINDOWS, "m_pHelper");
        }
    }
    else if (auto s_PFBoxEntity = p_EntityTreeNode->Entity.QueryInterface<ZPFBoxEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::PFBoxEntity]
    )) {
        if (EntityIDMatches(s_PFBoxEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZPFBoxEntity", DEBUGCHANNEL_GUIDES_PATHFINDER, "m_pHelper");
        }
    }
    else if (auto s_PFObstacleEntity = p_EntityTreeNode->Entity.QueryInterface<ZPFObstacleEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::PFObstacleEntity]
    )) {
        if (EntityIDMatches(s_PFObstacleEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZPFObstacleEntity", DEBUGCHANNEL_GUIDES_PATHFINDER, "m_pHelper");
        }
    }
    else if (auto s_PFSeedPoint = p_EntityTreeNode->Entity.QueryInterface<ZPFSeedPoint>(
        m_DebugEntityTypeIds[DebugEntityTypeName::PFSeedPoint]
    )) {
        if (EntityIDMatches(s_PFSeedPoint, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZPFSeedPoint", DEBUGCHANNEL_GUIDES_PATHFINDER, "m_pHelper");
        }
    }
    else if (auto s_DebugGizmoEntity = p_EntityTreeNode->Entity.QueryInterface<ZDebugGizmoEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::DebugGizmoEntity]
    )) {
        if (EntityIDMatches(s_DebugGizmoEntity, p_EntityTreeNode->EntityId)) {
            const ZDebugGizmoEntity_EDrawLayer s_DrawLayer = p_EntityTreeNode->Entity.GetProperty<ZDebugGizmoEntity_EDrawLayer>("m_eDrawLayer").Get();
            const EDebugChannel s_DebugChannel = ConvertDrawLayerToDebugChannel(s_DrawLayer);

            AddGizmoEntity(p_EntityTreeNode->Entity, "ZDebugGizmoEntity", s_DebugChannel, "m_GizmoGeomRID");
        }
    }
    else if (auto s_PureWaveModifierEntity = p_EntityTreeNode->Entity.QueryInterface<ZPureWaveModifierEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::PureWaveModifierEntity]
    )) {
        if (EntityIDMatches(s_PureWaveModifierEntity, p_EntityTreeNode->EntityId)) {
            const float4 s_DirectionGizmoScale(-3.f, -3.f, 3.f, 0.f);
            const float4 s_DirectionGizmoTranslate(0.f, 0.f, 0.f, 0.f);
            const SMatrix s_DirectionGizmoTransform = SMatrix::ScaleTranslate(s_DirectionGizmoScale, s_DirectionGizmoTranslate);

            const float s_Radius = p_EntityTreeNode->Entity.GetProperty<float>("m_fRadius").Get();
            const float4 s_RadiusGizmoScale(s_Radius, s_Radius, s_Radius, 0.f);
            const float4 s_RadiusGizmoTranslate(0.f, 0.f, 0.f, 0.f);
            const SMatrix s_RadiusGizmoTransform = SMatrix::ScaleTranslate(s_RadiusGizmoScale, s_RadiusGizmoTranslate);

            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZPureWaveModifierEntity",
                DEBUGCHANNEL_DEFAULT,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_arrow_01.prim].pc_prim">,
                s_Color,
                s_DirectionGizmoTransform
            );

            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZPureWaveModifierEntity",
                DEBUGCHANNEL_DEFAULT,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/unit_circle.prim].pc_prim">,
                s_Color,
                s_RadiusGizmoTransform
            );
        }
    }
    else if (s_LightEntity || s_DarkLightEntity) {
        if (EntityIDMatches(s_LightEntity, p_EntityTreeNode->EntityId) ||
            EntityIDMatches(s_DarkLightEntity, p_EntityTreeNode->EntityId)
        ) {
            std::string s_TypeName;

            if (s_LightEntity) {
                s_TypeName = "ZLightEntity";
            }
            else {
                s_TypeName = "ZDarkLightEntity";
            }

            const ILightEntity_ELightType s_LightType = p_EntityTreeNode->Entity.GetProperty<ILightEntity_ELightType>("m_eLightType").Get();

            switch (s_LightType) {
                case ILightEntity_ELightType::LT_DIRECTIONAL:
                case ILightEntity_ELightType::LT_ENVIRONMENT:
                case ILightEntity_ELightType::LT_AREA_QUAD:
                    AddGizmoEntity(
                        p_EntityTreeNode->Entity,
                        s_TypeName,
                        DEBUGCHANNEL_LIGHT,
                        ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_light_directional_01.prim].pc_prim">
                    );
                    break;
                case ILightEntity_ELightType::LT_OMNI:
                    AddGizmoEntity(
                        p_EntityTreeNode->Entity,
                        s_TypeName,
                        DEBUGCHANNEL_LIGHT,
                        ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_light_01.prim].pc_prim">
                    );
                    break;
                case ILightEntity_ELightType::LT_SPOT:
                case ILightEntity_ELightType::LT_SQUARESPOT:
                    AddGizmoEntity(
                        p_EntityTreeNode->Entity,
                        s_TypeName,
                        DEBUGCHANNEL_LIGHT,
                        ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_light_spot_01.prim].pc_prim">
                    );
                    break;
                case ILightEntity_ELightType::LT_CAPSULE:
                    AddGizmoEntity(
                        p_EntityTreeNode->Entity,
                        s_TypeName,
                        DEBUGCHANNEL_LIGHT,
                        ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_light_capsule_01.prim].pc_prim">
                    );
                    break;
            }
        }
    }
    else if (auto s_BoxReflectionEntity = p_EntityTreeNode->Entity.QueryInterface<ZBoxReflectionEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::BoxReflectionEntity]
    )) {
        if (EntityIDMatches(s_BoxReflectionEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZBoxReflectionEntity",
                DEBUGCHANNEL_LIGHT,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_cubemap_01.prim].pc_prim">
            );
        }
    }
    else if (auto s_CubemapProbeEntity = p_EntityTreeNode->Entity.QueryInterface<ZCubemapProbeEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::CubemapProbeEntity]
    )) {
        if (EntityIDMatches(s_CubemapProbeEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZCubemapProbeEntity", DEBUGCHANNEL_LIGHT, "m_pHelper");
        }
    }
    else if (auto s_VolumeLightEntity = p_EntityTreeNode->Entity.QueryInterface<ZVolumeLightEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::VolumeLightEntity]
    )) {
        if (EntityIDMatches(s_VolumeLightEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZVolumeLightEntity", DEBUGCHANNEL_LIGHT, "m_pHelper");
        }
    }
    else if (auto s_ParticleEmitterBoxEntity = p_EntityTreeNode->Entity.QueryInterface<ZParticleEmitterBoxEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::ParticleEmitterBoxEntity]
    )) {
        if (EntityIDMatches(s_ParticleEmitterBoxEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZParticleEmitterBoxEntity",
                DEBUGCHANNEL_PARTICLES,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_particles_01.prim].pc_prim">
            );
        }
    }
    else if (auto s_ParticleEmitterEmitterEntity = p_EntityTreeNode->Entity.QueryInterface<ZParticleEmitterEmitterEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::ParticleEmitterEmitterEntity]
    )) {
        if (EntityIDMatches(s_ParticleEmitterEmitterEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZParticleEmitterEmitterEntity",
                DEBUGCHANNEL_PARTICLES,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_particles_01.prim].pc_prim">
            );
        }
    }
    else if (auto s_ParticleEmitterMeshEntity = p_EntityTreeNode->Entity.QueryInterface<ZParticleEmitterMeshEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::ParticleEmitterMeshEntity]
    )) {
        if (EntityIDMatches(s_ParticleEmitterMeshEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZParticleEmitterMeshEntity",
                DEBUGCHANNEL_PARTICLES,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_particles_01.prim].pc_prim">
            );
        }
    }
    else if (auto s_ParticleEmitterPointEntity = p_EntityTreeNode->Entity.QueryInterface<ZParticleEmitterPointEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::ParticleEmitterPointEntity]
    )) {
        if (EntityIDMatches(s_ParticleEmitterPointEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZParticleEmitterPointEntity",
                DEBUGCHANNEL_PARTICLES,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_particles_01.prim].pc_prim">
            );
        }
    }
    else if (auto s_ParticleGlobalAttractorEntity = p_EntityTreeNode->Entity.QueryInterface<ZParticleGlobalAttractorEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::ParticleGlobalAttractorEntity]
    )) {
        if (EntityIDMatches(s_ParticleGlobalAttractorEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZParticleGlobalAttractorEntity", DEBUGCHANNEL_PARTICLES, "m_pHelper");
        }
    }
    else if (auto s_ParticleKillVolumeEntity = p_EntityTreeNode->Entity.QueryInterface<ZParticleKillVolumeEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::ParticleKillVolumeEntity]
    )) {
        if (EntityIDMatches(s_ParticleKillVolumeEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZParticleKillVolumeEntity", DEBUGCHANNEL_PARTICLES, "m_pHelper");
        }
    }
    else if (auto s_GateEntity = p_EntityTreeNode->Entity.QueryInterface<ZGateEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::GateEntity]
    )) {
        if (EntityIDMatches(s_GateEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZGateEntity", DEBUGCHANNEL_PARTITIONING, "m_pHelper");
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZGateEntity", DEBUGCHANNEL_PARTITIONING, "m_pHelperClosed");
        }
    }
    else if (auto s_OccluderEntity = p_EntityTreeNode->Entity.QueryInterface<ZOccluderEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::OccluderEntity]
    )) {
        if (EntityIDMatches(s_OccluderEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZOccluderEntity", DEBUGCHANNEL_PARTITIONING, "m_pHelper");
        }
    }
    else if (auto s_DecalsSpawnEntity = p_EntityTreeNode->Entity.QueryInterface<ZDecalsSpawnEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::DecalsSpawnEntity]
    )) {
        if (EntityIDMatches(s_DecalsSpawnEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZDecalsSpawnEntity", DEBUGCHANNEL_DECALS, "m_pHelper");
        }
    }
    else if (auto s_StaticDecalEntity = p_EntityTreeNode->Entity.QueryInterface<ZStaticDecalEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::StaticDecalEntity]
    )) {
        if (EntityIDMatches(s_StaticDecalEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZStaticDecalEntity",
                DEBUGCHANNEL_DECALS,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_decal_01.prim].pc_prim">
            );
        }
    }
    else if (auto s_LiquidTrailEntity = p_EntityTreeNode->Entity.QueryInterface<ZLiquidTrailEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::LiquidTrailEntity]
    )) {
        if (EntityIDMatches(s_LiquidTrailEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZLiquidTrailEntity", DEBUGCHANNEL_DECALS, "m_pHelper");
        }
    }
    else if (auto s_CrowdActorGroupEntity = p_EntityTreeNode->Entity.QueryInterface<ZCrowdActorGroupEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::CrowdActorGroupEntity]
    )) {
        if (EntityIDMatches(s_CrowdActorGroupEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZCrowdActorGroupEntity", DEBUGCHANNEL_CROWD, "m_pHelper");
        }
    }
    else if (auto s_CrowdActorGroupFocalPointEntity = p_EntityTreeNode->Entity.QueryInterface<ZCrowdActorGroupFocalPointEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::CrowdActorGroupFocalPointEntity]
    )) {
        if (EntityIDMatches(s_CrowdActorGroupFocalPointEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZCrowdActorGroupFocalPointEntity", DEBUGCHANNEL_CROWD, "m_pHelper");
        }
    }
    else if (auto s_CrowdEntity = p_EntityTreeNode->Entity.QueryInterface<ZCrowdEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::CrowdEntity]
    )) {
        if (EntityIDMatches(s_CrowdEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZCrowdEntity", DEBUGCHANNEL_CROWD, "m_pGizmo");
        }
    }
    else if (auto s_ManualActorEntity = p_EntityTreeNode->Entity.QueryInterface<ZManualActorEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::ManualActorEntity]
    )) {
        if (EntityIDMatches(s_ManualActorEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZManualActorEntity", DEBUGCHANNEL_CROWD, "m_pGizmo");
        }
    }
    else if (auto s_SplineCrowdFlowEntity = p_EntityTreeNode->Entity.QueryInterface<ZSplineCrowdFlowEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::SplineCrowdFlowEntity]
    )) {
        if (EntityIDMatches(s_SplineCrowdFlowEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZSplineCrowdFlowEntity", DEBUGCHANNEL_CROWD, "m_pHelper");
        }
    }
    else if (p_EntityTreeNode->Entity.QueryInterface<ZBoxShapeAspect>(
        m_DebugEntityTypeIds[DebugEntityTypeName::BoxShapeAspect]
    )) {
        const bool isComposite = p_EntityTreeNode->Entity.QueryInterface<ZCompositeEntity>(s_CompositeEntityTypeID);

        if (!isComposite) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZBoxShapeAspect",
                DEBUGCHANNEL_PHYSICS,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_colibox.prim].pc_prim">
            );
        }
    }
    else if (p_EntityTreeNode->Entity.QueryInterface<ZCapsuleShapeAspect>(
        m_DebugEntityTypeIds[DebugEntityTypeName::CapsuleShapeAspect]
    )) {
        const bool isComposite = p_EntityTreeNode->Entity.QueryInterface<ZCompositeEntity>(s_CompositeEntityTypeID);

        if (!isComposite) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZCapsuleShapeAspect",
                DEBUGCHANNEL_PHYSICS,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_colicapsule.prim].pc_prim">
            );
        }
    }
    else if (p_EntityTreeNode->Entity.QueryInterface<ZSphereShapeAspect>(
        m_DebugEntityTypeIds[DebugEntityTypeName::SphereShapeAspect]
    )) {
        const bool isComposite = p_EntityTreeNode->Entity.QueryInterface<ZCompositeEntity>(s_CompositeEntityTypeID);

        if (!isComposite) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZSphereShapeAspect",
                DEBUGCHANNEL_PHYSICS,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_colisphere.prim].pc_prim">
            );
        }
    }
    else if (auto s_WindEntity = p_EntityTreeNode->Entity.QueryInterface<ZWindEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::WindEntity]
    )) {
        if (EntityIDMatches(s_WindEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZWindEntity", DEBUGCHANNEL_PHYSICS, "m_pHelperGizmo");
        }
    }
    else if (auto s_AISoundConnector = p_EntityTreeNode->Entity.QueryInterface<ZAISoundConnector>(
        m_DebugEntityTypeIds[DebugEntityTypeName::AISoundConnector]
    )) {
        if (EntityIDMatches(s_AISoundConnector, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZAISoundConnector",
                DEBUGCHANNEL_AI,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_annotation_01.prim].pc_prim">
            );
        }
    }
    else if (auto s_AISoundConnectorTarget = p_EntityTreeNode->Entity.QueryInterface<ZAISoundConnectorTarget>(
        m_DebugEntityTypeIds[DebugEntityTypeName::AISoundConnectorTarget]
    )) {
        if (EntityIDMatches(s_AISoundConnectorTarget, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZAISoundConnectorTarget",
                DEBUGCHANNEL_AI,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_annotation_01.prim].pc_prim">
            );
        }
    }
    else if (auto s_AISoundModifierVolume = p_EntityTreeNode->Entity.QueryInterface<ZAISoundModifierVolume>(
        m_DebugEntityTypeIds[DebugEntityTypeName::AISoundModifierVolume]
    )) {
        if (EntityIDMatches(s_AISoundModifierVolume, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZAISoundModifierVolume",
                DEBUGCHANNEL_AI,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_annotation_01.prim].pc_prim">
            );
        }
    }
    else if (auto s_AIVisionBlockerPlane = p_EntityTreeNode->Entity.QueryInterface<ZAIVisionBlockerPlane>(
        m_DebugEntityTypeIds[DebugEntityTypeName::AIVisionBlockerPlane]
    )) {
        if (EntityIDMatches(s_AIVisionBlockerPlane, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZAIVisionBlockerPlane",
                DEBUGCHANNEL_AI,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_annotation_01.prim].pc_prim">
            );
        }
    }
    else if (auto s_AgitatedGuardPointEntity = p_EntityTreeNode->Entity.QueryInterface<ZAgitatedGuardPointEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::AgitatedGuardPointEntity]
    )) {
        if (EntityIDMatches(s_AgitatedGuardPointEntity, p_EntityTreeNode->EntityId)) {
            float4 s_Axis = float4(0.f, 0.f, 1.f, 0.f);
            SMatrix s_Rotation = SMatrix::RotationAxisAngle(s_Axis, DirectX::XM_PI);

            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZAgitatedGuardPointEntity",
                DEBUGCHANNEL_AI,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_arrow_01.prim].pc_prim">,
                s_Color,
                s_Rotation
            );
        }
    }
    else if (auto s_AgitatedWaypointEntity = p_EntityTreeNode->Entity.QueryInterface<ZAgitatedWaypointEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::AgitatedWaypointEntity]
    )) {
        if (EntityIDMatches(s_AgitatedWaypointEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZAgitatedWaypointEntity", DEBUGCHANNEL_AI, "m_pGizmo");
        }
    }
    else if (auto s_CombatActEntity = p_EntityTreeNode->Entity.QueryInterface<ZCombatActEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::CombatActEntity]
    )) {
        if (EntityIDMatches(s_CombatActEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZCombatActEntity", DEBUGCHANNEL_AI, "m_pGizmo");
        }
    }
    else if (auto s_LookAtEntity = p_EntityTreeNode->Entity.QueryInterface<ZLookAtEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::LookAtEntity]
    )) {
        if (EntityIDMatches(s_LookAtEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZLookAtEntity", DEBUGCHANNEL_AI, "m_pHelper");
        }
    }
    else if (auto s_OutfitProviderEntity = p_EntityTreeNode->Entity.QueryInterface<ZOutfitProviderEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::OutfitProviderEntity]
    )) {
        if (EntityIDMatches(s_OutfitProviderEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZOutfitProviderEntity",
                DEBUGCHANNEL_AI,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_actor_01.prim].pc_prim">
            );
        }
    }
    else if (auto s_PointOfInterestEntity = p_EntityTreeNode->Entity.QueryInterface<ZPointOfInterestEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::PointOfInterestEntity]
    )) {
        if (EntityIDMatches(s_PointOfInterestEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZPointOfInterestEntity", DEBUGCHANNEL_AI, "m_pHelper");
        }
    }
    else if (auto s_ActBehaviorEntity = p_EntityTreeNode->Entity.QueryInterface<ZActBehaviorEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::ActBehaviorEntity]
    )) {
        if (EntityIDMatches(s_ActBehaviorEntity, p_EntityTreeNode->EntityId)) {
            const TEntityRef<ZSpatialEntity> s_MoveToTransform =
                p_EntityTreeNode->Entity.GetProperty<TEntityRef<ZSpatialEntity>>("m_rMoveToTransform").Get();

            if (s_MoveToTransform.m_pInterfaceRef) {
                const TEntityRef<ZChildNetworkActEntity> s_Act =
                    p_EntityTreeNode->Entity.GetProperty<TEntityRef<ZChildNetworkActEntity>>("m_rAct").Get();

                if (s_Act.m_pInterfaceRef) {
                    SMatrix s_Transform;

                    s_Transform.Trans = float4(0.f, 0.f, 0.681f, 1.f);

                    AddGizmoEntity(
                        p_EntityTreeNode->Entity,
                        "ZActBehaviorEntity",
                        DEBUGCHANNEL_AI,
                        ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_cut_board_01.prim].pc_prim">,
                        s_Color,
                        s_Transform
                    );
                }

                const ZActBehaviorEntity_ERotationAlignment s_AlignRotation =
                    p_EntityTreeNode->Entity.GetProperty<ZActBehaviorEntity_ERotationAlignment>("m_AlignRotation").Get();

                if (s_AlignRotation != ZActBehaviorEntity_ERotationAlignment::RA_NONE) {
                    const SColorRGB s_Color = p_EntityTreeNode->Entity.GetProperty<SColorRGB>("m_Color").Get();
                    const SVector4 s_Color2 = SVector4(s_Color.r, s_Color.g, s_Color.b, 1.f);

                    AddGizmoEntity(
                        p_EntityTreeNode->Entity,
                        "ZActBehaviorEntity",
                        DEBUGCHANNEL_AI,
                        ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_arrow_act.prim].pc_prim">,
                        s_Color2
                    );
                }

                const bool s_AlignPosition = p_EntityTreeNode->Entity.GetProperty<bool>("m_bAlignPosition").Get();

                if (s_AlignPosition) {
                    AddGizmoEntity(
                        p_EntityTreeNode->Entity,
                        "ZActBehaviorEntity",
                        DEBUGCHANNEL_AI,
                        ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_point_01.prim].pc_prim">
                    );
                }
                else {
                    AddGizmoEntity(
                        p_EntityTreeNode->Entity,
                        "ZActBehaviorEntity",
                        DEBUGCHANNEL_AI,
                        ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_goto_green.prim].pc_prim">
                    );
                }
            }
        }
    }
    else if (auto s_SpawnPointEntity = p_EntityTreeNode->Entity.QueryInterface<ZSpawnPointEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::SpawnPointEntity]
    )) {
        if (EntityIDMatches(s_SpawnPointEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZSpawnPointEntity", DEBUGCHANNEL_AI, "m_pGizmo");
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZSpawnPointEntity", DEBUGCHANNEL_AI, "m_pAlignGizmo");
        }
    }
    else if (auto s_BystanderPointEntity = p_EntityTreeNode->Entity.QueryInterface<ZBystanderPointEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::BystanderPointEntity]
    )) {
        if (EntityIDMatches(s_BystanderPointEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZBystanderPointEntity", DEBUGCHANNEL_AI, "m_pGizmo");
        }
    }
    else if (auto s_WaypointEntity = p_EntityTreeNode->Entity.QueryInterface<ZWaypointEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::WaypointEntity]
    )) {
        if (EntityIDMatches(s_WaypointEntity, p_EntityTreeNode->EntityId)) {
            const TEntityRef<ZChildNetworkActEntity> s_Act =
                p_EntityTreeNode->Entity.GetProperty<TEntityRef<ZChildNetworkActEntity>>("m_pAct").Get();

            if (s_Act.m_pInterfaceRef) {
                SMatrix s_Transform;

                s_Transform.Trans = float4(0.f, 0.f, 0.681f, 1.f);

                AddGizmoEntity(
                    p_EntityTreeNode->Entity,
                    "ZWaypointEntity",
                    DEBUGCHANNEL_AI,
                    ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_cut_board_01.prim].pc_prim">,
                    s_Color,
                    s_Transform
                );
            }

            const EWaypointRotationAlignment s_AlignRotation =
                p_EntityTreeNode->Entity.GetProperty<EWaypointRotationAlignment>("m_AlignRotation").Get();

            if (s_AlignRotation != EWaypointRotationAlignment::RA_NONE) {
                AddGizmoEntity(
                    p_EntityTreeNode->Entity,
                    "ZWaypointEntity",
                    DEBUGCHANNEL_AI,
                    ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_arrow_01.prim].pc_prim">
                );
            }

            const bool s_AlignPosition = p_EntityTreeNode->Entity.GetProperty<bool>("m_bAlignPosition").Get();

            if (s_AlignPosition) {
                AddGizmoEntity(
                    p_EntityTreeNode->Entity,
                    "ZWaypointEntity",
                    DEBUGCHANNEL_AI,
                    ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_point_01.prim].pc_prim">
                );
            }
            else {
                AddGizmoEntity(
                    p_EntityTreeNode->Entity,
                    "ZWaypointEntity",
                    DEBUGCHANNEL_AI,
                    ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_goto_green.prim].pc_prim">
                );
            }
        }
    }
    else if (auto s_PostfilterAreaBoxEntit = p_EntityTreeNode->Entity.QueryInterface<ZPostfilterAreaBoxEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::PostfilterAreaBoxEntity]
    )) {
        if (EntityIDMatches(s_PostfilterAreaBoxEntit, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZPostfilterAreaBoxEntity", DEBUGCHANNEL_GAME, "m_pHelper");
        }
    }
    else if (auto s_FogBoxEntity = p_EntityTreeNode->Entity.QueryInterface<ZFogBoxEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::FogBoxEntity]
    )) {
        if (EntityIDMatches(s_FogBoxEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZFogBoxEntity", DEBUGCHANNEL_GAME, "m_pHelper");
        }
    }
    else if (auto s_BoxVolumeEntity = p_EntityTreeNode->Entity.QueryInterface<ZBoxVolumeEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::BoxVolumeEntity]
    )) {
        if (EntityIDMatches(s_BoxVolumeEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZBoxVolumeEntity", DEBUGCHANNEL_ENGINE, "m_pHelper");
        }
    }
    else if (auto s_SphereVolumeEntity = p_EntityTreeNode->Entity.QueryInterface<ZSphereVolumeEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::SphereVolumeEntity]
    )) {
        if (EntityIDMatches(s_SphereVolumeEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZSphereVolumeEntity", DEBUGCHANNEL_ENGINE, "m_pHelper");
        }
    }
    else if (auto s_VolumeShapeEntity = p_EntityTreeNode->Entity.QueryInterface<ZVolumeShapeEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::VolumeShapeEntity]
    )) {
        if (EntityIDMatches(s_VolumeShapeEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZVolumeShapeEntity", DEBUGCHANNEL_ENGINE, "m_pHelper");
        }
    }
    else if (auto s_CameraEntity = p_EntityTreeNode->Entity.QueryInterface<ZCameraEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::CameraEntity]
    )) {
        if (EntityIDMatches(s_CameraEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZCameraEntity", DEBUGCHANNEL_ENGINE, "m_pHelper");
        }
    }
    else if (auto s_OrientationEntity = p_EntityTreeNode->Entity.QueryInterface<ZOrientationEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::OrientationEntity]
    )) {
        if (EntityIDMatches(s_OrientationEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZOrientationEntity", DEBUGCHANNEL_ENGINE, "m_pHelper");
        }
    }
    else if (auto s_ScatterContainerEntity = p_EntityTreeNode->Entity.QueryInterface<ZScatterContainerEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::ScatterContainerEntity]
    )) {
        if (EntityIDMatches(s_ScatterContainerEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZScatterContainerEntity", DEBUGCHANNEL_ENGINE, "m_pHelperGizmo");
        }
    }
    else if (auto s_TrailShapeEntity = p_EntityTreeNode->Entity.QueryInterface<ZTrailShapeEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::TrailShapeEntity]
    )) {
        if (EntityIDMatches(s_TrailShapeEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZTrailShapeEntity", DEBUGCHANNEL_ENGINE, "m_pHelper");
        }
    }
    else if (auto s_SplineEntity = p_EntityTreeNode->Entity.QueryInterface<ZSplineEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::SplineEntity]
    )) {
        if (EntityIDMatches(s_SplineEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZSplineEntity", DEBUGCHANNEL_ENGINE, "m_pSplineGizmo");
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZSplineEntity", DEBUGCHANNEL_ENGINE, "m_pMarkerGizmo");
        }
    }
    else if (auto s_SplineControlPointEntity = p_EntityTreeNode->Entity.QueryInterface<ZSplineControlPointEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::SplineControlPointEntity]
    )) {
        if (EntityIDMatches(s_SplineControlPointEntity, p_EntityTreeNode->EntityId)) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZSplineControlPointEntity", DEBUGCHANNEL_ENGINE, "m_pControlPointGizmo");
        }
    }
    else if (p_EntityTreeNode->Entity.QueryInterface<ZAudioEmitterSpatialAspect>(
        m_DebugEntityTypeIds[DebugEntityTypeName::AudioEmitterSpatialAspect]
    )) {
        const bool isComposite = p_EntityTreeNode->Entity.QueryInterface<ZCompositeEntity>(s_CompositeEntityTypeID);

        if (!isComposite) {
            AddGizmoEntity(
                p_EntityTreeNode->Entity,
                "ZAudioEmitterSpatialAspect",
                DEBUGCHANNEL_SOUND,
                ResId<"[assembly:/geometry/g2/gizmos.wl2?/gizmo_sound_01.prim].pc_prim">
            );
        }
    }
    else if (p_EntityTreeNode->Entity.QueryInterface<ZAudioEmitterVolumetricAspect>(
        m_DebugEntityTypeIds[DebugEntityTypeName::AudioEmitterVolumetricAspect]
    )) {
        const bool isComposite = p_EntityTreeNode->Entity.QueryInterface<ZCompositeEntity>(s_CompositeEntityTypeID);

        if (!isComposite) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZAudioEmitterVolumetricAspect", DEBUGCHANNEL_SOUND, "m_pHelper");
        }
    }
    else if (p_EntityTreeNode->Entity.QueryInterface<ZClothWireEntity>(
        m_DebugEntityTypeIds[DebugEntityTypeName::ClothWireEntity]
    )) {
        const bool isComposite = p_EntityTreeNode->Entity.QueryInterface<ZCompositeEntity>(s_CompositeEntityTypeID);

        if (!isComposite) {
            AddGizmoEntity(p_EntityTreeNode->Entity, "ZClothWireEntity", DEBUGCHANNEL_CLOTH, "m_pHelper");
        }
    }

    for (const auto& [_, s_Child] : p_EntityTreeNode->Children) {
        GetDebugEntities(s_Child);
    }
}

void Editor::AddDebugEntity(
    const ZEntityRef p_EntityRef,
    const std::string& p_TypeName,
    const EDebugChannel p_DebugChannel
) {
    auto s_DebugEntity = std::make_unique<DebugEntity>();

    s_DebugEntity->m_TypeName = p_TypeName;
    s_DebugEntity->m_EntityRef = p_EntityRef;
    s_DebugEntity->m_DebugChannel = p_DebugChannel;
    s_DebugEntity->m_HasGizmo = false;

    m_DebugEntities.push_back(std::move(s_DebugEntity));

    ++m_DebugChannelToDebugEntityCount[p_DebugChannel];
    ++m_DebugChannelToTypeNameToDebugEntityCount[p_DebugChannel][p_TypeName];
}

void Editor::AddGizmoEntity(
    const ZEntityRef p_EntityRef,
    const std::string& p_TypeName,
    const EDebugChannel p_DebugChannel,
    const ZRuntimeResourceID p_RuntimeResourceID,
    const SVector4& p_Color,
    const SMatrix& p_Transform
) {
    ZResourcePtr s_ResourcePtr;

    Globals::ResourceManager->GetResourcePtr(s_ResourcePtr, p_RuntimeResourceID, 0);

    auto s_GizmoEntity = std::make_unique<GizmoEntity>();

    s_GizmoEntity->m_TypeName = p_TypeName;
    s_GizmoEntity->m_EntityRef = p_EntityRef;
    s_GizmoEntity->m_DebugChannel = p_DebugChannel;
    s_GizmoEntity->m_HasGizmo = true;
    s_GizmoEntity->m_RuntimeResourceID = p_RuntimeResourceID;
    s_GizmoEntity->m_PrimResourcePtr = s_ResourcePtr;
    s_GizmoEntity->m_Color = p_Color;
    s_GizmoEntity->m_Transform = p_Transform;

    m_DebugEntities.push_back(std::move(s_GizmoEntity));

    ++m_DebugChannelToDebugEntityCount[p_DebugChannel];
    ++m_DebugChannelToTypeNameToDebugEntityCount[p_DebugChannel][p_TypeName];
}

void Editor::AddGizmoEntity(
    const ZEntityRef p_EntityRef,
    const std::string& p_TypeName,
    const EDebugChannel p_DebugChannel,
    const std::string& p_PropertyName,
    const SVector4& p_Color,
    const SMatrix& p_Transform
) {
    const ZResourcePtr s_ResourcePtr = p_EntityRef.GetProperty<ZResourcePtr>(p_PropertyName).Get();
    const ZRuntimeResourceID s_RuntimeResourceID;

    if (s_ResourcePtr.m_nResourceIndex.val != -1) {
        const ZRuntimeResourceID s_RuntimeResourceID = (*Globals::ResourceContainer)->m_resources[s_ResourcePtr.m_nResourceIndex.val].rid;

        auto s_GizmoEntity = std::make_unique<GizmoEntity>();

        s_GizmoEntity->m_TypeName = p_TypeName;
        s_GizmoEntity->m_EntityRef = p_EntityRef;
        s_GizmoEntity->m_DebugChannel = p_DebugChannel;
        s_GizmoEntity->m_HasGizmo = true;
        s_GizmoEntity->m_RuntimeResourceID = s_RuntimeResourceID;
        s_GizmoEntity->m_PrimResourcePtr = s_ResourcePtr;
        s_GizmoEntity->m_Color = p_Color;
        s_GizmoEntity->m_Transform = p_Transform;

        m_DebugEntities.push_back(std::move(s_GizmoEntity));

        ++m_DebugChannelToDebugEntityCount[p_DebugChannel];
        ++m_DebugChannelToTypeNameToDebugEntityCount[p_DebugChannel][p_TypeName];
    }
    else {
        const uint64_t s_EntityId = p_EntityRef.GetEntity()->GetType()->m_nEntityId;

        Logger::Error("Hash of gizmo is missing for entity with {:016x} id and {} type!", s_EntityId, p_TypeName);
    }
}

EDebugChannel Editor::ConvertDrawLayerToDebugChannel(const ZDebugGizmoEntity_EDrawLayer p_DrawLayer) {
    switch (p_DrawLayer) {
        case ZDebugGizmoEntity_EDrawLayer::DL_DEFAULT: return DEBUGCHANNEL_DEFAULT;
        case ZDebugGizmoEntity_EDrawLayer::DL_LIGHT: return DEBUGCHANNEL_LIGHT;
        case ZDebugGizmoEntity_EDrawLayer::DL_PARTICLES: return DEBUGCHANNEL_PARTICLES;
        case ZDebugGizmoEntity_EDrawLayer::DL_PARTITIONING: return DEBUGCHANNEL_PARTITIONING;
        case ZDebugGizmoEntity_EDrawLayer::DL_DECALS: return DEBUGCHANNEL_DECALS;
        case ZDebugGizmoEntity_EDrawLayer::DL_CROWD: return DEBUGCHANNEL_CROWD;
        case ZDebugGizmoEntity_EDrawLayer::DL_PHYSICS: return DEBUGCHANNEL_PHYSICS;
        case ZDebugGizmoEntity_EDrawLayer::DL_HERO: return DEBUGCHANNEL_HERO;
        case ZDebugGizmoEntity_EDrawLayer::DL_AI: return DEBUGCHANNEL_AI;
        case ZDebugGizmoEntity_EDrawLayer::DL_AI_GRID: return DEBUGCHANNEL_AI_GRID;
        case ZDebugGizmoEntity_EDrawLayer::DL_AI_SITUATION: return DEBUGCHANNEL_AI_SITUATION;
        case ZDebugGizmoEntity_EDrawLayer::DL_AI_AREA: return DEBUGCHANNEL_AI_AREA;
        case ZDebugGizmoEntity_EDrawLayer::DL_NPC_LOCOMOTION: return DEBUGCHANNEL_NPC_LOCOMOTION;
        case ZDebugGizmoEntity_EDrawLayer::DL_GAME: return DEBUGCHANNEL_GAME;
        case ZDebugGizmoEntity_EDrawLayer::DL_ALIGNMENT: return DEBUGCHANNEL_ALIGNMENT;
        case ZDebugGizmoEntity_EDrawLayer::DL_ENGINE: return DEBUGCHANNEL_ENGINE;
        case ZDebugGizmoEntity_EDrawLayer::DL_SOUND: return DEBUGCHANNEL_SOUND;
        case ZDebugGizmoEntity_EDrawLayer::DL_ANIMATION: return DEBUGCHANNEL_ANIMATION;
        case ZDebugGizmoEntity_EDrawLayer::DL_CLOTH: return DEBUGCHANNEL_CLOTH;
        case ZDebugGizmoEntity_EDrawLayer::DL_SOUND_PARTITIONING: return DEBUGCHANNEL_SOUND_PARTITIONING;
        case ZDebugGizmoEntity_EDrawLayer::DL_UI: return DEBUGCHANNEL_UI;
        default: return DEBUGCHANNEL_NONE;
    }
}

bool Editor::EntityIDMatches(void* p_Interface, const uint64 p_EntityID) {
    auto s_EntityType = reinterpret_cast<ZEntityType**>(reinterpret_cast<uintptr_t>(p_Interface) + 8);

    if (s_EntityType && *s_EntityType && (*s_EntityType)->m_nEntityId == p_EntityID) {
        return true;
    }

    return false;
}

bool Editor::RayCastGizmos(const SVector3& p_WorldPosition, const SVector3& p_Direction) {
    DirectX::SimpleMath::Ray s_Ray(
        DirectX::SimpleMath::Vector3(p_WorldPosition.x, p_WorldPosition.y, p_WorldPosition.z),
        DirectX::SimpleMath::Vector3(p_Direction.x, p_Direction.y, p_Direction.z)
    );
    float s_ClosestDistance = FLT_MAX;
    int s_HitIndex = -1;

    static STypeID* s_SpatialEntityTypeID = (*Globals::TypeRegistry)->GetTypeID("ZSpatialEntity");

    for (size_t i = 0; i < m_DebugEntities.size(); ++i) {
        if (!m_DebugEntities[i]->m_HasGizmo) {
            continue;
        }

        GizmoEntity* s_GizmoEntity = static_cast<GizmoEntity*>(m_DebugEntities[i].get());

        if (!m_DebugChannelToVisibility[s_GizmoEntity->m_DebugChannel]) {
            continue;
        }

        if (!m_DebugChannelToTypeNameToVisibility[s_GizmoEntity->m_DebugChannel][s_GizmoEntity->m_TypeName]) {
            continue;
        }

        ZRenderPrimitiveResource* s_RenderPrimitiveResource = static_cast<ZRenderPrimitiveResource*>(s_GizmoEntity->m_PrimResourcePtr.GetResourceData());

        if (!s_RenderPrimitiveResource) {
            continue;
        }

        SVector3 s_Center = (s_RenderPrimitiveResource->m_vMin + s_RenderPrimitiveResource->m_vMax) * 0.5f;
        SVector3 s_Extents = (s_RenderPrimitiveResource->m_vMax - s_RenderPrimitiveResource->m_vMin) * 0.5f;

        DirectX::BoundingBox s_Box(
            DirectX::SimpleMath::Vector3(s_Center.x, s_Center.y, s_Center.z),
            DirectX::SimpleMath::Vector3(s_Extents.x, s_Extents.y, s_Extents.z)
        );
        
        SMatrix s_Transform;

        if (s_GizmoEntity->m_TypeName == "ZActBehaviorEntity") {
            const TEntityRef<ZSpatialEntity> s_MoveToTransform = s_GizmoEntity->m_EntityRef.GetProperty<TEntityRef<ZSpatialEntity>>("m_rMoveToTransform").Get();

            s_Transform = s_MoveToTransform.m_pInterfaceRef->GetWorldMatrix() * s_GizmoEntity->m_Transform;
        }
        else {
            auto s_SpatialEntity = s_GizmoEntity->m_EntityRef.QueryInterface<ZSpatialEntity>(s_SpatialEntityTypeID);

            s_Transform = s_SpatialEntity->GetWorldMatrix();
        }

        DirectX::XMMATRIX s_Transform2 = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&s_Transform));

        s_Box.Transform(s_Box, s_Transform2);

        float s_Distance = 0.f;

        if (s_Ray.Intersects(s_Box, s_Distance)) {
            if (s_Distance < s_ClosestDistance && s_Distance <= 200.f) {
                s_ClosestDistance = s_Distance;
                s_HitIndex = static_cast<int>(i);
            }
        }
    }

    if (s_HitIndex == -1) {
        if (m_raycastLogging)
            Logger::Debug("RaycastGizmos found no hits.");

        m_SelectedGizmoEntity = nullptr;

        return false;
    }

    m_SelectedGizmoEntity = static_cast<GizmoEntity*>(m_DebugEntities[s_HitIndex].get());

    if (m_raycastLogging)
    {
        Logger::Debug("RaycastGizmos hit gizmo '{}' (channel {}) at distance {}",
            m_SelectedGizmoEntity->m_TypeName, static_cast<int>(m_SelectedGizmoEntity->m_DebugChannel), s_ClosestDistance);
    }

    const auto s_SceneCtx = Globals::Hitman5Module->m_pEntitySceneContext;
    ZEntityRef s_SelectedEntity = m_SelectedGizmoEntity->m_EntityRef;

    for (int i = 0; i < s_SceneCtx->m_aLoadedBricks.size(); ++i) {
        const auto& s_Brick = s_SceneCtx->m_aLoadedBricks[i];

        if (s_SelectedEntity.IsAnyParent(s_Brick.m_EntityRef)) {
            Logger::Debug("Found gizmo entity in brick {} (idx = {}).", s_Brick.m_RuntimeResourceID, i);
            m_SelectedBrickIndex = i;
            break;
        }
    }

    if (m_SelectedGizmoEntity->m_EntityRef.GetEntity() && m_SelectedGizmoEntity->m_EntityRef.GetEntity()->GetType()) {
        const auto& s_Type = *m_SelectedGizmoEntity->m_EntityRef.GetEntity()->GetType();
        const auto& s_Interfaces = *s_Type.m_pInterfaces;

        Logger::Trace(
            "Hit entity of type '{}' with id '{:x}'.", s_Interfaces[0].m_pTypeId->typeInfo()->m_pTypeName,
            s_Type.m_nEntityId
        );
    }

    OnSelectEntity(m_SelectedGizmoEntity->m_EntityRef, std::nullopt);

    return true;
}