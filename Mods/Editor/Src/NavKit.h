#pragma once
#include <string>
#include <utility>
#include <vector>

#include <Glacier/ZEntity.h>
#include <Glacier/ZMath.h>

struct NavKitMatiTextures {
    std::string m_DiffuseTextureHash;
    std::string m_NormalTextureHash;
    std::string m_SpecularTextureHash;
};

struct NavKitMeshEntity {
    NavKitMeshEntity(
        std::string m_AlocHash,
        std::string m_PrimHash,
        const Quat m_Quat,
        std::string m_RoomName,
        std::string m_FolderName,
        const ZEntityRef m_Entity,
        const bool m_IsWater = false
    ) :
        m_AlocHash(std::move(m_AlocHash)),
        m_PrimHash(std::move(m_PrimHash)),
        m_Quat(m_Quat),
        m_RoomName(std::move(m_RoomName)),
        m_FolderName(std::move(m_FolderName)),
        m_Entity(m_Entity),
        m_IsWater(m_IsWater) {}

    std::string m_AlocHash;
    std::string m_PrimHash;
    Quat m_Quat;
    std::string m_RoomName;
    std::string m_FolderName;
    ZEntityRef m_Entity;
    // A water surface (an entity with a ZPureWaterAspect). NavKit keeps it out of the navmesh and draws it as
    // water on the map.
    bool m_IsWater;
};
