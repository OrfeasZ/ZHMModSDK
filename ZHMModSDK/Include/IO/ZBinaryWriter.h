#pragma once

#include <fstream>
#include <vector>

class ZBinaryWriter
{
public:
    enum class ESeekOrigin
    {
        begin,
        current,
        end
    };

    ZBinaryWriter(const std::string filePath)
    {
        stream = std::ofstream(filePath, std::ios::binary);
        capacity = 0;
        data = nullptr;
        position = 0;
    }

    ZBinaryWriter(std::vector<char>* data)
    {
        this->data = data->data();
        capacity = data->capacity();
        position = 0;
    }

    ZBinaryWriter(void* data, size_t capacity)
    {
        this->data = data;
        this->capacity = capacity;
        position = 0;
    }

    ~ZBinaryWriter()
    {
        if (stream.is_open())
        {
            stream.close();
        }
    }

    template <typename T>
    void Write(const T& data)
    {
        if (stream.is_open())
        {
            stream.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }
        else if (this->data && capacity > 0)
        {
            *reinterpret_cast<T*>(reinterpret_cast<char*>(this->data) + position) = data;
        }
    }

    void WriteString(const std::string& string)
    {
        stream.write(string.data(), string.length() + 1);
    }

    void Skip(size_t count)
    {
        if (stream.is_open())
        {
            stream.seekp(static_cast<size_t>(stream.tellp()) + count);
        }
        else if (this->data && capacity > 0)
        {
            position += count;
        }
    }

    void Seek(size_t offset, ESeekOrigin seekOrigin = ESeekOrigin::begin)
    {
        if (stream.is_open())
        {
            stream.seekp(offset, static_cast<std::ios_base::seekdir>(seekOrigin));
        }
        else if (this->data && capacity > 0)
        {
            switch (seekOrigin)
            {
                case ESeekOrigin::begin:
                {
                    position = offset;

                    break;
                }
                case ESeekOrigin::current:
                {
                    position += offset;

                    break;
                }
                case ESeekOrigin::end:
                {
                    position = capacity - offset;

                    break;
                }
            }
        }
    }

    size_t GetPosition()
    {
        if (stream.is_open())
        {
            return stream.tellp();
        }
        else if (this->data && capacity > 0)
        {
            return position;
        }

        return -1;
    }

    size_t GetCapacity() const
    {
        return capacity;
    }

private:
    std::ofstream stream;
    size_t capacity;
    void* data;
    size_t position;
};
