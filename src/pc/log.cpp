#include "log.h"

Log& Log::Get() {
    static Log instance;
    return instance;
}

Log::Log()
    : m_File() {}

Log::~Log() {
    Shutdown();
}

void Log::Init(const char* filename) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_File.is_open())
        return;

    m_File.open(filename, std::ios::out | std::ios::trunc);
}

void Log::Shutdown() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_File.is_open()) {
        m_File.flush();
        m_File.close();
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
    std::vsnprintf(messageBuffer, sizeof(messageBuffer), fmt, args);

    char finalBuffer[2050];
    std::snprintf(finalBuffer, sizeof(finalBuffer), "%s\n", messageBuffer);

    if (m_File.is_open()) {
        m_File << finalBuffer;
        m_File.flush();
    }

    WriteToConsole(finalBuffer);


    if (m_MessageStack.size() > 20) {
        m_MessageStack.erase(m_MessageStack.begin());
    }

    m_MessageStack.push_back(finalBuffer);
}

void Log::WriteToConsole(const char* text) {
    std::printf("%s", text);
}
