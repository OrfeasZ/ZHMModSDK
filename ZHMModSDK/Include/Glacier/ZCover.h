#pragma once

#include "ZSpatialEntity.h"
#include "ZCollision.h"
#include "ZColor.h"
#include "Enums.h"

#include "Logging.h"
#include "IRenderer.h"

class ZOutfitProfessionEntity;
class ZCoverPlane;

enum class ENeigborState {
    eNone_0 = 0,
    eValid = 1,
    eInvalid = 2
};

struct SNeighborInfo {
    TEntityRef<ZCoverPlane> m_pNPlane;
    float32 m_fAngle;
    ENeigborState m_NState;
    bool m_bConvex : 1;
};

struct SBox {
    SMatrix m_mvCenterMatPos;
    float4 m_vHalfSize;
};

enum class ECornerSide {
    NO_COVER_CORNER = 0,
    RIGHT_COVER_CORNER = 1,
    LEFT_COVER_CORNER = 2
};

struct SCoverCorner {
    float4 m_vCornerPoint;
    ECornerSide m_eSide;
    bool m_bValid : 1;
    bool m_bHighCoverCorner : 1;
    bool m_bWindowCorner : 1;
    bool m_bDisabled : 1;
};

class ZCoverPlane : public ZBoundedEntity {
public:
    enum class ECoverType {
        eCoverOnly = 0,
        eCoverAndRail = 1,
        eRailOnly = 2,
    };

    enum class ECoverSize {
        eLowCover = 0,
        eMediumCover = 1,
        eHighCover = 2,
    };

    enum class ESide {
        eSideRight = 0,
        eSideLeft = 1,
        eSideCountNoUse = 2
    };

    bool IsCoverValid() const {
        if (!PlanePropertiesValid()) {
            return false;
        }

        if (!m_bPlaneCollisionObjectFree) {
            return false;
        }

        return true;
    }

    void CalcEdgePoints(float4* p_EdgePoints) const {
        const float s_CoverHeight = GetCoverHeight();
        const float s_HalfLength = m_fCoverLength * 0.5f;

        const SVector3 s_Center = m_vCoverPos;
        const SVector3 s_Side = m_vCoverSideDir * s_HalfLength;
        const SVector3 s_Up(0.f, 0.f, s_CoverHeight);

        const SVector3 s_RightBottom = s_Center + s_Side;
        const SVector3 s_LeftBottom = s_Center - s_Side;
        const SVector3 s_RightTop = s_RightBottom + s_Up;
        const SVector3 s_LeftTop = s_LeftBottom + s_Up;

        p_EdgePoints[0] = float4(s_RightBottom, 1.f);
        p_EdgePoints[1] = float4(s_RightTop, 1.f);
        p_EdgePoints[2] = float4(s_LeftBottom, 1.f);
        p_EdgePoints[3] = float4(s_LeftTop, 1.f);
    }

    float GetCoverHeight() const {
        switch (m_eCoverSize) {
            case ECoverSize::eHighCover: return 2.0f;
            case ECoverSize::eMediumCover: return 1.3f;
            default: return 1.0f;
        }
    }

    bool IsHighCover() const {
        return m_eCoverSize == ECoverSize::eHighCover;
    }

    bool IsCoverPlaneACoverSpot() const {
        if (!m_bActivated || !m_bStreamStateReady) {
            return false;
        }

        if (m_bForceCoverSpot) {
            return true;
        }

        const float s_CoverLengthX = (m_vCoverSideDir.x * 2.0f) * m_fCornerT;
        const float s_CoverLengthY = (m_vCoverSideDir.y * 2.0f) * m_fCornerT;
        const float s_CoverLengthSq = s_CoverLengthX * s_CoverLengthX + s_CoverLengthY * s_CoverLengthY;

        return s_CoverLengthSq <= 0.0625f;
    }

    bool PlanePropertiesValid() const {
        const float s_HorizontalLength = GetHorizontalLength();
        const bool s_IsLengthValid = s_HorizontalLength >= 0.79f;
        const bool s_IsCornerValid =
            !IsCoverPlaneACoverSpot() || m_bCornerLeftEnabled || m_bCornerRightEnabled;

        return s_IsLengthValid && s_IsCornerValid;
    }

    float GetHorizontalLength() const {
        const float s_HorizontalMagnitude = std::sqrt(
            m_vCoverSideDir.x * m_vCoverSideDir.x +
            m_vCoverSideDir.y * m_vCoverSideDir.y
        );

        return s_HorizontalMagnitude * m_fCoverLength;
    }

    bool IsCornerFlagsSet(uint8 nFlag, ESide p_Side) const {
        if (p_Side == ESide::eSideLeft) {
            nFlag *= 16;
        }

        return (nFlag & m_nCornerFlags) == nFlag;
    }

    bool AIIsCornerValid(const ESide p_Side) const {
        if (!IsCornerFlagsSet(1, p_Side)) {
            return false;
        }

        if (IsCornerFlagsSet(8, p_Side)) {
            return false;
        }

        if (IsCornerFlagsSet(4, p_Side)) {
            return false;
        }

        return true;
    }

    float4 GetEdgePointLocal(const ESide p_Side) const {
        float s_HalfLength = m_fCoverLength * 0.5f;

        if (p_Side == ESide::eSideLeft) {
            s_HalfLength = -s_HalfLength;
        }

        return { s_HalfLength, 0.f, 0.f, 0.f };
    }

    float4 GetEdgePointWorld(const ESide p_Side) const {
        float4 s_LocalPosition = GetEdgePointLocal(p_Side);

        return GetObjectToWorldMatrix().WVectorTransform(s_LocalPosition);
    }

    float AIGetNPCRadiusX() const {
        return 0.4f;
    }

    float AIGetNPCRadiusY() const {
        if (m_eCoverSize == ECoverSize::eHighCover) {
            return 0.5f;
        }

        return 0.65f;
    }

