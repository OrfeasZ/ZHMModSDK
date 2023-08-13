#pragma once

#include <cstdint>
#include <RakNetSocket2.h>

class NetworkSocketOverride : public RakNet::SocketLayerOverride {
public:
    virtual bool NetworkSocketOverride_unk3(void* a2) const = 0;
    virtual void NetworkSocketOverride_unk4(void* a2) = 0;
    virtual void NetworkSocketOverride_unk5() = 0;
    virtual void NetworkSocketOverride_unk6() = 0;
    virtual void NetworkSocketOverride_unk7() = 0;
    virtual int64_t GetUserId() const = 0;
    virtual const char GetUserName() const = 0;
};
