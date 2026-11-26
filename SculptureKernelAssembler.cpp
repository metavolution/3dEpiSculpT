#include "SculptureKernelAssembler.h"
#include "SculptureFractalCatalog.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <array>

namespace epi_sculpt {
namespace {
// Component bodies become macro replacement lists. Apply line splicing before
// removing comments, just as OpenCL preprocessing does, then flatten only
// outside literals. In particular, a // comment must not swallow the next line.
std::string macroBody(std::string source, const std::filesystem::path& path) {
    const auto fail = [&](const char* reason) -> void {
        throw std::runtime_error("OpenCL component " + path.string() + ": " + reason);
    };
    std::string logical;
    for (std::size_t i = 0; i < source.size(); ++i) {
        if (source[i] == '\\' && i + 1 < source.size()) {
            if (source[i + 1] == '\n') { ++i; continue; }
            if (source[i + 1] == '\r' && i + 2 < source.size() && source[i + 2] == '\n') {
                i += 2; continue;
            }
        }
        logical += source[i] == '\r' ? '\n' : source[i];
    }
    std::string body;
    bool hasToken = false;
    for (std::size_t i = 0; i < logical.size();) {
        const char c = logical[i];
        if (c == '\0') fail("embedded NUL");
        if (c == '/' && i + 1 < logical.size() && logical[i + 1] == '/') {
            i = logical.find('\n', i + 2);
            if (i == std::string::npos) break;
            body += ' ';
        } else if (c == '/' && i + 1 < logical.size() && logical[i + 1] == '*') {
            const auto end = logical.find("*/", i + 2);
            if (end == std::string::npos) fail("unterminated block comment");
            body += ' '; // Preserve token boundaries: float/**/x is float x.
            i = end + 2;
        } else if (c == '\"' || c == '\'') {
            const char quote = c;
            body += logical[i++];
            bool closed = false;
            while (i < logical.size()) {
                const char value = logical[i++];
                if (value == '\n' || value == '\0') fail("newline/NUL in literal");
                body += value;
                if (value == quote) { closed = true; break; }
                if (value == '\\') {
                    if (i == logical.size() || logical[i] == '\n' || logical[i] == '\0')
                        fail("unterminated escape");
                    body += logical[i++];
                }
            }
            if (!closed) fail("unterminated literal");
            hasToken = true;
        } else {
            if (c == '#') fail("preprocessor directives/operators are not component bodies");
            const bool whitespace = std::isspace(static_cast<unsigned char>(c));
            body += whitespace ? ' ' : c;
            hasToken |= !whitespace;
            ++i;
        }
    }
    if (!hasToken) fail("empty component body");
    return body;
}
} // namespace


std::string SculptureKernelAssembler::resolveSnippet(
    std::string_view componentName,
    std::string_view fallbackCode,
    const std::filesystem::path& componentsDir,
    const std::string& category) {
    
    std::filesystem::path snippetPath = componentsDir / category / (std::string(componentName) + ".cl");
    if (std::filesystem::exists(snippetPath)) {
        std::ifstream file(snippetPath);
        if (file) {
            std::ostringstream ss;
            ss << file.rdbuf();
            if (file.bad()) throw std::runtime_error("Cannot read OpenCL component " + snippetPath.string());
            return macroBody(ss.str(), snippetPath);
        }
        throw std::runtime_error("Cannot open OpenCL component " + snippetPath.string());
    }
    return std::string(fallbackCode);
}

std::string SculptureKernelAssembler::assemblePrefix(
    const SculptureProgramParams& params,
    const std::filesystem::path& componentsDir) {
    
    struct Slot {
        std::string_view macro;
        std::string_view category;
        std::string_view component;
        std::string_view fallback;
    };
    using Catalog = SculptureFractalCatalog;
    const auto inner = [](const char* macro, InnerMap value) {
        const auto id = static_cast<std::uint32_t>(value);
        return Slot{macro, "inner", Catalog::getInnerName(id), Catalog::getInnerClCode(id)};
    };
    const auto seed = [](const char* macro, SeedType value) {
        const auto id = static_cast<std::uint32_t>(value);
        return Slot{macro, "seed", Catalog::getSeedName(id), Catalog::getSeedClCode(id)};
    };
    const auto reduction = static_cast<std::uint32_t>(params.reduction);
    const auto color = static_cast<std::uint32_t>(params.colorEffect);
    const std::array<Slot, 6> slots{{
        inner("EPI_INNER_A_CODE", params.inner),
        inner("EPI_INNER_B_CODE", params.innerB),
        seed("EPI_SEED_A_CODE", params.seedType),
        seed("EPI_SEED_B_CODE", params.seedTypeB),
        {"EPI_REDUCTION_CODE", "reduction", Catalog::getReductionName(reduction), {}},
        {"EPI_COLOR_EFFECT_CODE", "color", Catalog::getColorEffectName(color), {}},
    }};

    // Six fixed slots need no persistent cache or hash table. Resolve each
    // selected file once per build: A/B aliases share the exact same body even
    // when an editor replaces that file while this prefix is being assembled.
    std::array<std::string, slots.size()> bodies;
    std::string prefix;
    for (std::size_t i = 0; i < slots.size(); ++i) {
        const auto& slot = slots[i];
        std::size_t resolved = 0;
        while (resolved < i && (slots[resolved].category != slot.category
                               || slots[resolved].component != slot.component)) ++resolved;
        if (resolved == i) {
            bodies[i] = resolveSnippet(slot.component, slot.fallback, componentsDir,
                                       std::string(slot.category));
        }
        // Reduction and palette defaults belong to the runtime kernel, not
        // historical catalog snippets. Only explicit disk bodies opt in.
        if (slot.category == "reduction" || slot.category == "color") {
            if (bodies[resolved].empty()) continue;
            if (slot.category == "color" && color == 0u)
                throw std::runtime_error("Compact color override requires a nonzero palette ID: "
                    + (componentsDir / "color" / (std::string(slot.component) + ".cl")).string());
            const auto id = slot.category == "reduction" ? reduction : color;
            prefix += slot.category == "reduction" ? "#define EPI_REDUCTION_OVERRIDE_ID "
                                                   : "#define EPI_COLOR_EFFECT_OVERRIDE_ID ";
            prefix += std::to_string(id) + "u\n";
        }
        prefix += "#define ";
        prefix += slot.macro;
        prefix += ' ';
        prefix += bodies[resolved];
        prefix += '\n';
    }
    return prefix;
}

} // namespace epi_sculpt
