#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Common.h"

namespace Scaleform {
    class MemoryHeap;

    class File {
    public:
        virtual ~File() = default;
        virtual const char* GetFilePath() = 0;
        virtual bool IsValid() = 0;
        virtual bool IsWritable() = 0;
        virtual int32_t Tell() = 0;
        virtual int64_t LTell() = 0;
        virtual int32_t GetLength() = 0;
        virtual int64_t LGetLength() = 0;
        virtual int32_t GetErrorCode() = 0;
        virtual int32_t Write(const uint8_t* p_Buffer, int32_t p_NumBytes) = 0;
        virtual int32_t Read(uint8_t* p_Buffer, int32_t p_NumBytes) = 0;
        virtual int32_t SkipBytes(int32_t p_NumBytes) = 0;
        virtual int32_t BytesAvailable() = 0;
        virtual bool Flush() = 0;
        virtual int32_t Seek(int32_t p_Offset, int32_t p_Origin) = 0;
        virtual int64_t LSeek(int64_t p_Offset, int32_t p_Origin) = 0;
        virtual int32_t SeekStart() = 0;
        virtual bool ChangeSize(int32_t p_NewSize) = 0;
        virtual int32_t CopyFromStream(File* p_Stream, int32_t p_ByteSize) = 0;
        virtual bool Close() = 0;

        int32_t m_nRefCount;
    };

    class MemoryFile : public File {
    public:
        void* m_sFilePath;
        const uint8_t* m_pFileData;
        int32_t m_nFileSize;
        int32_t m_nFileIndex;
        bool m_bValid;

        ZHMSDK_API static File* Create(const char* p_Url, const std::vector<uint8_t>& p_Data);
    };

    static_assert(sizeof(MemoryFile) == 0x30);
    static_assert(offsetof(MemoryFile, m_nRefCount) == 0x8);
    static_assert(offsetof(MemoryFile, m_sFilePath) == 0x10);
    static_assert(offsetof(MemoryFile, m_pFileData) == 0x18);
    static_assert(offsetof(MemoryFile, m_nFileSize) == 0x20);
    static_assert(offsetof(MemoryFile, m_nFileIndex) == 0x24);
    static_assert(offsetof(MemoryFile, m_bValid) == 0x28);

    namespace GFx {
        namespace AS3 {
            class FlashUI {
            public:
                enum class OutputMessageType {
                    Output_Message = 0,
                    Output_Error = 1,
                    Output_Warning = 2,
                    Output_Action = 3
                };
            };

            class MovieRoot;
        }
    }
};