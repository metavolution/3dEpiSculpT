#include "SculptureProgramSourceFingerprint.h"

#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <limits>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace epi_sculpt {

namespace {

static BinaryCacheStats g_stats{};
static std::mutex g_statsMutex{};

// Each publisher owns an exclusively reserved directory on the cache's
// filesystem. Cleanup never touches another writer or the published artifact.
struct CacheWriteScratch {
    std::filesystem::path directory;
    std::filesystem::path binary;
    bool owned = false;

    CacheWriteScratch(const std::filesystem::path& parent, const std::string& name)
        : directory(parent / name), binary(directory / "program.bin") {}
    CacheWriteScratch(const CacheWriteScratch&) = delete;
    CacheWriteScratch& operator=(const CacheWriteScratch&) = delete;
    ~CacheWriteScratch() noexcept {
        if (!owned) return;
        std::error_code ignored;
        std::filesystem::remove(binary, ignored);
        std::filesystem::remove(directory, ignored);
    }
};

std::string sanitizeString(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-') {
            out.push_back(c);
        } else if (c == ' ' || c == '.' || c == ':') {
            out.push_back('_');
        }
    }
    return out;
}

// Length framing makes (prefix, source, options, headers) distinct even when
// two tuples concatenate to the same bytes. The on-disk key is versioned too.
struct SourceDigest {
    std::uint64_t value = 0xcbf29ce484222325ULL;
    void byte(std::uint8_t v) { value = (value ^ v) * 0x100000001b3ULL; }
    void field(std::string_view text) {
        auto size = static_cast<std::uint64_t>(text.size());
        for (unsigned i = 0; i < 8; ++i) { byte(size & 0xffU); size >>= 8; }
        for (unsigned char c : text) byte(c);
    }
};

std::vector<std::filesystem::path> extractIncludeDirectories(std::string_view options) {
    // Parse option tokens, not occurrences of "-I" inside macro values.
    std::vector<std::string> tokens;
    std::string token;
    char quote = 0;
    bool started = false;
    for (std::size_t i = 0; i < options.size(); ++i) {
        const char c = options[i];
        if (c == '\\') {
            if (++i == options.size()) throw std::invalid_argument("truncated build option");
            token += options[i]; started = true;
        } else if (quote) {
            if (c == quote) quote = 0; else token += c;
        } else if (c == '"' || c == '\'') {
            quote = c; started = true;
        } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if (started) { tokens.push_back(std::move(token)); token.clear(); started = false; }
        } else { token += c; started = true; }
    }
    if (quote) throw std::invalid_argument("unterminated build option");
    if (started) tokens.push_back(std::move(token));
    std::vector<std::filesystem::path> dirs;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& t = tokens[i];
        if (t.starts_with("-include") || t.starts_with("-imacros")
            || t.starts_with("-isystem") || t.starts_with("-iquote"))
            throw std::invalid_argument("unsupported dependency build option");
        if (!t.starts_with("-I")) continue;
        auto dir = t.substr(2);
        if (dir.empty()) {
            if (++i == tokens.size()) throw std::invalid_argument("missing include directory");
            dir = tokens[i];
        }
        if (dir.empty()) throw std::invalid_argument("empty include directory");
        dirs.emplace_back(dir); // Preserve compiler order, including missing roots.
    }
    return dirs;
}

std::string dependencyText(std::string_view source) {
    // Translation-phase order matters: splice lines before removing comments,
    // otherwise a continued // comment could become a false include directive.
    if (source.find('\0') != source.npos || source.find("?" "?") != source.npos
        || source.find("%:") != source.npos)
        throw std::invalid_argument("unsupported dependency text");
    std::string spliced;
    for (std::size_t i = 0; i < source.size(); ++i) {
        if (source[i] == '\\' && i + 1 < source.size()) {
            if (source[i + 1] == '\n') { ++i; continue; }
            if (source[i + 1] == '\r' && i + 2 < source.size() && source[i + 2] == '\n') {
                i += 2; continue;
            }
        }
        spliced += source[i];
    }
    std::string clean;
    bool block = false, line = false;
    char quote = 0;
    for (std::size_t i = 0; i < spliced.size(); ++i) {
        const char c = spliced[i];
        const char next = i + 1 < spliced.size() ? spliced[i + 1] : '\0';
        if (block) {
            if (c == '*' && next == '/') { block = false; clean += "  "; ++i; }
            else clean += c == '\n' ? '\n' : ' ';
        } else if (line) {
            clean += c == '\n' ? '\n' : ' ';
            if (c == '\n') line = false;
        } else if (quote) {
            clean += c;
            if (c == '\\' && next) { clean += next; ++i; }
            else if (c == quote) quote = 0;
        } else if (c == '/' && (next == '*' || next == '/')) {
            block = next == '*'; line = next == '/'; clean += "  "; ++i;
        } else {
            clean += c;
            if (c == '"' || c == '\'') quote = c;
        }
    }
    if (block || quote) throw std::invalid_argument("unterminated dependency text");
    return clean;
}