    float4 AIGetShootTestPosition(const ESide p_Side) const {
        const float4 s_EdgeWorld = GetEdgePointWorld(p_Side);

        SVector3 s_CoverSideDir = m_vCoverSideDir;

        if (p_Side == ESide::eSideLeft) {
            s_CoverSideDir = -s_CoverSideDir;
        }

        const SVector3 s_ShootPosition = SVector3(s_EdgeWorld) +
            (s_CoverSideDir * AIGetNPCRadiusX()) +
            (m_vCoverDir * AIGetNPCRadiusY());

        return float4(s_ShootPosition, 1.f);
    }

    float4 GetCoverCornerPoint(ESide p_Side) const {
        float4 s_CoverPosition = float4(m_vCoverPos, 1.f);
        float s_CornerT = m_fCornerT;

        if (p_Side == ESide::eSideLeft) {
            s_CornerT = -s_CornerT;
        }

        return s_CoverPosition + (m_vCoverSideDir * s_CornerT);
    }

    SCoverCorner UnpackCoverCorner(ESide eSide) const {
        SCoverCorner s_Corner;
        s_Corner.m_eSide = ECornerSide::NO_COVER_CORNER;

        s_Corner.m_bValid = IsCornerFlagsSet(1, eSide);
        s_Corner.m_bHighCoverCorner = IsCornerFlagsSet(2, eSide);
        s_Corner.m_bWindowCorner = IsCornerFlagsSet(4, eSide);
        s_Corner.m_bDisabled = IsCornerFlagsSet(8, eSide);

        if (s_Corner.m_bValid) {
            s_Corner.m_eSide = eSide == ESide::eSideLeft
                ? ECornerSide::LEFT_COVER_CORNER
                : ECornerSide::RIGHT_COVER_CORNER;
        }

        s_Corner.m_vCornerPoint = GetCoverCornerPoint(eSide);

        return s_Corner;
    }

    SNeighborInfo m_Neighbors[2];
    SVector3 m_vCoverPos;
    SVector3 m_vCoverSideDir;
    SVector2 m_vCoverDir;
    float32 m_fCornerT;
    uint8 m_nCornerFlags;
    uint8 m_nAIFlags;
    uint64 m_nAIReservingActorIdentifier;
    bool m_bActivated : 1;
    bool m_bEnabled : 1;
    bool m_bRailEnabled : 1;
    bool m_bPlaneCollisionObjectFree : 1;
    bool m_bInEditMode : 1;
    bool m_bStreamStateReady : 1;
    bool m_bInvalidCoverFacing : 1;
    ECoverType m_eCoverType; // 0x12C
    ECoverSize m_eCoverSize; // 0x130
    float32 m_fCoverLength; // 0x134
    float32 m_fCoverDepth; // 0x138
    bool m_bCornerRightEnabled; // 0x13C
    bool m_bCornerLeftEnabled; // 0x13D
    bool m_bForceCoverSpot; // 0x13E
    bool m_bWindowCover; // 0x13F
    bool m_bStartEnabled; // 0x140
    bool m_bRailStartEnabled; // 0x141
    bool m_bNPCUseCover; // 0x142
    bool m_bNPCShootOverCover; // 0x143
    bool m_bHeroPushNPCOverRail; // 0x144
    bool m_bHeroPacifyPushedNPC; // 0x145
    bool m_bHeroGiveFocusOnDumpBody; // 0x146
    bool m_bHeroDumpedBodyHidden; // 0x147
    bool m_bHeroCanDumpBody; // 0x148
    bool m_bIllegal; // 0x149
    TArray<TEntityRef<ZOutfitProfessionEntity>> m_bLegalProfessions; // 0x150
    bool m_bHeroCanTakedown; // 0x168
    bool m_bIsVrHintStayInAgility; // 0x169

    inline static float s_fCoverClearZoneDepth = 1.5f;
    inline static float s_fCoverClearZoneDepthHigh = 1.f;

    inline static float s_fLowCoverHeight = 1.f;
};

class ZCoverPlaneDebug {
public:
    enum class ENPCValidationError
    {
        VE_OK,
        VE_RightSideBlocked,
        VE_LeftSideBlocked,
        VE_OverBlocked,
        VE_TooShort,
        VE_NoValidCorner
    };

