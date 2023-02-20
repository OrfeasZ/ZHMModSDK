#pragma once

#include <fstream>

class ZBinaryReader
{
public:
    enum class ESeekOrigin
    {
        begin,
        current,
        end
    };

    ZBinaryReader(const std::string& filePath)
    {
        stream = std::ifstream(filePath, std::ios::binary | std::ios_base::ate);
        size = stream.tellg();
        data = nullptr;
        position = 0;

        stream.seekg(0);
    }

    ZBinaryReader(std::vector<char>* data)
    {
        this->data = data->data();
        size = data->size();
        position = 0;
    }

    ZBinaryReader(void* data, size_t dataSize)
    {
        this->data = data;
        size = dataSize;
        position = 0;
    }

    ~ZBinaryReader()
    {
        if (stream.is_open())
        {
            stream.close();
        }
    }

    template <typename T>
    T Read()
    {
        T data {};

        if (stream.is_open())
        {
            stream.read(reinterpret_cast<char*>(&data), sizeof(T));
        }
        else if (this->data && size > 0)
        {
            data = *reinterpret_cast<T*>(reinterpret_cast<char*>(this->data) + position);

            position += sizeof(T);
        }

        return data;
    }

    char* ReadChars(const size_t size)
    {
        char* buffer = new char[size];

        if (stream.is_open())
        {
            stream.read(buffer, size);
        }
        else if (this->data && size > 0)
        {
            memcpy(buffer, reinterpret_cast<char*>(this->data) + position, size);

            position += size;
        }

        return buffer;
    }

    std::string ReadString(const size_t size)
    {
        char* buffer = ReadChars(size + 1);
        std::string result = std::string(buffer, size);

        delete[] buffer;

        return result;
    }

    template <typename T>
    void ReadBytes(T* data, size_t count)
    {
        if (stream.is_open())
        {
            stream.read(reinterpret_cast<char*>(data), sizeof(T) * count);
        }
        else if (this->data && size > 0)
        {
            memcpy(data, this->data, sizeof(T) * count);

            position += count;
        }
    }

    void ReadBytes(void* data, size_t count)
    {
        if (stream.is_open())
        {
            stream.read(reinterpret_cast<char*>(data), count);
        }
        else if (this->data && size > 0)
        {
            memcpy(data, this->data, count);

            position += count;
        }
    }

    void Skip(size_t count)
    {
        if (stream.is_open())
        {
            stream.seekg(static_cast<size_t>(stream.tellg()) + count);
        }
        else if (this->data && size > 0)
        {
            position += count;
        }
    }

    void Seek(size_t offset, ESeekOrigin seekOrigin = ESeekOrigin::begin)
    {
        if (stream.is_open())
        {
            stream.seekg(offset, static_cast<std::ios_base::seekdir>(seekOrigin));
        }
        else if (this->data && size > 0)
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
                    position = size - offset;

                    break;
                }
            }
        }
    }

    size_t GetPosition()
    {
        if (stream.is_open())
        {
            return stream.tellg();
        }
        else if (this->data && size > 0)
        {
            return position;
        }

        return -1;
    }

    size_t GetSize() const
    {
        return size;
    }

    void* GetData()
    {
        return data;
    }

private:
    std::ifstream stream;
    size_t size;
    void* data;
    size_t position;
};
