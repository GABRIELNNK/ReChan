// linefile.cpp - LineFile reversed from PSX LINEFILE.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\LINEFILE.CPP
#include "gen/common.h"
#include "xclib/xcfile.h"
#include "gen/linefile.h"

// PSX: __8LineFile (0x80017F50)
LineFile::LineFile() {
    MARKFUNCTION(0x80017F50);
    fileSize = 0;
}

// PSX: _._8LineFile (0x80017F68)
LineFile::~LineFile() {
    MARKFUNCTION(0x80017F68);
    if (buffer) {
        delete[] buffer;
        buffer = nullptr;
    }
}

// PSX: Open__8LineFilePc (0x80017FD4)
void LineFile::Open(const char* filename) {
    MARKFUNCTION(0x80017FD4);

    // PSX: ccFile open, read entire file, null-terminate
    FILE* f = xcOpenFile(filename, "rb");
    if (!f) {
        LOG("[LineFile] Failed to open: %s", filename);
        return;
    }

    fseek(f, 0, SEEK_END);
    fileSize = (u32)ftell(f);
    fseek(f, 0, SEEK_SET);

    buffer = new char[fileSize + 1];
    fread(buffer, 1, fileSize, f);
    fclose(f);

    buffer[fileSize] = '\0';
    curPos = buffer;
    endPos = buffer + fileSize;
}

// PSX: Next__8LineFile (0x80018078)
bool LineFile::Next() {
    MARKFUNCTION(0x80018078);

    while (true) {
        if (!curPos || (uintptr_t)curPos >= (uintptr_t)endPos)
            return false;

        char c = *curPos;

        // PSX: '#' = comment, skip to next line
        if (c == '#') {
            char* nl = std::strchr(curPos, '\n');
            if (nl)
                curPos = nl + 1;
            else
                curPos = endPos;
            continue;
        }

        // PSX: '\r' skip (CR+LF handling)
        if (c == '\r') {
            curPos += 2;
            continue;
        }

        // PSX: '\n' = empty line, skip
        if (c == '\n') {
            curPos++;
            continue;
        }

        // Found a content line - tokenize it
        numWords = 0;
        memset(words, 0, sizeof(words));

        // Find end of line
        char* lineEnd = std::strchr(curPos, '\n');
        if (!lineEnd)
            lineEnd = endPos;

        // Save line end, temporarily null-terminate for strtok-like parsing
        char savedChar = *lineEnd;
        *lineEnd = '\0';

        // PSX: strtok with " \t" delimiters, copy up to 8 words of max 23 chars
        char* tok = strtok(curPos, " \t\r");
        while (tok && numWords < MAX_WORDS) {
            strncpy(words[numWords], tok, WORD_LEN - 1);
            words[numWords][WORD_LEN - 1] = '\0';
            numWords++;
            tok = strtok(nullptr, " \t\r");
        }

        // Restore and advance past line
        *lineEnd = savedChar;
        curPos = (lineEnd < endPos) ? lineEnd + 1 : endPos;

        if (numWords > 0)
            return true;
    }
}

// PSX: Word__8LineFilei (0x800181E0)
const char* LineFile::Word(s32 index) const {
    MARKFUNCTION(0x800181E0);
    if (index < 0 || index >= MAX_WORDS)
        return "";
    return words[index];
}
