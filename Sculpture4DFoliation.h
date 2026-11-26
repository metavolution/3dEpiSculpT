#pragma once

#include "SculptureProgramSchema.h"
#include "SculptureRotorKinematics.h"
#include "SculptureIsosurface.h"
#include <array>
#include <vector>
#include <string_view>
#include <cmath>
#include <cstdint>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: 4D HYPER-DIMENSIONAL FOLIATION & MANIFOLD TOPOLOGY
// -----------------------------------------------------------------------------
// Extends the 3D epimorphic sculpture lift into 4-dimensional Euclidean space
// (R^4 ~= C^2 ~= H) and the 3-sphere S^3.
//
// 1. SO(4) Isoclinic Decompositions:
//    SO(4) ~= (SU(2) x SU(2)) / Z_2 decomposes into commuting left and right
//    isoclinic rotations acting on unit quaternions q in H:
//      q' = q_L * q * q_R^*
//
// 2. 4D Regular Polytopes & Hyper-Surfaces:
//    - Tesseract (8-Cell hypercube)
//    - 16-Cell (Orthoplex / Hyper-octahedron)
//    - 24-Cell (Self-dual regular 4-polytope)
//    - 120-Cell & 600-Cell hyper-icosahedral honeycombs
//    - Clifford Flat Torus: S^1(r1) x S^1(r2) subset S^3
//    - Calabi-Yau K3 surface algebraic varieties
//
// 3. Foliation & Slicing:
//    Evaluates the 3D spatial foliation induced by 4D hyperplanes (n_4D . x = c)
//    and conformal stereographic projections pi: S^3 -> R^3.
// -----------------------------------------------------------------------------

struct SculptureVec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    [[nodiscard]] constexpr float lengthSq() const noexcept {
        return x * x + y * y + z * z + w * w;
    }

    [[nodiscard]] float length() const noexcept {
        return std::sqrt(lengthSq());
    }
};

struct Octonion8D {
    std::array<float, 8> e{}; // e0 (real) + e1..e7 (imaginary units)

    [[nodiscard]] static Octonion8D multiply(const Octonion8D& a, const Octonion8D& b) noexcept;
    [[nodiscard]] static Octonion8D associator(const Octonion8D& x, const Octonion8D& y, const Octonion8D& z) noexcept;
    [[nodiscard]] float normSq() const noexcept {
        float sum = 0.0f;
        for (float v : e) sum += v * v;
        return sum;
    }
    [[nodiscard]] float norm() const noexcept {
        return std::sqrt(normSq());
    }
};

enum class Manifold4DType : std::uint8_t {
    CliffordTorus = 0,    // S^1(r1) x S^1(r2) in S^3
    Tesseract8Cell = 1,   // 4D Hypercube (8-Cell)
    Hexadecachoron16 = 2, // 4D 16-Cell Cross-polytope
    Icositetrachoron24 = 3,// 4D 24-Cell
    CalabiYauK3 = 4,      // Algebraic quartic K3 surface: z1^4 + z2^4 = 1
    HopfFibrationS3 = 5,  // Fiber bundle pi: S^3 -> S^2
    Hexacosichoron600 = 6,// 600-Cell regular 4-polytope (120 vertices, Golden Ratio symmetries)
    OctonionicAssociatorS7 = 7, // 8D Octonionic S^7 -> S^4 Cayley-Dickson Associator Foliation
    Hecatonicosachoron120 = 8, // 120-Cell regular 4-polytope (600 vertices, Hyper-dodecahedron)
    SeifertHyperbolicSoliton = 9, // 4D Toroidal Seifert Fibration Soliton
    CalabiYauQuinticThreefold = 10, // Quintic 3-fold variety z1^5 + z2^5 = 1
    LieE8CoxeterProjection = 11, // E8 Exceptional Lie Root Lattice Projected into R^4
    DynamicQuaternionJulia = 12, // 4D Dynamic Quaternion Julia Fractal
    DynamicHyperMandelbulb = 13, // 4D Dynamic Hyper-Mandelbulb (8th Power)
    HyperbolicHoneycomb4D = 14, // 4D Hyperbolic {3,5,3} Icosahedral Honeycomb
    CliffordHopfSolitonLattice = 15, // Multi-instanton Clifford-Hopf Soliton Lattice
};

inline constexpr std::size_t kManifold4DTypeCount = 16U;