    static ENPCValidationError CheckNPCValidity(ZCoverPlane* p_CoverPlane, bool p_PrintErrors = false) {
        const bool s_IsCoverUsable =
            p_CoverPlane->m_bNPCUseCover &&
            (p_CoverPlane->m_bStreamStateReady || p_CoverPlane->m_bActivated) &&
            p_CoverPlane->m_eCoverType <= ZCoverPlane::ECoverType::eCoverAndRail;

        if (!s_IsCoverUsable) {
            return ENPCValidationError::VE_OK;
        }

        ENPCValidationError s_NPCValidationError = ENPCValidationError::VE_OK;

        if (p_CoverPlane->m_fCoverLength < 0.75f) {
            s_NPCValidationError = ENPCValidationError::VE_TooShort;
        }

        if ((p_CoverPlane->m_eCoverSize != ZCoverPlane::ECoverSize::eLowCover || !p_CoverPlane->m_bNPCShootOverCover) &&
            !p_CoverPlane->AIIsCornerValid(ZCoverPlane::ESide::eSideLeft) &&
            !p_CoverPlane->AIIsCornerValid(ZCoverPlane::ESide::eSideRight)) {
            s_NPCValidationError = ENPCValidationError::VE_NoValidCorner;
        }

        if (s_NPCValidationError == ENPCValidationError::VE_OK) {
            bool s_RightSideClear = true;
            bool s_LeftSideClear = true;
            bool s_OverCoverClear = true;

            static float4 s_ZOffset = float4(0.f, 0.f, 1.2f, 0.f);
            static float4 s_LowCornerZOffset = float4(0.f, 0.f, 1.2f, 0.f);

            if (p_CoverPlane->AIIsCornerValid(ZCoverPlane::ESide::eSideRight)) {
                const float4 s_ShootPosition = p_CoverPlane->AIGetShootTestPosition(ZCoverPlane::ESide::eSideRight);
                const float4 s_ZOffset2 = (p_CoverPlane->m_eCoverSize == ZCoverPlane::ECoverSize::eLowCover) ? s_LowCornerZOffset : s_ZOffset;

                const float4 s_Start = s_ShootPosition + s_ZOffset2;
                const float4 s_End = s_Start - p_CoverPlane->m_vCoverDir;

                s_RightSideClear = ZCoverPlaneDebug::HasClearLineOfFire(s_Start, s_End);
            }

            if (p_CoverPlane->AIIsCornerValid(ZCoverPlane::ESide::eSideLeft)) {
                const float4 s_ShootPosition = p_CoverPlane->AIGetShootTestPosition(ZCoverPlane::ESide::eSideLeft);
                const float4 s_ZOffset2 = (p_CoverPlane->m_eCoverSize == ZCoverPlane::ECoverSize::eLowCover) ? s_LowCornerZOffset : s_ZOffset;

                const float4 s_Start = s_ShootPosition + s_ZOffset2;
                const float4 s_End = s_Start - p_CoverPlane->m_vCoverDir;

                s_LeftSideClear = ZCoverPlaneDebug::HasClearLineOfFire(s_Start, s_End);
            }

            if (p_CoverPlane->m_eCoverSize == ZCoverPlane::ECoverSize::eLowCover && p_CoverPlane->m_bNPCShootOverCover) {
                const float s_NPCRadiusY = p_CoverPlane->AIGetNPCRadiusY();
                const float4 s_ForwardOffset = p_CoverPlane->m_vCoverDir * s_NPCRadiusY;

                const float4 s_LeftEdge = p_CoverPlane->GetEdgePointWorld(ZCoverPlane::ESide::eSideLeft) + s_ForwardOffset;
                const float4 s_RightEdge = p_CoverPlane->GetEdgePointWorld(ZCoverPlane::ESide::eSideRight) + s_ForwardOffset;

                const float4 s_Direction = s_RightEdge - s_LeftEdge;
                const int sampleCount = std::clamp(static_cast<int>(p_CoverPlane->m_fCoverLength * 2.f), 1, 3);

                s_OverCoverClear = false;

                for (int i = 0; i < sampleCount; ++i) {
                    const float t = sampleCount == 1 ? 0.5f : static_cast<float>(i) / (sampleCount - 1);
                    const float offsetT = t * 0.8f + 0.1f;

                    const float4 s_SamplePosition = s_LeftEdge + s_Direction * offsetT + s_LowCornerZOffset;
                    const float4 s_End = s_SamplePosition - p_CoverPlane->m_vCoverDir * 2.f;

                    if (ZCoverPlaneDebug::HasClearLineOfFire(s_SamplePosition, s_End)) {
                        s_OverCoverClear = true;
                        break;
                    }
                }
            }

            if (!s_RightSideClear) {
                s_NPCValidationError = ENPCValidationError::VE_RightSideBlocked;
            }
            else if (!s_LeftSideClear) {
                s_NPCValidationError = ENPCValidationError::VE_LeftSideBlocked;
            }
            else if (!s_OverCoverClear) {
                s_NPCValidationError = ENPCValidationError::VE_OverBlocked;
            }
        }

        if (p_PrintErrors && s_NPCValidationError != ENPCValidationError::VE_OK) {
            const char* s_Message = nullptr;

            switch (s_NPCValidationError) {
                case ENPCValidationError::VE_RightSideBlocked:
                    s_Message = "Coverplane invalid for NPC's: right side line-of-fire blocked!";
                    break;
                case ENPCValidationError::VE_LeftSideBlocked:
                    s_Message = "Coverplane invalid for NPC's: left side line-of-fire blocked!";
                    break;
                case ENPCValidationError::VE_OverBlocked:
                    s_Message = "Coverplane invalid for NPC's: line-of-fire blocked from over cover!";
                    break;
                case ENPCValidationError::VE_TooShort:
                    s_Message = "Coverplane invalid for NPC's: cover is too short (minimum 0.75m)!";
                    break;
                case ENPCValidationError::VE_NoValidCorner:
                    s_Message = "Coverplane invalid for NPC's: no valid cover corners to shoot from!";
                    break;
                default:
                    s_Message = "Coverplane invalid for NPC's: unknown error!";
                    break;
            }

            Logger::Error("[LevelSetup] {}", s_Message);
        }

        return s_NPCValidationError;
    }

    static bool HasClearLineOfFire(const float4& p_Start, const float4& p_End) {
        if (!*Globals::CollisionManager) {
            Logger::Error("Collision manager is null!");

            return false;
        }

        ZRayQueryInput s_RayInput{};
        s_RayInput.m_vFrom = p_Start;
        s_RayInput.m_vTo = p_End;
        s_RayInput.m_nRayFilter = (*Globals::CollisionManager)->GetCollisionFilter(
            static_cast<uint16>(ERayLayer::eRayLayer_ACTOR_LINE_OF_FIRE)
        );

        ZRayQueryOutput s_RayOutput{};
        (*Globals::CollisionManager)->RayCastClosestHit(s_RayInput, &s_RayOutput);

        return !s_RayOutput.m_bHasHit;
    }

