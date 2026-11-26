#pragma once

#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace epi_sculpt::modulation_routing {

// -----------------------------------------------------------------------------
// Educational Annotation: Authored vs. Resolved Modulation Routing Policy
// -----------------------------------------------------------------------------
// An empty authored route map represents "use the host's default modulation
// baseline" (visual LFOs alone when audio is disabled, or visual + 3-band audio
// reactivity when audio is enabled). Keeping default materialization and comparison
// in this pure header lets cold load admission, per-frame reconciliation, and
// offline CLI inspectors share one exact definition without allocating on frames.
// -----------------------------------------------------------------------------

// Application policy: an empty authored map selects visual defaults, with
// additional audio routes only when the host has explicitly enabled audio.
[[nodiscard]] inline std::span<const std::string_view> defaults(bool audioEnabled) noexcept {
    static constexpr std::array<std::string_view, 5> routes{
        "latentLFO -> latentW : 1.0",
        "hueLFO -> hueRotation : 1.0",
        "audioBass -> preScale : 0.5",
        "audioMid -> hueRotation : 1.5",
        "audioFlux -> seedPulseAmplitude : 0.8"
    };
    return std::span<const std::string_view>{routes}.first(audioEnabled ? 5U : 2U);
}

[[nodiscard]] constexpr std::string_view originName(bool emptyAuthored, bool audioEnabled) noexcept {
    return emptyAuthored ? (audioEnabled ? "audio-defaults" : "visual-defaults") : "authored";
}

// Materialize the effective route list for cold preparation or installation.
[[nodiscard]] inline std::vector<std::string> resolve(
    std::span<const std::string> authored, bool audioEnabled) {
    if (!authored.empty()) {
        return std::vector<std::string>(authored.begin(), authored.end());
    }
    const auto expected = defaults(audioEnabled);
    std::vector<std::string> resolved;
    resolved.reserve(expected.size());
    for (const auto route : expected) resolved.emplace_back(route);
    return resolved;
}

// Compare resolved intent without constructing strings on the frame path.
// Authored text and resolved defaults are different representations of the same
// request; comparing an empty authored vector directly would never converge.
[[nodiscard]] inline bool matches(const std::vector<std::string>& authored,
                                  bool audioEnabled,
                                  const std::vector<std::string>& active) noexcept {
    if (!authored.empty()) return authored == active;
    const auto expected = defaults(audioEnabled);
    return active.size() == expected.size()
        && std::equal(active.begin(), active.end(), expected.begin(),
                      [](const std::string& route, std::string_view expectedRoute) {
                          return std::string_view{route} == expectedRoute;
                      });
}

} // namespace epi_sculpt::modulation_routing
