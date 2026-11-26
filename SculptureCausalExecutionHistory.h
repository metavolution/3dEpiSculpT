#pragma once

#include "SculptureCausalExecutionGraph.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace epi_sculpt {

// Value-only incremental update for graph consumers. The delta contains no
// pointers back into the runtime and therefore cannot become a second source of
// scheduling or resource ownership truth.
struct SculptureCausalExecutionDelta final {
    static constexpr std::size_t kMaxNodeChanges =
        SculptureCausalExecutionSnapshot::kMaxNodes * 2U;
    static constexpr std::size_t kMaxEdgeChanges =
        SculptureCausalExecutionSnapshot::kMaxEdges * 2U;

    std::uint64_t fromRevision = 0U;
    std::uint64_t toRevision = 0U;
    std::uint64_t frameSequence = 0U;
    std::array<SculptureCausalNode, kMaxNodeChanges> upsertedNodes{};
    std::array<SculptureCausalNodeKey, kMaxNodeChanges> removedNodes{};
    std::array<SculptureCausalEdge, kMaxEdgeChanges> addedEdges{};
    std::array<SculptureCausalEdge, kMaxEdgeChanges> removedEdges{};
    std::size_t upsertedNodeCount = 0U;
    std::size_t removedNodeCount = 0U;
    std::size_t addedEdgeCount = 0U;
    std::size_t removedEdgeCount = 0U;

    [[nodiscard]] bool valid() const noexcept {
        return fromRevision != 0U && toRevision != 0U
            && fromRevision != toRevision;
    }
};

enum class SculptureCausalExecutionUpdateKind : std::uint8_t {
    None,
    Delta,
    FullSnapshot,
};

// Poll result for protocol adapters. A consumer that is current receives None;
// a consumer whose predecessor is retained receives exactly one deterministic
// revision delta; a consumer that fell behind retention receives the latest
// full snapshot and can resume polling from that revision.
struct SculptureCausalExecutionUpdate final {
    SculptureCausalExecutionUpdateKind kind = SculptureCausalExecutionUpdateKind::None;
    SculptureCausalExecutionDelta delta{};
    SculptureCausalExecutionSnapshot snapshot{};

    [[nodiscard]] std::uint64_t revision() const noexcept {
        switch (kind) {
        case SculptureCausalExecutionUpdateKind::Delta:
            return delta.toRevision;
        case SculptureCausalExecutionUpdateKind::FullSnapshot:
            return snapshot.revision();
        case SculptureCausalExecutionUpdateKind::None:
            return 0U;
        }
        return 0U;
    }
};

class SculptureCausalExecutionDiffer final {
public:
    [[nodiscard]] static SculptureCausalExecutionDelta diff(
        const SculptureCausalExecutionSnapshot& previous,
        const SculptureCausalExecutionSnapshot& current) noexcept {
        SculptureCausalExecutionDelta delta{};
        delta.fromRevision = previous.revision();
        delta.toRevision = current.revision();
        delta.frameSequence = current.frameSequence();

        for (const SculptureCausalNode& node : current.nodes()) {
            const SculptureCausalNode* old = previous.find(node.key);
            if (old == nullptr || !(*old == node)) {
                append(delta.upsertedNodes, delta.upsertedNodeCount, node);
            }
        }
        for (const SculptureCausalNode& node : previous.nodes()) {
            if (current.find(node.key) == nullptr) {
                append(delta.removedNodes, delta.removedNodeCount, node.key);
            }
        }

        for (const SculptureCausalEdge& edge : current.edges()) {
            if (!contains(previous.edges(), edge)) {
                append(delta.addedEdges, delta.addedEdgeCount, edge);
            }
        }
        for (const SculptureCausalEdge& edge : previous.edges()) {
            if (!contains(current.edges(), edge)) {
                append(delta.removedEdges, delta.removedEdgeCount, edge);
            }
        }

        return delta;
    }

private:
    template <typename TValue, std::size_t Capacity>
    static void append(
        std::array<TValue, Capacity>& values,
        std::size_t& count,
        const TValue& value) noexcept {
        if (count < values.size()) values[count++] = value;
    }

    [[nodiscard]] static bool contains(
        std::span<const SculptureCausalEdge> edges,
        const SculptureCausalEdge& candidate) noexcept {
        for (const SculptureCausalEdge& edge : edges) {
            if (edge == candidate) return true;
        }
        return false;
    }
};

// Bounded settled-frame history for debugger/dashboard/export consumers. The
// ring stores copied snapshots only. Overwrite never changes runtime behavior;
// it merely determines whether a lagging consumer can receive a delta or must
// resynchronize with a full current snapshot.
template <std::size_t Capacity = 256U>
class SculptureCausalExecutionHistory final {
    static_assert(Capacity > 1U);

public:
    void publish(const SculptureCausalExecutionSnapshot& snapshot) noexcept {
        if (snapshot.revision() == 0U) return;
        snapshots_[indexOf(snapshot.revision())] = snapshot;
        latestRevision_ = snapshot.revision();
        if (count_ < Capacity) ++count_;
    }

    [[nodiscard]] const SculptureCausalExecutionSnapshot* latest() const noexcept {
        return find(latestRevision_);
    }

    [[nodiscard]] const SculptureCausalExecutionSnapshot* find(
        std::uint64_t revision) const noexcept {
        if (revision == 0U) return nullptr;
        const SculptureCausalExecutionSnapshot& candidate = snapshots_[indexOf(revision)];
        return candidate.revision() == revision ? &candidate : nullptr;
    }

    [[nodiscard]] std::size_t size() const noexcept { return count_; }
    [[nodiscard]] std::uint64_t latestRevision() const noexcept { return latestRevision_; }

    [[nodiscard]] SculptureCausalExecutionUpdate readAfter(
        std::uint64_t consumerRevision) const noexcept {
        SculptureCausalExecutionUpdate update{};
        const SculptureCausalExecutionSnapshot* current = latest();
        if (current == nullptr || consumerRevision == current->revision()) return update;

        // A fresh or stale consumer gets an authoritative resynchronization
        // point. This also handles revision wrap conservatively.
        if (consumerRevision == 0U) {
            update.kind = SculptureCausalExecutionUpdateKind::FullSnapshot;
            update.snapshot = *current;
            return update;
        }

        const SculptureCausalExecutionSnapshot* previous = find(consumerRevision);
        const std::uint64_t nextRevision = consumerRevision + 1U;
        const SculptureCausalExecutionSnapshot* next = nextRevision == 0U
            ? nullptr : find(nextRevision);
        if (previous == nullptr || next == nullptr) {
            update.kind = SculptureCausalExecutionUpdateKind::FullSnapshot;
            update.snapshot = *current;
            return update;
        }

        update.kind = SculptureCausalExecutionUpdateKind::Delta;
        update.delta = SculptureCausalExecutionDiffer::diff(*previous, *next);
        return update;
    }

private:
    [[nodiscard]] static constexpr std::size_t indexOf(
        std::uint64_t revision) noexcept {
        return static_cast<std::size_t>(revision % Capacity);
    }

    std::array<SculptureCausalExecutionSnapshot, Capacity> snapshots_{};
    std::size_t count_ = 0U;
    std::uint64_t latestRevision_ = 0U;
};

} // namespace epi_sculpt
