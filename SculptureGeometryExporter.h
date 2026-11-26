#pragma once

#include "SculptureProgram.h"
#include "SculptureIsosurface.h"
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: Multi-Format 3D Geometry & Spatial Manifold Exporter
// -----------------------------------------------------------------------------
// Bridges the procedural mathematical engine with standard DCC (Digital Content
// Creation) pipelines and additive manufacturing (3D printing).
// Supports:
//   1. Wavefront OBJ (.obj + per-vertex normals + UV material coordinates)
//   2. Binary Stereolithography STL (.stl, watertight 3D-printable solid)
//   3. Stanford Polygon PLY (.ply, colored particle cloud and instance manifolds)
//   4. 3D Streamline Tube Polygonizer (generates cylindrical solid quad/tri meshes
//      with rotation-minimizing parallel transport frames along 4D Hopf and
//      chaotic attractor trajectory curves).
// -----------------------------------------------------------------------------

struct StreamlineTubeConfig {
    float radius = 0.035f;
    std::uint32_t radialSegments = 8;
    bool generateCaps = true;
    bool smoothNormals = true;
};

class SculptureGeometryExporter {
public:
    // Exports an IsosurfaceMesh to a standard Wavefront OBJ file
    [[nodiscard]] static bool exportWavefrontObj(
        const IsosurfaceMesh& mesh,
        const std::string& filepath,
        const std::string& objectName = "EpimorphicSculpture"
    );

    // Exports an IsosurfaceMesh to a binary STL file (IEEE 754 float, watertight solid)
    [[nodiscard]] static bool exportBinaryStl(
        const IsosurfaceMesh& mesh,
        const std::string& filepath
    );

    // Exports 16,384 instance sample particles to a colored binary/ASCII Stanford PLY file
    [[nodiscard]] static bool exportPointCloudPly(
        const SculptureSnapshot& snapshot,
        const std::string& filepath
    );

    // Generates a 3D cylindrical tube polygon mesh from a collection of curve streamlines
    [[nodiscard]] static IsosurfaceMesh generateStreamlineTubesMesh(
        const std::vector<std::vector<SculptureVec3>>& streamlines,
        const StreamlineTubeConfig& config = {}
    );

    // Integrates and extracts streamlines from a strange attractor / vector field in the program
    [[nodiscard]] static std::vector<std::vector<SculptureVec3>> traceAttractorStreamlines(
        const SculptureProgram& program,
        float time,
        float morph,
        std::size_t numStreamlines = 16,
        std::size_t stepsPerStreamline = 120,
        float stepSize = 0.02f
    );
};

} // namespace epi_sculpt
