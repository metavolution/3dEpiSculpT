#include "RecursiveHopfTorusPersistence.h"

#include <charconv>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <vector>

#if defined(__linux__)
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace epi_sculpt::recursive_hopf {

namespace {

std::string_view trim(std::string_view sv) noexcept {
    while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' || sv.front() == '\r' || sv.front() == '\n')) {
        sv.remove_prefix(1U);
    }
    while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' || sv.back() == '\r' || sv.back() == '\n')) {
        sv.remove_suffix(1U);
    }
    return sv;
}

template <typename T>
T parseScalar(std::string_view text, std::string_view key) {
    text = trim(text);
    T value{};
    const auto res = std::from_chars(text.data(), text.data() + text.size(), value);
    if (res.ec != std::errc{} || res.ptr != text.data() + text.size()) {
        throw std::invalid_argument("Invalid scalar value for key '" + std::string(key) + "': " + std::string(text));
    }
    return value;
}

void writePointJson(std::ostringstream& os, const Point& p) {
    os << '[' << p[0] << ',' << p[1] << ',' << p[2] << ']';
}

void writeBasisJson(std::ostringstream& os, const Basis& axes) {
    os << '[';
    writePointJson(os, axes.x);
    os << ',';
    writePointJson(os, axes.y);
    os << ',';
    writePointJson(os, axes.z);
    os << ']';
}

void writeAllTextChecked(const std::filesystem::path& path, std::string_view contents) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to open file for writing: " + path.string());
    }
    out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    out.flush();
    out.close();
    if (!out) {
        throw std::runtime_error("Failed to write and close complete file: " + path.string());
    }
}

// This attempt owns only the directory returned by exclusive creation. In
// particular, neither a pre-existing destination nor an old .tmp is ours.
class CaptureStagingDirectory final {
public:
    explicit CaptureStagingDirectory(const std::filesystem::path& destination) {
        auto pattern = destination.parent_path() /
            ("." + destination.filename().string() + ".tmp.XXXXXX");
        const auto text = pattern.string();
        name_.assign(text.begin(), text.end());
        name_.push_back('\0');
#if defined(__linux__)
        if (::mkdtemp(name_.data()) == nullptr) {
            throw std::system_error(errno, std::generic_category(), "create Hopf capture staging");
        }
        owned_ = true;
#else
        throw std::system_error(std::make_error_code(std::errc::operation_not_supported),
                                "exclusive Hopf capture staging requires Linux");
#endif
    }

    CaptureStagingDirectory(const CaptureStagingDirectory&) = delete;
    CaptureStagingDirectory& operator=(const CaptureStagingDirectory&) = delete;

    ~CaptureStagingDirectory() noexcept {
        if (!owned_) return;
        try {
            std::error_code ignored;
            std::filesystem::remove_all(path(), ignored);
        } catch (...) {
            // Cleanup must not replace the original write/commit exception.
        }
    }

    [[nodiscard]] std::filesystem::path path() const { return name_.data(); }

    void publish(const std::filesystem::path& destination) {
#if defined(__linux__) && defined(SYS_renameat2)
        // A separate exists() check plus ordinary rename has a race and can
        // replace an empty destination directory. The kernel must arbitrate.
        if (::syscall(SYS_renameat2, AT_FDCWD, name_.data(), AT_FDCWD,
                      destination.c_str(), RENAME_NOREPLACE) != 0) {
            const std::error_code error(errno, std::generic_category());
            throw std::filesystem::filesystem_error(
                "publish Hopf capture without replacement", path(), destination,
                error);
        }
        owned_ = false;
#else
        (void)destination;
        throw std::system_error(std::make_error_code(std::errc::operation_not_supported),
                                "atomic no-replacement Hopf capture requires renameat2");
#endif
    }

private:
    std::vector<char> name_;
    bool owned_ = false;
};

} // namespace

PlacementMode parsePlacementMode(std::string_view text) {
    text = trim(text);
    if (text == "uniform" || text == "uniform-similarity" || text == "0") {
        return PlacementMode::UniformSimilarity;
    }
    if (text == "conformal" || text == "conformal-metric" || text == "1") {
        return PlacementMode::ConformalMetric;
    }
    if (text == "clearance" || text == "strict-local-clearance" || text == "2") {
        return PlacementMode::StrictLocalClearance;
    }
    throw std::invalid_argument("Unknown placement mode: " + std::string(text));
}

