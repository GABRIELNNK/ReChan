#pragma once

#include <string>

namespace p3d {

// Returns the on-disk spelling of each existing path segment. This preserves
// the original game's case-insensitive asset lookup on Linux filesystems.
std::string ResolvePathCaseInsensitive(const char* path);
std::string ResolvePathCaseInsensitive(const std::string& path);

} // namespace p3d