    static void DrawDebugCoverValid(
        ZCoverPlane* p_CoverPlane,
        IRenderer* p_Renderer,
        const bool p_DrawHeroGuidesSolid,
        const bool p_AlwaysDrawDebugBoxForColiValidityCheck
    ) {
        float4 s_EdgePoints[4];

        p_CoverPlane->CalcEdgePoints(s_EdgePoints);

        uint32_t s_Color = 0xFF808080;

        if (p_CoverPlane->m_bEnabled) {
            s_Color = 0xFFFF0000;
        }

        if (p_CoverPlane->IsCoverPlaneACoverSpot()) {
            const auto& s_LeftNeighbor = p_CoverPlane->m_Neighbors[0];
            const auto& s_RightNeighbor = p_CoverPlane->m_Neighbors[1];

            const bool isLeftNeighborInvalid =
                !s_LeftNeighbor.m_pNPlane.m_pInterfaceRef ||
                s_LeftNeighbor.m_NState == ENeigborState::eInvalid;

            const bool isrightNeighborInvalid =
                !s_RightNeighbor.m_pNPlane.m_pInterfaceRef ||
                s_RightNeighbor.m_NState == ENeigborState::eInvalid;

            if (isLeftNeighborInvalid && isrightNeighborInvalid) {
                s_Color = 0xFFFFFF00;
            }
        }

        switch (p_CoverPlane->m_eCoverType) {
            case ZCoverPlane::ECoverType::eRailOnly:
                s_Color = 0xF0F077FF;
                break;
            case ZCoverPlane::ECoverType::eCoverAndRail:
                s_Color = 0xFF0063A5;
                break;
        }

        const SVector4 s_Color2 = ZColor::UnpackUnsigned(s_Color);
        static const SVector4 s_Green = ZColor::UnpackUnsigned(0xFF00FF00);

        if (p_DrawHeroGuidesSolid) {
            p_Renderer->DrawLine3D(s_EdgePoints[0], s_EdgePoints[2], s_Green, s_Green);

            p_Renderer->DrawTriangle3D(
                s_EdgePoints[1], s_Color2,
                s_EdgePoints[0], s_Color2,
                s_EdgePoints[2], s_Color2
            );

            p_Renderer->DrawTriangle3D(
                s_EdgePoints[2], s_Color2,
                s_EdgePoints[3], s_Color2,
                s_EdgePoints[1], s_Color2
            );
        }
        else {
            p_Renderer->DrawLine3D(s_EdgePoints[0], s_EdgePoints[2], s_Green, s_Green);
            p_Renderer->DrawLine3D(s_EdgePoints[1], s_EdgePoints[3], s_Color2, s_Color2);
            p_Renderer->DrawLine3D(s_EdgePoints[0], s_EdgePoints[1], s_Color2, s_Color2);
            p_Renderer->DrawLine3D(s_EdgePoints[2], s_EdgePoints[3], s_Color2, s_Color2);
        }

        if (p_AlwaysDrawDebugBoxForColiValidityCheck) {
            static const SVector4 s_Green = ZColor::UnpackUnsigned(0x8800FF00);
            SBox s_Box = GetPlaneCollisionWorldBox(p_CoverPlane);

            const float4 s_Center = s_Box.m_mvCenterMatPos.Trans;
            const float4 s_HalfSize = s_Box.m_vHalfSize;

            if (p_DrawHeroGuidesSolid) {
                p_Renderer->DrawBox3D(s_Center, s_HalfSize, s_Box.m_mvCenterMatPos, s_Green);
            }
            else {
                p_Renderer->DrawBoxWire3D(s_Center, s_HalfSize, s_Box.m_mvCenterMatPos, s_Green);
            }
        }
    }

    static SBox GetPlaneCollisionWorldBox(ZCoverPlane* p_CoverPlane) {
        static const float s_CoverDistanceThreshold = 0.15f;
        static const SVector3 s_UpAlign(0.f, 0.f, 0.7f);

        const SVector3 s_Up = SVector3::CrossProduct(p_CoverPlane->m_vCoverSideDir, p_CoverPlane->m_vCoverDir);
        const float4 s_HalfSize = float4((p_CoverPlane->m_fCoverLength - 0.4f) * 0.5f, 0.2125f, 0.025f, 0.f);
        const SVector3 s_Position = s_Up * s_UpAlign.z +
            p_CoverPlane->m_vCoverPos +
            p_CoverPlane->m_vCoverDir * s_HalfSize.y +
            p_CoverPlane->m_vCoverDir * s_CoverDistanceThreshold;

        SBox box;

        box.m_mvCenterMatPos = SMatrix(
            p_CoverPlane->m_vCoverSideDir,
            p_CoverPlane->m_vCoverDir,
            s_Up,
            float4(s_Position, 1.f)
        );
        box.m_vHalfSize = s_HalfSize;

        return box;
    }

    static void DrawDebugNeighborConnectedState(
        ZCoverPlane* p_CoverPlane,
        IRenderer* p_Renderer,
        const bool p_DrawHeroGuidesSolid
    ) {
        static const SVector4 s_Green = ZColor::UnpackUnsigned(0xFF00FF00);
        const float4 s_From = float4(p_CoverPlane->m_vCoverPos, 1.f);
        const float4 s_To = s_From + p_CoverPlane->m_vCoverDir;

        p_Renderer->DrawLine3D(s_From, s_To, s_Green, s_Green);

        const float4 s_ZOffset(0.f, 0.f, ZCoverPlane::s_fLowCoverHeight * 0.5f, 0.f);

        for (int i = 0; i < 2; ++i) {
            const auto& s_Neighbor = p_CoverPlane->m_Neighbors[i];

            if (!s_Neighbor.m_pNPlane.m_pInterfaceRef) {
                continue;
            }

            const ZCoverPlane::ESide s_Side =
                i == 0 ? ZCoverPlane::ESide::eSideRight : ZCoverPlane::ESide::eSideLeft;

            const float4 s_edgePosition = p_CoverPlane->GetEdgePointWorld(s_Side);
            const float4 s_BoxPosition = s_edgePosition + s_ZOffset;

            static const float4 s_EdgeBoxSizeValid(0.07f, 0.07f, 0.07f, 0.f);
            static const float4 s_EdgeBoxSizeInvalid(0.25f, 0.25f, 0.25f, 0.f);

            static const SMatrix s_Identity = SMatrix();

            if (s_Neighbor.m_NState == ENeigborState::eValid) {
                static const SVector4 s_White = ZColor::UnpackUnsigned(0xFFFFFFFF);

                if (p_DrawHeroGuidesSolid) {
                    p_Renderer->DrawBox3D(s_BoxPosition, s_EdgeBoxSizeValid * 0.5f, s_Identity, s_White);
                }
                else {
                    p_Renderer->DrawBoxWire3D(s_BoxPosition, s_EdgeBoxSizeValid * 0.5f, s_Identity, s_White);
                }
            }
            else if (s_Neighbor.m_NState == ENeigborState::eInvalid) {
                static const SVector4 s_Red = ZColor::UnpackUnsigned(0xFF0000FF);

                if (p_DrawHeroGuidesSolid) {
                    p_Renderer->DrawBox3D(s_BoxPosition, s_EdgeBoxSizeInvalid * 0.5f, s_Identity, s_Red);
                }
                else {
                    p_Renderer->DrawBoxWire3D(s_BoxPosition, s_EdgeBoxSizeInvalid * 0.5f, s_Identity, s_Red);
                }
            }
        }
    }

