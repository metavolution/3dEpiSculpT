#pragma once

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <ostream>
#include <string>
#include <streambuf>
#include <sys/stat.h>
#include <unistd.h>

namespace epi_sculpt::file_publication_detail {

// Internal Linux/POSIX policy shared by state files and capture attachments.
// Rename publishes one complete file; it does not commit an entire capture or
// make the directory entry durable across power loss. Callers own bundle policy.
// Destination admission assumes a single writer; parent symlinks resolve normally.

[[nodiscard]] inline bool admittedFilePath(const std::string& filepath) {
    if (filepath.empty() || filepath.find('\0') != std::string::npos) return false;
    return !std::filesystem::path(filepath).filename().empty();
}

[[nodiscard]] inline bool writeAll(int fd, const char* data, std::size_t size) {
    std::size_t offset = 0;
    while (offset < size) {
        const auto count = std::min(size - offset,
            static_cast<std::size_t>(std::numeric_limits<ssize_t>::max()));
        const ssize_t written = ::write(fd, data + offset, count);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) return false;
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

// This transaction alone owns the descriptor and unpublished sibling. Encoders
// may write private staging, but only a complete, synced, closed file is renamed.
template <typename WriteContents>
[[nodiscard]] bool publishFile(const std::string& filepath, WriteContents writeContents) {
    if (!admittedFilePath(filepath)) return false;
    const std::filesystem::path destination(filepath);

    const auto parent = destination.has_parent_path()
        ? destination.parent_path() : std::filesystem::path(".");
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) return false;

    mode_t permissions = 0600;
    struct stat previous{};
    if (::lstat(destination.c_str(), &previous) == 0) {
        // Publication replaces a directory entry; never follow a destination
        // symlink or try to write a device/FIFO/directory as a state artifact.
        if (!S_ISREG(previous.st_mode)) return false;
        permissions = previous.st_mode & 0777;
    } else if (errno != ENOENT) {
        return false;
    }

    // mkstemp exclusively reserves a sibling on the same filesystem. Retain the
    // historical state-save prefix; its fixed length also supports long filenames.
    std::string staging = (parent / ".epic-world-save-XXXXXX").string();
    const int fd = ::mkstemp(staging.data());
    if (fd < 0) return false;
    struct PendingFile final {
        int fd;
        const char* path;
        bool published = false;
        ~PendingFile() {
            if (fd >= 0) ::close(fd);
            if (!published) ::unlink(path);
        }
    } pending{fd, staging.c_str()};

    if (!writeContents(fd)) return false;
    if (::fchmod(fd, permissions) != 0 || ::fsync(fd) != 0) return false;
    // On Linux, do not retry close or let cleanup close the same descriptor
    // again after an error: its numeric slot may already have been released.
    pending.fd = -1;
    if (::close(fd) != 0) return false;
    if (::rename(staging.c_str(), destination.c_str()) != 0) return false;
    pending.published = true;
    return true;
}

// No encoded-field buffer: block writes go directly to the transaction's fd.
// A sticky error is independent of ostream flags, which an encoder can clear.
class DescriptorStreamBuffer final : public std::streambuf {
public:
    explicit DescriptorStreamBuffer(int fd) : fd_(fd) {}
    [[nodiscard]] bool failed() const noexcept { return failed_; }

private:
    std::streamsize xsputn(const char* data, std::streamsize count) override {
        if (failed_) return 0;
        if (count < 0 || !writeAll(fd_, data, static_cast<std::size_t>(count))) {
            failed_ = true;
            return 0;
        }
        return count;
    }

    int_type overflow(int_type value) override {
        if (failed_) return traits_type::eof();
        if (traits_type::eq_int_type(value, traits_type::eof()))
            return traits_type::not_eof(value);
        const char byte = traits_type::to_char_type(value);
        return xsputn(&byte, 1) == 1 ? value : traits_type::eof();
    }

    int sync() override { return failed_ ? -1 : 0; }

    int fd_;
    bool failed_ = false;
};

template <typename Serialize>
[[nodiscard]] bool publishStreamFile(const std::string& filepath, Serialize serialize) {
    return publishFile(filepath, [&](int fd) {
        DescriptorStreamBuffer sink(fd);
        std::ostream stream(&sink);
        const bool encoded = serialize(stream);
        stream.flush();
        return encoded && stream.good() && !sink.failed();
    });
}

// Text retains its original encode-before-filesystem contract; binary callers
// stream private staging instead of allocating a second full field as a string.
template <typename Serialize>
[[nodiscard]] bool publishTextFile(const std::string& filepath, Serialize serialize) {
    if (!admittedFilePath(filepath)) return false;
    const std::string payload = serialize();
    return publishFile(filepath, [&](int fd) {
        return writeAll(fd, payload.data(), payload.size());
    });
}

} // namespace epi_sculpt::file_publication_detail