[[nodiscard]] constexpr std::string_view name(Manifold4DType type) noexcept {
    switch (type) {
    case Manifold4DType::CliffordTorus: return "Clifford Torus (S1 x S1)";
    case Manifold4DType::Tesseract8Cell: return "Tesseract (8-Cell)";
    case Manifold4DType::Hexadecachoron16: return "16-Cell (Cross-Polytope)";
    case Manifold4DType::Icositetrachoron24: return "24-Cell (Icositetrachoron)";
    case Manifold4DType::CalabiYauK3: return "Calabi-Yau (K3 Surface)";
    case Manifold4DType::HopfFibrationS3: return "Hopf Fibration (S3 -> S2)";
    case Manifold4DType::Hexacosichoron600: return "600-Cell (Hexacosichoron)";
    case Manifold4DType::OctonionicAssociatorS7: return "8D Octonion Associator (S7 -> S4)";
    case Manifold4DType::Hecatonicosachoron120: return "120-Cell (Hecatonicosachoron)";
    case Manifold4DType::SeifertHyperbolicSoliton: return "Seifert Hyperbolic Soliton (T2 x S1)";
    case Manifold4DType::CalabiYauQuinticThreefold: return "Calabi-Yau Quintic Threefold";
    case Manifold4DType::LieE8CoxeterProjection: return "E8 Exceptional Lie Root Lattice";
    case Manifold4DType::DynamicQuaternionJulia: return "4D Dynamic Quaternion Julia Fractal";
    case Manifold4DType::DynamicHyperMandelbulb: return "4D Dynamic Hyper-Mandelbulb";
    case Manifold4DType::HyperbolicHoneycomb4D: return "4D Hyperbolic Icosahedral Honeycomb ({3,5,3})";
    case Manifold4DType::CliffordHopfSolitonLattice: return "Clifford-Hopf Multi-Soliton Lattice (S3 -> S2)";
    }
    return "Clifford Torus";
}

struct Foliation4DConfig {
    Manifold4DType manifoldType = Manifold4DType::CliffordTorus;
    float rotationLeftSpeed = 0.35f;
    float rotationRightSpeed = 0.25f;
    float sliceOffsetW = 0.0f;
    float stereographicScale = 1.25f;
    bool useConformalProjection = true;
    float polytopeThickness = 0.06f;
};

class Sculpture4DFoliation {
public:
    // Evaluate 4D rotation matrix: SO(4) Left-Right isoclinic composition
    [[nodiscard]] static SculptureVec4 rotateSO4(
        const SculptureVec4& p,
        float thetaL,
        float thetaR,
        float phiL = 0.0f,
        float phiR = 0.0f
    ) noexcept;

    // Conformal stereographic projection from 4D S^3 sphere to 3D Euclidean space
    [[nodiscard]] static SculptureVec3 projectStereographic(
        const SculptureVec4& p4,
        float scale = 1.0f
    ) noexcept;

    // Hyperplane slice projection (Orthographic 4D slice at w = sliceW)
    [[nodiscard]] static SculptureVec3 projectSlice(
        const SculptureVec4& p4,
        float sliceW = 0.0f,
        float thickness = 0.1f
    ) noexcept;

    // Evaluates 4D Signed Distance Function for regular polytopes and manifolds
    [[nodiscard]] static float sample4DManifoldSdf(
        const SculptureVec4& p,
        Manifold4DType type,
        float time = 0.0f
    ) noexcept;

    // Evaluates 4D tangent geodesic velocity vector field for advection and kinematic flow
    [[nodiscard]] static SculptureVec4 evaluate4DGeodesicVelocity(
        const SculptureVec4& p,
        Manifold4DType type,
        float time = 0.0f
    ) noexcept;

    // Generates 4D lattice vertices and edges for hyper-polytope wireframe structures
    [[nodiscard]] static std::vector<SculptureVec4> getPolytopeVertices(
        Manifold4DType type
    );

    [[nodiscard]] static std::vector<std::pair<std::uint32_t, std::uint32_t>> getPolytopeEdges(
        Manifold4DType type
    );

    // Parallel transport geodesic ribbon mesh generation using Bishop rotation-minimizing frame
    [[nodiscard]] static IsosurfaceMesh generateGeodesicRibbonMesh(
        const std::vector<SculptureVec4>& curve4D,
        float width,
        float thetaL,
        float thetaR,
        float scale = 1.0f
    );

    // Hardware 3D strut mesh generation for regular 4D polytopes under SO(4) rotation
    [[nodiscard]] static IsosurfaceMesh generatePolytopeMesh(
        Manifold4DType type,
        float thickness,
        float thetaL,
        float thetaR,
        float scale = 1.0f
    );

    // Audio-reactive spectral harmonic modulation of 4D isoclinic rotors
    struct AudioReactive4DRotors {
        float bassEnergy = 0.0f;     // Sub-bass / bass band [20-150 Hz]
        float midEnergy = 0.0f;      // Midrange / vocal band [300-2500 Hz]
        float highEnergy = 0.0f;     // Treble / air band [5-16 kHz]
        float spectralFlux = 0.0f;   // Onset / transient spike
    };

