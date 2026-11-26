#pragma once

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace epi_sculpt {

// A borrowed read model of authored metadata, independent of native resources.
struct SculpturePresetSearchText {
    std::string_view id;
    std::string_view title;
    std::string_view category;
    std::string_view description;
};

inline constexpr std::string_view kPresetSearchWhitespace = " \t\r\n\f\v";

[[nodiscard]] constexpr bool hasPresetSearchTerms(std::string_view query) noexcept {
    return query.find_first_not_of(kPresetSearchWhitespace) != std::string_view::npos;
}

namespace preset_search_detail {

[[nodiscard]] constexpr char fold(char c) noexcept {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c;
}

[[nodiscard]] constexpr bool equal(std::string_view a, std::string_view b) noexcept {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
        [](char x, char y) { return fold(x) == fold(y); });
}

[[nodiscard]] inline bool contains(std::string_view field, std::string_view term) noexcept {
    return std::search(field.begin(), field.end(), term.begin(), term.end(),
        [](char x, char y) { return fold(x) == fold(y); }) != field.end();
}

[[nodiscard]] constexpr std::string_view takeTerm(std::string_view& remaining) noexcept {
    const auto start = remaining.find_first_not_of(kPresetSearchWhitespace);
    if (start == std::string_view::npos) {
        remaining = {};
        return {};
    }
    remaining.remove_prefix(start);
    // A balanced, nonempty quoted phrase is one contiguous field match.
    // Unmatched or empty quotes retain the ordinary literal-token behavior.
    if (remaining.front() == '"') {
        const auto end = remaining.find('"', 1);
        if (end != std::string_view::npos && end > 1
            && remaining.substr(1, end - 1).find_first_not_of(kPresetSearchWhitespace)
                != std::string_view::npos) {
            const auto phrase = remaining.substr(1, end - 1);
            remaining.remove_prefix(end + 1);
            return phrase;
        }
    }
    const auto term = remaining.substr(0, remaining.find_first_of(kPresetSearchWhitespace));
    remaining.remove_prefix(term.size());
    return term;
}

enum class SearchField { Any, Id, Title, Category, Description };

struct TermScore {
    std::size_t score = 0;
    SearchField field = SearchField::Any;
    bool exact = false;
};

struct SearchTerm {
    std::string_view text;
    bool excluded = false;
    SearchField field = SearchField::Any;
};

// A leading '-' excludes a nonempty term or quoted phrase. Known field
// prefixes scope substring matching; a fully quoted token stays literal.
[[nodiscard]] constexpr SearchTerm takeSearchTerm(std::string_view& remaining) noexcept {
    const auto start = remaining.find_first_not_of(kPresetSearchWhitespace);
    if (start == std::string_view::npos) {
        remaining = {};
        return {};
    }
    remaining.remove_prefix(start);
    const bool excluded = remaining.front() == '-' && remaining.size() > 1
        && kPresetSearchWhitespace.find(remaining[1]) == std::string_view::npos;
    if (excluded) remaining.remove_prefix(1);
    SearchField field = SearchField::Any;
    // Recognize only known prefixes with an immediate, nonempty value. A
    // quoted token, an unknown prefix, or a bare "title:" stays literal.
    if (remaining.front() != '"') {
        const auto colon = remaining.find(':');
        if (colon != std::string_view::npos && colon + 1 < remaining.size()
            && kPresetSearchWhitespace.find(remaining[colon + 1]) == std::string_view::npos) {
            const auto name = remaining.substr(0, colon);
            if (equal(name, "id")) field = SearchField::Id;
            else if (equal(name, "title")) field = SearchField::Title;
            else if (equal(name, "category")) field = SearchField::Category;
            else if (equal(name, "description")) field = SearchField::Description;
            if (field != SearchField::Any) remaining.remove_prefix(colon + 1);
        }
    }
    return {takeTerm(remaining), excluded, field};
}

[[nodiscard]] inline bool matchesTerm(
    const SculpturePresetSearchText& preset, std::string_view term,
    SearchField field = SearchField::Any) noexcept {
    switch (field) {
    case SearchField::Id: return contains(preset.id, term);
    case SearchField::Title: return contains(preset.title, term);
    case SearchField::Category: return contains(preset.category, term);
    case SearchField::Description: return contains(preset.description, term);
    case SearchField::Any: break;
    }
    const std::array fields{preset.id, preset.title, preset.category, preset.description};
    return std::any_of(fields.begin(), fields.end(),
        [&](auto field) { return contains(field, term); });
}

[[nodiscard]] inline TermScore explainTermScore(
    const SculpturePresetSearchText& preset, std::string_view term,
    SearchField field = SearchField::Any) noexcept {
    const std::array fields{preset.id, preset.title, preset.category, preset.description};
    constexpr std::array fieldNames{SearchField::Id, SearchField::Title,
        SearchField::Category, SearchField::Description};
    constexpr std::array<std::size_t, 4> substringWeights{4, 8, 2, 1};
    constexpr std::array<std::size_t, 4> exactWeights{32, 16, 6, 1};
    TermScore best{};
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (field != SearchField::Any && field != fieldNames[i]) continue;
        const bool exact = equal(fields[i], term);
        const auto score = exact ? exactWeights[i]
            : contains(fields[i], term) ? substringWeights[i] : 0U;
        // First field wins a tie, making explanations stable without changing
        // the pre-existing maximum-field scoring policy.
        if (score > best.score) best = {score, fieldNames[i], exact};
    }
    return best;
}

