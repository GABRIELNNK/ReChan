#include "p3d/fileio.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <unordered_map>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace p3d::io {
    namespace fs = std::filesystem;

    std::string NormalizeSeparators(const std::string& path) {
        std::string out = path;
        std::replace(out.begin(), out.end(), '\\', '/');
        return out;
    }

    std::string JoinPath(const std::string& a, const std::string& b) {
        if (a.empty()) return b;
        if (b.empty()) return a;
        const bool aSlash = a.back() == '/' || a.back() == '\\';
        const bool bSlash = b.front() == '/' || b.front() == '\\';
        if (aSlash && bSlash) return a + b.substr(1);
        if (aSlash || bSlash) return a + b;
        return a + "/" + b;
    }

#if defined(__linux__)
    namespace {

        // Cache of directory listing -> {lowercase name -> on-disk spelling}, so
        // ResolvePath only needs one directory_iterator pass per directory instead
        // of one per call. Guarded by g_cacheMutex since asset loading (ModLoader,
        // PsxDiscExtractor) may resolve paths from background threads.
        struct DirCache {
            std::unordered_map<std::string, std::string> lowerToActual;
        };

        std::mutex g_cacheMutex;
        std::unordered_map<std::string, DirCache> g_dirCache;

        std::string ToLowerAscii(const std::string& s) {
            std::string out = s;
            std::transform(out.begin(), out.end(), out.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
            return out;
        }

        // Caller must hold g_cacheMutex.
        const DirCache* GetOrLoadDirCacheLocked(const std::string& parentPath) {
            auto it = g_dirCache.find(parentPath);
            if (it != g_dirCache.end()) {
                return &it->second;
            }

            std::error_code ec;
            const fs::path dir = parentPath.empty() ? fs::path(".") : fs::path(parentPath);
            fs::directory_iterator iter(dir, ec);
            if (ec) {
                return nullptr;
            }

            DirCache cache;
            const fs::directory_iterator end;
            for (; iter != end; iter.increment(ec)) {
                if (ec) break;
                const std::string name = iter->path().filename().string();
                cache.lowerToActual[ToLowerAscii(name)] = name;
            }

            auto result = g_dirCache.emplace(parentPath, std::move(cache));
            return &result.first->second;
        }

    }
#endif

    std::string ResolvePath(const char* path) {
        if (!path || !path[0]) {
            return {};
        }
        return ResolvePath(std::string(path));
    }

    std::string ResolvePath(const std::string& path) {
#if !defined(__linux__)
        return path;
#else
        std::string normalized = NormalizeSeparators(path);

        std::error_code ec;
        const fs::path requested(normalized);
        if (fs::exists(requested, ec)) {
            return normalized;
        }

        std::lock_guard<std::mutex> lock(g_cacheMutex);

        fs::path resolved = requested.root_path();
        for (const fs::path& component : requested.relative_path()) {
            const fs::path exactCandidate = resolved / component;
            ec.clear();
            if (fs::exists(exactCandidate, ec)) {
                resolved = exactCandidate;
                continue;
            }

            const std::string parentKey = resolved.empty() ? std::string(".") : resolved.string();
            const DirCache* cache = GetOrLoadDirCacheLocked(parentKey);
            if (!cache) {
                return normalized;
            }

            const auto found = cache->lowerToActual.find(ToLowerAscii(component.string()));
            if (found == cache->lowerToActual.end()) {
                return normalized;
            }

            resolved /= found->second;
        }

        return resolved.string();
#endif
    }

    void InvalidateResolveCache(const std::string& dir) {
#if defined(__linux__)
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        if (dir.empty()) {
            g_dirCache.clear();
            return;
        }

        const std::string normalized = NormalizeSeparators(dir);
        for (auto it = g_dirCache.begin(); it != g_dirCache.end();) {
            if (it->first == normalized || it->first.rfind(normalized, 0) == 0) {
                it = g_dirCache.erase(it);
            }
            else {
                ++it;
            }
        }
#else
        (void)dir;
#endif
    }

    bool FileExists(const std::string& path) {
        std::error_code ec;
        return fs::is_regular_file(path, ec);
    }

    bool DirExists(const std::string& path) {
        std::error_code ec;
        return fs::is_directory(path, ec);
    }

    bool CreateDirectories(const std::string& path) {
        if (path.empty()) {
            return true;
        }
        std::error_code ec;
        fs::create_directories(path, ec);
        return !ec;
    }

    std::vector<DirEntryInfo> ListDirectory(const std::string& path, bool recursive) {
        std::vector<DirEntryInfo> result;
        std::error_code ec;

        auto addEntry = [&](const fs::directory_entry& entry) {
            DirEntryInfo info;
            info.name = entry.path().filename().string();
            info.fullPath = entry.path().string();
            info.isDirectory = entry.is_directory(ec);
            result.push_back(std::move(info));
        };

        if (recursive) {
            fs::recursive_directory_iterator iter(path, ec);
            if (ec) return result;
            const fs::recursive_directory_iterator end;
            for (; iter != end; iter.increment(ec)) {
                if (ec) break;
                addEntry(*iter);
            }
        }
        else {
            fs::directory_iterator iter(path, ec);
            if (ec) return result;
            const fs::directory_iterator end;
            for (; iter != end; iter.increment(ec)) {
                if (ec) break;
                addEntry(*iter);
            }
        }

        return result;
    }

    std::optional<std::vector<uint8_t>> ReadFile(const std::string& path) {
        FileHandle file;
        if (!file.Open(path, FileHandle::Mode::Read)) {
            return std::nullopt;
        }

        const int64_t size = file.Size();
        if (size < 0) {
            return std::nullopt;
        }

        std::vector<uint8_t> data(static_cast<size_t>(size));
        if (size > 0 && file.Read(data.data(), data.size()) != data.size()) {
            return std::nullopt;
        }

        return data;
    }

    bool WriteFile(const std::string& path, const void* data, size_t size) {
        FileHandle file;
        if (!file.Open(path, FileHandle::Mode::Write, /*resolveCase=*/false)) {
            return false;
        }
        if (size > 0 && file.Write(data, size) != size) {
            return false;
        }
        return true;
    }

    bool WriteFile(const std::string& path, const std::vector<uint8_t>& data) {
        return WriteFile(path, data.data(), data.size());
    }

    std::optional<std::string> ReadTextFile(const std::string& path) {
        auto data = ReadFile(path);
        if (!data) {
            return std::nullopt;
        }
        return std::string(reinterpret_cast<const char*>(data->data()), data->size());
    }

    bool WriteTextFile(const std::string& path, const std::string& text) {
        return WriteFile(path, text.data(), text.size());
    }

    FileHandle::~FileHandle() {
        Close();
    }

    FileHandle::FileHandle(FileHandle&& other) noexcept
        : m_file(other.m_file), m_size(other.m_size) {
        other.m_file = nullptr;
        other.m_size = -1;
    }

    FileHandle& FileHandle::operator=(FileHandle&& other) noexcept {
        if (this != &other) {
            Close();
            m_file = other.m_file;
            m_size = other.m_size;
            other.m_file = nullptr;
            other.m_size = -1;
        }
        return *this;
    }

    bool FileHandle::Open(const std::string& path, Mode mode, bool resolveCase) {
        Close();

        const char* modeStr = "rb";
        if (mode == Mode::Write) modeStr = "wb";
        else if (mode == Mode::Append) modeStr = "ab";

        const std::string resolvedPath = resolveCase ? ResolvePath(path) : path;

        FILE* file = std::fopen(resolvedPath.c_str(), modeStr);
        if (!file) {
            return false;
        }

        // Probe the size unconditionally: for "wb" this yields 0 (fopen already
        // truncated), for "ab" it yields the pre-existing length, for "rb" the
        // full file length -- matches the original ccFile::Open behaviour.
        std::fseek(file, 0, SEEK_END);
        m_size = std::ftell(file);
        std::fseek(file, 0, SEEK_SET);

        m_file = file;
        return true;
    }

    void FileHandle::Close() {
        if (m_file) {
            std::fclose(static_cast<FILE*>(m_file));
            m_file = nullptr;
        }
        m_size = -1;
    }

    int64_t FileHandle::Tell() const {
        if (!m_file) {
            return -1;
        }
        return std::ftell(static_cast<FILE*>(m_file));
    }

    bool FileHandle::Seek(int64_t offset, int whence) {
        if (!m_file) {
            return false;
        }
        return std::fseek(static_cast<FILE*>(m_file), static_cast<long>(offset), whence) == 0;
    }

    size_t FileHandle::Read(void* dst, size_t bytes) {
        if (!m_file) {
            return 0;
        }
        return std::fread(dst, 1, bytes, static_cast<FILE*>(m_file));
    }

    size_t FileHandle::Write(const void* src, size_t bytes) {
        if (!m_file) {
            return 0;
        }
        return std::fwrite(src, 1, bytes, static_cast<FILE*>(m_file));
    }

    void FileHandle::Flush() {
        if (m_file) {
            std::fflush(static_cast<FILE*>(m_file));
        }
    }

    std::string GetExecutablePath() {
#if defined(_WIN32)
        char buf[MAX_PATH];
        DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
        if (len > 0 && len < MAX_PATH) {
            return std::string(buf, len);
        }
        return {};
#elif defined(RC_PLATFORM_SWITCH)
        // No /proc/self/exe equivalent on Horizon OS, and this is dead code
        // anyway: the only caller (autoupdater.cpp) is compiled out whenever
        // RC_PLATFORM_NULL is set, which every current Switch build defines.
        return {};
#else
        char buf[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            return std::string(buf);
        }
        return {};
#endif
    }

    std::string GetExecutableDir() {
        const std::string path = GetExecutablePath();
        const size_t pos = path.find_last_of("\\/");
        return (pos != std::string::npos) ? path.substr(0, pos) : ".";
    }

    std::string GetTempDir() {
#if defined(_WIN32)
        char buf[MAX_PATH];
        DWORD len = GetTempPathA(MAX_PATH, buf);
        if (len > 0 && len < MAX_PATH) {
            return std::string(buf);
        }
        return "C:\\Windows\\Temp\\";
#else
        const char* tmp = std::getenv("TMPDIR");
        if (!tmp) tmp = "/tmp";
        std::string result(tmp);
        if (!result.empty() && result.back() != '/') result += '/';
        return result;
#endif
    }
}
