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

    u8* rawData = nullptr;
    if (!xcReadFileLow(filename, &rawData, &fileSize)) {
        LOG("[LineFile] Failed to open: %s", filename);
        return;
    }

    buffer = new char[fileSize + 1];
    memcpy(buffer, rawData, fileSize);
    delete[] rawData;

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

        char* token = curPos;
        while (token < lineEnd && numWords < MAX_WORDS) {
            while (token < lineEnd &&
                   (*token == ' ' || *token == '\t' || *token == '\r')) {
                token++;
            }

            if (token >= lineEnd) {
                break;
            }

            char* tokenEnd = token;
            while (tokenEnd < lineEnd &&
                   *tokenEnd != ' ' && *tokenEnd != '\t' && *tokenEnd != '\r') {
                tokenEnd++;
            }

            s32 len = (s32)(tokenEnd - token);
            if (len >= WORD_LEN) {
                len = WORD_LEN - 1;
            }

            memcpy(words[numWords], token, len);
            words[numWords][len] = '\0';
            numWords++;
            token = tokenEnd;
        }

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
