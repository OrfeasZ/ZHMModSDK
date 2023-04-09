#pragma once

#include <cmath>
#include <cstdlib>
#include <string>
#include <cstring>

class BinaryStreamWriter
{
public:
    BinaryStreamWriter(size_t p_InitialCapacity = 64)
    {
        m_Position = 0;
        m_Capacity = p_InitialCapacity;
        m_Buffer = malloc(m_Capacity);
    }

    ~BinaryStreamWriter()
    {
        free(m_Buffer);
    }

    template <class T>
    void Write(T p_Value)
    {
        EnsureCanWrite(sizeof(T));
        *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_Buffer) + m_Position) = p_Value;
        m_Position += sizeof(T);
    }

    void WriteBinary(const void* p_Memory, size_t p_Size)
    {
        EnsureCanWrite(p_Size);

        memcpy(CurrentPtr(), p_Memory, p_Size);
        m_Position += p_Size;
    }

    void WriteString(const std::string& p_String)
    {
        // + 1 for null terminator.
        Write<uint32_t>(p_String.size() + 1);

        WriteBinary(p_String.data(), p_String.size());
        Write<uint8_t>(0);
    }

    void WriteShortString(const std::string& p_String)
    {
        Write<uint16_t>(p_String.size());
        WriteBinary(p_String.data(), p_String.size());
    }

    [[nodiscard]]
    std::string ToString() const
    {
        return std::string(static_cast<char*>(m_Buffer), m_Position);
    }

    void AlignTo(size_t p_Alignment)
    {
        if (m_Position % p_Alignment != 0)
        {
            const auto s_BytesToSkip = p_Alignment - (m_Position % p_Alignment);
            EnsureCanWrite(s_BytesToSkip);

            memset(CurrentPtr(), 0x00, s_BytesToSkip);
            m_Position += s_BytesToSkip;
        }
    }

    [[nodiscard]]
    void* Buffer() const
    {
        return m_Buffer;
    }

    [[nodiscard]]
    size_t WrittenBytes() const
    {
        return m_Position;
    }

private:
    void EnsureCanWrite(size_t p_AdditionalBytes)
    {
        if (m_Position + p_AdditionalBytes <= m_Capacity)
            return;

        size_t s_NewCapacity = ceil(m_Capacity * 1.5);

        while (s_NewCapacity < m_Position + p_AdditionalBytes)
            s_NewCapacity = ceil(s_NewCapacity * 1.5);

        m_Buffer = realloc(m_Buffer, s_NewCapacity);
        m_Capacity = s_NewCapacity;
    }

    void* CurrentPtr()
    {
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_Buffer) + m_Position);
    }

private:
    size_t m_Position;
    size_t m_Capacity;
    void* m_Buffer;
};