std::string serializeRecipeText(const Recipe& recipe, std::string_view title) {
    const auto issue = validateRecipe(recipe);
    if (issue != RecipeIssue::None) {
        throw std::invalid_argument("Cannot serialize invalid Hopf recipe: " + std::string(describe(issue)));
    }

    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << std::setprecision(std::numeric_limits<double>::max_digits10);
    os << "# epiC Recursive Counter-Winding Hopf Torus Recipe (.epihopf)\n";
    if (!title.empty()) {
        os << "# title: " << title << '\n';
    }
    os << "version = " << recipe.version << '\n'
       << "fibers_per_family = " << recipe.fibersPerFamily << '\n'
       << "logical_depth = " << recipe.logicalDepth << '\n'
       << "placement_mode = " << describePlacementMode(recipe.placementMode) << '\n'
       << "plus_initial_radians = " << recipe.plus.initialRadians << '\n'
       << "plus_radians_per_second = " << recipe.plus.radiansPerSecond << '\n'
       << "minus_initial_radians = " << recipe.minus.initialRadians << '\n'
       << "minus_radians_per_second = " << recipe.minus.radiansPerSecond << '\n'
       << "child_scale = " << recipe.childScale << '\n'
       << "child_twist_radians = " << recipe.childTwistRadians << '\n'
       << "child_normal_offset = " << recipe.childNormalOffset << '\n'
       << "fiber_radius = " << recipe.fiberRadius << '\n'
       << "conformal_scale_exponent = " << recipe.conformalScaleExponent << '\n'
       << "weave_amplitude = " << recipe.weaveAmplitude << '\n';
    return os.str();
}

Recipe parseRecipeText(std::string_view text) {
    Recipe recipe{};
    std::size_t lineStart = 0U;
    while (lineStart <= text.size()) {
        const std::size_t lineEnd = text.find('\n', lineStart);
        std::string_view line = (lineEnd == std::string_view::npos)
            ? text.substr(lineStart)
            : text.substr(lineStart, lineEnd - lineStart);
        lineStart = (lineEnd == std::string_view::npos) ? (text.size() + 1U) : (lineEnd + 1U);

        const std::size_t commentPos = line.find('#');
        if (commentPos != std::string_view::npos) {
            line = line.substr(0U, commentPos);
        }
        line = trim(line);
        if (line.empty()) continue;

        const std::size_t eqPos = line.find('=');
        if (eqPos == std::string_view::npos) {
            throw std::invalid_argument("Malformed .epihopf line (missing '='): " + std::string(line));
        }
        const std::string_view key = trim(line.substr(0U, eqPos));
        const std::string_view val = trim(line.substr(eqPos + 1U));

        if (key == "version") recipe.version = parseScalar<std::uint32_t>(val, key);
        else if (key == "fibers_per_family") recipe.fibersPerFamily = parseScalar<std::uint32_t>(val, key);
        else if (key == "logical_depth") recipe.logicalDepth = parseScalar<std::uint32_t>(val, key);
        else if (key == "placement_mode") recipe.placementMode = parsePlacementMode(val);
        else if (key == "plus_initial_radians") recipe.plus.initialRadians = parseScalar<double>(val, key);
        else if (key == "plus_radians_per_second") recipe.plus.radiansPerSecond = parseScalar<double>(val, key);
        else if (key == "minus_initial_radians") recipe.minus.initialRadians = parseScalar<double>(val, key);
        else if (key == "minus_radians_per_second") recipe.minus.radiansPerSecond = parseScalar<double>(val, key);
        else if (key == "child_scale") recipe.childScale = parseScalar<double>(val, key);
        else if (key == "child_twist_radians") recipe.childTwistRadians = parseScalar<double>(val, key);
        else if (key == "child_normal_offset") recipe.childNormalOffset = parseScalar<double>(val, key);
        else if (key == "fiber_radius") recipe.fiberRadius = parseScalar<double>(val, key);
        else if (key == "conformal_scale_exponent") recipe.conformalScaleExponent = parseScalar<double>(val, key);
        else if (key == "weave_amplitude") recipe.weaveAmplitude = parseScalar<double>(val, key);
        else {
            throw std::invalid_argument("Unknown .epihopf recipe key: " + std::string(key));
        }
    }

    const auto issue = validateRecipe(recipe);
    if (issue != RecipeIssue::None) {
        throw std::invalid_argument("Parsed Hopf recipe is invalid: " + std::string(describe(issue)));
    }
    return recipe;
}

