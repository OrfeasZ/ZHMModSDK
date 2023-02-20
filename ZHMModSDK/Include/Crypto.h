#pragma once

#include <cstdint>

class Crypto
{
public:
    static void XORData(char* data, size_t dataSize)
    {
        constexpr unsigned char xorArray[] = { 0xDC, 0x45, 0xA6, 0x9C, 0xD3, 0x72, 0x4C, 0xAB };
        constexpr int xorLength = sizeof(xorArray);

        for (size_t i = 0; i < dataSize; i++)
        {
            data[i] ^= xorArray[i % xorLength];
        }
    }
};