struct SourceDependencyScan {
    SourceDigest digest;
    std::vector<std::filesystem::path> roots;
    std::unordered_map<std::string, std::filesystem::path> visited;
    std::vector<std::filesystem::path> headers;

    bool include(const std::filesystem::path& candidate) {
        std::error_code ec;
        const auto status = std::filesystem::status(candidate, ec);
        if (ec == std::errc::no_such_file_or_directory || ec == std::errc::not_a_directory
            || (!ec && !std::filesystem::exists(status))) return false;
        if (ec || !std::filesystem::is_regular_file(status))
            throw std::runtime_error("unreadable include candidate");
        const auto identity = std::filesystem::canonical(candidate);
        const auto logical = std::filesystem::absolute(candidate).lexically_normal();
        if (const auto it = visited.find(identity.string()); it != visited.end()) {
            // Different aliases can change relative nested-include resolution.
            if (it->second != logical) throw std::runtime_error("ambiguous include alias");
            return true; // Already read; breaks include cycles.
        }
        std::ifstream file(candidate, std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("include open failed");
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (file.bad() || file.fail()) throw std::runtime_error("include read failed");
        visited.emplace(identity.string(), logical);
        headers.push_back(candidate.lexically_normal());
        digest.field(logical.string());
        digest.field(content); // Hash exactly the bytes scanned; never reopen.
        scan(content, candidate.parent_path());
        return true;
    }

    void scan(std::string_view source, const std::filesystem::path& parent) {
        const auto clean = dependencyText(source);
        if (clean.find("__has_include") != std::string::npos)
            throw std::invalid_argument("filesystem-dependent preprocessing");
        std::istringstream lines(clean);
        std::string line;
        while (std::getline(lines, line)) {
            auto i = line.find_first_not_of(" \t\r\v\f");
            if (i == line.npos || line[i] != '#') continue;
            i = line.find_first_not_of(" \t\r\v\f", i + 1);
            if (i == line.npos) continue;
            const auto end = line.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", i);
            const auto directive = line.substr(i, end == line.npos ? end : end - i);
            if (directive == "include_next" || directive == "import" || directive == "embed")
                throw std::invalid_argument("unsupported dependency directive");
            if (directive != "include") continue;
            i = end == line.npos ? line.npos : line.find_first_not_of(" \t\r\v\f", end);
            if (i == line.npos || (line[i] != '"' && line[i] != '<'))
                throw std::invalid_argument("nonliteral include");
            const bool quoted = line[i] == '"';
            const auto close = line.find(quoted ? '"' : '>', i + 1);
            if (close == line.npos || close == i + 1
                || line.find_first_not_of(" \t\r\v\f", close + 1) != line.npos)
                throw std::invalid_argument("malformed include");
            const auto name = line.substr(i + 1, close - i - 1);
            if (name.find('\\') != name.npos)
                throw std::invalid_argument("escaped include name");
            if (quoted && !parent.empty() && include(parent / name)) continue;
            bool found = false;
            for (const auto& root : roots) {
                if (include(root / name)) { found = true; break; }
            }
            if (!found) throw std::runtime_error("unresolved include");
        }
    }
};

ProgramSourceFingerprint fingerprintInputs(std::string_view source, std::string_view options,
    std::string_view prefix, const std::filesystem::path& includeDir,
    std::vector<std::filesystem::path>* headers = nullptr) {
    SourceDependencyScan scan;
    scan.digest.field("epi-opencl-source-v2");
    scan.digest.field(prefix);
    scan.digest.field(source);
    scan.digest.field(options);
    try {
        scan.roots = extractIncludeDirectories(options);
        // A caller-supplied source directory is the quoted-include parent. For
        // legacy no-options callers it also supplies the sole include root.
        if (scan.roots.empty() && !includeDir.empty()) scan.roots.push_back(includeDir);
        // OpenCL concatenates its source strings before preprocessing. An
        // include/comment may cross this boundary even though hash fields do not.
        std::string compilerSource(prefix);
        compilerSource.append(source);
        scan.scan(compilerSource, includeDir);
    } catch (...) { return {scan.digest.value, false}; }
    if (headers) *headers = std::move(scan.headers);
    return {scan.digest.value, true};
}

} // namespace

std::string DeviceSignature::formatDescriptor() const {
    std::ostringstream ss;
    ss << sanitizeString(vendorName) << "_"
       << sanitizeString(deviceName) << "_"
       << sanitizeString(driverVersion);
    return ss.str();
}

std::string ProgramBinaryKey::computeFileName() const {
    std::ostringstream ss;
    ss << "v2_"; // Never reuse the old unqualified source-key namespace.
    if (!kernelTag.empty()) {
        ss << sanitizeString(kernelTag) << "_";
    }
    ss << sanitizeString(deviceSignature) << "_"
       << std::hex << std::setw(16) << std::setfill('0') << sourceHash
       << ".clbin";
    return ss.str();
}

std::filesystem::path ProgramBinaryKey::resolvePath(const std::filesystem::path& cacheDir) const {
    return cacheDir / computeFileName();
}

