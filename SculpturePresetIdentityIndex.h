#pragma once

#include "SculpturePresetAsset.h"

#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epi_sculpt {

// A request-local accelerator for repeated exact-ID queries, not a catalog
// cache/owner. Assets AND their ID strings must outlive the index and stay
// unchanged. Never retain this across a catalog reload, mutation or destruction.
class SculpturePresetIdentityIndex {
public:
    explicit SculpturePresetIdentityIndex(std::span<const SculpturePresetAsset> assets) {
        entries_.reserve(assets.size());
        for (const auto& asset : assets) {
            if (asset.id.empty()) continue;
            const auto [entry, inserted] = entries_.try_emplace(asset.id, &asset);
            // A null value records ambiguity, even for three or more matches.
            // Keep the key so duplicates never regain a discovery-order winner.
            if (!inserted) entry->second = nullptr;
        }
    }
    SculpturePresetIdentityIndex(std::vector<SculpturePresetAsset>&&) = delete;
    SculpturePresetIdentityIndex(const std::vector<SculpturePresetAsset>&&) = delete;

    [[nodiscard]] SculpturePresetIdResolution resolveExactId(std::string_view id) const noexcept {
        const auto entry = entries_.find(id);
        if (entry == entries_.end()) return {};
        return entry->second ? SculpturePresetIdResolution{SculpturePresetIdStatus::Found, entry->second}
                             : SculpturePresetIdResolution{SculpturePresetIdStatus::Ambiguous, nullptr};
    }

    [[nodiscard]] bool isUnique(std::string_view id) const noexcept {
        return resolveExactId(id).status == SculpturePresetIdStatus::Found;
    }

private:
    std::unordered_map<std::string_view, const SculpturePresetAsset*> entries_;
};

} // namespace epi_sculpt
