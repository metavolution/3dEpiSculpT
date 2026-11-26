#pragma once

#include "SculptureParameterRegistry.h"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string_view>

namespace epi_sculpt {
namespace capture_epimode_detail {

// Validate the recorder's JSON object before handing it to the permissive legacy
// epimode decoder. This is bounded by StateLoadLimits and a nesting limit.
class JsonAdmission final {
public:
    explicit JsonAdmission(std::string_view text) noexcept : text_(text) {}

    [[nodiscard]] bool capture() noexcept {
        skipSpace();
        if (!object(0)) return false;
        skipSpace();
        if (pos_ != text_.size() || required_ != kRequired) return false;
        for (bool seen : floatFields_) if (!seen) return false;
        for (bool seen : uintFields_) if (!seen) return false;
        // The legacy decoder searches text rather than JSON object members.
        // Its chosen optional presentation must be this object's member, never
        // a similarly named nested value or string fragment. Macro extraction
        // is structural and already selects only this object's member.
        if (legacyValueStart("presentation") != presentationValue_) return false;
        if (text_.find("\"route_", routeSearch_) != std::string_view::npos) return false;
        return true;
    }

private:
    static constexpr std::uint32_t kRequired = (1U << 24U) - 1U;
    std::string_view text_;
    std::size_t pos_ = 0;
    std::uint32_t required_ = 0;
    std::array<bool, kFloatParams.size()> floatFields_{};
    std::array<bool, kUintParams.size()> uintFields_{};
    std::optional<std::size_t> presentationValue_;
    std::optional<std::size_t> macroKey_;
    std::size_t routeSearch_ = 0;

    // Mirrors the decoder's search, including its treatment of nested keys.
    // Compare it with the parsed top-level value before admitting the input.
    [[nodiscard]] std::optional<std::size_t> legacyValueStart(std::string_view key) const noexcept {
        std::size_t search = 0;
        for (;;) {
            const auto found = text_.find(key, search);
            if (found == std::string_view::npos) return std::nullopt;
            const auto end = found + key.size();
            if (found > 0 && text_[found - 1] == '"' && end < text_.size() && text_[end] == '"') {
                auto pos = end + 1;
                while (pos < text_.size() && (text_[pos] == ' ' || text_[pos] == '\n'
                       || text_[pos] == '\r' || text_[pos] == '\t' || text_[pos] == '\f' || text_[pos] == '\v')) ++pos;
                if (pos < text_.size() && text_[pos] == ':') {
                    ++pos;
                    while (pos < text_.size() && (text_[pos] == ' ' || text_[pos] == '\n'
                           || text_[pos] == '\r' || text_[pos] == '\t' || text_[pos] == '\f' || text_[pos] == '\v')) ++pos;
                    return pos;
                }
            }
            search = found + key.size();
        }
    }