std::vector<std::filesystem::path> SculptureProgramSourceFingerprint::resolveIncludedHeaders(
    std::string_view source, std::string_view buildOptions,
    const std::filesystem::path& includeDir
) noexcept {
    try {
        std::vector<std::filesystem::path> headers;
        (void)fingerprintInputs(source, buildOptions, "", includeDir, &headers);
        return headers;
    } catch (...) { return {}; }
}

ProgramSourceFingerprint SculptureProgramSourceFingerprint::computeSourceFingerprint(
    std::string_view source, std::string_view buildOptions, std::string_view prefix,
    const std::filesystem::path& includeDir
) noexcept {
    try { return fingerprintInputs(source, buildOptions, prefix, includeDir); }
    catch (...) { return {}; }
}

std::uint64_t SculptureProgramSourceFingerprint::computeSourceHash(
    std::string_view source, std::string_view buildOptions, std::string_view prefix,
    const std::filesystem::path& includeDir
) noexcept {
    return computeSourceFingerprint(source, buildOptions, prefix, includeDir).digest;
}

bool SculptureProgramSourceFingerprint::hasBinaryOnDisk(
    const std::filesystem::path& cacheDir,
    const ProgramBinaryKey& key
) noexcept {
    try {
        const auto path = key.resolvePath(cacheDir);
        std::error_code ec;
        return std::filesystem::is_regular_file(path, ec) && std::filesystem::file_size(path, ec) > 0;
    } catch (...) {
        return false;
    }
}

std::vector<std::uint8_t> SculptureProgramSourceFingerprint::loadBinaryFromDisk(
    const std::filesystem::path& cacheDir,
    const ProgramBinaryKey& key
) noexcept {
    try {
        const auto path = key.resolvePath(cacheDir);
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return {};

        const auto size = file.tellg();
        if (size <= 0) return {};

        file.seekg(0, std::ios::beg);
        std::vector<std::uint8_t> buffer(static_cast<std::size_t>(size));
        if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            std::lock_guard<std::mutex> lock(g_statsMutex);
            g_stats.bytesLoadedFromDisk += buffer.size();
            return buffer;
        }
    } catch (...) {
    }
    return {};
}

bool SculptureProgramSourceFingerprint::saveBinaryToDisk(
    const std::filesystem::path& cacheDir,
    const ProgramBinaryKey& key,
    std::span<const std::uint8_t> binary
) noexcept {
    if (binary.empty()
        || binary.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
        return false;
    try {
        std::error_code ec;
        std::filesystem::create_directories(cacheDir, ec);
        if (ec) return false;

        const auto targetPath = key.resolvePath(cacheDir);
        static std::atomic<std::uint64_t> sequence{0};
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        // Atomic directory creation settles even cross-process name collisions.
        // Bound retries: optional caching must not trap runtime initialization.
        for (unsigned attempt = 0; attempt < 64; ++attempt) {
            CacheWriteScratch scratch(cacheDir,
                ".clbin-tmp-" + std::to_string(stamp) + "-"
                + std::to_string(sequence.fetch_add(1, std::memory_order_relaxed)));
            if (!std::filesystem::create_directory(scratch.directory, ec)) {
                if (ec) return false;
                continue; // Existing directory belongs to another attempt.
            }
            scratch.owned = true;

            std::ofstream out;
            out.exceptions(std::ios::failbit | std::ios::badbit);
            out.open(scratch.binary, std::ios::binary | std::ios::trunc);
            out.write(reinterpret_cast<const char*>(binary.data()),
                static_cast<std::streamsize>(binary.size()));
            // close() can report a delayed flush error. Do not publish until it
            // succeeds; stream destruction alone cannot establish write success.
            out.close();

            std::filesystem::rename(scratch.binary, targetPath, ec);
            if (ec) return false; // Preserve the existing target on failure.

            // Publication has succeeded. Optional telemetry cannot undo that
            // fact or falsely report an unsuccessful cache save to callers.
            try {
                std::lock_guard<std::mutex> lock(g_statsMutex);
                g_stats.bytesSavedToDisk += binary.size();
            } catch (...) { }
            return true;
        }
        return false;
    } catch (...) {
        return false;
    }
}

BinaryCacheStats SculptureProgramSourceFingerprint::stats() noexcept {
    std::lock_guard<std::mutex> lock(g_statsMutex);
    return g_stats;
}

void SculptureProgramSourceFingerprint::resetStats() noexcept {
    std::lock_guard<std::mutex> lock(g_statsMutex);
    g_stats = {};
}

void SculptureProgramSourceFingerprint::recordDiskHit(float savedCompileTimeMs) noexcept {
    try {
        std::lock_guard<std::mutex> lock(g_statsMutex);
        ++g_stats.diskHits;
        g_stats.totalCompileTimeSavedMs += savedCompileTimeMs;
    } catch (...) { }
}

void SculptureProgramSourceFingerprint::recordDiskMiss() noexcept {
    try {
        std::lock_guard<std::mutex> lock(g_statsMutex);
        ++g_stats.diskMisses;
    } catch (...) { }
}

} // namespace epi_sculpt
