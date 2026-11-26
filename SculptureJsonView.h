#pragma once

#include <initializer_list>
#include <string_view>

namespace epi_sculpt::json_view {

// Borrowed, bounded JSON traversal shared by macro decoding and outer epimode
// payload selection. No DOM, allocation, IO or semantic state. Callbacks receive
// raw member names/values; schema owners decide which names and types to accept.
// Unknown nested values are traversed to a maximum depth of 32.
class Reader {
public:
    explicit Reader(std::string_view text) : text_(text) {}

    bool take(char token) noexcept {
        whitespace();
        if (pos_ == text_.size() || text_[pos_] != token) return false;
        ++pos_;
        return true;
    }
    bool done() noexcept { whitespace(); return pos_ == text_.size(); }

    bool string(std::string_view& result) noexcept {
        if (!take('"')) return false;
        const auto start = pos_;
        while (pos_ < text_.size()) {
            const auto c = static_cast<unsigned char>(text_[pos_++]);
            if (c == '"') {
                result = text_.substr(start, pos_ - start - 1U);
                return true;
            }
            if (c < 0x20U) return false;
            if (c != '\\') continue;
            if (pos_ == text_.size()) return false;
            const char escape = text_[pos_++];
            if (escape == 'u') {
                for (int digit = 0; digit < 4; ++digit) {
                    if (pos_ == text_.size()) return false;
                    const char h = text_[pos_++];
                    if (!((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f')
                          || (h >= 'A' && h <= 'F'))) return false;
                }
            } else if (std::string_view("\"\\/bfnrt").find(escape) == std::string_view::npos) {
                return false;
            }
        }
        return false;
    }

    bool value(std::string_view& result) noexcept {
        whitespace();
        const auto start = pos_;
        if (!skipValue(0U)) return false;
        result = text_.substr(start, pos_ - start);
        return true;
    }

private:
    void whitespace() noexcept {
        while (pos_ < text_.size() && std::string_view(" \t\r\n").find(text_[pos_])
               != std::string_view::npos) ++pos_;
    }
    bool digit() const noexcept {
        return pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9';
    }
    bool skipValue(unsigned depth) noexcept {
        whitespace();
        if (depth > 32U || pos_ == text_.size()) return false;
        const char first = text_[pos_];
        if (first == '"') {
            std::string_view ignored;
            return string(ignored);
        }
        if (first == '{' || first == '[') {
            ++pos_;
            const char close = first == '{' ? '}' : ']';
            if (take(close)) return true;
            do {
                if (first == '{') {
                    std::string_view key;
                    if (!string(key) || !take(':')) return false;
                }
                if (!skipValue(depth + 1U)) return false;
                if (take(close)) return true;
            } while (take(','));
            return false;
        }
        for (const auto literal : {std::string_view("true"), std::string_view("false"),
                                   std::string_view("null")}) {
            if (text_.substr(pos_).starts_with(literal)) {
                pos_ += literal.size();
                return true;
            }
        }
        if (first == '-') ++pos_;
        if (!digit()) return false;
        if (text_[pos_] == '0') ++pos_;
        else while (digit()) ++pos_;
        if (pos_ < text_.size() && text_[pos_] == '.') {
            ++pos_;
            if (!digit()) return false;
            while (digit()) ++pos_;
        }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
            if (!digit()) return false;
            while (digit()) ++pos_;
        }
        return true;
    }
    std::string_view text_;
    std::size_t pos_ = 0U;
};

template <typename Field>
bool object(std::string_view text, Field&& field) {
    Reader reader(text);
    if (!reader.take('{')) return false;
    if (reader.take('}')) return reader.done();
    do {
        std::string_view key, value;
        if (!reader.string(key) || !reader.take(':') || !reader.value(value) || !field(key, value)) return false;
        if (reader.take('}')) return reader.done();
    } while (reader.take(','));
    return false;
}

template <typename Element>
bool array(std::string_view text, Element&& element) {
    Reader reader(text);
    if (!reader.take('[')) return false;
    if (reader.take(']')) return reader.done();
    do {
        std::string_view value;
        if (!reader.value(value) || !element(value)) return false;
        if (reader.take(']')) return reader.done();
    } while (reader.take(','));
    return false;
}

} // namespace epi_sculpt::json_view
