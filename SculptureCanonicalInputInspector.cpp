#include "SculptureCanonicalCaptureRestore.h"

#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace {
using namespace epi_sculpt;

// Derived evidence only. This tool never selects an application source or owns
// a producer. All content admission comes from the same preparation as LoadState.
struct Report {
    bool admitted = false;
    bool manifestAdmitted = false;
    bool prepared = false;
    const char* assessment = "manifest_acquisition_failed";
    const char* declaration = "unavailable";
    std::optional<bool> imageRequired;
    const char* imageDecode = nullptr;
};

Report checkInput(const std::filesystem::path& manifest) {
    Report report;
    const auto refs = readCaptureManifestReferences(manifest);
    if (!refs) return report;
    report.declaration = canonicalCaptureDeclarationCode(assessCanonicalCaptureDeclaration(*refs));
    const auto gate = captureManifestRestoreAdmission(*refs);
    // Optional absence in the canonical helper is a no-op, not permission to
    // bypass LoadState's independent versioned-manifest gate.
    if (gate.format != CaptureManifestFormatAdmission::V1 || !gate.admitted()) {
        report.assessment = "manifest_rejected";
        return report;
    }
    report.manifestAdmitted = true;
    const auto candidate = prepareCanonicalCaptureRestore(*refs, manifest);
    report.admitted = candidate.admitted;
    report.prepared = candidate.admitted && candidate.mode.has_value();
    report.assessment = candidate.diagnosticCode();
    if (candidate.imageDecodeStatus) {
        report.imageRequired = true;
        report.imageDecode = epi::backend::seed_image_runtime::seed_image_decode_code(*candidate.imageDecodeStatus);
    } else if (candidate.mode) {
        report.imageRequired = epi::pattern_mirror::seed_type_from_mode_int(candidate.mode->seed_type)
            == epi::SeedType::Image;
        if (*report.imageRequired) report.imageDecode = epi::backend::seed_image_runtime::seed_image_decode_code(
            epi::backend::seed_image_runtime::SeedImageDecodeStatus::Decoded);
    }
    return report;
}

std::string render(const Report& report, bool json) {
    std::ostringstream out;
    out.exceptions(std::ios::badbit | std::ios::failbit);
    if (json) {
        // Every string here comes from a closed code vocabulary, never a path
        // or authored metadata. Arbitrary input bytes cannot corrupt JSON/logs.
        out << "{\"schema\":\"epic-canonical-input-check-v1\",\"scope\":\"canonical-input\","
            << "\"admitted\":" << (report.admitted ? "true" : "false")
            << ",\"manifest_admitted\":" << (report.manifestAdmitted ? "true" : "false")
            << ",\"assessment\":\"" << report.assessment
            << "\",\"declaration_assessment\":\"" << report.declaration
            << "\",\"prepared\":" << (report.prepared ? "true" : "false")
            << ",\"image_seed\":{\"required\":";
        if (report.imageRequired) out << (*report.imageRequired ? "true" : "false");
        else out << "null";
        out << ",\"decode_assessment\":";
        if (report.imageDecode) out << '"' << report.imageDecode << '"';
        else out << "null";
        out << "},\"verification\":{\"world_state_checked\":false,\"native_upload_checked\":false,"
               "\"recursive_history_restored\":false,\"exact_replay_established\":false}}\n";
    } else {
        out << "Canonical input check: " << report.assessment << '\n'
            << "Manifest references: " << (report.manifestAdmitted ? "admitted" : "not admitted") << '\n'
            << "Canonical declaration: " << report.declaration << '\n'
            << "Image seed required: ";
        if (report.imageRequired) out << (*report.imageRequired ? "yes" : "no");
        else out << "unknown";
        out << "\nImage decode: " << (report.imageDecode ? report.imageDecode : "not checked") << '\n'
            << "Canonical input only. World state, native upload and exact replay are not checked.\n";
    }
    return out.str();
}

void usage(std::ostream& out) {
    out << "Usage: mgfx_epi_canonical_input_check [--json] [--] <capture.manifest>\n"
           "       mgfx_epi_canonical_input_check --help\n"
           "Read-only canonical mode/image admission. No world restore or GPU execution.\n"
           "Exit codes: 0 admitted (or not recorded), 1 check failed, 2 usage error.\n";
}
} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        usage(std::cout);
        return 0;
    }
    bool json = false, options = true;
    std::optional<std::filesystem::path> manifest;
    for (int index = 1; index < argc; ++index) {
        const std::string_view arg(argv[index]);
        if (options && arg == "--") { options = false; continue; }
        if (options && arg == "--json" && !json) { json = true; continue; }
        if ((options && arg.starts_with('-')) || manifest || arg.empty()) {
            usage(std::cerr);
            return 2;
        }
        manifest.emplace(arg);
    }
    if (!manifest) { usage(std::cerr); return 2; }
    try {
        const auto report = checkInput(*manifest);
        // Publish one complete report, including content rejections in JSON mode.
        std::cout << render(report, json);
        if (!std::cout) return 1;
        return report.admitted ? 0 : 1;
    } catch (const std::exception&) {
        std::cerr << "Cannot complete canonical input check.\n";
        return 1;
    }
}
