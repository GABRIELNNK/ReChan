#include "gen/common.h"

#if AUTO_UPDATER
#include "extra/autoupdater.h"
#include "gen/game.h"
#include "gen/time.h"
#include "p3d/fileio.h"
#include "version.h"

#ifdef RC_PLATFORM_WINDOWS
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#endif

#ifdef RC_PLATFORM_WINDOWS
static std::string shellExec(const char* cmd) {
    std::string result;
    SECURITY_ATTRIBUTES security = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &security, 0)) return result;
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    HANDLE nullInput = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE nullError = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    startup.hStdInput = nullInput;
    startup.hStdOutput = writePipe;
    startup.hStdError = nullError;

    PROCESS_INFORMATION process = {};
    std::string commandLine(cmd);
    const BOOL started = CreateProcessA(nullptr, commandLine.data(), nullptr, nullptr, TRUE,
                                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    CloseHandle(writePipe);
    if (nullInput != INVALID_HANDLE_VALUE) CloseHandle(nullInput);
    if (nullError != INVALID_HANDLE_VALUE) CloseHandle(nullError);

    if (!started) {
        CloseHandle(readPipe);
        return result;
    }

    char buffer[4096];
    DWORD bytesRead = 0;
    while (ReadFile(readPipe, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0) {
        result.append(buffer, bytesRead);
    }
    CloseHandle(readPipe);
    WaitForSingleObject(process.hProcess, INFINITE);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return result;
}

static bool launchHidden(const std::string& commandLine) {
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION process = {};
    std::string mutableCommandLine = commandLine;
    const BOOL started = CreateProcessA(nullptr, mutableCommandLine.data(), nullptr, nullptr,
                                        FALSE, CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
                                        nullptr, nullptr, &startup, &process);
    if (!started) return false;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}
#else
static std::string shellExec(const char* cmd) {
    std::string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return result;
    char buf[4096];
    while (fgets(buf, sizeof(buf), pipe)) {
        result += buf;
    }
    pclose(pipe);
    return result;
}
#endif

static std::string jsonGetString(const std::string& json, const char* key) {
    std::string search = "\"";
    search += key;
    search += "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    size_t end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";
    std::string val = json.substr(pos + 1, end - pos - 1);
    std::string result;
    for (size_t i = 0; i < val.length(); i++) {
        if (val[i] == '\\' && i + 1 < val.length()) {
            char next = val[i + 1];
            if (next == 'n') { result += '\n'; i++; }
            else if (next == 'r') { result += '\r'; i++; }
            else if (next == 't') { result += '\t'; i++; }
            else if (next == '"') { result += '"'; i++; }
            else if (next == '\\') { result += '\\'; i++; }
            else { result += val[i]; }
        }
        else {
            result += val[i];
        }
    }
    return result;
}

static s64 jsonGetInt(const std::string& json, const char* key) {
    std::string search = "\"";
    search += key;
    search += "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return 0;
    pos = json.find(':', pos + search.length());
    if (pos == std::string::npos) return 0;
    pos++;
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    s64 val = 0;
    bool neg = false;
    if (pos < json.length() && json[pos] == '-') { neg = true; pos++; }
    while (pos < json.length() && json[pos] >= '0' && json[pos] <= '9') {
        val = val * 10 + (json[pos] - '0');
        pos++;
    }
    return neg ? -val : val;
}

static std::string jsonGetAssetUrl(const std::string& json, const std::string& platformStr) {
    std::string lowerPlat = platformStr;
    for (auto& c : lowerPlat) c = (char)tolower((unsigned char)c);

    size_t pos = 0;
    while (true) {
        pos = json.find("\"browser_download_url\"", pos);
        if (pos == std::string::npos) break;
        pos = json.find('"', json.find(':', pos) + 1);
        if (pos == std::string::npos) break;
        size_t end = json.find('"', pos + 1);
        if (end == std::string::npos) break;
        std::string url = json.substr(pos + 1, end - pos - 1);
        std::string urlLower = url;
        for (auto& c : urlLower) c = (char)tolower((unsigned char)c);
        if (urlLower.find(lowerPlat) != std::string::npos) {
            return url;
        }
        pos = end + 1;
    }
    return "";
}

static s64 jsonGetAssetSize(const std::string& json, const std::string& platformStr) {
    std::string lowerPlat = platformStr;
    for (auto& c : lowerPlat) c = (char)tolower((unsigned char)c);

    size_t pos = 0;
    while (true) {
        pos = json.find("\"browser_download_url\"", pos);
        if (pos == std::string::npos) break;
        size_t urlStart = json.find('"', json.find(':', pos) + 1);
        if (urlStart == std::string::npos) break;
        size_t urlEnd = json.find('"', urlStart + 1);
        if (urlEnd == std::string::npos) break;
        std::string url = json.substr(urlStart + 1, urlEnd - urlStart - 1);
        std::string urlLower = url;
        for (auto& c : urlLower) c = (char)tolower((unsigned char)c);

        if (urlLower.find(lowerPlat) != std::string::npos) {
            size_t sizeSearch = json.rfind("\"size\"", urlEnd);
            s64 sizeVal = 0;
            if (sizeSearch != std::string::npos && urlEnd - sizeSearch < 4096) {
                size_t colon = json.find(':', sizeSearch + 6);
                if (colon != std::string::npos) {
                    size_t s = colon + 1;
                    while (s < json.length() && (json[s] == ' ' || json[s] == '\t')) s++;
                    while (s < json.length() && json[s] >= '0' && json[s] <= '9') {
                        sizeVal = sizeVal * 10 + (json[s] - '0');
                        s++;
                    }
                }
            }
            return sizeVal;
        }
        pos = urlEnd + 1;
    }
    return 0;
}

AutoUpdater* g_autoUpdater = nullptr;

std::string AutoUpdater::GetPlatformString() {
#ifdef RC_PLATFORM_WINDOWS
    return "win64";
#elif defined(__APPLE__)
    return "macos64";
#else
    return "linux64";
#endif
}

std::string AutoUpdater::GetAssetExtension() {
#ifdef RC_PLATFORM_WINDOWS
    return ".zip";
#else
    return ".tar.gz";
#endif
}

std::string AutoUpdater::GetExecutableName() {
#ifdef RC_PLATFORM_WINDOWS
    return "rechan.exe";
#elif defined(__APPLE__)
    return "rechan";
#else
    return "rechan";
#endif
}

void AutoUpdater::Init() {
    m_state = State::Idle;
    m_currentVersion = GAME_VERSION;
    m_currentSemver.Parse(GAME_VERSION);
    m_downloadSize = 0;
    m_downloadedBytes = 0;
    m_cancelDownload = false;
    m_menu = nullptr;
    m_exePath = p3d::io::GetExecutablePath();
    m_tempArchivePath = p3d::io::GetTempDir() + "rechan_update" + GetAssetExtension();
}

void AutoUpdater::Shutdown() {
    if (m_workerThread.joinable()) {
        m_cancelDownload = true;
        m_workerThread.join();
    }
}

void AutoUpdater::CheckAsync() {
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_state = State::Checking;
    m_error.clear();
    m_workerThread = std::thread(&AutoUpdater::CheckThreadFunc, this);
}

float AutoUpdater::GetDownloadProgress() const {
    if (m_downloadSize <= 0) return 0.0f;
    return (float)((double)m_downloadedBytes / (double)m_downloadSize);
}

void AutoUpdater::StartDownload() {
    if (m_state != State::UpdateAvailable || m_updateAssetUrl.empty()) return;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_state = State::Downloading;
    m_downloadedBytes = 0;
    m_cancelDownload = false;
    m_workerThread = std::thread(&AutoUpdater::DownloadThreadFunc, this);
}

void AutoUpdater::CancelDownload() {
    m_cancelDownload = true;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_state = State::UpdateAvailable;
}

void AutoUpdater::InstallAndRelaunch() {
    if (m_state != State::ReadyToInstall) return;
    m_state = State::Installing;

    std::string exeDir = p3d::io::GetExecutableDir();
    std::string exeName = GetExecutableName();

    char lineBuf[1024];

#ifdef RC_PLATFORM_WINDOWS
    std::string scriptPath = p3d::io::GetTempDir() + "rechan_update.bat";
    std::string script = "@echo off\r\ntimeout /t 3 /nobreak > nul\r\n";
    std::snprintf(lineBuf, sizeof(lineBuf),
                  "powershell -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"\r\n",
                  m_tempArchivePath.c_str(), exeDir.c_str());
    script += lineBuf;
    std::snprintf(lineBuf, sizeof(lineBuf), "del \"%s\"\r\n", m_tempArchivePath.c_str());
    script += lineBuf;
    std::snprintf(lineBuf, sizeof(lineBuf), "start \"\" \"%s\\%s\"\r\n", exeDir.c_str(), exeName.c_str());
    script += lineBuf;
    script += "del \"%~f0\"\r\n";
    p3d::io::WriteTextFile(scriptPath, script);

    std::string cmd = "cmd.exe /D /S /C \"\"" + scriptPath + "\"\"";
    launchHidden(cmd);
#else
    std::string scriptPath = p3d::io::GetTempDir() + "rechan_update.sh";
    std::string script = "#!/bin/sh\nsleep 3\n";
    std::snprintf(lineBuf, sizeof(lineBuf), "tar -xzf \"%s\" -C \"%s\"\n",
                  m_tempArchivePath.c_str(), exeDir.c_str());
    script += lineBuf;
    std::snprintf(lineBuf, sizeof(lineBuf), "rm -f \"%s\"\n", m_tempArchivePath.c_str());
    script += lineBuf;
    std::snprintf(lineBuf, sizeof(lineBuf), "\"%s/%s\" &\n", exeDir.c_str(), exeName.c_str());
    script += lineBuf;
    script += "rm -f \"$0\"\n";
    p3d::io::WriteTextFile(scriptPath, script);
    chmod(scriptPath.c_str(), 0755);

    std::string cmd = "sh \"" + scriptPath + "\" &";
    system(cmd.c_str());
#endif

    if (g_game) {
        g_game->SetState(GameState::End);
    }
}

void AutoUpdater::CheckThreadFunc() {
    std::string owner = UPDATE_REPO_OWNER;
    std::string repo = UPDATE_REPO_NAME;
    std::string url = "https://api.github.com/repos/" + owner + "/" + repo + "/releases/latest";

    std::string apiResponse;

#ifdef RC_PLATFORM_WINDOWS
    std::string curlCmd = "curl -sL -H \"Accept: application/vnd.github+json\" -H \"X-GitHub-Api-Version: 2022-11-28\" -H \"User-Agent: rechan-updater\" \"" + url + "\"";
#else
    std::string curlCmd = "curl -sL -H 'Accept: application/vnd.github+json' -H 'X-GitHub-Api-Version: 2022-11-28' -H 'User-Agent: rechan-updater' '" + url + "'";
#endif
    apiResponse = shellExec(curlCmd.c_str());

    if (apiResponse.empty()) {
        m_error = "No response from GitHub API";
        m_state = State::Error;
        return;
    }

    m_latestTag = jsonGetString(apiResponse, "tag_name");
    if (m_latestTag.empty()) {
        m_error = "Could not parse release tag from API response";
        m_state = State::Error;
        return;
    }

    if (!m_latestSemver.Parse(m_latestTag.c_str())) {
        m_error = "Could not parse version: " + m_latestTag;
        m_state = State::Error;
        return;
    }

    if (!(m_latestSemver > m_currentSemver)) {
        m_state = State::UpToDate;
        return;
    }

    std::string platformStr = GetPlatformString();
    m_updateAssetUrl = jsonGetAssetUrl(apiResponse, platformStr);
    if (m_updateAssetUrl.empty()) {
        m_error = "No update asset found for platform: " + platformStr;
        m_state = State::Error;
        return;
    }

    m_downloadSize = jsonGetAssetSize(apiResponse, platformStr);

    m_releaseNotes = jsonGetString(apiResponse, "body");
    m_state = State::UpdateAvailable;
}

void AutoUpdater::DownloadThreadFunc() {
    std::string url = m_updateAssetUrl;
    std::string outPath = m_tempArchivePath;

    // -f makes curl fail (non-zero, no output file) on HTTP 4xx/5xx instead of
    // silently writing the error-page body into the archive; -L follows the
    // release-asset redirect to the CDN.
#ifdef RC_PLATFORM_WINDOWS
    std::string cmd = "curl -fsSL -o \"" + outPath + "\" -w \"%{size_download}\" \"" + url + "\"";
#else
    std::string cmd = "curl -fsSL -o '" + outPath + "' -w '%{size_download}' '" + url + "'";
#endif

    std::string result = shellExec(cmd.c_str());

    if (m_cancelDownload) {
        remove(outPath.c_str());
        return;
    }

    if (!p3d::io::FileExists(outPath.c_str())) {
        m_error = "Download failed: output file not created";
        m_state = State::Error;
        return;
    }

    // curl -w reports the bytes actually transferred; when the release metadata
    // told us the expected size, reject a truncated/mismatched download before it
    // reaches the extractor.
    if (m_downloadSize > 0) {
        const s64 actualBytes = (s64)strtoll(result.c_str(), nullptr, 10);
        if (actualBytes > 0 && actualBytes != m_downloadSize) {
            remove(outPath.c_str());
            m_error = "Download failed: size mismatch";
            m_state = State::Error;
            return;
        }
    }

    m_downloadedBytes = m_downloadSize;
    m_state = State::ReadyToInstall;
}

#endif