    void skipSpace() noexcept {
        while (pos_ < text_.size() && (text_[pos_] == ' ' || text_[pos_] == '\n'
               || text_[pos_] == '\r' || text_[pos_] == '\t')) ++pos_;
    }
    bool take(char c) noexcept {
        if (pos_ == text_.size() || text_[pos_] != c) return false;
        ++pos_; return true;
    }
    bool string(std::string_view* raw = nullptr) noexcept {
        if (!take('"')) return false;
        const auto start = pos_;
        while (pos_ < text_.size()) {
            const unsigned char c = static_cast<unsigned char>(text_[pos_++]);
            if (c == '"') {
                if (raw) *raw = text_.substr(start, pos_ - start - 1);
                return true;
            }
            if (c < 0x20) return false;
            if (c != '\\') continue;
            if (pos_ == text_.size()) return false;
            const char escaped = text_[pos_++];
            if (escaped == 'u') {
                for (int i = 0; i < 4; ++i) {
                    if (pos_ == text_.size()) return false;
                    const char h = text_[pos_++];
                    if (!((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f')
                          || (h >= 'A' && h <= 'F'))) return false;
                }
            } else if (std::string_view("\"\\/bfnrt").find(escaped) == std::string_view::npos) {
                return false;
            }
        }
        return false;
    }
    bool number() noexcept {
        take('-');
        if (pos_ == text_.size()) return false;
        if (text_[pos_] == '0') ++pos_;
        else {
            if (text_[pos_] < '1' || text_[pos_] > '9') return false;
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') ++pos_;
        }
        if (take('.')) {
            const auto start = pos_;
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') ++pos_;
            if (pos_ == start) return false;
        }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
            const auto start = pos_;
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') ++pos_;
            if (pos_ == start) return false;
        }
        return true;
    }
    bool value(unsigned depth) noexcept {
        if (depth > 32 || pos_ == text_.size()) return false;
        if (text_[pos_] == '{') return object(depth);
        if (text_[pos_] == '[') return array(depth);
        if (text_[pos_] == '"') return string();
        for (auto literal : {std::string_view("true"), std::string_view("false"), std::string_view("null")}) {
            if (text_.substr(pos_, literal.size()) == literal) { pos_ += literal.size(); return true; }
        }
        return number();
    }
    bool array(unsigned depth) noexcept {
        if (depth > 32 || !take('[')) return false;
        skipSpace();
        if (take(']')) return true;
        for (;;) {
            if (!value(depth + 1)) return false;
            skipSpace();
            if (take(']')) return true;
            if (!take(',')) return false;
            skipSpace();
        }
    }
    bool object(unsigned depth) noexcept {
        if (depth > 32 || !take('{')) return false;
        skipSpace();
        if (take('}')) return true;
        for (;;) {
            std::string_view key;
            const auto keyStart = pos_;
            if (!string(&key)) return false;
            skipSpace();
            if (!take(':')) return false;
            skipSpace();
            const auto start = pos_;
            if (!value(depth + 1)) return false;
            if (depth == 0) {
                constexpr std::string_view keys[]{"version", "preset", "time", "morph", "cameraAngle",
                    "camera_override", "cameraEye", "cameraTarget", "cameraUp", "inner", "innerB",
                    "reduction", "reductionB", "seedType", "seedTypeB", "colorEffect", "pre_scaleReal",
                    "pre_scaleImag", "pre_offsetReal", "pre_offsetImag", "post_scaleReal", "post_scaleImag",
                    "post_offsetReal", "post_offsetImag"};
                for (unsigned i = 0; i < 24; ++i) {
                    if (key != keys[i]) continue;
                    const auto bit = 1U << i;
                    if ((required_ & bit) || legacyValueStart(key) != start
                        || (i == 0 && text_.substr(start, pos_ - start) != "1")) return false;
                    required_ |= bit;
                }
                for (std::size_t i = 0; i < kFloatParams.size(); ++i) {
                    if (key == kFloatParams[i].name) {
                        if (floatFields_[i] || legacyValueStart(key) != start) return false;
                        floatFields_[i] = true;
                    }
                }
                for (std::size_t i = 0; i < kUintParams.size(); ++i) {
                    if (key == kUintParams[i].name) {
                        if (uintFields_[i] || legacyValueStart(key) != start) return false;
                        uintFields_[i] = true;
                    }
                }
                if (key == "presentation") {
                    if (presentationValue_) return false;
                    presentationValue_ = start;
                }
                if (key.starts_with("route_")) {
                    if (text_.find("\"route_", routeSearch_) != keyStart
                        || text_[start] != '"'
                        || text_.substr(start, pos_ - start).find('\\') != std::string_view::npos) return false;
                    routeSearch_ = pos_;
                }
                if (key == "macro_automation") {
                    if (macroKey_ || text_[start] != '{') return false;
                    macroKey_ = keyStart;
                }
            }
            skipSpace();
            if (take('}')) return true;
            if (!take(',')) return false;
            skipSpace();
        }
    }
};
} // namespace capture_epimode_detail

[[nodiscard]] inline bool captureEpimodeAdmitted(std::string_view text) noexcept {
    return capture_epimode_detail::JsonAdmission(text).capture();
}
} // namespace epi_sculpt