[[nodiscard]] inline std::size_t scoreTerm(
    const SculpturePresetSearchText& preset, std::string_view term,
    SearchField field = SearchField::Any) noexcept {
    return explainTermScore(preset, term, field).score;
}

[[nodiscard]] constexpr std::size_t addScore(std::size_t score, std::size_t termScore) noexcept {
    return score + std::min(termScore, std::numeric_limits<std::size_t>::max() - score);
}

} // namespace preset_search_detail

// Category filters compare the complete authored category, not a substring.
[[nodiscard]] constexpr bool matchesPresetCategory(
    std::string_view category, std::string_view requested) noexcept {
    return preset_search_detail::equal(category, requested);
}

// Positive terms must match; excluded terms must not match any field. An empty
// or exclusion-only query scores 0 when admitted. Distinct positive ASCII-folded
// terms contribute their strongest field match. The fixed
// weights favor exact identity, then exact/title matches over incidental text.
// This read model allocates nothing and never changes authored selection/state.
[[nodiscard]] inline std::optional<std::size_t> scorePresetSearch(
    const SculpturePresetSearchText& preset, std::string_view query) noexcept {
    std::size_t score = 0;
    auto remaining = query;
    while (hasPresetSearchTerms(remaining)) {
        auto previous = query.substr(0, query.size() - remaining.size());
        const auto term = preset_search_detail::takeSearchTerm(remaining);
        if (term.excluded) {
            if (preset_search_detail::matchesTerm(preset, term.text, term.field)) return std::nullopt;
            continue;
        }
        bool duplicate = false;
        for (auto seen = preset_search_detail::takeSearchTerm(previous); !seen.text.empty();
             seen = preset_search_detail::takeSearchTerm(previous)) {
            if (!seen.excluded && seen.field == term.field
                && preset_search_detail::equal(seen.text, term.text)) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;

        const auto best = preset_search_detail::scoreTerm(preset, term.text, term.field);
        if (best == 0) return std::nullopt;
        score = preset_search_detail::addScore(score, best);
    }
    return score;
}

// ASCII case folding is locale-independent; other UTF-8 bytes match unchanged.
// Empty queries match everything so callers can compose optional filters.
[[nodiscard]] inline bool matchesPresetSearch(
    const SculpturePresetSearchText& preset, std::string_view query) noexcept {
    for (auto term = preset_search_detail::takeSearchTerm(query); !term.text.empty();
         term = preset_search_detail::takeSearchTerm(query)) {
        if (preset_search_detail::matchesTerm(preset, term.text, term.field) == term.excluded) return false;
    }
    return true;
}

// Repeated catalog queries prepare text once. Owning the distinct terms keeps
// copies/moves safe even when callers replace or destroy their original input.
// Matching/scoring after construction allocate nothing and borrow only metadata.
class SculpturePresetSearchQuery {
public:
    explicit SculpturePresetSearchQuery(std::string_view query) {
        for (auto term = preset_search_detail::takeSearchTerm(query); !term.text.empty();
             term = preset_search_detail::takeSearchTerm(query)) {
            const bool duplicate = std::any_of(terms_.begin(), terms_.end(),
                [&](const auto& seen) {
                    return seen.excluded == term.excluded && seen.field == term.field
                        && preset_search_detail::equal(seen.text, term.text);
                });
            if (!duplicate) terms_.push_back({std::string(term.text), term.excluded, term.field});
        }
    }

    [[nodiscard]] bool matches(const SculpturePresetSearchText& preset) const noexcept {
        return std::all_of(terms_.begin(), terms_.end(),
            [&](const auto& term) {
                return preset_search_detail::matchesTerm(preset, term.text, term.field) != term.excluded;
            });
    }

    [[nodiscard]] std::optional<std::size_t> score(
        const SculpturePresetSearchText& preset) const noexcept {
        std::size_t result = 0;
        for (const auto& term : terms_) {
            if (term.excluded) {
                if (preset_search_detail::matchesTerm(preset, term.text, term.field)) return std::nullopt;
                continue;
            }
            const auto best = preset_search_detail::scoreTerm(preset, term.text, term.field);
            if (best == 0) return std::nullopt;
            result = preset_search_detail::addScore(result, best);
        }
        return result;
    }

    // Explain only admitted matches: callers never receive partial evidence for
    // a later missing positive term or matched exclusion. Term text is borrowed
    // from this query for the duration of the callback.
    template <typename Visitor>
    [[nodiscard]] std::optional<std::size_t> explain(
        const SculpturePresetSearchText& preset, Visitor&& visitor) const {
        const auto total = score(preset);
        if (!total) return std::nullopt;
        for (const auto& term : terms_) {
            if (term.excluded) continue;
            const auto match = preset_search_detail::explainTermScore(preset, term.text, term.field);
            visitor(std::string_view(term.text), term.field, match);
        }
        return total;
    }

private:
    struct OwnedTerm {
        std::string text;
        bool excluded = false;
        preset_search_detail::SearchField field = preset_search_detail::SearchField::Any;
    };
    std::vector<OwnedTerm> terms_;
};

} // namespace epi_sculpt
