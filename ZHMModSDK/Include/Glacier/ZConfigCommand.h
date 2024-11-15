#pragma once

#include <cstdint>

enum class ZConfigCommand_ECLASSTYPE
{
    ECLASS_FLOAT = 0,
    ECLASS_INT = 1,
    ECLASS_STRING = 2,
    ECLASS_UNKNOWN = 3,
};

class ZConfigFloat;
class ZConfigInt;
class ZConfigString;

class ZConfigCommand
{
public:
    virtual ZConfigCommand_ECLASSTYPE GetType() = 0;

    uint32_t GetNameHash() const { return m_iNameHash; }
    ZConfigCommand* GetNext() { return m_pNext; }

    template <typename T>
    T* As() { return GetType() == GetEnumForType<T>() ? dynamic_cast<T*>(this) : nullptr; }

protected:
    uint32_t m_iNameHash;
    ZConfigCommand* m_pNext;

private:
    template <typename T>
    static ZConfigCommand_ECLASSTYPE GetEnumForType() {
        if (std::is_same<T, ZConfigFloat>::value) {
            return ZConfigCommand_ECLASSTYPE::ECLASS_FLOAT;
        } else if (std::is_same<T, ZConfigInt>::value) {
            return ZConfigCommand_ECLASSTYPE::ECLASS_INT;
        } else if (std::is_same<T, ZConfigString>::value) {
            return ZConfigCommand_ECLASSTYPE::ECLASS_STRING;
        }

        return ZConfigCommand_ECLASSTYPE::ECLASS_UNKNOWN;
    }
};

class ZConfigFloat : public ZConfigCommand
{
public:
    float GetValue() const { return m_Value; }

private:
    float m_Value;
};

class ZConfigInt : public ZConfigCommand
{
public:
    uint32_t GetValue() const { return m_Value; }

private:
    uint32_t m_Value;
};

class ZConfigString : public ZConfigCommand
{
public:
    const char* GetValue() const { return m_szValue; }

private:
    char m_szValue[256];
};
