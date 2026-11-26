#pragma once

#include "epi/io/bounded_text.hpp"
#include "epi/io/sha256.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace epi_sculpt {

// V2 is a bounded, content-addressed configuration bundle, not an authenticated
// document or a mathematical checkpoint. An entry attests the bytes of one
// completed file in this capture generation; relocation preserves its identity.
enum class CaptureV2Role : std::size_t { Image, World, Epimode, Epi, Profile, Causal, CanonicalMode, SeedImage, Count };
inline constexpr std::size_t kCaptureV2Roles = static_cast<std::size_t>(CaptureV2Role::Count);
inline constexpr std::array<std::string_view, kCaptureV2Roles> kCaptureV2RoleNames{
    "png", "world", "epimode", "epi", "profile", "causal", "canonical_feedback_mode", "canonical_feedback_seed_image"};
inline constexpr std::array<std::size_t, kCaptureV2Roles> kCaptureV2ByteLimits{
    256U*1024U*1024U, 8U*1024U*1024U, 8U*1024U*1024U, 8U*1024U*1024U,
    8U*1024U*1024U, 8U*1024U*1024U, 4U*1024U*1024U, 64U*1024U*1024U};
inline constexpr std::size_t kCaptureV2TotalBytes = 320U*1024U*1024U;
inline constexpr std::uint64_t kCaptureV2MaxPixels = 64ULL*1024ULL*1024ULL;

struct CaptureV2Entry {
    std::string name;
    std::size_t size = 0;
    std::array<std::uint8_t, 32> sha256{};
    bool present = false;
};
struct CaptureV2Document {
    std::string generation;
    std::uint32_t width = 0, height = 0;
    std::array<CaptureV2Entry, kCaptureV2Roles> entries;
};
struct CaptureV2Bytes {
    CaptureV2Document document;
    std::array<std::string, kCaptureV2Roles> bytes;
    [[nodiscard]] const std::string& get(CaptureV2Role role) const noexcept {
        return bytes[static_cast<std::size_t>(role)];
    }
};
[[nodiscard]] inline bool captureV2Leaf(std::string_view name) noexcept {
    return !name.empty() && name.size() <= 240 && name != "." && name != ".." && name != "none" &&
        name.find_first_of("/\\|\r\n\0", 0, 6) == std::string_view::npos;
}
[[nodiscard]] inline std::string captureV2Hex(const std::array<std::uint8_t, 32>& hash) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result; result.reserve(64);
    for (auto byte : hash) { result.push_back(digits[byte >> 4]); result.push_back(digits[byte & 15]); }
    return result;
}
[[nodiscard]] inline std::array<std::uint8_t,32> captureV2Hash(std::string_view bytes) noexcept {
    return epi::io::sha256(std::as_bytes(std::span(bytes.data(), bytes.size())));
}
[[nodiscard]] inline std::optional<CaptureV2Document> parseCaptureV2(std::string_view text) {
    CaptureV2Document doc;
    std::array<std::string, kCaptureV2Roles> references;
    std::array<unsigned, kCaptureV2Roles> referenceCounts{};
    unsigned formats=0, generations=0, volumes=0, widths=0, heights=0;
    std::size_t total=0;
    while (!text.empty()) {
        const auto end=text.find('\n'); const auto line=text.substr(0,end);
        if (line.starts_with("format=")) {
            if (++formats != 1 || line.substr(7) != "epic-capture-bundle-v2") return std::nullopt;
        } else if (line.starts_with("capture_generation=")) {
            if (++generations != 1) return std::nullopt;
            doc.generation=line.substr(19);
            if (doc.generation.size()>200 || !captureV2Leaf(doc.generation)) return std::nullopt;
        } else if (line.starts_with("capture_width=") || line.starts_with("capture_height=")) {
            const bool width = line.starts_with("capture_width=");
            auto& count = width ? widths : heights;
            auto& extent = width ? doc.width : doc.height;
            const auto value = line.substr(width ? 14 : 15);
            const auto parsed = std::from_chars(value.data(), value.data()+value.size(), extent);
            if (++count != 1 || value.empty() || parsed.ec != std::errc{} ||
                parsed.ptr != value.data()+value.size() || extent == 0) return std::nullopt;
        } else if (line.starts_with("volmem=")) {
            if (++volumes != 1 || line.substr(7) != "none") return std::nullopt;
        } else if (line.starts_with("attachment=")) {
            auto value=line.substr(11);
            auto a=value.find('|'); if (a==value.npos) return std::nullopt;
            const auto role=value.substr(0,a); value.remove_prefix(a+1);
            a=value.find('|'); if (a==value.npos) return std::nullopt;
            const auto name=value.substr(0,a); value.remove_prefix(a+1);
            a=value.find('|'); if (a==value.npos) return std::nullopt;
            const auto sizeText=value.substr(0,a); value.remove_prefix(a+1);
            std::size_t i=0; while (i<kCaptureV2Roles && role!=kCaptureV2RoleNames[i]) ++i;
            if (i==kCaptureV2Roles || doc.entries[i].present || !captureV2Leaf(name) || value.size()!=64) return std::nullopt;
            std::size_t size=0; const auto parsed=std::from_chars(sizeText.data(), sizeText.data()+sizeText.size(), size);
            if (sizeText.empty() || parsed.ec!=std::errc{} || parsed.ptr!=sizeText.data()+sizeText.size() ||
                size==0 || size>kCaptureV2ByteLimits[i] || size>kCaptureV2TotalBytes-total) return std::nullopt;
            auto& item=doc.entries[i];
            for (std::size_t j=0;j<32;++j) {
                const auto nibble=[](char c)->int { return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1; };
                const int hi=nibble(value[2*j]), lo=nibble(value[2*j+1]);
                if (hi<0 || lo<0) return std::nullopt;
                item.sha256[j]=static_cast<std::uint8_t>((hi<<4)|lo);
            }
            item.name=name; item.size=size; item.present=true; total+=size;
        } else {
            for (std::size_t i=0;i<kCaptureV2Roles;++i) {
                const auto key=kCaptureV2RoleNames[i];
                if (line.size()>key.size() && line.starts_with(key) && line[key.size()]=='=') {
                    if (++referenceCounts[i]!=1) return std::nullopt;
                    references[i]=line.substr(key.size()+1);
                    break;
                }
            }
        }
        if(end==text.npos) break;
        text.remove_prefix(end+1);
    }
    if(formats!=1 || generations!=1 || volumes!=1 || widths!=1 || heights!=1 ||
       std::uint64_t(doc.width)*doc.height > kCaptureV2MaxPixels ||
       !doc.entries[0].present || !doc.entries[1].present) return std::nullopt;
    for(std::size_t i=0;i<kCaptureV2Roles;++i) {
        const auto& e=doc.entries[i];
        if (e.present) {
            if(referenceCounts[i]!=1 || references[i]!=e.name || !e.name.starts_with(doc.generation + ".")) return std::nullopt;
            for(std::size_t j=0;j<i;++j) if(doc.entries[j].present && doc.entries[j].name==e.name) return std::nullopt;
        } else if (referenceCounts[i] && !references[i].empty() && references[i]!="none") return std::nullopt;
    }
    if(doc.entries[7].present && !doc.entries[6].present) return std::nullopt;
    return doc;
}

