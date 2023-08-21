#include "ModSettings.h"
#include <optional>
#include <string>

ModSetting::~ModSetting()
{
}

ModSetting::ModSetting(std::string p_Value)
{
    m_Value = move(p_Value);
}

const std::string& ModSetting::Get() const
{
    return m_Value;
}

void ModSetting::Set(std::string p_Value)
{
    m_Value = move(p_Value);
}

void ModSetting::Set(bool p_Value)
{
    m_Value = p_Value ? "yes" : "no";
}

bool ModSetting::AsBool() const
{
    if (m_Value.empty())
        return false;

    // Assume f(alse)/n(o)/o(ff)/0 based on first character.
    switch (std::tolower(m_Value[0])) {
        case 'f':
        case 'n':
        case '0':
            return false;
        case 'o':
            // Except 'o' could be 'on' so check for that.
            return m_Value.size() > 1 && m_Value[1] == 'n';
    }

    return true;
}

bool ModSetting::Read(ZString& p_Out)
{
    p_Out = ZString(Get());
    return true;
}

bool ModSetting::ReadBool(bool& p_Out)
{
    p_Out = AsBool();
    return true;
}

bool ModSetting::ReadInt(int32& p_Out)
{
    auto s_Result = AsInt<int32>();
    if (!s_Result)
        return false;
    p_Out = *s_Result;
    return true;
}

bool ModSetting::ReadUnsignedInt(uint32& p_Out)
{
    auto s_Result = AsInt<uint32>();
    if (!s_Result)
        return false;
    p_Out = *s_Result;
    return true;
}

bool ModSetting::ReadDouble(float64& p_Out)
{
    auto s_Result = AsFloat<float64>();
    if (!s_Result)
        return false;
    p_Out = *s_Result;
    return true;
}