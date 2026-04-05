#include "log.h"

#include <cstring>

Log& Log::Get() {
    static Log instance;
    return instance;
}

Log::Log()
    : m_File(nullptr) {}

Log::~Log() {
    Shutdown();
}

void Log::Init(const char* filename) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_File)
        return;

#if defined(_MSC_VER)
    fopen_s(&m_File, filename, "w");
#else
    m_File = fopen(filename, "w");
#endif
}

void Log::Shutdown() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_File) {
        fflush(m_File);
        fclose(m_File);
        m_File = nullptr;
    }
}

void Log::LogMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogMessageV(fmt, args);
    va_end(args);
}

void Log::LogMessageV(const char* fmt, va_list args) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    char messageBuffer[2048];

#if defined(_MSC_VER)
    vsnprintf_s(messageBuffer, sizeof(messageBuffer), _TRUNCATE, fmt, args);
#else
    vsnprintf(messageBuffer, sizeof(messageBuffer), fmt, args);
#endif

    char finalBuffer[2050];

#if defined(_MSC_VER)
    snprintf(finalBuffer, sizeof(finalBuffer), "%s\n", messageBuffer);
#else
    std::snprintf(finalBuffer, sizeof(finalBuffer), "%s\n", messageBuffer);
#endif

    if (m_File) {
        std::fputs(finalBuffer, m_File);
        std::fflush(m_File);
    }

    WriteToConsole(finalBuffer);
}

void Log::WriteToConsole(const char* text) {
    std::printf("%s", text);
}