// Both streams are bounded and already owned. Duplicate or missing receipt
// fields are rejected before comparing the manifest to the hashed world bytes.
[[nodiscard]] inline std::optional<std::string_view> captureV2Field(
        std::string_view text, std::string_view key) {
    std::optional<std::string_view> result;
    while (!text.empty()) {
        const auto end = text.find('\n');
        const auto line = text.substr(0, end);
        if (line.size() > key.size() && line.starts_with(key) && line[key.size()] == '=') {
            if (result) return std::nullopt;
            result = line.substr(key.size() + 1);
        }
        if (end == text.npos) break;
        text.remove_prefix(end + 1);
    }
    return result;
}
[[nodiscard]] inline bool captureV2ReceiptMatches(std::string_view manifest, std::string_view world) {
    constexpr std::array keys{
        "compact_generation", "world_generation", "implicit_generation", "canonical_feedback_generation",
        "presentation_frame_sequence", "presentation_device_generation", "presentation_surface_generation",
        "capture_submission_serial", "selected_compact_generation", "selected_compact_production_receipt_sequence",
        "selected_world_generation", "selected_world_production_receipt_sequence", "selected_canonical_feedback_generation",
        "selected_canonical_feedback_reset_epoch", "selected_canonical_feedback_production_receipt_sequence",
        "selected_implicit_generation", "selected_implicit_production_receipt_sequence", "presentation_outcome",
        "presentation_outcome_known", "presentation_confirmed"};
    for (auto key : keys) {
        const auto a = captureV2Field(manifest, key), b = captureV2Field(world, key);
        if (!a || !b || *a != *b) return false;
    }
    for (auto key : {"capture_submission_serial", "presentation_frame_sequence", "presentation_device_generation", "presentation_surface_generation"}) {
        const auto value = captureV2Field(world, key);
        if (!value || *value == "0" || value->empty()) return false;
    }
    if (captureV2Field(world, "presentation_outcome_known") != std::optional<std::string_view>{"1"}) return false;
    const auto canonical = captureV2Field(manifest, "canonical_feedback_capture_version");
    if (canonical) {
        if (*canonical != "1") return false;
        for (const auto& [manifestKey, worldKey] : std::array<std::pair<std::string_view, std::string_view>, 4>{{
                {"canonical_feedback_field_generation", "selected_canonical_feedback_generation"},
                {"canonical_feedback_field_reset_epoch", "selected_canonical_feedback_reset_epoch"},
                {"canonical_feedback_submission_serial", "capture_submission_serial"},
                {"canonical_feedback_readback_submission_serial", "capture_submission_serial"}}}) {
            const auto a = captureV2Field(manifest, manifestKey), b = captureV2Field(world, worldKey);
            if (!a || !b || *a != *b) return false;
        }
    }
    return true;
}

[[nodiscard]] inline std::optional<CaptureV2Bytes> acquireCaptureV2(
        const CaptureV2Document& doc, const std::filesystem::path& parent) {
    CaptureV2Bytes result; result.document=doc;
    try {
        for (std::size_t i=0;i<kCaptureV2Roles;++i) {
            const auto& item=doc.entries[i]; if(!item.present) continue;
            auto bytes=epi::io::readBoundedText(parent/item.name,item.size,"capture attachment");
            if(bytes.size()!=item.size || captureV2Hash(bytes)!=item.sha256) return std::nullopt;
            result.bytes[i]=std::move(bytes);
        }
    } catch (const std::exception&) { return std::nullopt; }
    return result;
}
} // namespace epi_sculpt
