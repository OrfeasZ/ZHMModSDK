#pragma once

#include "Glacier/ZPrimitives.h"

class ISetting
{
public:
    virtual bool Read(ZString& p_Out) = 0;
    virtual bool ReadBool(bool& p_Out) = 0;
    virtual bool ReadInt(int32& p_Out) = 0;
    virtual bool ReadUnsignedInt(uint32& p_Out) = 0;
    virtual bool ReadDouble(float64& p_Out) = 0;
};