    static void DrawDebugCorners(ZCoverPlane* p_CoverPlane, IRenderer* p_Renderer, bool p_DrawHeroGuidesSolid) {
        SCoverCorner corners[2];
        corners[0] = p_CoverPlane->UnpackCoverCorner(ZCoverPlane::ESide::eSideRight);
        corners[1] = p_CoverPlane->UnpackCoverCorner(ZCoverPlane::ESide::eSideLeft);

        const SMatrix s_World = p_CoverPlane->GetObjectToWorldMatrix();

        static const float s_PullInLength = 0.15f;
        static const SVector3 s_ZOffset(0.f, 0.f, 0.25f);
        static const SVector3 s_CornerSpotBoxSize(0.05f, 0.05f, 0.5f);
        static const SVector3 s_CornerBoxSize(0.05f, 0.05f, 0.05f);

        static const SVector4 s_Red = ZColor::UnpackUnsigned(0xFF0000FF);
        static const SVector4 s_Green = ZColor::UnpackUnsigned(0xFF00FF00);
        static const SVector4 s_Blue = ZColor::UnpackUnsigned(0xFFFF0000);
        static const SVector4 s_White = ZColor::UnpackUnsigned(0xFFFFFFFF);

        for (int i = 0; i < 2; ++i) {
            const SCoverCorner& s_Corner = corners[i];

            if (!s_Corner.m_bValid) {
                continue;
            }

            const SVector3 s_Flat = { p_CoverPlane->m_vCoverSideDir.x, p_CoverPlane->m_vCoverSideDir.y, 0.f };
            const SVector3 s_Normalized = s_Flat.Normalized();

            SVector3 s_PullIn = s_Normalized.SetLength(SVector3::DotProduct(s_Flat, s_Normalized) * s_PullInLength);

            if (i == 0) {
                s_PullIn = -s_PullIn;
            }

            const ZCoverPlane::ESide s_Side = (i == 0)
                ? ZCoverPlane::ESide::eSideRight
                : ZCoverPlane::ESide::eSideLeft;

            const float4 s_EdgePoint = p_CoverPlane->GetEdgePointWorld(s_Side);
            const float4 s_DebugCornerPos = s_EdgePoint + s_PullIn + s_ZOffset;

            const SVector4 s_SpotColor = s_Corner.m_bDisabled ? s_Red : s_Green;

            if (p_DrawHeroGuidesSolid) {
                p_Renderer->DrawBox3D(s_DebugCornerPos, s_CornerSpotBoxSize * 0.5f, s_World, s_SpotColor);
            }
            else {
                p_Renderer->DrawBoxWire3D(s_DebugCornerPos, s_CornerSpotBoxSize * 0.5f, s_World, s_SpotColor);
            }

            const SVector4 s_CornerColor = i == 1 ? s_White : s_Blue;

            if (p_DrawHeroGuidesSolid) {
                p_Renderer->DrawBox3D(s_Corner.m_vCornerPoint, s_CornerBoxSize * 0.5f, s_World, s_CornerColor);
            }
            else {
                p_Renderer->DrawBoxWire3D(s_Corner.m_vCornerPoint, s_CornerBoxSize * 0.5f, s_World, s_CornerColor);
            }
        }
    }

    static void DrawDebugHeroPushNPCOverRail(
        ZCoverPlane* p_CoverPlane,
        IRenderer* p_Renderer,
        bool p_DrawHeroGuidesSolid
    ) {
        if (!p_CoverPlane->m_bHeroPushNPCOverRail ||
            p_CoverPlane->m_eCoverSize == ZCoverPlane::ECoverSize::eHighCover ||
            p_CoverPlane->m_eCoverType == ZCoverPlane::ECoverType::eCoverOnly) {
            return;
        }

        static const float s_PullInLength = 0.15f;

        const SMatrix s_World = p_CoverPlane->GetObjectToWorldMatrix();
        const float4 s_WorldDirection = p_CoverPlane->GetWorldDirection(SVector3(0.f, 0.f, 1.f));
        const float4 s_WorldPosition = s_World.Trans +
            s_WorldDirection * (p_CoverPlane->GetCoverHeight() - s_PullInLength);

        const SVector3 s_Size = SVector3(p_CoverPlane->m_fCoverLength, 0.05f, 0.05f);
        static const SVector4 s_DarkBlue = ZColor::UnpackUnsigned(0xFF990000);

        if (p_DrawHeroGuidesSolid) {
            p_Renderer->DrawBox3D(s_WorldPosition, s_Size * 0.5f, s_World, s_DarkBlue);
        }
        else {
            p_Renderer->DrawBoxWire3D(s_WorldPosition, s_Size * 0.5f, s_World, s_DarkBlue);
        }
    }

    static void DrawDebugRailDebug(ZCoverPlane* p_CoverPlane, IRenderer* p_Renderer) {
        if (p_CoverPlane->m_eCoverType == ZCoverPlane::ECoverType::eCoverOnly) {
            return;
        }

        float4 s_EdgePoints[4];

        p_CoverPlane->CalcEdgePoints(s_EdgePoints);

        static const SVector4 s_Orange = ZColor::UnpackUnsigned(0xFF0077FF);

        p_Renderer->DrawLine3D(s_EdgePoints[0], s_EdgePoints[3], s_Orange, s_Orange);
        p_Renderer->DrawLine3D(s_EdgePoints[1], s_EdgePoints[2], s_Orange, s_Orange);
    }

