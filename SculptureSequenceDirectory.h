#pragma once

#include <filesystem>
#include <string>
#include <system_error>

namespace epi_sculpt::detail {

// The final component is claimed atomically; existing directories and files
// are never reused. A bounded suffix search makes failure explicit.
inline std::filesystem::path createSequenceDirectory(
    const std::filesystem::path& base, const std::string& name,
    std::error_code& error, unsigned maxCandidates = 256) {
    error.clear();
    const std::filesystem::path component(name);
    if (name.empty() || component != component.filename() || name == "." || name == "..") {
        error = std::make_error_code(std::errc::invalid_argument);
        return {};
    }
    std::filesystem::create_directories(base, error);
    if (error) return {};
    for (unsigned i = 0; i < maxCandidates; ++i) {
        const auto candidate = base / (name + (i == 0 ? "" : "_" + std::to_string(i)));
        if (std::filesystem::create_directory(candidate, error)) return candidate;
        if (error && error != std::errc::file_exists) return {};
        error.clear();
    }
    error = std::make_error_code(std::errc::file_exists);
    return {};
}

} // namespace epi_sculpt::detail
