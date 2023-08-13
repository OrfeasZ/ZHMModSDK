#pragma once

#include "Reflection.h"
#include "Enums.h"

class IEngineMode : public IComponentInterface {
public:
    virtual void IEngineMode_unk5() = 0;
    virtual void IEngineMode_unk6() = 0;
    virtual void IEngineMode_unk7() = 0;
    virtual void IEngineMode_unk8() = 0;
    virtual void IEngineMode_unk9() = 0;
    virtual void IEngineMode_unk10() = 0;
    virtual void IEngineMode_unk11() = 0;
    virtual void IEngineMode_unk12() = 0;
    virtual void IEngineMode_unk13() = 0;
    virtual EEngineModeId GetEngineModeId() const = 0;
    virtual void IEngineMode_unk15() = 0;
    virtual void IEngineMode_unk16() = 0;
    virtual void IEngineMode_unk17() = 0;
    virtual void IEngineMode_unk18() = 0;
    virtual void IEngineMode_unk19() = 0;
    virtual void IEngineMode_unk20() = 0;
    virtual void IEngineMode_unk21() = 0;
};

class IGameMode : public IComponentInterface {
public:
    virtual void IGameMode_unk5() = 0;
    virtual void IGameMode_unk6() = 0;
    virtual void IGameMode_unk7() = 0;
    virtual void IGameMode_unk8() = 0;
    virtual void IGameMode_unk9() = 0;
    virtual void IGameMode_unk10() = 0;
    virtual void IGameMode_unk11() = 0;
    virtual void IGameMode_unk12() = 0;
    virtual void IGameMode_unk13() = 0;
    virtual void IGameMode_unk14() = 0;
    virtual void IGameMode_unk15() = 0;
    virtual void IGameMode_unk16() = 0;
    virtual void IGameMode_unk17() = 0;
    virtual void IGameMode_unk18() = 0;
    virtual void IGameMode_unk19() = 0;
    virtual EGameModeId GetGameModeId() const = 0;
    virtual bool IsLocalGamemode() = 0; // true for normal, false for versus, true when not EngineMode_Multiplayer for sniper
    virtual void IGameMode_unk22() = 0;
};
