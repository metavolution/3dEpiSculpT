#include "SculptureMorphSpace.h"

#include <cstdio>
#include <limits>

namespace epi_sculpt {

SculptureMorphSpace::SculptureMorphSpace() noexcept {
    nodes_.resize(4);

    // Default corners (SW, SE, NW, NE)
    nodes_[0] = MorphSpaceNode{.id = "SW", .title = "SouthWest Node", .x = 0.0f, .y = 0.0f, .params = SculptureProgramParams{}, .weight = 1.0f, .active = true};
    nodes_[1] = MorphSpaceNode{.id = "SE", .title = "SouthEast Node", .x = 1.0f, .y = 0.0f, .params = SculptureProgramParams{}, .weight = 1.0f, .active = true};
    nodes_[2] = MorphSpaceNode{.id = "NW", .title = "NorthWest Node", .x = 0.0f, .y = 1.0f, .params = SculptureProgramParams{}, .weight = 1.0f, .active = true};
    nodes_[3] = MorphSpaceNode{.id = "NE", .title = "NorthEast Node", .x = 1.0f, .y = 1.0f, .params = SculptureProgramParams{}, .weight = 1.0f, .active = true};

    // Pre-populate 4 Curated Thematic Quadrant Banks
    quadrantBanks_.push_back(MorphSpaceQuadrantBank{
        .id = "cosmological",
        .title = "Cosmological Quad",
        .cornerPresetNames = {
            "authored/cosmological/83_golden_phyllotaxis_cathedral",
            "authored/cosmological/84_rossler_lorenz_garden",
            "authored/cosmological/85_clifford_hopf_lanterns",
            "authored/cosmological/86_hyperbolic_horosphere_canopy"
        },
        .cornerParams = {}
    });

    quadrantBanks_.push_back(MorphSpaceQuadrantBank{
        .id = "dimensional_4d",
        .title = "Hyperdimensional 4D Quad",
        .cornerPresetNames = {
            "authored/dimensional/181_hecatonicosachoron_120cell_4d",
            "authored/dimensional/182_hexacosichoron_600cell_4d",
            "authored/dimensional/183_hyperbolic_honeycomb_4d",
            "authored/dimensional/184_lie_e8_coxeter_root_lattice"
        },
        .cornerParams = {}
    });

    quadrantBanks_.push_back(MorphSpaceQuadrantBank{
        .id = "kinetic_solitons",
        .title = "Kinetic Solitons Quad",
        .cornerPresetNames = {
            "authored/kinetic/185_toroidal_foliation_seifert_soliton",
            "authored/kinetic/186_clifford_torus_soliton_lattice",
            "authored/kinetic/187_hopf_fibration_quantum_vortex",
            "authored/kinetic/188_dirac_monopole_flux_braid"
        },
        .cornerParams = {}
    });

    quadrantBanks_.push_back(MorphSpaceQuadrantBank{
        .id = "mathematical_attractors",
        .title = "Mathematical Attractors Quad",
        .cornerPresetNames = {
            "authored/mathematical/192_weierstrass_fractal_foam_35d",
            "authored/mathematical/193_talbot_moire_carpet_flux_4d",
            "authored/mathematical/194_feigenbaum_attractor_cascade_35d",
            "authored/dimensional/191_su2_yang_mills_instanton"
        },
        .cornerParams = {}
    });

    loadBank(0U);

    // Default 5-Point Waypoint Tour Path
    waypoints_ = {
        MorphSpaceWaypoint{.x = 0.15f, .y = 0.15f, .dwellSeconds = 1.5f, .travelSeconds = 2.0f, .tag = "SW_Anchor"},
        MorphSpaceWaypoint{.x = 0.85f, .y = 0.15f, .dwellSeconds = 1.5f, .travelSeconds = 2.0f, .tag = "SE_Anchor"},
        MorphSpaceWaypoint{.x = 0.85f, .y = 0.85f, .dwellSeconds = 1.5f, .travelSeconds = 2.0f, .tag = "NE_Anchor"},
        MorphSpaceWaypoint{.x = 0.15f, .y = 0.85f, .dwellSeconds = 1.5f, .travelSeconds = 2.0f, .tag = "NW_Anchor"},
        MorphSpaceWaypoint{.x = 0.50f, .y = 0.50f, .dwellSeconds = 2.0f, .travelSeconds = 2.5f, .tag = "Quad_Center"}
    };
}

void SculptureMorphSpace::setCorner(MorphSpaceCorner corner, const SculptureProgramParams& params,
                                     std::string_view id, std::string_view title) {
    const auto idx = static_cast<std::size_t>(corner);
    if (nodes_.size() < 4) {
        nodes_.resize(4);
    }
    nodes_[idx].params = params;
    if (!id.empty()) nodes_[idx].id = std::string(id);
    if (!title.empty()) nodes_[idx].title = std::string(title);

    switch (corner) {
    case MorphSpaceCorner::SouthWest: nodes_[idx].x = 0.0f; nodes_[idx].y = 0.0f; break;
    case MorphSpaceCorner::SouthEast: nodes_[idx].x = 1.0f; nodes_[idx].y = 0.0f; break;
    case MorphSpaceCorner::NorthWest: nodes_[idx].x = 0.0f; nodes_[idx].y = 1.0f; break;
    case MorphSpaceCorner::NorthEast: nodes_[idx].x = 1.0f; nodes_[idx].y = 1.0f; break;
    }
    nodes_[idx].active = true;
}

void SculptureMorphSpace::setNode(std::size_t index, float x, float y, const SculptureProgramParams& params,
                                   std::string_view id, std::string_view title) {
    if (index >= nodes_.size()) {
        nodes_.resize(index + 1U);
    }
    nodes_[index].x = std::clamp(x, 0.0f, 1.0f);
    nodes_[index].y = std::clamp(y, 0.0f, 1.0f);
    nodes_[index].params = params;
    if (!id.empty()) nodes_[index].id = std::string(id);
    if (!title.empty()) nodes_[index].title = std::string(title);
    nodes_[index].active = true;
}

void SculptureMorphSpace::addNode(float x, float y, const SculptureProgramParams& params,
                                   std::string_view id, std::string_view title) {
    nodes_.push_back(MorphSpaceNode{
        .id = std::string(id),
        .title = std::string(title),
        .x = std::clamp(x, 0.0f, 1.0f),
        .y = std::clamp(y, 0.0f, 1.0f),
        .params = params,
        .weight = 1.0f,
        .active = true
    });
}

void SculptureMorphSpace::clearNodes() noexcept {
    nodes_.clear();
}

const MorphSpaceNode* SculptureMorphSpace::node(std::size_t index) const noexcept {
    if (index < nodes_.size()) {
        return &nodes_[index];
    }
    return nullptr;
}

MorphSpaceNode* SculptureMorphSpace::node(std::size_t index) noexcept {
    if (index < nodes_.size()) {
        return &nodes_[index];
    }
    return nullptr;
}

void SculptureMorphSpace::setCursor(float x, float y) noexcept {
    cursorX_ = std::clamp(x, 0.0f, 1.0f);
    cursorY_ = std::clamp(y, 0.0f, 1.0f);
}

void SculptureMorphSpace::nudgeCursor(float dx, float dy) noexcept {
    cursorX_ = std::clamp(cursorX_ + dx, 0.0f, 1.0f);
    cursorY_ = std::clamp(cursorY_ + dy, 0.0f, 1.0f);
}

void SculptureMorphSpace::addQuadrantBank(MorphSpaceQuadrantBank bank) {
    quadrantBanks_.push_back(std::move(bank));
}

void SculptureMorphSpace::loadBank(std::size_t index) {
    if (index >= quadrantBanks_.size()) return;
    activeBankIndex_ = index;
    const auto& bank = quadrantBanks_[index];
    if (nodes_.size() < 4) nodes_.resize(4);
    for (std::size_t c = 0; c < 4; ++c) {
        nodes_[c].params = bank.cornerParams[c];
        nodes_[c].id = bank.cornerPresetNames[c];
        nodes_[c].title = bank.cornerPresetNames[c];
        nodes_[c].active = true;
        switch (static_cast<MorphSpaceCorner>(c)) {
        case MorphSpaceCorner::SouthWest: nodes_[c].x = 0.0f; nodes_[c].y = 0.0f; break;
        case MorphSpaceCorner::SouthEast: nodes_[c].x = 1.0f; nodes_[c].y = 0.0f; break;
        case MorphSpaceCorner::NorthWest: nodes_[c].x = 0.0f; nodes_[c].y = 1.0f; break;
        case MorphSpaceCorner::NorthEast: nodes_[c].x = 1.0f; nodes_[c].y = 1.0f; break;
        }
    }
}

void SculptureMorphSpace::loadBank(std::string_view nameOrId) {
    for (std::size_t i = 0; i < quadrantBanks_.size(); ++i) {
        if (quadrantBanks_[i].id == nameOrId || quadrantBanks_[i].title == nameOrId) {
            loadBank(i);
            return;
        }
    }
}

void SculptureMorphSpace::cycleBank() {
    if (!quadrantBanks_.empty()) {
        loadBank((activeBankIndex_ + 1U) % quadrantBanks_.size());
    }
}

std::string_view SculptureMorphSpace::activeBankName() const noexcept {
    if (activeBankIndex_ < quadrantBanks_.size()) {
        return quadrantBanks_[activeBankIndex_].id;
    }
    return "default";
}

std::string_view SculptureMorphSpace::activeBankTitle() const noexcept {
    if (activeBankIndex_ < quadrantBanks_.size()) {
        return quadrantBanks_[activeBankIndex_].title;
    }
    return "Default Quad";
}

std::vector<std::string_view> SculptureMorphSpace::availableBankNames() const {
    std::vector<std::string_view> names;
    names.reserve(quadrantBanks_.size());
    for (const auto& b : quadrantBanks_) {
        names.emplace_back(b.id);
    }
    return names;
}

const MorphSpaceQuadrantBank* SculptureMorphSpace::activeBank() const noexcept {
    if (activeBankIndex_ < quadrantBanks_.size()) {
        return &quadrantBanks_[activeBankIndex_];
    }
    return nullptr;
}

void SculptureMorphSpace::setBankCornerParams(std::size_t bankIndex, MorphSpaceCorner corner, const SculptureProgramParams& params) {
    if (bankIndex < quadrantBanks_.size()) {
        const auto c = static_cast<std::size_t>(corner);
        quadrantBanks_[bankIndex].cornerParams[c] = params;
        if (bankIndex == activeBankIndex_ && c < nodes_.size()) {
            nodes_[c].params = params;
        }
    }
}

std::optional<std::size_t> SculptureMorphSpace::findBankIndex(std::string_view nameOrId) const noexcept {
    for (std::size_t i = 0; i < quadrantBanks_.size(); ++i) {
        if (quadrantBanks_[i].id == nameOrId || quadrantBanks_[i].title == nameOrId) {
            return i;
        }
    }
    return std::nullopt;
}

bool SculptureMorphSpace::hasBank(std::string_view nameOrId) const noexcept {
    return findBankIndex(nameOrId).has_value();
}

MorphSpaceBankIdResolution SculptureMorphSpace::resolveExactBankId(std::string_view id) const noexcept {
    if (id.empty()) return {};
    std::optional<std::size_t> found;
    for (std::size_t i = 0; i < quadrantBanks_.size(); ++i) {
        if (quadrantBanks_[i].id != id) continue;
        if (found) return {MorphSpaceBankIdStatus::Ambiguous, std::nullopt};
        found = i;
    }
    return found ? MorphSpaceBankIdResolution{MorphSpaceBankIdStatus::Found, found}
                 : MorphSpaceBankIdResolution{};
}

std::optional<std::size_t> SculptureMorphSpace::tryUpsertCategoryBank(
    std::string_view categoryId,
    std::string_view categoryTitle,
    const std::array<std::string, 4>& cornerPresetNames,
    const std::array<SculptureProgramParams, 4>& cornerParams,
    bool autoLoad
) {
    if (categoryId.empty()) return std::nullopt;
    const auto identity = resolveExactBankId(categoryId);
    if (identity.status == MorphSpaceBankIdStatus::Ambiguous) return std::nullopt;

    // Own all incoming text/arrays before replacing an existing bank. This also
    // supports callers borrowing names or corner values from that same bank.
    MorphSpaceQuadrantBank candidate{
        .id = std::string(categoryId),
        .title = std::string(categoryTitle),
        .cornerPresetNames = cornerPresetNames,
        .cornerParams = cornerParams
    };
    std::size_t index;
    if (identity.index) {
        index = *identity.index;
        quadrantBanks_[index] = std::move(candidate);
    } else {
        quadrantBanks_.push_back(std::move(candidate));
        index = quadrantBanks_.size() - 1U;
    }
    if (autoLoad) {
        loadBank(index);
    }
    return index;
}

std::size_t SculptureMorphSpace::upsertCategoryBank(
    std::string_view categoryId,
    std::string_view categoryTitle,
    const std::array<std::string, 4>& cornerPresetNames,
    const std::array<SculptureProgramParams, 4>& cornerParams,
    bool autoLoad
) {
    return tryUpsertCategoryBank(categoryId, categoryTitle, cornerPresetNames, cornerParams, autoLoad)
        .value_or(kInvalidBankIndex);
}


void SculptureMorphSpace::addWaypoint(float x, float y, float dwellSec, float travelSec, std::string_view tag) {
    waypoints_.push_back(MorphSpaceWaypoint{
        .x = std::clamp(x, 0.0f, 1.0f),
        .y = std::clamp(y, 0.0f, 1.0f),
        .dwellSeconds = std::max(0.05f, dwellSec),
        .travelSeconds = std::max(0.05f, travelSec),
        .tag = std::string(tag)
    });
}

void SculptureMorphSpace::clearPath() noexcept {
    waypoints_.clear();
    activeWaypointIndex_ = 0;
    waypointTimer_ = 0.0f;
    isTravelling_ = false;
}

float SculptureMorphSpace::pathProgress() const noexcept {
    if (waypoints_.empty()) return 0.0f;
    const float segWeight = 1.0f / static_cast<float>(waypoints_.size());
    const auto& curWp = waypoints_[activeWaypointIndex_];
    const float dur = isTravelling_ ? curWp.travelSeconds : curWp.dwellSeconds;
    const float localT = std::clamp(waypointTimer_ / std::max(0.01f, dur), 0.0f, 1.0f);
    return (static_cast<float>(activeWaypointIndex_) + localT * 0.5f) * segWeight;
}

void SculptureMorphSpace::togglePathPlayback() noexcept {
    if (motionMode_ == MorphSpaceMotion::Path) {
        motionMode_ = MorphSpaceMotion::Static;
    } else {
        motionMode_ = MorphSpaceMotion::Path;
        waypointTimer_ = 0.0f;
        isTravelling_ = false;
    }
}

void SculptureMorphSpace::cycleMotionMode() noexcept {
    const auto next = (static_cast<std::uint8_t>(motionMode_) + 1U) % 6U;
    motionMode_ = static_cast<MorphSpaceMotion>(next);
}

void SculptureMorphSpace::update(float dtSeconds) noexcept {
    if (!active_ || dtSeconds <= 0.0f) return;

    motionPhase_ += motionSpeed_ * dtSeconds;

    switch (motionMode_) {
    case MorphSpaceMotion::Static:
        break;

    case MorphSpaceMotion::Orbital: {
        constexpr float kRadius = 0.38f;
        cursorX_ = std::clamp(0.5f + kRadius * std::cos(motionPhase_), 0.0f, 1.0f);
        cursorY_ = std::clamp(0.5f + kRadius * std::sin(motionPhase_), 0.0f, 1.0f);
        break;
    }

    case MorphSpaceMotion::Lissajous: {
        // 3:2 frequency ratio
        constexpr float kRadius = 0.42f;
        cursorX_ = std::clamp(0.5f + kRadius * std::sin(3.0f * motionPhase_), 0.0f, 1.0f);
        cursorY_ = std::clamp(0.5f + kRadius * std::sin(2.0f * motionPhase_ + 0.5f * std::numbers::pi_v<float>), 0.0f, 1.0f);
        break;
    }

    case MorphSpaceMotion::Brownian: {
        brownianNoise_ = std::fmod(brownianNoise_ + dtSeconds * 3.14159f, 1000.0f);
        const float dx = (std::sin(brownianNoise_ * 5.17f) + std::sin(brownianNoise_ * 11.31f)) * 0.15f * motionSpeed_ * dtSeconds;
        const float dy = (std::cos(brownianNoise_ * 7.43f) + std::cos(brownianNoise_ * 13.67f)) * 0.15f * motionSpeed_ * dtSeconds;
        cursorX_ = std::clamp(cursorX_ + dx, 0.0f, 1.0f);
        cursorY_ = std::clamp(cursorY_ + dy, 0.0f, 1.0f);
        break;
    }

    case MorphSpaceMotion::Bounce: {
        cursorX_ += velX_ * motionSpeed_ * dtSeconds;
        cursorY_ += velY_ * motionSpeed_ * dtSeconds;
        if (cursorX_ < 0.0f) { cursorX_ = 0.0f; velX_ = -velX_; }
        else if (cursorX_ > 1.0f) { cursorX_ = 1.0f; velX_ = -velX_; }
        if (cursorY_ < 0.0f) { cursorY_ = 0.0f; velY_ = -velY_; }
        else if (cursorY_ > 1.0f) { cursorY_ = 1.0f; velY_ = -velY_; }
        break;
    }

    case MorphSpaceMotion::Path: {
        if (waypoints_.empty()) break;
        if (waypoints_.size() == 1) {
            cursorX_ = waypoints_[0].x;
            cursorY_ = waypoints_[0].y;
            break;
        }
        if (activeWaypointIndex_ >= waypoints_.size()) {
            activeWaypointIndex_ = 0;
        }
        const auto& curWp = waypoints_[activeWaypointIndex_];
        if (!isTravelling_) {
            cursorX_ = curWp.x;
            cursorY_ = curWp.y;
            waypointTimer_ += dtSeconds * motionSpeed_;
            if (waypointTimer_ >= std::max(0.01f, curWp.dwellSeconds)) {
                waypointTimer_ = 0.0f;
                isTravelling_ = true;
            }
        } else {
            waypointTimer_ += dtSeconds * motionSpeed_;
            const float travelDur = std::max(0.01f, curWp.travelSeconds);
            const float raw = std::clamp(waypointTimer_ / travelDur, 0.0f, 1.0f);
            // Quintic smootherstep easing
            const float t = raw * raw * raw * (raw * (raw * 6.0f - 15.0f) + 10.0f);
            const auto fromIdx = activeWaypointIndex_;
            const auto toIdx = (activeWaypointIndex_ + 1U) % waypoints_.size();
            cursorX_ = std::clamp(std::lerp(waypoints_[fromIdx].x, waypoints_[toIdx].x, t), 0.0f, 1.0f);
            cursorY_ = std::clamp(std::lerp(waypoints_[fromIdx].y, waypoints_[toIdx].y, t), 0.0f, 1.0f);
            if (raw >= 1.0f) {
                waypointTimer_ = 0.0f;
                isTravelling_ = false;
                activeWaypointIndex_ = toIdx;
                if (!loopPath_ && activeWaypointIndex_ == waypoints_.size() - 1U) {
                    motionMode_ = MorphSpaceMotion::Static;
                }
            }
        }
        break;
    }
    }
}

std::vector<float> SculptureMorphSpace::computeWeights() const noexcept {
    if (nodes_.empty()) {
        return {};
    }
    if (nodes_.size() == 1) {
        return {1.0f};
    }

    // Fast path: standard 4-corner quad bilinear interpolation
    if (nodes_.size() == 4 &&
        nodes_[0].x == 0.0f && nodes_[0].y == 0.0f &&
        nodes_[1].x == 1.0f && nodes_[1].y == 0.0f &&
        nodes_[2].x == 0.0f && nodes_[2].y == 1.0f &&
        nodes_[3].x == 1.0f && nodes_[3].y == 1.0f) {
        const float x = cursorX_;
        const float y = cursorY_;
        return {
            (1.0f - x) * (1.0f - y),
            x * (1.0f - y),
            (1.0f - x) * y,
            x * y
        };
    }

    // General Shepard Inverse Distance Weighting (IDW)
    std::vector<float> rawWeights(nodes_.size(), 0.0f);
    float sumWeights = 0.0f;

    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        if (!nodes_[i].active) continue;
        const float dx = cursorX_ - nodes_[i].x;
        const float dy = cursorY_ - nodes_[i].y;
        const float distSq = dx * dx + dy * dy;

        // Exact match with node position
        if (distSq < 1e-8f) {
            std::vector<float> exact(nodes_.size(), 0.0f);
            exact[i] = 1.0f;
            return exact;
        }

        const float dist = std::sqrt(distSq);
        const float w = (1.0f / std::pow(dist, shepardPower_)) * std::max(0.0f, nodes_[i].weight);
        rawWeights[i] = w;
        sumWeights += w;
    }