    static void DrawDebugWindowDebug(ZCoverPlane* p_CoverPlane, IRenderer* p_Renderer) {
        if (p_CoverPlane->m_eCoverSize != ZCoverPlane::ECoverSize::eLowCover || !p_CoverPlane->m_bWindowCover) {
            return;
        }

        float4 s_EdgePoints[4];

        p_CoverPlane->CalcEdgePoints(s_EdgePoints);

        static const float4 s_UnitZ(0.f, 0.f, 1.f, 0.f);
        static const float4 s_HalfOfUnitZ(0.f, 0.f, 0.5f, 0.f);

        const float4 s_Mid = s_EdgePoints[0] + (s_EdgePoints[2] - s_EdgePoints[0]) * 0.5f;

        static const SVector4 s_GreenCyan = ZColor::UnpackUnsigned(0xFF3FFF00);

        p_Renderer->DrawLine3D(s_Mid, s_Mid + s_UnitZ, s_GreenCyan, s_GreenCyan);
        p_Renderer->DrawLine3D(s_EdgePoints[0] + s_HalfOfUnitZ, s_EdgePoints[2] + s_HalfOfUnitZ, s_GreenCyan, s_GreenCyan);
    }

    static void DrawDebugPushDebug(
        ZCoverPlane* p_CoverPlane,
        IRenderer* p_Renderer,
        bool p_DrawHeroGuidesSolid,
        bool p_DrawPushDebug
    ) {
        if (!p_DrawPushDebug) {
            return;
        }

        if (p_CoverPlane->m_eCoverSize == ZCoverPlane::ECoverSize::eHighCover ||
            p_CoverPlane->m_eCoverType == ZCoverPlane::ECoverType::eCoverOnly) {
            return;
        }

        static const SVector4 s_Green = ZColor::UnpackUnsigned(0xFF00FF00);
        static const SVector4 s_Red = ZColor::UnpackUnsigned(0xFF0000FF);
        const SVector4 s_Color = p_CoverPlane->m_bHeroPushNPCOverRail ? s_Green : s_Red;

        float4 s_EdgePoints[4];

        p_CoverPlane->CalcEdgePoints(s_EdgePoints);

        const float s_Height = p_CoverPlane->GetCoverHeight();
        static const float s_Depth = ZCoverPlane::s_fCoverClearZoneDepth;

        static const float4 s_Up = float4(0.f, 0.f, 1.f, 0.f);
        const float4 s_Forward = -p_CoverPlane->m_vCoverDir;
        const float4 s_Down = s_Up * -1.5f;

        const float4 s_Top = s_Up * s_Height;
        const float4 s_DepthOffset = s_Forward * s_Depth;

        const float4 s_TopFront = s_Top;
        const float4 s_TopBack = s_Top + s_DepthOffset;
        const float4 s_BottomFront = s_Down;
        const float4 s_BottomBack = s_Down + s_DepthOffset;

        const float4 s_L1 = s_EdgePoints[0] + s_TopFront;
        const float4 s_L2 = s_EdgePoints[0] + s_TopBack;
        const float4 s_L3 = s_EdgePoints[0] + s_BottomBack;
        const float4 s_L4 = s_EdgePoints[0] + s_BottomFront;

        const float4 s_R1 = s_EdgePoints[2] + s_TopFront;
        const float4 s_R2 = s_EdgePoints[2] + s_TopBack;
        const float4 s_R3 = s_EdgePoints[2] + s_BottomBack;
        const float4 s_R4 = s_EdgePoints[2] + s_BottomFront;

        if (p_DrawHeroGuidesSolid) {
            // Top face
            p_Renderer->DrawTriangle3D(s_R1, s_Color, s_R2, s_Color, s_L1, s_Color);
            p_Renderer->DrawTriangle3D(s_L1, s_Color, s_L2, s_Color, s_R2, s_Color);

            // Left face
            p_Renderer->DrawTriangle3D(s_L1, s_Color, s_L2, s_Color, s_L3, s_Color);
            p_Renderer->DrawTriangle3D(s_L1, s_Color, s_L4, s_Color, s_L3, s_Color);

            // Right face
            p_Renderer->DrawTriangle3D(s_R1, s_Color, s_R4, s_Color, s_R3, s_Color);
            p_Renderer->DrawTriangle3D(s_R1, s_Color, s_R2, s_Color, s_R3, s_Color);

            // Bottom face
            p_Renderer->DrawTriangle3D(s_R4, s_Color, s_R3, s_Color, s_L4, s_Color);
            p_Renderer->DrawTriangle3D(s_L4, s_Color, s_L3, s_Color, s_R3, s_Color);

            // Back face
            p_Renderer->DrawTriangle3D(s_R1, s_Color, s_R4, s_Color, s_L4, s_Color);
            p_Renderer->DrawTriangle3D(s_L1, s_Color, s_L4, s_Color, s_R1, s_Color);

            // Front face
            p_Renderer->DrawTriangle3D(s_R2, s_Color, s_R3, s_Color, s_L3, s_Color);
            p_Renderer->DrawTriangle3D(s_L2, s_Color, s_L3, s_Color, s_R2, s_Color);
        }
        else {
            p_Renderer->DrawLine3D(s_L1, s_L2, s_Color, s_Color);
            p_Renderer->DrawLine3D(s_L2, s_R2, s_Color, s_Color);
            p_Renderer->DrawLine3D(s_R1, s_R2, s_Color, s_Color);
            p_Renderer->DrawLine3D(s_R1, s_L1, s_Color, s_Color);
            p_Renderer->DrawLine3D(s_L3, s_L4, s_Color, s_Color);
            p_Renderer->DrawLine3D(s_L4, s_R4, s_Color, s_Color);
            p_Renderer->DrawLine3D(s_R3, s_R4, s_Color, s_Color);
            p_Renderer->DrawLine3D(s_R3, s_L3, s_Color, s_Color);
            p_Renderer->DrawLine3D(s_L1, s_L4, s_Color, s_Color);
            p_Renderer->DrawLine3D(s_L2, s_L3, s_Color, s_Color);
            p_Renderer->DrawLine3D(s_R1, s_R4, s_Color, s_Color);
            p_Renderer->DrawLine3D(s_R2, s_R3, s_Color, s_Color);
        }
    }