Recipe loadRecipeFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open Hopf recipe file: " + path.string());
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return parseRecipeText(ss.str());
}

void saveRecipeFile(
    const std::filesystem::path& path,
    const Recipe& recipe,
    std::string_view title) {
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    const std::string text = serializeRecipeText(recipe, title);
    writeAllTextChecked(path, text);
}

std::string serializeGrammarJson(const Grammar& grammar) {
    const auto& recipe = grammar.recipe();
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << std::setprecision(std::numeric_limits<double>::max_digits10)
       << "{\n  \"kind\":\"recursive-counter-winding-hopf-torus\",\n"
          "  \"geometry\":\"" << (recipe.weaveAmplitude > 0.0 ? "woven-villarceau-fibers" : "exact-circles") << "\",\n"
          "  \"placement\":\"" << describePlacementMode(recipe.placementMode) << "\",\n"
          "  \"coordinateSystem\":\"mathematical-z-up\",\n"
          "  \"basisLayout\":\"columns\",\n"
       << "  \"recipe\":{\"version\":" << recipe.version
       << ",\"fibersPerFamily\":" << recipe.fibersPerFamily
       << ",\"logicalDepth\":" << recipe.logicalDepth
       << ",\"placementMode\":\"" << describePlacementMode(recipe.placementMode) << "\""
       << ",\"plusInitialRadians\":" << recipe.plus.initialRadians
       << ",\"plusRadiansPerSecond\":" << recipe.plus.radiansPerSecond
       << ",\"minusInitialRadians\":" << recipe.minus.initialRadians
       << ",\"minusRadiansPerSecond\":" << recipe.minus.radiansPerSecond
       << ",\"childScale\":" << recipe.childScale
       << ",\"childTwistRadians\":" << recipe.childTwistRadians
       << ",\"childNormalOffset\":" << recipe.childNormalOffset
       << ",\"fiberRadius\":" << recipe.fiberRadius
       << ",\"conformalScaleExponent\":" << recipe.conformalScaleExponent
       << ",\"weaveAmplitude\":" << recipe.weaveAmplitude << "},\n"
       << "  \"domainTime\":" << grammar.domainTime() << ",\n"
       << "  \"phases\":[" << grammar.phases().plus << ',' << grammar.phases().minus << "],\n"
       << "  \"childrenPerNode\":" << grammar.counts().childrenPerNode << ",\n"
       << "  \"logicalNodes\":\"" << grammar.counts().nodes << "\",\n"
       << "  \"logicalFibers\":\"" << grammar.counts().fibers << "\",\n"
       << "  \"subtreeRadius\":" << grammar.subtreeRadius() << ",\n"
          "  \"circles\":[\n";
    bool separator = false;
    for (const auto family : {Chirality::Plus, Chirality::Minus}) {
        for (std::uint32_t i = 0U; i < recipe.fibersPerFamily; ++i) {
            if (separator) os << ",\n";
            separator = true;
            const auto circle = grammar.circle(family, i);
            os << "    {\"chirality\":\"" << (family == Chirality::Plus ? "plus" : "minus")
               << "\",\"fiber\":" << i << ",\"center\":";
            writePointJson(os, circle.center);
            os << ",\"axes\":";
            writeBasisJson(os, circle.axes);
            os << ",\"radius\":" << circle.radius << '}';
        }
    }
    os << "\n  ],\n  \"children\":[\n";
    separator = false;
    for (std::size_t idx = 0U; idx < grammar.children().size(); ++idx) {
        const auto& child = grammar.children()[idx];
        if (separator) os << ",\n";
        separator = true;
        os << "    {\"plusFiber\":" << child.site.plusFiber
           << ",\"minusFiber\":" << child.site.minusFiber
           << ",\"branch\":" << child.site.branch
           << ",\"uv\":[" << child.site.u << ',' << child.site.v << ']'
           << ",\"clearanceRadius\":" << grammar.siteClearanceRadius(static_cast<std::uint32_t>(idx))
           << ",\"translation\":";
        writePointJson(os, child.transform.translation);
        os << ",\"axes\":";
        writeBasisJson(os, child.transform.axes);
        os << ",\"scale\":" << child.transform.scale << '}';
    }
    os << "\n  ]\n}\n";
    return os.str();
}

