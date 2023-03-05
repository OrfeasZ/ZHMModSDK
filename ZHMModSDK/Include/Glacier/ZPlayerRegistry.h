#pragma once

class ZBaseReplica
{
public:
    virtual ~ZBaseReplica() = default;
    virtual void ZBaseReplica_unk1() {}
    virtual void ZBaseReplica_unk2() {}
    virtual void ZBaseReplica_unk3() {}
    virtual void ZBaseReplica_unk4() {}
    virtual void ZBaseReplica_unk5() {}
    virtual void ZBaseReplica_unk6() {}
    virtual void ZBaseReplica_unk7() {}
    virtual void ZBaseReplica_unk8() {}
    virtual void ZBaseReplica_unk9() {}
    virtual void ZBaseReplica_unk10() {}
    virtual void ZBaseReplica_unk11() {}
    virtual void ZBaseReplica_unk12() {}
    virtual void ZBaseReplica_unk13() {}
    virtual void ZBaseReplica_unk14() {}
    virtual void ZBaseReplica_unk15() {}
    virtual void ZBaseReplica_unk16() {}
    virtual void ZBaseReplica_unk17() {}
    virtual void ZBaseReplica_unk18() {}
    virtual void ZBaseReplica_unk19() {}
};

class ZRakNetReplica;
class ZNetPlayer;

class ZNetPlayerController :
    public ZBaseReplica
{
public:
    uint64_t m_nUnkCounter; // 0x10 (-8) some sort of counter, only set for local player, maybe time sync?
    uint32_t m_nFlags0x10; // 0x18 (-8)
    ZRakNetReplica* m_pRakNetReplica; // 0x20 (-8)
    uint32_t m_nFlags0x20; // 0x28 (-8) 0000000B when in multiplayer, 0 otherwise
    void* m_nFlags0x28; // 0x30 (-8) 0x55bd4b73 for player one, 0x55bd4b74 for player two (in mp)
    bool m_bLocalPlayer; // 0x38 (-8)
    uint32_t m_nFlags0x34; // 0x3C (-8) always 0
    uint32_t m_nFlags0x38; // 0x40 (-8) always 5
    uint32_t m_nFlags0x3C; // 0x44 (-8) always 1
    uint16_t m_nFlags0x40; // 0x48 (-8) always 0
    bool m_bConnectedToMultiplayer; // 0x4A (-8) true for both players when in multiplayer, false in singleplayer
    ZNetPlayer* m_pNetPlayer; // 0x50 (-8) 
    ZGuid m_SelectedCharacterId; // 0x58 (-8) 
    ZString m_sUnk0x60; // 0x68 (-8) set to some guid in multiplayer for both players
    uint32_t m_nFlags0x70; // 0x78 (-8) always 0
    ZGuid m_OutfitId; // 0x80 (-8)
    ZString s_sSessionId; // 0x90 (-8) maybe a contract session id or something? timestamp-guid
    uint32_t m_nFlags0x98; // 0xA0 (-8) set to 0 for local player on player one, 0 for both players on player two, -1 for everyone else
    ZEntityRef m_HitmanEntity; // 0xA8 (-8)
    void* m_pEntityVtables; // 0xB0 (-8) a pointer to the entity vtables, probably something related to the aspect dummy
    void* m_unk0xB0; // 0xB8 (-8)
    void* m_unk0xB8; // 0xC0 (-8)
    void* m_unk0xC0; // 0xC8 (-8)
};

static_assert(offsetof(ZNetPlayerController, m_nFlags0x34) == 0x34);
static_assert(offsetof(ZNetPlayerController, m_bConnectedToMultiplayer) == 0x42);

struct SNetPlayerData
{
    int32_t m_nPlayerId; // 0x00
    ZNetPlayerController m_Controller; // 0x08
};

static_assert(sizeof(SNetPlayerData) == 0xD0);

class ZPlayerRegistry :
    public IComponentInterface,
    public ZBaseReplica
{
public:
    virtual ~ZPlayerRegistry() = 0;

public:
    PAD(0x40);
    SNetPlayerData m_aPlayerData[4]; // 0x50
    SNetPlayerData* m_pLocalPlayer; // 0x390
};

static_assert(offsetof(ZPlayerRegistry, m_aPlayerData) == 0x50);
static_assert(offsetof(ZPlayerRegistry, m_pLocalPlayer) == 0x390);
