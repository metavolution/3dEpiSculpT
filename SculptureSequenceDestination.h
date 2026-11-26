#pragma once
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>

namespace epi_sculpt {
// App-owned destination, resolved before readback admission. Event handlers do
// no IO; accepted callbacks copy the path and never consult mutable session state.
class SculptureSequenceDestination {
public:
    static std::optional<std::string> reserve(const std::filesystem::path& root,
                                              const std::string& stem) noexcept {
        try {
            // Stems are internal identifiers, never paths from event labels.
            if (stem.empty() || stem.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string::npos)
                return std::nullopt;
            const auto base = std::filesystem::absolute(root);
            std::filesystem::create_directories(base);
            for (unsigned i = 0; i < 128; ++i) {
                const auto path = base / (stem + "-" + std::to_string(i));
                auto name = path.string(); // Allocate before reserving ownership.
                std::error_code error;
                if (std::filesystem::create_directory(path, error)) return name;
                if (error && error != std::errc::file_exists) return std::nullopt;
            }
        } catch (...) {}
        return std::nullopt;
    }

    std::optional<std::string> directoryFor(std::uint64_t session,
                                          const std::filesystem::path& root = "snapshots") {
        if (directory_ && session_ == session) return directory_;
        const auto tick = std::chrono::system_clock::now().time_since_epoch().count();
        auto next = reserve(root, "sequence-" + std::to_string(tick));
        if (!next) return std::nullopt;
        directory_ = std::move(next);
        session_ = session;
        return directory_;
    }
private:
    std::uint64_t session_ = 0;
    std::optional<std::string> directory_;
};
} // namespace epi_sculpt