std::string serializeObservationReport(
    const Grammar& grammar,
    const TraversalBudget& budget,
    const TraversalResult& traversal,
    const CaptureObservationMetadata& metadata) {
    const auto& r = grammar.recipe();
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << std::setprecision(9);
    os << "schema_version = 1\n"
       << "kind = recursive-counter-winding-hopf-torus\n"
       << "generation = " << metadata.generation << '\n'
       << "producer_device = " << metadata.producerDevice << '\n'
       << "produced_by_opencl = " << (metadata.producedByOpenCl ? "true" : "false") << '\n'
       << "evaluation_ms = " << metadata.evaluationMilliseconds << '\n'
       << "presentation_confirmed = " << (metadata.presentationConfirmed ? "true" : "false") << '\n'
       << "submission_serial = " << metadata.submissionSerial << '\n'
       << "stereo_side_by_side = " << (metadata.stereoSideBySide ? "true" : "false") << '\n'
       << "preset_name = " << metadata.presetName << '\n'
       << "palette_name = " << metadata.paletteName << '\n'
       << "domain_time = " << grammar.domainTime() << '\n'
       << "plus_phase = " << grammar.phases().plus << '\n'
       << "minus_phase = " << grammar.phases().minus << '\n'
       << "fibers_per_family = " << r.fibersPerFamily << '\n'
       << "semantic_logical_depth = " << r.logicalDepth << '\n'
       << "placement_mode = " << describePlacementMode(r.placementMode) << '\n'
       << "effective_max_depth = " << traversal.effectiveMaxDepth << '\n'
       << "deepest_observed_depth = " << traversal.deepestObservedDepth << '\n'
       << "visited_nodes = " << traversal.visitedNodes << '\n'
       << "emitted_fibers = " << traversal.emittedFibers << '\n'
       << "frustum_culled_nodes = " << traversal.frustumCulledNodes << '\n'
       << "lod_culled_nodes = " << traversal.lodCulledNodes << '\n'
       << "budget_omitted_nodes = " << traversal.budgetOmittedNodes << '\n'
       << "truncated_by_budget = " << (traversal.truncatedByBudget ? "true" : "false") << '\n'
       << "view_culling_enabled = " << (budget.enableViewCulling ? "true" : "false") << '\n'
       << "active_eye_count = " << budget.eyeCount << '\n'
       << "min_projected_radius_px = " << budget.minProjectedRadiusPixels << '\n';
    os << "traversal_source = cpu-reference\n"
       << "instance_stream = " << metadata.instanceStream << '\n';
    if (metadata.submittedInstanceCount)
        os << "submitted_instance_count = " << *metadata.submittedInstanceCount << '\n';
    if (metadata.submittedNodeCount)
        os << "submitted_node_count = " << *metadata.submittedNodeCount << '\n';
    os << "geometry_submission_deferred = " << (metadata.geometrySubmissionDeferred ? "true" : "false") << '\n';
    if (metadata.openClMatchesPresentationDevice)
        os << "opencl_matches_presentation_device = "
           << (*metadata.openClMatchesPresentationDevice ? "true" : "false") << '\n';
    if (metadata.externalGrammarAllocation)
        os << "external_grammar_allocation = " << *metadata.externalGrammarAllocation << '\n';
    if (metadata.externalGrammarHandoff)
        os << "external_grammar_handoff = " << *metadata.externalGrammarHandoff << '\n';
    return os.str();
}

void writeCaptureBundle(
    const std::filesystem::path& bundleDirectory,
    const Grammar& grammar,
    const TraversalBudget& budget,
    const TraversalResult& traversal,
    const CaptureObservationMetadata& metadata) {
    const auto filename = bundleDirectory.filename();
    if (filename.empty() || filename == "." || filename == "..") {
        throw std::invalid_argument("Hopf capture requires a named destination directory");
    }
    const auto parent = bundleDirectory.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    CaptureStagingDirectory staging(bundleDirectory);

    writeAllTextChecked(
        staging.path() / "recipe.epihopf",
        serializeRecipeText(grammar.recipe(), metadata.presetName));
    writeAllTextChecked(
        staging.path() / "grammar.json",
        serializeGrammarJson(grammar));
    writeAllTextChecked(
        staging.path() / "observation.txt",
        serializeObservationReport(grammar, budget, traversal, metadata));

    staging.publish(bundleDirectory);
}

} // namespace epi_sculpt::recursive_hopf
