#pragma once

#include "ZString.h"

class IApplication {
public:
    virtual ~IApplication() = 0;
    virtual void IApplication_unk1() = 0;
    virtual void IApplication_unk2() = 0;
    virtual void IApplication_unk3() = 0;
    virtual void IApplication_unk4() = 0;
    virtual void IApplication_unk5() = 0;
    virtual void IApplication_unk6() = 0;
    virtual void IApplication_unk7() = 0;
    virtual void IApplication_unk8() = 0;
    virtual void IApplication_unk9() = 0;
    virtual void IApplication_unk10() = 0;
    virtual void IApplication_unk11() = 0;
    virtual void IApplication_unk12() = 0;
    virtual void IApplication_unk13() = 0;
    virtual void IApplication_unk14() = 0;
    virtual ZString& GetOption(const ZString& sOption) = 0;
};
