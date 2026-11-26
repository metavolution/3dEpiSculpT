#include "RecursiveHopfTorusMath.h"
#include "RecursiveHopfTorusPersistence.h"

#include <charconv>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace {

using namespace epi_sculpt::recursive_hopf;

void usage() {
    std::cout << "Usage: mgfx_epi_recursive_hopf_inspect [options]\n"
                 "Emit a compact exact-circle or woven grammar as JSON; no native device is needed.\n"
                 "  --load-recipe P  Load authored .epihopf recipe file before applying CLI overrides\n"
                 "  --save-recipe P  Save resolved recipe to an authored .epihopf file\n"
                 "  --fibers N       Fibers per chirality (1..8; default 3)\n"
                 "  --depth N        Logical depth, root=0 (0..8; default 2)\n"
                 "  --placement M    Placement mode: uniform | conformal | clearance\n"
                 "  --time SECONDS   Nonnegative mathematical observation time\n"
                 "  --scale VALUE    Child contraction, strictly between 0 and 1\n"
                 "  --tube-radius R  Positive dimensionless fiber radius\n"
                 "  --plus-phase A   Initial positive-family phase in radians\n"
                 "  --minus-phase A  Initial negative-family phase in radians\n"
                 "  --plus-rate W    Positive-family radians per second\n"
                 "  --minus-rate W   Negative-family radians per second\n"
                 "  --twist A        Child twist in radians\n"
                 "  --offset VALUE   Child offset along the outward torus normal\n"
                 "  --conformal-exp E Stereographic conformal metric exponent (0..4)\n"
                 "  --weave VALUE    Normal over/under weave amplitude (0..0.5)\n"
                 "  --help           Show this help\n";
}

template <class T>
T number(std::string_view text, std::string_view option) {
    T value{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
        throw std::invalid_argument("Invalid value for " + std::string(option));
    return value;
}

int inspect(int argc, char** argv) {
    Recipe recipe;
    double time = 0.0;
    std::filesystem::path savePath{};
    for (int i = 1; i < argc; ++i) {
        const std::string_view option(argv[i]);
        if (option == "--help" || option == "-h") { usage(); return 0; }
        if (++i >= argc) throw std::invalid_argument("Missing value for " + std::string(option));
        const std::string_view value(argv[i]);
        if (option == "--load-recipe") recipe = loadRecipeFile(std::filesystem::path(value));
        else if (option == "--save-recipe") savePath = std::filesystem::path(value);
        else if (option == "--fibers") recipe.fibersPerFamily = number<std::uint32_t>(value, option);
        else if (option == "--depth") recipe.logicalDepth = number<std::uint32_t>(value, option);
        else if (option == "--placement") recipe.placementMode = parsePlacementMode(value);
        else if (option == "--time") time = number<double>(value, option);
        else if (option == "--scale") recipe.childScale = number<double>(value, option);
        else if (option == "--tube-radius") recipe.fiberRadius = number<double>(value, option);
        else if (option == "--plus-phase") recipe.plus.initialRadians = number<double>(value, option);
        else if (option == "--minus-phase") recipe.minus.initialRadians = number<double>(value, option);
        else if (option == "--plus-rate") recipe.plus.radiansPerSecond = number<double>(value, option);
        else if (option == "--minus-rate") recipe.minus.radiansPerSecond = number<double>(value, option);
        else if (option == "--twist") recipe.childTwistRadians = number<double>(value, option);
        else if (option == "--offset") recipe.childNormalOffset = number<double>(value, option);
        else if (option == "--conformal-exp") recipe.conformalScaleExponent = number<double>(value, option);
        else if (option == "--weave") recipe.weaveAmplitude = number<double>(value, option);
        else throw std::invalid_argument("Unknown option: " + std::string(option));
    }
    const Grammar grammar = Grammar::prepare(recipe, time);
    if (!savePath.empty()) {
        saveRecipeFile(savePath, recipe, "CLI-Inspected Recursive Hopf Torus");
    }
    std::cout << serializeGrammarJson(grammar);
    return std::cout ? 0 : 1;
}

} // namespace

int main(int argc, char** argv) {
    try {
        return inspect(argc, argv);
    } catch (const std::invalid_argument& error) {
        std::cerr << "Recursive Hopf recipe rejected: " << error.what() << '\n';
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "Recursive Hopf inspection failed: " << error.what() << '\n';
        return 1;
    }
}
