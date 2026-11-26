#pragma once

#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace epi_sculpt {

// A deliberately small OpenCL include expander for the compact kernel only.
// It freezes the literal quoted-include closure before asynchronous compilation;
// it is not a general C preprocessor and rejects syntax outside this contract.
struct SculptureCompactSourceBundle {
    std::string source;
    std::string diagnostic;

    [[nodiscard]] static std::optional<SculptureCompactSourceBundle> prepare(
        const std::filesystem::path& kernel) noexcept {
        try {
            namespace fs = std::filesystem;
            const fs::path root = fs::weakly_canonical(kernel.parent_path());
            const fs::path entry = fs::weakly_canonical(kernel);
            if (root.empty() || entry.empty() || entry.parent_path() != root) return std::nullopt;
            State state{root};
            if (!state.expand(entry, entry.filename().generic_string(), 0U)) return std::nullopt;
            return SculptureCompactSourceBundle{std::move(state.output), {}};
        } catch (...) {
            return std::nullopt;
        }
    }

private:
    static constexpr std::size_t kMaxFileBytes = 256U * 1024U;
    static constexpr std::size_t kMaxExpandedBytes = 1024U * 1024U;
    static constexpr std::size_t kMaxUniqueFiles = 64U;
    static constexpr std::size_t kMaxIncludeDepth = 16U;
    static constexpr std::size_t kMaxIncludeExpansions = 128U;

    struct State {
        explicit State(std::filesystem::path rootPath) : root(std::move(rootPath)) {}
        std::filesystem::path root;
        std::string output;
        std::unordered_set<std::string> active;
        std::unordered_set<std::string> unique;
        std::size_t expansions = 0U;

        [[nodiscard]] static std::optional<std::string> readBounded(
            const std::filesystem::path& path) noexcept {
            try {
                std::error_code ec;
                const auto status = std::filesystem::symlink_status(path, ec);
                if (ec || !std::filesystem::is_regular_file(status)
                    || std::filesystem::is_symlink(status)) return std::nullopt;
                std::ifstream input(path, std::ios::binary);
                if (!input) return std::nullopt;
                std::string content;
                std::array<char, 4096> block{};
                while (input.read(block.data(), static_cast<std::streamsize>(block.size()))
                       || input.gcount() > 0) {
                    const auto count = static_cast<std::size_t>(input.gcount());
                    if (content.size() + count > kMaxFileBytes) return std::nullopt;
                    content.append(block.data(), count);
                }
                if (!input.eof() || input.bad() || content.find('\0') != std::string::npos) {
                    return std::nullopt;
                }
                return content;
            } catch (...) { return std::nullopt; }
        }

        [[nodiscard]] static std::optional<std::string> includeName(std::string_view line) noexcept {
            std::size_t i = 0U;
            const auto skip = [&] { while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) ++i; };
            skip();
            if (i == line.size() || line[i++] != '#') return std::string{};
            skip();
            if (line.substr(i, 7U) != "include") return std::string{};
            i += 7U; skip();
            if (i == line.size() || line[i++] != '"') return std::nullopt;
            const auto end = line.find('"', i);
            if (end == std::string_view::npos) return std::nullopt;
            const std::string name(line.substr(i, end - i));
            i = end + 1U; skip();
            if (i != line.size() || name.empty()) return std::nullopt;
            const std::filesystem::path relative(name);
            if (relative.is_absolute() || name.find('\\') != std::string::npos
                || name.find("..") != std::string::npos) return std::nullopt;
            return name;
        }

        [[nodiscard]] bool append(std::string_view text) noexcept {
            if (output.size() + text.size() > kMaxExpandedBytes) return false;
            output.append(text);
            return true;
        }

        [[nodiscard]] bool expand(const std::filesystem::path& path,
                                  const std::string& key, std::size_t depth) noexcept {
            if (depth > kMaxIncludeDepth || ++expansions > kMaxIncludeExpansions
                || !active.insert(key).second) return false;
            if (unique.insert(key).second && unique.size() > kMaxUniqueFiles) return false;
            const auto content = readBounded(path);
            if (!content) return false;
            std::size_t start = 0U;
            while (start < content->size()) {
                const auto end = content->find('\n', start);
                const auto length = (end == std::string::npos ? content->size() : end) - start;
                const std::string_view line(content->data() + start, length);
                const auto include = includeName(line);
                if (!include) return false;
                if (include->empty()) {
                    if (!append(line) || !append("\n")) return false;
                } else {
                    const auto child = std::filesystem::weakly_canonical(path.parent_path() / *include);
                    const auto relative = child.lexically_relative(root);
                    const auto relativeText = relative.generic_string();
                    if (relative.empty() || relativeText.compare(0U, 2U, "..") == 0
                        || !expand(child, relative.generic_string(), depth + 1U)) return false;
                }
                if (end == std::string::npos) break;
                start = end + 1U;
            }
            active.erase(key);
            return true;
        }
    };
};

} // namespace epi_sculpt