    if (sumWeights > 0.0f) {
        const float invSum = 1.0f / sumWeights;
        for (float& w : rawWeights) {
            w *= invSum;
        }
    } else {
        const float uniform = 1.0f / static_cast<float>(nodes_.size());
        std::fill(rawWeights.begin(), rawWeights.end(), uniform);
    }

    return rawWeights;
}

SculptureProgramParams SculptureMorphSpace::synthesizeParams() const noexcept {
    if (nodes_.empty()) {
        return {};
    }
    if (nodes_.size() == 1) {
        return nodes_[0].params;
    }

    const auto weights = computeWeights();

    // Determine dominant node for discrete categorical states
    std::size_t maxWeightIdx = 0;
    float maxWeight = -1.0f;
    for (std::size_t i = 0; i < weights.size(); ++i) {
        if (weights[i] > maxWeight) {
            maxWeight = weights[i];
            maxWeightIdx = i;
        }
    }
    SculptureProgramParams result = nodes_[maxWeightIdx].params;

    // Reset continuous dynamic fields for weighted synthesis
    result.pre = ComplexAffine{0.0f, 0.0f, 0.0f, 0.0f};
    result.post = ComplexAffine{0.0f, 0.0f, 0.0f, 0.0f};
    result.latentW = 0.0f;
    result.seedAlpha = 0.0f;
    result.seedPulseAmplitude = 0.0f;
    result.volumetricDensityScale = 0.0f;
    result.curlStrength = 0.0f;
    result.seedWidth = 0.0f;
    result.temporalFade = 0.0f;
    result.topologyBlend = 0.0f;
    result.patternSpeed = 0.0f;
    result.worldScale = 0.0f;
    result.radiusScale = 0.0f;
    result.advectionSpeed = 0.0f;
    result.filmThickness = 0.0f;
    result.filmIOR = 0.0f;

    // Linear synthesis of affine transforms and continuous dynamic scalars
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        const float w = weights[i];
        if (w <= 0.0f) continue;
        const auto& p = nodes_[i].params;

        result.pre.scaleReal += w * p.pre.scaleReal;
        result.pre.scaleImag += w * p.pre.scaleImag;
        result.pre.offsetReal += w * p.pre.offsetReal;
        result.pre.offsetImag += w * p.pre.offsetImag;

        result.post.scaleReal += w * p.post.scaleReal;
        result.post.scaleImag += w * p.post.scaleImag;
        result.post.offsetReal += w * p.post.offsetReal;
        result.post.offsetImag += w * p.post.offsetImag;

        result.latentW += w * p.latentW;
        result.seedAlpha += w * p.seedAlpha;
        result.seedPulseAmplitude += w * p.seedPulseAmplitude;
        result.volumetricDensityScale += w * p.volumetricDensityScale;
        result.curlStrength += w * p.curlStrength;
        result.seedWidth += w * p.seedWidth;
        result.temporalFade += w * p.temporalFade;
        result.topologyBlend += w * p.topologyBlend;
        result.patternSpeed += w * p.patternSpeed;
        result.worldScale += w * p.worldScale;
        result.radiusScale += w * p.radiusScale;
        result.advectionSpeed += w * p.advectionSpeed;
        result.filmThickness += w * p.filmThickness;
        result.filmIOR += w * p.filmIOR;
    }

    // Directional circular mean on S^1 for phase and rotation angles
    float sumCosA = 0.0f, sumSinA = 0.0f;
    float sumCosB = 0.0f, sumSinB = 0.0f;
    float sumCosT = 0.0f, sumSinT = 0.0f;
    float sumCosHue = 0.0f, sumSinHue = 0.0f;

    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        const float w = weights[i];
        if (w <= 0.0f) continue;
        const auto& p = nodes_[i].params;

        sumCosA += w * std::cos(p.rotorAlpha);
        sumSinA += w * std::sin(p.rotorAlpha);

        sumCosB += w * std::cos(p.rotorBeta);
        sumSinB += w * std::sin(p.rotorBeta);

        sumCosT += w * std::cos(p.rotorTheta4D);
        sumSinT += w * std::sin(p.rotorTheta4D);

        const float hueRad = p.hueRotation * (2.0f * std::numbers::pi_v<float>);
        sumCosHue += w * std::cos(hueRad);
        sumSinHue += w * std::sin(hueRad);
    }

    auto normalizeAngle = [](float y, float x) -> float {
        float a = std::atan2(y, x);
        if (a < 0.0f) a += 2.0f * std::numbers::pi_v<float>;
        return a;
    };

    result.rotorAlpha = normalizeAngle(sumSinA, sumCosA);
    result.rotorBeta = normalizeAngle(sumSinB, sumCosB);
    result.rotorTheta4D = normalizeAngle(sumSinT, sumCosT);

    const float hueAng = normalizeAngle(sumSinHue, sumCosHue);
    result.hueRotation = std::fmod(hueAng / (2.0f * std::numbers::pi_v<float>), 1.0f);

    return result;
}

