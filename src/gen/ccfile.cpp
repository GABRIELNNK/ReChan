#include "gen/ccfile.h"
#include "p3d/fileio.h"
#include <string>

static constexpr uintptr_t kWriteHandleSentinel = 2;

ccFile::ccFile() {
    MARKFUNCTION(0x8004C008);
    position = 0;
    length = 0;
    reserved32 = 0;
    isMemory = 0;
    error = 0;
    handle = nullptr;
    byteOrder = 2;
    SetDebug(0);
}

ccFile::~ccFile() {
    MARKFUNCTION(0x8004C068);
    if (handle) {
        Close();
    }
}

void ccFile::SetDebug(s16 value) {
    MARKFUNCTION(0x8004C000);
    debug = value;
}

u32 ccFile::ConvertLong(u32 value) {
    MARKFUNCTION(0x8004BF88);
    if (byteOrder != 2) {
        convert.bytes[1] = (u8)(value >> 8);
        convert.bytes[2] = (u8)(value >> 16);
        convert.bytes[0] = (u8)value;
        convert.bytes[3] = (u8)(value >> 24);
        return convert.longValue;
    }

    convert.bytes[2] = (u8)(value >> 8);
    convert.bytes[1] = (u8)(value >> 16);
    convert.bytes[3] = (u8)value;
    convert.bytes[0] = (u8)(value >> 24);
    return convert.longValue;
}

u16 ccFile::ConvertWord(u16 value) {
    MARKFUNCTION(0x8004BFD4);
    if (byteOrder != 2) {
        convert.bytes[1] = (u8)value;
        convert.bytes[0] = (u8)(value >> 8);
        return convert.words[0];
    }

    convert.bytes[0] = (u8)value;
    convert.bytes[1] = (u8)(value >> 8);
    return convert.words[0];
}

s32 ccFile::OpenMem(u8* data, u32 size) {
    MARKFUNCTION(0x8004C0BC);
    isMemory = 1;
    position = 0;
    length = (s32)size;
    handle = data;
    return 0;
}

s32 ccFile::Open(const char* path, u16 mode) {
    MARKFUNCTION(0x8004C0D4);
    isMemory = 0;
    error = 0;

    SetName(path, 1);

    p3d::io::FileHandle::Mode ioMode = p3d::io::FileHandle::Mode::Read;
    if (mode & OPEN_WRITE) {
        ioMode = p3d::io::FileHandle::Mode::Write;
    }
    else if (mode & OPEN_APPEND) {
        ioMode = p3d::io::FileHandle::Mode::Append;
    }

    const bool shouldResolve = (mode & (OPEN_WRITE | OPEN_APPEND)) == 0;

    auto* fh = new p3d::io::FileHandle();
    if (fh->Open(GetName(), ioMode, shouldResolve)) {
        length = (s32)fh->Size();
    }
    else {
        delete fh;
        fh = nullptr;
        length = 0;
    }

    s32 result = 0;
    void* outHandle = fh;
    if (length != 0) {
        result = 1;
    }
    else if (mode & OPEN_WRITE || mode & OPEN_APPEND) {
        result = 1;
        if (!outHandle) {
            outHandle = reinterpret_cast<void*>(kWriteHandleSentinel);
        }
    }

    handle = outHandle;
    position = 0;
    return result;
}

s32 ccFile::Close() {
    MARKFUNCTION(0x8004C18C);
    if (handle && !isMemory) {
        if ((uintptr_t)handle != kWriteHandleSentinel) {
            delete reinterpret_cast<p3d::io::FileHandle*>(handle);
        }
        handle = nullptr;
    }

    SetName(nullptr, 0);
    handle = nullptr;
    length = 0;
    position = 0;
    return 0;
}

s32 ccFile::ReadString(void* dst, u32 maxLen) {
    MARKFUNCTION(0x8004C1F0);
    u8* cursor = (u8*)dst;
    u32 written = 0;

    while (written < maxLen) {
        if (Read(cursor, 1) != 1) {
            if (written < maxLen) {
                *cursor = 0;
            }
            return (s32)written;
        }

        if (*cursor != 13) {
            ++cursor;
            ++written;
            if (*(cursor - 1) == 10) {
                if (written < maxLen) {
                    *cursor = 0;
                }
                return (s32)written;
            }
        }
    }

    return (s32)written;
}

