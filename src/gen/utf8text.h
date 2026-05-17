#pragma once

#include "core.h"
#include <string>

struct Utf8TextView {
    const char* data = nullptr;
    s32 length = 0;

    Utf8TextView() = default;

    Utf8TextView(const char* text) {
        Assign(text);
    }

    Utf8TextView(const char* text, s32 textLength)
        : data(text)
        , length(textLength) {
    }

    Utf8TextView(const std::string& text)
        : data(text.c_str())
        , length((s32)text.size()) {
    }

    void Assign(const char* text) {
        data = text;
        if (!text) {
            length = 0;
            return;
        }

        s32 count = 0;
        while (text[count] != '\0') {
            count++;
        }
        length = count;
    }

    bool IsEmpty() const {
        return !data || length <= 0;
    }
};