std::string SculptureMorphSpace::formatHudStatus() const {
    if (!active_) return "";
    char buf[256];
    if (motionMode_ == MorphSpaceMotion::Path && !waypoints_.empty()) {
        std::snprintf(buf, sizeof(buf), "[MORPH 2D: %s Path:%zu/%zu (%.2f, %.2f) %s]",
                      activeBankTitle().data(),
                      activeWaypointIndex_ + 1U, waypoints_.size(),
                      cursorX_, cursorY_, isTravelling_ ? "Travel" : "Dwell");
    } else {
        std::snprintf(buf, sizeof(buf), "[MORPH 2D: %s (%.2f, %.2f) %s]",
                      activeBankTitle().data(),
                      cursorX_, cursorY_, motionName(motionMode_).data());
    }
    return std::string(buf);
}

SculptureMorphSpaceState SculptureMorphSpace::exportState() const noexcept {
    SculptureMorphSpaceState st{};
    st.active = active_;
    st.cursorX = cursorX_;
    st.cursorY = cursorY_;
    st.motionMode = motionMode_;
    st.motionSpeed = motionSpeed_;
    st.shepardPower = shepardPower_;
    st.activeBankName = std::string(activeBankName());
    st.pathLooping = loopPath_;
    st.waypoints.reserve(waypoints_.size());
    for (const auto& wp : waypoints_) {
        st.waypoints.push_back(SculptureMorphSpaceState::WaypointState{
            .x = wp.x,
            .y = wp.y,
            .dwellSeconds = wp.dwellSeconds,
            .travelSeconds = wp.travelSeconds,
            .tag = wp.tag
        });
    }
    return st;
}