    [[nodiscard]] static SculptureVec4 evaluateAudioReactiveSO4(
        const SculptureVec4& p,
        float baseTime,
        const AudioReactive4DRotors& audio,
        float modulationDepth = 0.5f
    ) noexcept;

    // Generates 4D (p, q) Hopf torus knot tube mesh lying on S^3 and stereographically projected to R^3
    [[nodiscard]] static IsosurfaceMesh generateHopfTorusKnotMesh(
        std::uint32_t p,
        std::uint32_t q,
        std::size_t samplePoints,
        float tubeRadius,
        float thetaL,
        float thetaR,
        float scale = 1.0f
    );

    // Generates linked Villarceau circle foliation mesh on Clifford torus in S^3
    [[nodiscard]] static IsosurfaceMesh generateVillarceauFoliationMesh(
        std::size_t circleCount,
        std::size_t pointsPerCircle,
        float tubeRadius,
        float thetaL,
        float thetaR,
        float scale = 1.0f
    );

    // Generates complex 2D algebraic Calabi-Yau quintic threefold cross-section mesh
    [[nodiscard]] static IsosurfaceMesh generateCalabiYauCrossSectionMesh(
        std::size_t uSegments,
        std::size_t vSegments,
        float thetaL,
        float thetaR,
        float phaseAngle = 0.0f,
        float scale = 1.0f
    );

    // Generates 240-root E8 Gosset 4_21 root lattice polytope mesh under SO(4) projection
    [[nodiscard]] static IsosurfaceMesh generateLieE8CoxeterProjectionMesh(
        float strutThickness,
        float thetaL,
        float thetaR,
        float scale = 1.0f
    );

    // Generates orientable 2-manifold Seifert surface bounded by (p, q) torus knot in S^3
    [[nodiscard]] static IsosurfaceMesh generateSeifertSurfaceMesh(
        std::uint32_t p,
        std::uint32_t q,
        std::size_t radialSegments,
        std::size_t angularSegments,
        float thetaL,
        float thetaR,
        float scale = 1.0f
    );

    // Generates non-associative G2 exceptional Lie group associator foliation mesh on S^7 -> S^3 -> R^3
    [[nodiscard]] static IsosurfaceMesh generateOctonionicG2FibrationMesh(
        std::size_t fiberCount,
        std::size_t pointsPerFiber,
        float ribbonWidth,
        float thetaL,
        float thetaR,
        float time = 0.0f,
        float scale = 1.0f
    );

    // Generates 4D regular hyperbolic {3,5,3} icosahedral honeycomb cell projection mesh
    [[nodiscard]] static IsosurfaceMesh generateHyperbolicHoneycombMesh(
        std::size_t cellCount,
        float strutThickness,
        float thetaL,
        float thetaR,
        float scale = 1.0f
    );

    // Generates multi-instanton Clifford-Hopf soliton lattice mesh in S^3 -> R^3
    [[nodiscard]] static IsosurfaceMesh generateCliffordHopfSolitonLatticeMesh(
        std::size_t ringCount,
        std::size_t pointsPerRing,
        float tubeRadius,
        float thetaL,
        float thetaR,
        float scale = 1.0f
    );

    // Conformal inverse stereographic lift: R^3 -> S^3 subset R^4 with exact unit norm
    [[nodiscard]] static SculptureVec4 lift3DtoS3(
        const SculptureVec3& p3
    ) noexcept;

    // Evaluates S^3 hyperspherical instanton trajectory flow under SO(4) rotation
    [[nodiscard]] static SculptureVec3 evaluateS3InstantonFlow(
        const SculptureVec3& p3,
        float thetaL,
        float thetaR,
        float phiL = 0.0f,
        float phiR = 0.0f,
        float scale = 1.0f
    ) noexcept;

    // Generates complex twistor null shear-free geodesic fibration ribbon mesh on S^3 -> R^3
    [[nodiscard]] static IsosurfaceMesh generateTwistorFibrationRibbonMesh(
        std::size_t fiberCount,
        std::size_t pointsPerFiber,
        float ribbonWidth,
        float thetaL,
        float thetaR,
        float time = 0.0f,
        float scale = 1.0f
    );

    // Evaluates phase-locked Kuramoto foliation hypersurface wave offsets W(x, t) across sample points
    [[nodiscard]] static std::vector<float> evaluateKuramotoFoliationWaveField(
        const std::vector<SculptureVec3>& samplePoints,
        float time,
        float couplingK = 1.5f,
        float naturalFreq = 2.0f
    );
};

} // namespace epi_sculpt
