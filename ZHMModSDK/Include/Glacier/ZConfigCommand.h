#pragma once

#include <cstdint>
#include "ZString.h"

class ZConfigFloat;
class ZConfigInt;
class ZConfigString;

class ZConfigCommand {
public:
    enum class ECLASSTYPE {
        ECLASS_FLOAT,
        ECLASS_INT,
        ECLASS_STRING,
        ECLASS_UNKNOWN,
    };

    virtual ECLASSTYPE GetType() = 0;

    // Get a pointer to a config command from the command name.
    // Returns 0/nullptr if it does not exist.
    static ZConfigCommand* Get(ZString p_CommandName);

    uint32_t GetNameHash() const { return m_iNameHash; }
    ZConfigCommand* GetNext() { return m_pNext; }

    template <typename T>
    T* As() { return GetType() == GetClassType<T>() ? static_cast<T*>(this) : nullptr; }

protected:
    uint32_t m_iNameHash;
    ZConfigCommand* m_pNext;

private:
    template <typename T>
    static ECLASSTYPE GetClassType() {
        if (std::is_same<T, ZConfigFloat>::value) {
            return ECLASSTYPE::ECLASS_FLOAT;
        }
        else if (std::is_same<T, ZConfigInt>::value) {
            return ECLASSTYPE::ECLASS_INT;
        }
        else if (std::is_same<T, ZConfigString>::value) {
            return ECLASSTYPE::ECLASS_STRING;
        }

        return ECLASSTYPE::ECLASS_UNKNOWN;
    }
};

class ZConfigFloat : public ZConfigCommand {
public:
    float GetValue() const { return m_Value; }

private:
    float m_Value;
};

class ZConfigInt : public ZConfigCommand {
public:
    uint32_t GetValue() const { return m_Value; }

private:
    uint32_t m_Value;
};

class ZConfigString : public ZConfigCommand {
public:
    const char* GetValue() const { return m_szValue; }

private:
    char m_szValue[256];
};