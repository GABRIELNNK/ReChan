#include "p3d/filepath.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace p3d {

#if defined(__linux__)
static bool EqualsIgnoreCase(const std::string& lhs, const std::string& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (size_t i = 0; i < lhs.size(); ++i) {
        const unsigned char a = static_cast<unsigned char>(lhs[i]);
        const unsigned char b = static_cast<unsigned char>(rhs[i]);
        if (std::tolower(a) != std::tolower(b)) {
            return false;
        }
    }
    return true;
}
#endif

std::string ResolvePathCaseInsensitive(const char* path) {
    if (!path || !path[0]) {
        return {};
    }
    return ResolvePathCaseInsensitive(std::string(path));
}

std::string ResolvePathCaseInsensitive(const std::string& path) {
#if !defined(__linux__)
    return path;
#else
    namespace fs = std::filesystem;

    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    std::error_code ec;
    const fs::path requested(normalized);
    if (fs::exists(requested, ec)) {
        return normalized;
    }

    fs::path resolved = requested.root_path();
    for (const fs::path& component : requested.relative_path()) {
        const fs::path exactCandidate = resolved / component;
        ec.clear();
        if (fs::exists(exactCandidate, ec)) {
            resolved = exactCandidate;
            continue;
        }

        const fs::path parent = resolved.empty() ? fs::path(".") : resolved;
        ec.clear();
        const fs::directory_iterator end;
        fs::directory_iterator iterator(parent, ec);
        if (ec) {
            return normalized;
        }

        bool matched = false;
        const std::string wanted = component.string();
        for (; iterator != end; iterator.increment(ec)) {
            if (ec) {
                return normalized;
            }
            if (EqualsIgnoreCase(iterator->path().filename().string(), wanted)) {
                resolved /= iterator->path().filename();
                matched = true;
                break;
            }
        }

        if (!matched) {
            return normalized;
        }
    }

    return resolved.string();
#endif
}

} // namespace p3d
