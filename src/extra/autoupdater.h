#pragma once
#include "core.h"
#include "extra/semver.h"
#include <thread>
#include <atomic>
#include <string>

class feCustomMenuMgr;

struct UpdateAsset {
    std::string name;
    std::string url;
    s64 size;
};

class AutoUpdater {
public:
    enum class State {
        Idle,
        Checking,
        UpdateAvailable,
        UpToDate,
        Error,
        Downloading,
        ReadyToInstall,
        Installing,
    };

    void Init();
    void Shutdown();

    void CheckAsync();
    bool IsCheckComplete() const { return m_state != State::Idle && m_state != State::Checking; }
    bool IsUpdateAvailable() const { return m_state == State::UpdateAvailable || m_state == State::Downloading || m_state == State::ReadyToInstall; }

    State GetState() const { return m_state; }
    const char* GetLatestVersionTag() const { return m_latestTag.c_str(); }
    const char* GetCurrentVersion() const { return m_currentVersion.c_str(); }
    const char* GetReleaseNotes() const { return m_releaseNotes.c_str(); }
    const char* GetError() const { return m_error.c_str(); }
    s64 GetDownloadSize() const { return m_downloadSize; }
    float GetDownloadProgress() const;

    void StartDownload();
    void CancelDownload();
    void InstallAndRelaunch();

private:
    static std::string GetPlatformString();
    static std::string GetAssetExtension();
    static std::string GetExecutableName();

    void CheckThreadFunc();
    void DownloadThreadFunc();

    SemVer m_currentSemver;
    SemVer m_latestSemver;
    std::string m_currentVersion;
    std::string m_latestTag;
    std::string m_releaseNotes;
    std::string m_error;
    std::string m_exePath;
    std::string m_tempArchivePath;
    std::string m_updateAssetUrl;
    s64 m_downloadSize;
    s64 m_downloadedBytes;
    std::atomic<State> m_state;
    std::atomic<bool> m_cancelDownload;
    std::thread m_workerThread;
    feCustomMenuMgr* m_menu;
};

extern AutoUpdater* g_autoUpdater;