s32 ccFile::Read(void* dst, u32 size) {
    MARKFUNCTION(0x8004C2B4);

    u32 readSize = size;
    s32 bytesRead = 0;

    if (handle) {
        if (position >= length) {
            readSize = 0;
        }
        else {
            u32 available = (u32)(length - position);
            if (readSize > available) {
                readSize = available;
            }
        }

        if (readSize) {
            if (isMemory == 1) {
                memcpy(dst, (u8*)handle + position, readSize);
                bytesRead = (s32)readSize;
            }
            else {
                bytesRead = (s32)reinterpret_cast<p3d::io::FileHandle*>(handle)->Read(dst, readSize);
            }
        }
    }

    position += (s32)readSize;
    return bytesRead;
}

s32 ccFile::Write(void* src, u32 size) {
    MARKFUNCTION(0x8004C374);

    s32 bytesWritten = 0;

    if (!handle) {
        return 0;
    }

    if ((uintptr_t)handle == kWriteHandleSentinel) {
        p3d::io::FileHandle fh;
        if (fh.Open(GetName(), p3d::io::FileHandle::Mode::Write, /*resolveCase=*/false)) {
            bytesWritten = (s32)fh.Write(src, size);
        }
        handle = nullptr;
        return bytesWritten;
    }

    if (!isMemory) {
        bytesWritten = (s32)reinterpret_cast<p3d::io::FileHandle*>(handle)->Write(src, size);
        position += bytesWritten;
        if (position > length) {
            length = position;
        }
    }

    return bytesWritten;
}

s32 ccFile::Seek(u32 offset, u16 mode) {
    MARKFUNCTION(0x8004C3BC);

    if (mode == SEEK_FROM_CURRENT) {
        position += (s32)offset;
    }
    else if (mode < 2u) {
        position = (s32)offset;
    }
    else {
        if (mode == SEEK_FROM_END) {
            position = length - (s32)offset;
        }
        else {
            position = (s32)offset;
        }
    }

    if (!isMemory && handle && (uintptr_t)handle != kWriteHandleSentinel) {
        int whence = SEEK_SET;
        long seekValue = (long)offset;

        if (mode == SEEK_FROM_CURRENT) {
            whence = SEEK_CUR;
            seekValue = (long)offset;
        }
        else if (mode == SEEK_FROM_END) {
            whence = SEEK_END;
            seekValue = -(long)offset;
        }

        reinterpret_cast<p3d::io::FileHandle*>(handle)->Seek(seekValue, whence);
    }

    return position;
}

s32 ccFile::WriteLong(u32 value) {
    MARKFUNCTION(0x8004C4A0);
    ConvertLong(value);
    return Write(convert.bytes, 4);
}

s32 ccFile::WriteWord(u16 value) {
    MARKFUNCTION(0x8004C4E0);
    ConvertWord(value);
    return Write(convert.bytes, 2);
}

s32 ccFile::WriteByte(u8 value) {
    MARKFUNCTION(0x8004C524);
    return Write(&value, 1);
}

s32 ccFile::ReadLong(u32* outValue) {
    MARKFUNCTION(0x8004C558);
    u32 rawValue = 0;
    if (Read(&rawValue, 4) != 4) {
        return 1;
    }

    ConvertLong(rawValue);
    *outValue = convert.longValue;
    return 0;
}

s32 ccFile::ReadWord(u16* outValue) {
    MARKFUNCTION(0x8004C5D0);
    u16 rawValue = 0;
    if (Read(&rawValue, 2) != 2) {
        return 1;
    }

    ConvertWord(rawValue);
    *outValue = convert.words[0];
    return 0;
}

s32 ccFile::ReadByte(u8* outValue) {
    MARKFUNCTION(0x8004C648);
    u8 rawValue = 0;
    if (Read(&rawValue, 1) != 1) {
        return 1;
    }

    *outValue = rawValue;
    return 0;
}

s32 ccFile::GetError() const {
    MARKFUNCTION(0x8004C6A4);
    return error;
}

s32 ccFile::GetPosition() const {
    MARKFUNCTION(0x8004C6B0);
    return position;
}

s32 ccFile::GetLength() const {
    MARKFUNCTION(0x8004C6BC);
    return length;
}
