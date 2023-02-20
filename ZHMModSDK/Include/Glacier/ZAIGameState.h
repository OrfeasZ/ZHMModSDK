#pragma once

#include "ZEntity.h"
#include "ZPrimitives.h"
#include "Enums.h"

struct SActorCounts
{
    int m_nEnemiesIsAlerted; // 40
    int m_nEnemiesIsAlertedArmed; // 44
    int m_nEnemiesIsEngaged; // 48
    int m_nLastEnemyKilled; // 52
    int m_nEnemiesInCombat; // 56
    int m_nAlertableActorOnScreen; // 60
    int m_nAlertableGuardOnScreen; // 64
};

class ZAIEventEmitterEntity;

class ZAIGameState
{
public:
    float m_fDisguiseHealth; // 0
    STokenID m_outfitToken; // 8
    PAD(20); // 16
    int m_nUnconsciousWitnesses; // 36
    SActorCounts m_actorCounts; // 40
    EGameTension m_eGameTension; // 68
    EGameTension m_eGameTensionGuard; // 72
    EGameTension m_eGameTensionCivilian; // 76
    EGameTension m_eGameTensionToHitmanGuard; // 80
    EGameTension m_eGameTensionToHitmanCivilian; // 84
    int m_nAliveEnemies; // 88 (unverified)
    int m_nAliveGuards; // 92
    PAD(8); // 96
    float m_fAttentionMax; // 104
    float m_fAttentionMaxPan; // 108
    float m_fAudibleAttentionMax; // 112
    float m_fAudibleAttentionMaxPan;  // 116
    float m_fSecurityCameraAttentionMax; // 120
    float m_fSecurityCameraAttentionMaxPan; // 124
    float m_fTrespassingAttentionMax; // 128
    float m_fTrespassingAttentionMaxPan; // 132
    PAD(8); // 136
    float m_fDisguiseAttentionMax; // 144
    float m_fDisguiseAttentionMaxPan; // 148
    PAD(24); // 152
    ZString m_sBodyId; // 176

    union
    {
        uint64_t m_nFlags;

        struct
        {
            bool m_bDisguiseBroken : 1;  // 192 (0xC0)
            bool m_bDisguiseSuspicious : 1; // & 2 (>> 1)
            bool m_bInDisguise : 1; // & 4 (>> 2)
            bool m_bUnk3 : 1; // & 8 (>> 3)
            bool m_bUnk4 : 1; // & 0x10 (>> 4)
            bool m_bBulletFlyByHitman : 1; // & 0x20 (>> 5)
            bool m_bStealthKill : 1; // & 0x40 (>> 6)
            bool m_bThroughWallKill : 1; // & 0x80 (>> 7)
            bool m_bTrespassing : 1; // 193 (0xC1)
            bool m_bInDisguiseZone : 1; // & 2 (>> 1)
            bool m_bUnk10 : 1; // & 4 (>> 2)
            bool m_bHitmanTrespassingSpotted : 1; // & 8 (>> 3)
            bool m_bHoldingIllegalWeapon01 : 1; // & 0x10 (>> 4)
            bool m_bUnk13 : 1; // & 0x20 (>> 5)
            bool m_bHoldingIllegalWeapon02 : 1; // & 0x40 (>> 6)
            bool m_bUnk15 : 1; // & 0x80 (>> 7)
            bool m_bUnk16 : 1; // 194 (0xC2)
            bool m_bAttentionOSDVisible : 1; // & 2 (>> 1)
            bool m_bBodyFound : 1; // & 4 (>> 2)
            bool m_bBodyFoundPacify : 1; // & 8 (>> 3)
            bool m_bBodyFoundMurder : 1; // & 0x10 (>> 4)
            bool m_bCanNotOpenCPDoor : 1; // & 0x20 (>> 5)
            bool m_bDisguiseDirty : 1; // & 0x40 (>> 6)
            bool m_bSpottedEnteringCloset : 1; // & 0x80 (>> 7)
            bool m_bUnk24 : 1; // 195 (0xC3)
            bool m_bBeingFrisked : 1; // & 2 (>> 1)
            bool m_bFriskSuccess : 1; // & 4 (>> 2)
            bool m_bFriskFailed : 1; // & 8 (>> 3)
            bool m_bUnk28 : 1; // & 0x10 (>> 4)
            bool m_bHitmanInVision : 1; // & 0x20 (>> 5)
            bool m_bNPCHasWhiteDot : 1; // & 0x40 (>> 6)
            bool m_bNPCHasExclamationMark : 1; // & 0x80 (>> 7)
            bool m_bNPCHasQuestionMark : 1; // 196 (0xC4)
            bool m_bDisguiseBeingInvestigated : 1; // & 2 (>> 1)
            bool m_bDeadBodySeen : 1; // & 4 (>> 2)
            bool m_bDeadBodySeenMurder : 1; // & 8 (>> 3)
            bool m_bUnk36 : 1; // & 0x10 (>> 4)
            bool m_bUnk37 : 1; // & 0x20 (>> 5)
            bool m_bUnk38 : 1; // & 0x40 (>> 6)
            bool m_bUnk39 : 1; // & 0x80 (>> 7)
            bool m_bUnk40 : 1; // 197 (0xC5)
            bool m_bUnk41 : 1; // & 2 (>> 1)
            bool m_bUnk42 : 1; // & 4 (>> 2)
            bool m_bUnk43 : 1; // & 8 (>> 3)
            bool m_bHitmanBumping : 1; // & 0x10 (>> 4)
            bool m_bUnk45 : 1; // & 0x20 (>> 5)
            bool m_bUnk46 : 1; // & 0x40 (>> 6)
            bool m_bUnk47 : 1; // & 0x80 (>> 7)
        };
    };
};

class ZGameStatsManager
{
public:
    PAD(0x48);
    ZAIGameState m_gameState; // 72
    ZAIGameState m_gameState2; // 272
    ZAIGameState m_gameState3; // 472
    ZAIGameState m_oldGameState; // 672
    ZAIGameState m_gameState5; // 872
    PAD(0x18); // 1072
    TArray<ZAIEventEmitterEntity*> m_nAIEventEmitters; // 1096
};
