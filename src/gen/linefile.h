#pragma once
#include "core.h"

// Simple line-by-line text file parser. Reads a file into memory,
// then iterates lines tokenizing into up to 8 words per line.
// Comments are lines starting with '#'.

// LineFile (216 bytes on PSX)
// PSX layout:
//   +0:   buffer (char*)
//   +4:   fileSize (u32)
//   +8:   curPos (char*)
//   +12:  endPos (char*)
//   +16:  words[8][24] - 8 words x 24 chars each
//   +208: numWords (s32)
//   +212: vtable
class LineFile {
public:
    static constexpr s32 MAX_WORDS = 8;
    static constexpr s32 WORD_LEN = 24;

    char* buffer = nullptr;     // +0: allocated file buffer
    u32 fileSize = 0;           // +4: size of file data
    char* curPos = nullptr;     // +8: current read position
    char* endPos = nullptr;     // +12: end of buffer
    char words[MAX_WORDS][WORD_LEN] = {}; // +16: tokenized words
    s32 numWords = 0;           // +208: word count for current line

    LineFile();
    virtual ~LineFile();
    void Open(const char* filename);

    // Advances to next non-comment line. Returns true if a line was read.
    bool Next();

    // Returns pointer to word buffer at given index.
    const char* Word(s32 index) const;
};
