#pragma once

#include "gen/cclist.h"

class ccFile : public ccNode {
public:
    enum OpenMode : u16 {
        OPEN_READ = 1,
        OPEN_WRITE = 2,
        OPEN_APPEND = 4,
    };

    enum SeekMode : u16 {
        SEEK_FROM_START = 0,
        SEEK_FROM_CURRENT = 1,
        SEEK_FROM_END = 2,
    };

    ccFile();
    ~ccFile() override;

    void SetDebug(s16 value);

    u32 ConvertLong(u32 value);
    u16 ConvertWord(u16 value);

    s32 OpenMem(u8* data, u32 size);
    s32 Open(const char* path, u16 mode);
    s32 Close();

    s32 ReadString(void* dst, u32 maxLen);
    virtual s32 Read(void* dst, u32 size);
    virtual s32 Write(void* src, u32 size);
    s32 Seek(u32 offset, u16 mode);

    s32 WriteLong(u32 value);
    s32 WriteWord(u16 value);
    s32 WriteByte(u8 value);

    s32 ReadLong(u32* outValue);
    s32 ReadWord(u16* outValue);
    s32 ReadByte(u8* outValue);

    s32 GetError() const;
    s32 GetPosition() const;
    s32 GetLength() const;

private:
    s32 position = 0;
    s32 length = 0;
    u16 reserved32 = 0;
    s32 isMemory = 0;
    s32 error = 0;
    void* handle = nullptr;
    s32 byteOrder = 2;
    s16 debug = 0;

    union {
        u8 bytes[4];
        u16 words[2];
        u32 longValue;
    } convert = {};
};