    static void DrawDebugSafeZones(
        ZCoverPlane* p_CoverPlane,
        IRenderer* p_Renderer,
        bool p_DrawHeroGuidesSolid,
        bool p_DrawSafeZones
    ) {
        if (!p_DrawSafeZones) {
            return;
        }

        float4 s_EdgePoints[4];
        p_CoverPlane->CalcEdgePoints(s_EdgePoints);

        const float s_Depth =
            p_CoverPlane->m_eCoverSize == ZCoverPlane::ECoverSize::eHighCover
            ? ZCoverPlane::s_fCoverClearZoneDepthHigh
            : ZCoverPlane::s_fCoverClearZoneDepth;

        const float4 s_Forward = p_CoverPlane->m_vCoverDir;
        const float4 s_ForwardOffset = s_Forward * s_Depth;

        const float4 s_FrontL = s_EdgePoints[0];
        const float4 s_FrontR = s_EdgePoints[2];
        const float4 s_BackL = s_FrontL + s_ForwardOffset;
        const float4 s_BackR = s_FrontR + s_ForwardOffset;

        const uint32_t s_FrontColorValue = p_DrawHeroGuidesSolid ? 0x7FFF0077 : p_CoverPlane->m_bEnabled
            ? 0xFFFF0000
            : 0xFF808080;

        const SVector4 s_FrontColor = ZColor::UnpackUnsigned(s_FrontColorValue);

        if (p_DrawHeroGuidesSolid) {
            p_Renderer->DrawTriangle3D(s_FrontL, s_FrontColor, s_BackL, s_FrontColor, s_FrontR, s_FrontColor);
            p_Renderer->DrawTriangle3D(s_FrontR, s_FrontColor, s_BackL, s_FrontColor, s_BackR, s_FrontColor);
        }
        else {
            p_Renderer->DrawLine3D(s_FrontL, s_BackL, s_FrontColor, s_FrontColor);
            p_Renderer->DrawLine3D(s_FrontR, s_BackR, s_FrontColor, s_FrontColor);
            p_Renderer->DrawLine3D(s_BackL, s_BackR, s_FrontColor, s_FrontColor);
        }

        if (p_CoverPlane->m_fCoverDepth > 0.0f) {
            const float4 s_BackOffset = -s_Forward * p_CoverPlane->m_fCoverDepth;

            const float4 s_RearL = s_EdgePoints[1];
            const float4 s_RearR = s_EdgePoints[3];
            const float4 s_RearBackL = s_RearL + s_BackOffset;
            const float4 s_RearBackR = s_RearR + s_BackOffset;

            const SVector4 s_RearColor = ZColor::UnpackUnsigned(
                p_DrawHeroGuidesSolid ? 0x7F00FF00 : 0xFF00FF00
            );

            if (p_DrawHeroGuidesSolid) {
                p_Renderer->DrawTriangle3D(s_RearBackL, s_RearColor, s_RearL, s_RearColor, s_RearR, s_RearColor);
                p_Renderer->DrawTriangle3D(s_RearR, s_RearColor, s_RearBackR, s_RearColor, s_RearBackL, s_RearColor);
            }
            else {
                p_Renderer->DrawLine3D(s_RearL, s_RearBackL, s_RearColor, s_RearColor);
                p_Renderer->DrawLine3D(s_RearR, s_RearBackR, s_RearColor, s_RearColor);
                p_Renderer->DrawLine3D(s_RearBackL, s_RearBackR, s_RearColor, s_RearColor);
            }
        }
    }

    static void DrawDebugNPCFlags(
        ZCoverPlane* p_CoverPlane,
        IRenderer* p_Renderer,
        bool p_DrawNPCFlags
    ) {
        if (!p_DrawNPCFlags) {
            return;
        }

        static const SVector4 s_White = ZColor::UnpackUnsigned(0xFFFFFFFF);
        static const SVector4 s_Green = ZColor::UnpackUnsigned(0xFF00FF00);
        static const SVector4 s_Red = ZColor::UnpackUnsigned(0xFF0000FF);

        const SMatrix& s_WorldMatrix = p_CoverPlane->GetObjectToWorldMatrix();

        const float4 s_StartDir = s_WorldMatrix.XAxis;
        const float4 s_Normal = -s_WorldMatrix.YAxis;

        const float4 s_Center =
            p_CoverPlane->GetObjectToWorldMatrix().Trans +
            s_WorldMatrix.YAxis * 0.06f +
            float4(0.f, 0.f, ZCoverPlane::s_fLowCoverHeight * 0.5f, 0.f);

        const float4 s_OuterSize(0.08f, 0.08f, 0.f, 0.f);
        const float4 s_InnerSize(0.05f, 0.05f, 0.f, 0.f);

        p_Renderer->DrawCircle3D(
            s_Center,
            s_OuterSize,
            s_Normal,
            s_StartDir,
            s_White
        );

        if (p_CoverPlane->m_eCoverType <= ZCoverPlane::ECoverType::eCoverAndRail) {
            p_Renderer->DrawCircle3D(
                s_Center,
                s_InnerSize,
                s_Normal,
                s_StartDir,
                p_CoverPlane->m_bNPCUseCover ? s_Green : s_Red
            );

            if (p_CoverPlane->m_bNPCUseCover &&
                p_CoverPlane->m_eCoverSize == ZCoverPlane::ECoverSize::eLowCover) {
                static const float s_CircleOffset = 2.f * std::sqrt(2.f * 0.0064f);

                p_Renderer->DrawCircle3D(
                    s_Center + float4(0.f, 0.f, s_CircleOffset, 0.f),
                    s_InnerSize,
                    s_Normal,
                    s_StartDir,
                    p_CoverPlane->m_bNPCShootOverCover ? s_Green : s_Red
                );
            }
        }
    }

