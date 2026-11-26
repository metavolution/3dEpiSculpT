#pragma once

namespace epi_sculpt {

enum class CanonicalReleaseAdmission { Ready, ReleaseRejected, Stale };
enum class CanonicalPublicationResult { Published, MissingDelivery, Rejected };

// The release has been submitted by Vulkan before this bookkeeping runs.
// A stale decision quarantines its signal and disables external delivery.
// The caller retains ownership of cadence, tickets and production receipts.
template <typename RecordRelease, typename IsCurrent, typename Quarantine,
          typename Defer, typename DisableExternal>
[[nodiscard]] CanonicalReleaseAdmission admitCanonicalReleasedWork(
    RecordRelease&& recordRelease, IsCurrent&& isCurrent,
    Quarantine&& quarantine, Defer&& defer, DisableExternal&& disableExternal) {
    if (!recordRelease()) return CanonicalReleaseAdmission::ReleaseRejected;
    if (!isCurrent()) {
        quarantine();
        defer();
        disableExternal();
        return CanonicalReleaseAdmission::Stale;
    }
    return CanonicalReleaseAdmission::Ready;
}

// Accepted mathematics is committed before any fallible publication work.
// The application callbacks retain all cadence, receipt and transport ownership.
template <typename RejectProduction, typename Commit, typename Publish,
          typename RejectPublication>
void settleCanonicalProduction(bool mathematicsAccepted,
    RejectProduction&& rejectProduction, Commit&& commit, Publish&& publish,
    RejectPublication&& rejectPublication) {
    if (!mathematicsAccepted) {
        rejectProduction();
        return;
    }
    commit();
    CanonicalPublicationResult result;
    try {
        result = publish();
    } catch (...) {
        // Acceptance is irreversible. Close publication and native ownership
        // before propagating the failure; never retry the mathematical step.
        // Rejection callbacks must not throw during this cleanup.
        rejectPublication(CanonicalPublicationResult::Rejected);
        throw;
    }
    if (result != CanonicalPublicationResult::Published) rejectPublication(result);
}

} // namespace epi_sculpt
