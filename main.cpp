// 3dEpiSculpTMorph — minimal epiC-inspired procedural sculpture.
//
// The core application flow is owned by SculptureApplication. Runtime resources
// are resolved as explicit values; the entrypoint never changes process cwd.

#include "SculptureApplication.h"
#include <cstdio>
#include <exception>

int main(int argc, char** argv) try {
    epi_sculpt::SculptureApplication app(argc, argv);
    return app.run() ? 0 : 1;
} catch (const std::exception& error) {
    std::fprintf(stderr, "[3dEpiSculpTMorph] fatal: %s\n", error.what());
    return 1;
}
