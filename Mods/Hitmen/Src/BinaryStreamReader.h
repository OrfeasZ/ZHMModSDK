#pragma once

#include <cassert>
#include <cstring>
#include <string>
#include <stdexcept>

class BinaryStreamReader
{
public:
    BinaryStreamReader(void* p_Buffer, size_t p_Size) :
        m_Buffer(p_Buffer),
        m_Size(p_Size),
        m_StreamPos(0),
        m_CanWrite(true)
    {
    }

    BinaryStreamReader(const void* p_Buffer, size_t p_Size) :
        m_Buffer(const_cast<void*>(p_Buffer)),
        m_Size(p_Size),
        m_StreamPos(0),
        m_CanWrite(false)
    {
    }

    template <typename T>
    T Read()
    {
        assert(m_StreamPos + sizeof(T) <= m_Size);

        auto s_Value = *reinterpret_cast<T*>(GetCurrentPtr());
        m_StreamPos += sizeof(T);

        return s_Value;
    }

    template <typename T>
    void Write(T p_Value)
    {
        if (!m_CanWrite)
            throw std::runtime_error("Tried to write to a read-only stream.");

        assert(m_StreamPos + sizeof(T) <= m_Size);

        *reinterpret_cast<T*>(GetCurrentPtr()) = p_Value;
        m_StreamPos += sizeof(T);
    }

    void Skip(size_t p_BytesToSkip)
    {
        assert(m_StreamPos + p_BytesToSkip <= m_Size);
        m_StreamPos += p_BytesToSkip;
    }

    void* CurrentPtr() const
    {
        return reinterpret_cast<void*>(GetCurrentPtr());
    }

    void ReadBytes(void* p_Destination, size_t p_BytesToRead)
    {
        assert(m_StreamPos + p_BytesToRead <= m_Size);
        memcpy(p_Destination, CurrentPtr(), p_BytesToRead);
        m_StreamPos += p_BytesToRead;
    }

    void Seek(uintptr_t p_Pos)
    {
        assert(p_Pos <= m_Size);
        m_StreamPos = p_Pos;
    }

    uintptr_t Position() const
    {
        return m_StreamPos;
    }

    void* Buffer() const
    {
        return m_Buffer;
    }

    void AlignReadTo(uintptr_t p_Alignment)
    {
        if (m_StreamPos % p_Alignment == 0)
            return;

        const auto s_BytesToSkip = p_Alignment - (m_StreamPos % p_Alignment);
        m_StreamPos += s_BytesToSkip;
    }

    std::string ReadString()
    {
        const auto s_StringLength = Read<uint32_t>();

        std::string s_String;
        s_String.resize(s_StringLength - 1); // Sub 1 for null terminator.

        ReadBytes(s_String.data(), s_StringLength - 1);
        Skip(1);

        return s_String;
    }

    std::string ReadShortString()
    {
        const auto s_StringLength = Read<uint16_t>();

        std::string s_String;
        s_String.resize(s_StringLength);

        ReadBytes(s_String.data(), s_StringLength);

        return s_String;
    }

    std::string_view ReadShortStringView()
    {
        const auto s_StringLength = Read<uint16_t>();
        const auto s_String = std::string_view(static_cast<const char*>(CurrentPtr()), s_StringLength);
        Skip(s_StringLength);
        return s_String;
    }

private:
    uintptr_t GetCurrentPtr() const
    {
        return reinterpret_cast<uintptr_t>(m_Buffer) + m_StreamPos;
    }

private:
    void* m_Buffer;
    size_t m_Size;
    uintptr_t m_StreamPos;
    bool m_CanWrite;
};
