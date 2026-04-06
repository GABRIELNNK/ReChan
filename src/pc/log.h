#pragma once
#include <cstdio>
#include <cstdarg>
#include <fstream>
#include <mutex>

class Log {
public:
    static Log& Get();

    void Init(const char* filename = "rechan.log");
    void Shutdown();

    void LogMessage(const char* fmt, ...);
    void LogMessageV(const char* fmt, va_list args);

private:
    Log();
    ~Log();

    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    void WriteToConsole(const char* text);

private:
    std::ofstream m_File;
    std::mutex m_Mutex;
};

#define LOG(fmt, ...) Log::Get().LogMessage(fmt, ##__VA_ARGS__)
