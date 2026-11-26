#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace epi_sculpt {

// Component identities alone cannot distinguish an edited .cl override from
// its old program. Own the exact direct build inputs; discovered includes use
// the existing disk-cache fingerprint. Equality compares direct source text,
// not merely its hash. This key owns no OpenCL handles.
struct SculptureProgramVariantKey {
    std::uint32_t innerA = 0, innerB = 0, seedA = 0, seedB = 0;
    std::uint32_t reduction = 0, colorEffect = 0;
    std::string prefix;
    std::string source{};
    std::string buildOptions{};
    // Best-effort include discovery shares the disk-cache contract; this is
    // not an atomic filesystem snapshot or complete preprocessor dependency set.
    std::uint64_t sourceFingerprint = 0;

    bool operator==(const SculptureProgramVariantKey&) const noexcept = default;

    [[nodiscard]] std::uint64_t hash() const noexcept {
        std::uint64_t value = 0xcbf29ce484222325ULL;
        const auto mix = [&](std::uint64_t part) {
            value ^= part;
            value *= 0x100000001b3ULL;
        };
        mix(innerA); mix(innerB); mix(seedA); mix(seedB);
        mix(reduction); mix(colorEffect);
        const auto mixText = [&](const std::string& text) {
            mix(text.size());
            for (unsigned char byte : text) mix(byte);
        };
        mixText(prefix);
        mixText(source);
        mixText(buildOptions);
        mix(sourceFingerprint);
        return value;
    }
};

struct SculptureProgramVariantKeyHash {
    std::size_t operator()(const SculptureProgramVariantKey& key) const noexcept {
        return static_cast<std::size_t>(key.hash());
    }
};

} // namespace epi_sculpt