    static void DrawDebugNPCValidation(
        ZCoverPlane* p_CoverPlane,
        IRenderer* p_Renderer,
        bool p_DrawNPCValidation
    ) {
        if (!p_DrawNPCValidation) {
            return;
        }

        const auto s_ValidationError = CheckNPCValidity(p_CoverPlane);

        if (s_ValidationError == ENPCValidationError::VE_OK) {
            return;
        }

        const SMatrix& s_WorldMatrix = p_CoverPlane->GetObjectToWorldMatrix();

        SMatrix s_Matrix = s_WorldMatrix;

        s_Matrix.XAxis = -s_WorldMatrix.XAxis;
        s_Matrix.YAxis = s_WorldMatrix.ZAxis;
        s_Matrix.ZAxis = -s_WorldMatrix.YAxis;

        s_Matrix.Trans =
            p_CoverPlane->GetObjectToWorldMatrix().Trans +
            s_WorldMatrix.YAxis * 0.05f +
            float4(0.f, 0.f, 0.6f, 0.f);

        const uint32_t s_Color =
            s_ValidationError == ENPCValidationError::VE_TooShort ||
            s_ValidationError == ENPCValidationError::VE_NoValidCorner
            ? 0xFF00FFFF
            : 0xFF000080;

        const char* s_ValidationMessage = nullptr;

        switch (s_ValidationError) {
            case ENPCValidationError::VE_TooShort:
                s_ValidationMessage = "Cover too short";
                break;

            case ENPCValidationError::VE_NoValidCorner:
                s_ValidationMessage = "No valid corner";
                break;

            case ENPCValidationError::VE_LeftSideBlocked:
                s_ValidationMessage = "Left LOF blocked";
                break;

            case ENPCValidationError::VE_RightSideBlocked:
                s_ValidationMessage = "Right LOF blocked";
                break;

            case ENPCValidationError::VE_OverBlocked:
                s_ValidationMessage = "Over LOF blocked";
                break;

            default:
                s_ValidationMessage = "?";
                break;
        }

        p_Renderer->DrawText3D(
            "Bad NPC cover",
            s_Matrix,
            ZColor::UnpackUnsigned(s_Color),
            0.2f,
            TextAlignment::Left,
            TextAlignment::Center,
            false
        );

        s_Matrix.Trans -= float4(0.f, 0.f, 0.2f, 0.f);

        p_Renderer->DrawText3D(
            s_ValidationMessage,
            s_Matrix,
            ZColor::UnpackUnsigned(s_Color),
            0.2f,
            TextAlignment::Left,
            TextAlignment::Center,
            false
        );
    }

    static void DrawDebugCoverInvalid(
        ZCoverPlane* p_CoverPlane,
        IRenderer* p_Renderer,
        bool p_DrawHeroGuidesSolid,
        bool p_AlwaysDrawDebugBoxForCollisionValidityCheck
    ) {
        float4 s_EdgePoints[4];
        p_CoverPlane->CalcEdgePoints(s_EdgePoints);

        const bool s_IsInvalid =
            p_CoverPlane->m_bInvalidCoverFacing ||
            !p_CoverPlane->m_bStartEnabled;

        const SVector4 s_Color = ZColor::UnpackUnsigned(
            s_IsInvalid ? 0xFF00FFFF : 0xFF0000FF
        );

        if (p_DrawHeroGuidesSolid) {
            p_Renderer->DrawTriangle3D(
                s_EdgePoints[1], s_Color,
                s_EdgePoints[0], s_Color,
                s_EdgePoints[2], s_Color
            );

            p_Renderer->DrawTriangle3D(
                s_EdgePoints[2], s_Color,
                s_EdgePoints[3], s_Color,
                s_EdgePoints[1], s_Color
            );

            if (!s_IsInvalid) {
                const float s_Depth =
                    p_CoverPlane->m_eCoverSize == ZCoverPlane::ECoverSize::eHighCover
                    ? ZCoverPlane::s_fCoverClearZoneDepthHigh
                    : ZCoverPlane::s_fCoverClearZoneDepth;

                const float4 s_Offset = float4(
                    p_CoverPlane->m_vCoverDir.x,
                    p_CoverPlane->m_vCoverDir.y,
                    0.f,
                    0.f
                ) * s_Depth;

                const float4 s_BackLeft = s_EdgePoints[0] + s_Offset;
                const float4 s_BackRight = s_EdgePoints[2] + s_Offset;

                p_Renderer->DrawTriangle3D(
                    s_BackLeft, s_Color,
                    s_EdgePoints[0], s_Color,
                    s_EdgePoints[2], s_Color
                );

                p_Renderer->DrawTriangle3D(
                    s_EdgePoints[2], s_Color,
                    s_BackRight, s_Color,
                    s_BackLeft, s_Color
                );
            }
        }
        else {
            p_Renderer->DrawLine3D(
                s_EdgePoints[0], s_EdgePoints[2],
                s_Color, s_Color
            );

            p_Renderer->DrawLine3D(
                s_EdgePoints[1], s_EdgePoints[3],
                s_Color, s_Color
            );

            p_Renderer->DrawLine3D(
                s_EdgePoints[0], s_EdgePoints[1],
                s_Color, s_Color
            );

            p_Renderer->DrawLine3D(
                s_EdgePoints[2], s_EdgePoints[3],
                s_Color, s_Color
            );

            if (!p_CoverPlane->m_bPlaneCollisionObjectFree) {
                p_Renderer->DrawLine3D(
                    s_EdgePoints[0], s_EdgePoints[3],
                    s_Color, s_Color
                );

                p_Renderer->DrawLine3D(
                    s_EdgePoints[1], s_EdgePoints[2],
                    s_Color, s_Color
                );
            }
        }

        if (p_AlwaysDrawDebugBoxForCollisionValidityCheck || !p_CoverPlane->m_bPlaneCollisionObjectFree) {
            const SBox s_Box = GetPlaneCollisionWorldBox(p_CoverPlane);

            const SVector4 s_BoxColor = ZColor::UnpackUnsigned(
                p_CoverPlane->m_bPlaneCollisionObjectFree
                ? 0x8800FF00
                : 0x88000088
            );

            const float4 s_Center = s_Box.m_mvCenterMatPos.Trans;
            const float4 s_HalfSize = s_Box.m_vHalfSize;

            if (p_DrawHeroGuidesSolid) {
                p_Renderer->DrawBox3D(
                    s_Center,
                    s_HalfSize,
                    s_Box.m_mvCenterMatPos,
                    s_BoxColor
                );
            }
            else {
                p_Renderer->DrawBoxWire3D(
                    s_Center,
                    s_HalfSize,
                    s_Box.m_mvCenterMatPos,
                    s_BoxColor
                );
            }
        }
    }
};