void SculptureMorphSpace::importState(const SculptureMorphSpaceState& state) {
    if (!state.activeBankName.empty()) {
        loadBank(state.activeBankName);
    }
    importControllerState(state);
}

bool SculptureMorphSpace::importCapturedState(const SculptureMorphSpaceState& state) {
    std::optional<std::size_t> bankIndex;
    if (!state.activeBankName.empty()) {
        const auto identity = resolveExactBankId(state.activeBankName);
        if (identity.status != MorphSpaceBankIdStatus::Found) return false;
        bankIndex = identity.index;
    } else if (state.active) {
        return false;
    }

    if (bankIndex) {
        // The captured recipe describes four corners, not old ad-hoc nodes.
        clearNodes();
        loadBank(*bankIndex);
    }
    importControllerState(state);
    return true;
}

void SculptureMorphSpace::importControllerState(const SculptureMorphSpaceState& state) {
    active_ = state.active;
    cursorX_ = std::clamp(state.cursorX, 0.0f, 1.0f);
    cursorY_ = std::clamp(state.cursorY, 0.0f, 1.0f);
    motionMode_ = state.motionMode;
    motionSpeed_ = std::clamp(state.motionSpeed, 0.01f, 10.0f);
    shepardPower_ = std::clamp(state.shepardPower, 0.5f, 8.0f);
    loopPath_ = state.pathLooping;

    waypoints_.clear();
    waypoints_.reserve(state.waypoints.size());
    for (const auto& wp : state.waypoints) {
        addWaypoint(wp.x, wp.y, wp.dwellSeconds, wp.travelSeconds, wp.tag);
    }
    activeWaypointIndex_ = 0;
    waypointTimer_ = 0.0f;
    isTravelling_ = false;
}

} // namespace epi_sculpt
