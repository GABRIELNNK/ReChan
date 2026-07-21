#pragma once
#include "core.h"
#include "pc/apppaths.h"
#include <thread>
#include <atomic>
#include <string>
#include <vector>

class PsxDiscExtractor {
public:
    enum class State {
        Idle,
        Scanning,
        ScanNoneFound,
        ScanMultipleFound,
        ScanFoundOne,
        Extracting,
        Done,
        Error,
    };

    void Init();
    void Shutdown();

    void ScanAsync();
    void StartExtractAsync();

    State GetState() const { return m_state; }
    bool IsBusy() const { return m_state == State::Scanning || m_state == State::Extracting; }
    float GetProgress() const;
    const char* GetError() const { return m_error.c_str(); }
    const std::vector<std::string>& GetCandidates() const { return m_candidates; }

    // Sentinel check for whether extracted PSX assets are already present (relative to CWD).
    static bool AreAssetsPresent();

    static constexpr const char* kDiscImageDir = apppaths::kDiscImageDir;

private:
    void ScanThreadFunc();
    void ExtractThreadFunc(std::string imagePath);

    std::atomic<State> m_state{ State::Idle };
    std::atomic<s64> m_bytesDone{ 0 };
    std::atomic<s64> m_bytesTotal{ 0 };
    std::string m_error;
    std::vector<std::string> m_candidates;
    std::thread m_workerThread;
};

extern PsxDiscExtractor* g_psxDiscExtractor;
