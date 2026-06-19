#pragma once
#include "core.h"

struct SemVer {
    s32 major;
    s32 minor;
    s32 patch;

    SemVer() : major(0), minor(0), patch(0) {}

    bool Parse(const char* str) {
        major = 0;
        minor = 0;
        patch = 0;
        if (!str || str[0] != 'v') {
            return false;
        }
        const char* p = str + 1;
        major = ReadInt(&p);
        if (major < 0) return false;
        if (*p != '.') return false;
        p++;
        minor = ReadInt(&p);
        if (minor < 0) return false;
        if (*p == '.') {
            p++;
            patch = ReadInt(&p);
            if (patch < 0) return false;
        }
        return true;
    }

    bool operator>(const SemVer& other) const {
        if (major != other.major) return major > other.major;
        if (minor != other.minor) return minor > other.minor;
        return patch > other.patch;
    }

    bool operator==(const SemVer& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    }

    bool operator!=(const SemVer& other) const {
        return !(*this == other);
    }

private:
    static s32 ReadInt(const char** p) {
        s32 val = 0;
        s32 count = 0;
        while (**p >= '0' && **p <= '9') {
            val = val * 10 + (**p - '0');
            (*p)++;
            count++;
        }
        return (count > 0) ? val : -1;
    }
};