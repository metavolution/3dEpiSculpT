#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <stdexcept>
#include <utility>

namespace epi_sculpt {

// Watcher-thread source identity, independent of timestamps and OpenCL ownership.
// Full bounded bytes avoid hash collisions; incomplete scans never become deletions.
class SculptureKernelSourceWatch {
public:
    using Snapshot = std::map<std::string, std::string>;
    static std::optional<Snapshot> scan(const std::filesystem::path& kernel) noexcept {
        try {
            namespace fs = std::filesystem;
            auto root = kernel.parent_path();
            if (root.empty()) root = ".";
            Snapshot result;
            std::size_t bytes = 0, entries = 0;
            constexpr std::size_t maxBytes = 16 * 1024 * 1024;
            constexpr std::size_t maxEntries = 4096;
            auto inspect = [&](const fs::directory_entry& entry) {
                if (++entries > maxEntries) throw std::runtime_error("source watch entry limit");
                const auto status = entry.symlink_status();
                if (!fs::is_regular_file(status)) return;
                const auto path = entry.path();
                if (path.lexically_normal() != kernel.lexically_normal() && path.extension() != ".cl" && path.extension() != ".clh") return;
                std::ifstream input(path, std::ios::binary);
                if (!input) throw std::runtime_error("source watch open failed");
                std::string content;
                char block[4096];
                while (input.read(block, sizeof(block)) || input.gcount()) {
                    bytes += static_cast<std::size_t>(input.gcount());
                    if (bytes > maxBytes) throw std::runtime_error("source watch byte limit");
                    content.append(block, static_cast<std::size_t>(input.gcount()));
                }
                if (!input.eof() || input.bad()) throw std::runtime_error("source watch read failed");
                result.emplace(path.lexically_relative(root).generic_string(), std::move(content));
            };
            for (const auto& entry : fs::directory_iterator(root)) inspect(entry);
            const auto components = root / "components";
            const auto status = fs::symlink_status(components);
            if (fs::is_directory(status)) {
                // Default recursive iteration does not follow directory symlinks.
                for (const auto& entry : fs::recursive_directory_iterator(components)) inspect(entry);
            }
            return result;
        } catch (...) {
            return std::nullopt;
        }
    }

    void initialize(std::optional<Snapshot> initial) { accepted_ = std::move(initial); }

    // False/throwing enqueue or failed scan retains the prior accepted identity.
    // The caller supplies active IDs through enqueue, after scanning off-thread.
    template<class Scan, class Enqueue>
    bool poll(Scan&& scanSource, Enqueue&& enqueue) {
        auto current = std::forward<Scan>(scanSource)();
        if (!current || (accepted_ && *accepted_ == *current)) return false;
        if (!std::forward<Enqueue>(enqueue)()) return false;
        accepted_.swap(current);
        return true;
    }
private:
    std::optional<Snapshot> accepted_;
};

} // namespace epi_sculpt
