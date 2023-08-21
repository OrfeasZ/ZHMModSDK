#pragma once
#include "ISetting.h"
#include <charconv>
#include <format>
#include <optional>
#include <string>
#include <type_traits>

class ModSetting : public ISetting
{
public:
    ModSetting() noexcept = default;
    ModSetting(std::string p_Value);
    ~ModSetting();

    const std::string& Get() const;

    void Set(std::string p_Value);
    void Set(bool p_Value);

    template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> && std::is_integral_v<T> && !std::is_same_v<T, bool>>> 
    void Set(T p_Value)
    {
        m_Value = std::format("{}", p_Value);
    }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
    std::optional<T> AsInt(std::optional<T> p_Default = std::nullopt) const
    {
        return AsNumber<T>(p_Default);
    }

    template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    std::optional<T> AsFloat(std::optional<T> p_Default = std::nullopt) const
    {
        return AsNumber<T>(p_Default);
    }

    template<typename T, typename = std::enable_if_t<(std::is_integral_v<T> || std::is_floating_point_v<T>) && !std::is_same_v<T, bool>>>
    std::optional<T> AsNumber(std::optional<T> p_Default = std::nullopt) const
    {
        if (!m_Value.empty()) {
            T value;
            auto result = std::from_chars(m_Value.data(), m_Value.data() + m_Value.size(), value);
            if (result.ec == std::errc())
                return value;
        }
        return p_Default;
    }

    bool AsBool() const;

    bool Read(ZString& p_Out) override;
    bool ReadBool(bool& p_Out) override;
    bool ReadInt(int32& p_Out) override;
    bool ReadUnsignedInt(uint32& p_Out) override;
    bool ReadDouble(float64& p_Out) override;

private:
    std::string m_Value;
};