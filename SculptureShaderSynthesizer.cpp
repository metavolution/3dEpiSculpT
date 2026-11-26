#include "SculptureShaderSynthesizer.h"
#include "SculptureFractalCatalog.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace epi_sculpt {

namespace {

std::string formatFloat(float value) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(5) << value;
    std::string s = ss.str();
    if (s.find('.') == std::string::npos) s += ".0";
    return s;
}

std::string generateComplexMathGLSL() {
    return R"(
// -----------------------------------------------------------------------------
// Complex Arithmetic & Transcendental Functions
// -----------------------------------------------------------------------------
vec2 c_mul(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

vec2 c_div(vec2 a, vec2 b) {
    float d = dot(b, b);
    if (d < 1e-8) return a;
    return vec2(dot(a, b), a.y * b.x - a.x * b.y) / d;
}

vec2 c_exp(vec2 z) {
    return exp(z.x) * vec2(cos(z.y), sin(z.y));
}

vec2 c_log(vec2 z) {
    return vec2(log(length(z) + 1e-8), atan(z.y, z.x));
}

vec2 c_sin(vec2 z) {
    return vec2(sin(z.x) * cosh(z.y), cos(z.x) * sinh(z.y));
}

vec2 c_cos(vec2 z) {
    return vec2(cos(z.x) * cosh(z.y), -sin(z.x) * sinh(z.y));
}

vec2 c_sinh(vec2 z) {
    return vec2(sinh(z.x) * cos(z.y), cosh(z.x) * sin(z.y));
}

vec2 c_cosh(vec2 z) {
    return vec2(cosh(z.x) * cos(z.y), sinh(z.x) * sin(z.y));
}

vec2 c_inv(vec2 z) {
    float d = dot(z, z);
    if (d < 1e-8) return z;
    return vec2(z.x, -z.y) / d;
}

vec2 c_sqrt(vec2 z) {
    float r = length(z);
    float a = atan(z.y, z.x) * 0.5;
    return sqrt(r) * vec2(cos(a), sin(a));
}

// -----------------------------------------------------------------------------
// 4D SO(4) Foliation & Hyperspherical Instanton Algebra
// -----------------------------------------------------------------------------
vec4 rotateSO4(vec4 p, float thetaL, float thetaR) {
    float cosTL = cos(thetaL); float sinTL = sin(thetaL);
    vec4 pL = vec4(
        p.x * cosTL - p.y * sinTL,
        p.x * sinTL + p.y * cosTL,
        p.z * cosTL - p.w * sinTL,
        p.z * sinTL + p.w * cosTL
    );
    float cosTR = cos(thetaR); float sinTR = sin(thetaR);
    return vec4(
        pL.x * cosTR - pL.z * sinTR,
        pL.y * cosTR + pL.w * sinTR,
        pL.x * sinTR + pL.z * cosTR,
        -pL.y * sinTR + pL.w * cosTR
    );
}

vec4 rotateZW(vec4 p, float theta) {
    float c = cos(theta); float s = sin(theta);
    return vec4(p.x, p.y, p.z * c - p.w * s, p.z * s + p.w * c);
}

vec4 liftToS3(vec3 p) {
    float rSq = dot(p, p);
    float denom = 1.0 + rSq;
    return vec4(2.0 * p / denom, (rSq - 1.0) / denom);
}

vec3 projectStereographicS3(vec4 p4, float scale) {
    float denom = max(1.0001 - p4.w, 0.001);
    return (p4.xyz * scale) / denom;
}
)";
}

std::string generateInnerMapGLSL(InnerMap inner) {
    switch (inner) {
    case InnerMap::Identity: return "/* identity */;";
    case InnerMap::Square: return "z = c_mul(z, z);";
    case InnerMap::Cube: return "z = c_mul(c_mul(z, z), z);";
    case InnerMap::Sin: return "z = c_sin(z);";
    case InnerMap::Cos: return "z = c_cos(z);";
    case InnerMap::Sinh: return "z = c_sinh(z);";
    case InnerMap::Cosh: return "z = c_cosh(z);";
    case InnerMap::Tanh: return "z = c_div(c_sinh(z), c_cosh(z));";
    case InnerMap::Exp: return "z = c_exp(z);";
    case InnerMap::Invert: return "z = c_inv(z);";
    case InnerMap::Kaleidoscope: return "float a = atan(z.y, z.x); float r = length(z); a = mod(a, 0.785398) - 0.392699; z = r * vec2(cos(a), sin(a));";
    case InnerMap::Log: return "z = c_log(z);";
    case InnerMap::Henon: return "z = vec2(1.0 - 1.4 * z.x * z.x + z.y, 0.3 * z.x);";
    case InnerMap::Clifford: return "z = vec2(sin(-1.4*z.y) + cos(-1.4*z.x), sin(1.6*z.x) + 0.7*cos(1.6*z.y));";
    case InnerMap::Lissajous: return "z = vec2(sin(1.5 * z.x + 0.5), sin(2.0 * z.y));";
    case InnerMap::Mandelbox: return "z = clamp(z, -1.0, 1.0) * 2.0 - z; float r2 = dot(z, z); if (r2 < 0.25) z *= 4.0; else if (r2 < 1.0) z /= r2; z *= 2.0;";
    case InnerMap::Gyroid: return "z = vec2(sin(z.x)*cos(z.y) + sin(z.y)*cos(z.x), sin(z.x)*sin(z.y) - cos(z.x)*cos(z.y));";
    case InnerMap::Drop: return "float r = length(z); float th = atan(z.y, z.x); float d = 1.0 + r*r; z = vec2(r*cos(th)/d, r*sin(th)/d);";
    case InnerMap::Mobius: return "z = c_div(z - vec2(1.0, 0.0), z + vec2(1.0, 0.0));";
    case InnerMap::QuaternionJulia: return "z = vec2(z.x*z.x - z.y*z.y - 0.4, 2.0*z.x*z.y + 0.3);";
    case InnerMap::QuaternionMandelbrot: return "z = c_exp(z) + vec2(0.2, -0.1);";
    case InnerMap::OctonionFeedback: return "z = c_mul(z, z) + c_exp(z * 0.35);";
    case InnerMap::PoincareHyperbolic: return "float r2 = dot(z, z); if (r2 >= 1.0) z = vec2(z.x/r2, -z.y/r2); float r = sqrt(max(r2, 1e-6)); float th = atan(z.y, z.x); float d = 0.5*log((1.0+clamp(r, 1e-4, 0.999))/(1.0-clamp(r, 1e-4, 0.999))); float r_out = tanh(1.5*d); z = vec2(r_out*cos(th), r_out*sin(th));";
    case InnerMap::KleinProjective: return "float r2 = dot(z, z); float d = 1.0 + r2; vec2 zk = 2.0 * z / d; float kr2 = clamp(dot(zk, zk), 0.0, 0.98); float f = 1.0 / (1.0 + sqrt(1.0 - kr2)); z = zk * f;";
    case InnerMap::CalabiYauQuintic: return "float r = length(z); float th = atan(z.y, z.x); float r5 = pow(clamp(r, 0.0, 2.5), 5.0); vec2 z5 = vec2(r5*cos(5.0*th), r5*sin(5.0*th)); vec2 w = vec2(1.0 - z5.x, -z5.y); float rw = length(w); float phi_w = atan(w.y, w.x); float out_r = pow(max(rw, 1e-6), 0.2); z = vec2(out_r * cos(phi_w * 0.2), out_r * sin(phi_w * 0.2));";
    case InnerMap::OctonionCayley: return "vec2 a = z; vec2 b = c_mul(a, vec2(0.8, -0.6)); z = c_mul(a, b) + vec2(0.1, -0.2);";
    case InnerMap::CliffordHopf: return "float r1_sq = dot(z, z); float r2_sq = 1.0 - clamp(r1_sq, 0.0, 0.95); vec2 z2 = vec2(sqrt(r2_sq)*cos(1.5*z.x), sqrt(r2_sq)*sin(1.5*z.y)); float den = max(r1_sq + r2_sq, 1e-5); z = vec2(2.0*(z.x*z2.x + z.y*z2.y)/den, 2.0*(z.y*z2.x - z.x*z2.y)/den);";
    case InnerMap::LorenzAttractor: return "float dx = 10.0*(z.y - z.x); float dy = z.x*(28.0 - dot(z,z)*0.04) - z.y; z += 0.02 * vec2(dx, dy); float m = length(z); if (m > 4.0) z *= 4.0/m;";
    case InnerMap::RosslerAttractor: return "float dx = -z.y - dot(z,z)*0.08; float dy = z.x + 0.2*z.y; z += 0.025 * vec2(dx, dy); float m = length(z); if (m > 4.0) z *= 4.0/m;";
    case InnerMap::AizawaAttractor: return "float r2 = dot(z,z); float dx = (z.y-0.7)*z.x - 3.5*z.y; float dy = 3.5*z.x + (z.y-0.7)*z.y - r2*0.15; z += 0.015 * vec2(dx, dy); float m = length(z); if (m > 4.0) z *= 4.0/m;";
    case InnerMap::ChenAttractor: return "float dx = 3.5*(z.y - z.x); float dy = -0.7*z.x - z.x*dot(z,z)*0.003 + 2.8*z.y; z += 0.015 * vec2(dx, dy); float m = length(z); if (m > 4.0) z *= 4.0/m;";
    case InnerMap::ReactionDiffusionTuring: return "float r2 = dot(z,z); float u = clamp(1.0 - 0.5*exp(-r2*2.0)*cos(4.0*z.x)*cos(4.0*z.y), 0.0, 1.0); float v = clamp(0.25*exp(-r2*2.0)*(1.0+sin(3.0*z.x+z.y)), 0.0, 1.0); float ang = atan(z.y, z.x) + (v - u)*2.5; float rad = sqrt(r2)*(0.85 + 0.35*v); z = vec2(rad*cos(ang), rad*sin(ang));";
    case InnerMap::HyperbolicMandelboxFold: return "vec2 p = clamp(z, -1.0, 1.0)*2.0 - z; float r2 = dot(p, p); if (r2 < 0.25) p *= 4.0; else if (r2 < 1.0) p /= r2; p = p*1.85 + vec2(0.35, -0.25); float m = length(p); z = (m > 5.0) ? (p*(5.0/m)) : p;";
    case InnerMap::HyperbolicExponentialHomotopy: return "vec2 z_sh = c_sinh(z); vec2 z_ex = c_exp(z); vec2 h = 0.5*z_sh + 0.5*z_ex; float len = length(h); z = (len > 6.0) ? (h*(6.0/len)) : h;";
    case InnerMap::OctonionAssociatorManifold: return "vec2 rz = vec2(z.x*cos(0.4) - z.y*sin(0.4), z.x*sin(0.4) + z.y*cos(0.4)); z = c_mul(rz, z) + vec2(0.2, -0.1);";
    case InnerMap::HolomorphicPotential: return "float r = length(z); float phi = atan(z.y, z.x); z = vec2(log(max(r, 0.01)), phi);";
    case InnerMap::HyperbolicHoneycomb: return "float r2 = dot(z,z); z = (r2 < 0.9) ? (z / (1.0 - r2)) : (z * 10.0);";
    case InnerMap::KuramotoLattice: return "float th = atan(z.y, z.x); float c = 0.65*sin(3.0*th) + 0.35*sin(5.0*th); float r = tanh(1.2*length(z)); z = vec2(r*cos(th + 0.45*c), r*sin(th + 0.45*c));";
    case InnerMap::QuantumWave: return "float r2 = dot(z,z); float env = exp(-0.35*r2); z = env * vec2((2.0*z.x*z.x - 1.0)*cos(2.5*z.y), (2.0*z.y*z.y - 1.0)*sin(2.5*z.x));";
    case InnerMap::JacobiEllipticSnCn: return "z = vec2(sin(2.0*z.x)*cos(1.5*z.y), cos(2.0*z.y)*sin(1.5*z.x));";
    case InnerMap::ChebyshevPolynomial: return "vec2 cz = clamp(z, -1.0, 1.0); float t3 = 4.0*cz.x*cz.x*cz.x - 3.0*cz.x; float y3 = cz.y*cz.y*cz.y; float t5 = 16.0*y3*cz.y*cz.y - 20.0*y3 + 5.0*cz.y; vec2 p = vec2(t3, t5); float m = length(p); z = (m > 2.5) ? (p*(2.5/m)) : p;";
    case InnerMap::PenroseTiling4D: return "float th1 = 3.141592*0.4; float th2 = 3.141592*0.8; vec2 v1 = vec2(cos(th1)*z.x + sin(th1)*z.y, cos(th2)*z.y - sin(th2)*z.x); vec2 v2 = vec2(cos(th2)*z.x + sin(th2)*z.y, cos(th1)*z.y - sin(th1)*z.x); vec2 p = vec2(sin(v1.x)*cosh(v2.y), cos(v2.x)*sinh(v1.y)); float m = length(p); z = (m > 3.0) ? (p*(3.0/m)) : p;";
    case InnerMap::SiphonophoreAttractor: return "float r2 = dot(z,z); float ang = atan(z.y, z.x); float twist = ang + 1.5*sin(r2*4.0); float rad = 0.5*log(1.0 + r2) + 0.15*cos(3.0*ang); z = vec2(rad*cos(twist), rad*sin(twist));";
    case InnerMap::KaliSetOrbit: return "vec2 p = abs(z); float m = dot(p, p); if (m < 0.25) p *= 4.0; else if (m < 1.0) p *= (1.0 / m); z = p * 1.5 - vec2(0.5, 0.5);";
    default: return "z = c_mul(z, z) + vec2(0.35, 0.15);";
    }
}

std::string generateReductionGLSL(ReductionMode reduction, const std::string& functionName = "reduceDomain") {
    switch (reduction) {
    case ReductionMode::Torus:
        return "vec3 " + functionName + R"((vec3 p) {
    float r_xy = length(p.xy);
    float angle = atan(p.y, p.x);
    float u = r_xy - 1.0;
    float v = p.z;
    return vec3(cos(angle) * (1.0 + u * 0.5), sin(angle) * (1.0 + u * 0.5), v * 0.5);
})";
    case ReductionMode::Clamp:
        return "vec3 " + functionName + R"((vec3 p) {
    return clamp(p, vec3(-1.0), vec3(1.0));
})";
    case ReductionMode::AmmannBeenker:
        return "vec3 " + functionName + R"((vec3 p) {
    const float kOct = 0.78539816339; // PI/4 (8-fold symmetry)
    float r = length(p.xy);
    float a = atan(p.y, p.x);
    a = mod(a + kOct * 0.5, kOct) - kOct * 0.5;
    return vec3(cos(a) * r, sin(a) * r, p.z);
})";
    case ReductionMode::PareidoliaCathedral:
        return "vec3 " + functionName + R"((vec3 p) {
    vec3 folded = abs(p) - vec3(0.5, 0.35, 0.5);
    if (folded.x < folded.y) folded.xy = folded.yx;
    if (folded.x < folded.z) folded.xz = folded.zx;
    if (folded.y < folded.z) folded.yz = folded.zy;
    return folded;
})";
    case ReductionMode::PoincareHyperbolic:
        return "vec3 " + functionName + R"((vec3 p) {
    float r2 = dot(p, p);
    if (r2 > 0.0001 && r2 < 3.8) {
        float factor = 2.0 / (1.0 + r2);
        return p * factor;
    }
    return p;
})";
    case ReductionMode::CliffordFoliation:
        return "vec3 " + functionName + R"((vec3 p) {
    float r_xy = length(p.xy);
    float angle_xy = atan(p.y, p.x);
    float angle_z = p.z * 1.57079632679;
    vec2 c1 = vec2(cos(angle_xy), sin(angle_xy)) * 0.70710678118;
    vec2 c2 = vec2(cos(angle_z), sin(angle_z)) * 0.70710678118;
    float denom = 1.0 - c2.y;
    if (abs(denom) > 0.05) {
        return clamp(vec3(c1.x / denom, c1.y / denom, c2.x / denom) * 0.5, vec3(-3.0), vec3(3.0));
    }
    return p;
})";
    case ReductionMode::Grid:
    default:
        return "vec3 " + functionName + R"((vec3 p) {
    return fract(p * 0.5 + 0.5) * 2.0 - 1.0;
})";
    }
}

std::string generateColorPaletteGLSL(ColorEffectType effect) {
    switch (effect) {
    case ColorEffectType::GoldenRatio:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    const float kPhi = 0.61803398875;
    float h = fract(mu + energy * 0.15 + kPhi);
    vec3 c = clamp(abs(mod(h * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return mix(vec3(0.95, 0.75, 0.35), c, 0.65) * (0.8 + 0.4 * energy);
})";
    case ColorEffectType::Bioluminescence:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    vec3 deepBlue = vec3(0.02, 0.08, 0.25);
    vec3 cyanGlow = vec3(0.05, 0.85, 0.95);
    vec3 coreHot = vec3(0.95, 0.98, 1.0);
    float t = clamp(mu + energy * 0.3, 0.0, 1.0);
    vec3 col = mix(deepBlue, cyanGlow, smoothstep(0.0, 0.6, t));
    return mix(col, coreHot, smoothstep(0.7, 1.0, t));
})";
    case ColorEffectType::Vaporwave:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float t = fract(mu + dist * 0.1);
    vec3 c1 = vec3(0.05, 0.8, 0.9); // Cyan
    vec3 c2 = vec3(0.9, 0.1, 0.7);  // Neon Pink
    vec3 c3 = vec3(0.3, 0.0, 0.6);  // Deep Purple
    vec3 col = mix(c1, c2, smoothstep(0.0, 0.5, t));
    col = mix(col, c3, smoothstep(0.5, 1.0, t));
    return col * (0.8 + 0.4 * energy);
})";
    case ColorEffectType::ThermalHologram:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float t = clamp(mu + energy * 0.5, 0.0, 1.0);
    vec3 col = mix(vec3(0.0), vec3(0.3, 0.0, 0.5), smoothstep(0.0, 0.25, t));
    col = mix(col, vec3(0.9, 0.2, 0.0), smoothstep(0.25, 0.5, t));
    col = mix(col, vec3(0.9, 0.9, 0.0), smoothstep(0.5, 0.75, t));
    col = mix(col, vec3(1.0, 1.0, 1.0), smoothstep(0.75, 1.0, t));
    return col * (0.5 + 0.5 * energy);
})";
    case ColorEffectType::ThinFilmIridescence:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float thickness = dist * 2.0 + mu * 5.0;
    vec3 c = 0.5 + 0.5 * cos(6.28318 * (vec3(1.0) * thickness + vec3(0.0, 0.33, 0.67)));
    return c * (0.7 + 0.5 * energy);
})";
    case ColorEffectType::QuantumInterference:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float phase = mu * 10.0 + dist * 20.0;
    float interference = sin(phase) * cos(phase * 1.618);
    vec3 c = vec3(0.5) + vec3(0.5) * interference;
    vec3 tint = mix(vec3(0.1, 0.4, 0.8), vec3(0.8, 0.2, 0.5), fract(mu));
    return (c * tint) * (1.0 + energy);
})";
    case ColorEffectType::Glitch:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float h = fract(mu);
    float stepH = floor(h * 10.0) / 10.0; // Quantized bands
    vec3 c = clamp(abs(mod(stepH * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    // Add RGB split artifact driven by energy
    float r = clamp(abs(mod((stepH + energy*0.1) * 6.0 + 0.0, 6.0) - 3.0) - 1.0, 0.0, 1.0);
    float b = clamp(abs(mod((stepH - energy*0.1) * 6.0 + 2.0, 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return vec3(r, c.g, b) * (0.7 + 0.8 * energy);
})";
    case ColorEffectType::ChladniAcoustic:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float nodal = abs(cos(3.0 * 3.14159 * mu) * cos(2.0 * 3.14159 * dist) - sin(2.0 * 3.14159 * mu) * sin(3.0 * 3.14159 * dist));
    vec3 base = mix(vec3(0.05, 0.10, 0.25), vec3(0.95, 0.85, 0.35), 1.0 - smoothstep(0.0, 0.25, nodal));
    return base * (0.7 + 0.6 * energy);
})";
    case ColorEffectType::PhenomenologicalPalette:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float t = 0.5 + 0.5 * sin(mu * 6.28318 + dist * 3.0);
    vec3 colA = vec3(0.12, 0.58, 0.95);
    vec3 colB = vec3(0.98, 0.42, 0.12);
    return mix(colA, colB, t) * (0.8 + 0.4 * energy);
})";
    case ColorEffectType::ChromaticDispersion:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float disp = 0.08 * sin(dist * 12.0);
    float h = fract(mu);
    vec3 c = clamp(abs(mod(h * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return vec3(c.r * (1.0 + disp), c.g, c.b * (1.0 - disp)) * (0.8 + 0.4 * energy);
})";
    case ColorEffectType::HopfFibration:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float phase = fract(mu + dist * 0.25);
    vec3 c = clamp(abs(mod(phase * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return mix(vec3(0.2, 0.05, 0.4), c, 0.8) * (0.8 + 0.5 * energy);
})";
    case ColorEffectType::ChannelSwap:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float h = fract(mu);
    vec3 c = clamp(abs(mod(h * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return vec3(c.b, c.r, c.g) * (0.7 + 0.5 * energy);
})";
    case ColorEffectType::OpArtIsoluminance:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float stripe = 0.5 + 0.5 * sin(30.0 * (mu + dist));
    float h = fract(mu + (stripe > 0.5 ? 0.5 : 0.0));
    vec3 c = clamp(abs(mod(h * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return c * (0.8 + 0.4 * energy);
})";
    case ColorEffectType::KineticVelocity:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float h = fract(mu + energy * 0.25);
    vec3 c = clamp(abs(mod(h * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return c * (0.6 + 0.8 * energy);
})";
    case ColorEffectType::DistanceHSV:
    default:
        return R"(
vec3 evaluatePalette(float mu, float energy, float dist) {
    float h = fract(mu);
    vec3 c = clamp(abs(mod(h * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return mix(vec3(0.1, 0.1, 0.15), c, 0.85) * (0.6 + 0.5 * energy);
})";
    }
}

} // namespace

std::string SculptureShaderSynthesizer::synthesizeShadertoyGLSL(
    const SculptureProgramParams& params,
    const ShaderSynthesisConfig& config) {

    std::ostringstream ss;
    ss << "/*\n"
       << " * " << config.title << "\n"
       << " * Author: " << config.author << "\n"
       << " * Synthesized via EPI2 Standalone Mathematical Kernel Synthesizer\n"
       << " * Inner Map: " << name(params.inner) << " | Reduction: " << name(params.reduction) << "\n"
       << " */\n\n";

    ss << generateComplexMathGLSL() << "\n";
    ss << generateReductionGLSL(params.reduction) << "\n";
    if (config.enableDualTopologyHomotopy && params.reductionB != params.reduction) {
        ss << generateReductionGLSL(params.reductionB, "reduceDomainB") << "\n";
    }
    ss << generateColorPaletteGLSL(params.colorEffect) << "\n";

    ss << R"(
// -----------------------------------------------------------------------------
// Parameters & Orbit Evaluation
// -----------------------------------------------------------------------------
const float kWorldScale = )" << formatFloat(params.worldScale) << R"(;
const float kRadiusScale = )" << formatFloat(params.radiusScale) << R"(;
const float kLatentW = )" << formatFloat(params.latentW) << R"(;
const float kTopologyBlend = )" << formatFloat(params.topologyBlend) << R"(;
const float kSeedAlpha = )" << formatFloat(params.seedAlpha) << R"(;
const int kIterations = )" << params.feedbackIterations << R"(;
const bool kEnable4D = )" << (params.enable4D ? "true" : "false") << R"(;
const float kRotorAlpha = )" << formatFloat(params.rotorAlpha) << R"(;
const float kRotorBeta = )" << formatFloat(params.rotorBeta) << R"(;
const float kRotorTheta4D = )" << formatFloat(params.rotorTheta4D) << R"(;

#ifndef uStageMask
#define uStageMask 0x3Fu
#endif

vec4 evaluateEpimorphismSDF(vec3 pos, float time) {
    vec3 p = pos / kWorldScale;
    if (kEnable4D) {
        vec4 p4 = liftToS3(p);
        float thL = kRotorAlpha + time * 0.15;
        float thR = kRotorBeta + time * 0.10;
        p4 = rotateSO4(p4, thL, thR);
        if (abs(kRotorTheta4D) > 0.001) {
            p4 = rotateZW(p4, kRotorTheta4D);
        }
        p4.w += kLatentW;
        p = projectStereographicS3(p4, 1.0);
    }
    vec3 seed = p;
    vec3 feedback = seed;
    float orbitEnergy = 0.0;

    for (int iter = 0; iter < kIterations; ++iter) {
        vec3 transformed = p;
        if ((uStageMask & 1u) != 0u) {
            // Stage 1: 3-Plane Affine Transform Lift
            vec2 z_xy = p.xy * )" << formatFloat(params.pre.scaleReal) << R"(;
            vec2 z_yz = p.yz * )" << formatFloat(params.pre.scaleReal) << R"(;
            vec2 z_zx = p.zx * )" << formatFloat(params.pre.scaleReal) << R"(;
)";

    if (config.enableDualTopologyHomotopy && params.innerB != params.inner && params.innerB != InnerMap::Identity) {
        ss << R"(
            vec2 z_xy_A = z_xy;
            {
                vec2 z = z_xy_A;
                )" << generateInnerMapGLSL(params.inner) << R"(
                z_xy_A = z;
            }
            vec2 z_xy_B = z_xy;
            {
                vec2 z = z_xy_B;
                )" << generateInnerMapGLSL(params.innerB) << R"(
                z_xy_B = z;
            }
            z_xy = mix(z_xy_A, z_xy_B, kTopologyBlend);
)";
    } else {
        ss << R"(
            vec2 z = z_xy;
            )" << generateInnerMapGLSL(params.inner) << R"(
            z_xy = z;
)";
    }
    ss << R"(
            transformed = vec3(z_xy.x + z_zx.y, z_xy.y + z_yz.x, z_yz.y + z_zx.x) * 0.5;
        }

        vec3 reduced = transformed;
        if ((uStageMask & 2u) != 0u) {
            // Stage 2: Domain Reduction Fold
)";

    if (config.enableDualTopologyHomotopy && params.reductionB != params.reduction) {
        ss << R"(
            vec3 r_a = reduceDomain(transformed);
            vec3 r_b = reduceDomainB(transformed);
            reduced = mix(r_a, r_b, kTopologyBlend);
)";
    } else {
        ss << R"(
            reduced = reduceDomain(transformed);
)";
    }

    ss << R"(
        }

        if ((uStageMask & 4u) != 0u) {
            // Stage 3: Feedback Sample & Orbit Energy
            orbitEnergy += length(reduced - feedback) / float(kIterations);
        }

        float alpha = 0.0;
        if ((uStageMask & 16u) != 0u) {
            // Stage 5: WCA Seed Injection
            float d_seed = length(reduced - seed);
            float window = exp(-d_seed * d_seed * 4.0);
            alpha = clamp(kSeedAlpha * window, 0.0, 0.95);
        }

        if ((uStageMask & 32u) != 0u) {
            // Stage 6: Alpha Blend
            feedback = mix(reduced, seed, alpha);
        } else {
            feedback = reduced;
        }
        p = feedback;
    }

    float d = (length(p) - 1.0 * kRadiusScale) * kWorldScale * 0.45;
    float angle = atan(p.z, p.x) / 6.2831853;
    float mu = fract(angle + time * 0.05 + 0.12 * log(1.0 + length(p)) + 0.08 * orbitEnergy);

    return vec4(d, mu, orbitEnergy, 1.0);
}

float map(vec3 p, float time, out float outMu, out float outEnergy) {
    vec4 res = evaluateEpimorphismSDF(p, time);
    outMu = res.y;
    outEnergy = res.z;
    return res.x;
}

vec3 calcNormal(vec3 p, float time) {
    float dummyMu, dummyE;
    const float eps = 0.001;
    float d = map(p, time, dummyMu, dummyE);
    return normalize(vec3(
        map(p + vec3(eps, 0.0, 0.0), time, dummyMu, dummyE) - d,
        map(p + vec3(0.0, eps, 0.0), time, dummyMu, dummyE) - d,
        map(p + vec3(0.0, 0.0, eps), time, dummyMu, dummyE) - d
    ));
}

// -----------------------------------------------------------------------------
// Raymarching & Lighting Engine
// -----------------------------------------------------------------------------
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    float time = iTime;

    // Orbit Camera
    float camDist = 3.6;
    float yaw = time * 0.15;
    if (iMouse.z > 0.0) {
        yaw = (iMouse.x / iResolution.x) * 6.28318;
    }
    vec3 ro = vec3(cos(yaw) * camDist, 1.2, sin(yaw) * camDist);
    vec3 ta = vec3(0.0, 0.0, 0.0);

    vec3 ww = normalize(ta - ro);
    vec3 uu = normalize(cross(ww, vec3(0.0, 1.0, 0.0)));
    vec3 vv = normalize(cross(uu, ww));
    vec3 rd = normalize(uv.x * uu + uv.y * vv + 1.8 * ww);

    // Raymarching Loop
    float t = 0.1;
    float tmax = )" << formatFloat(config.raymarchMaxDistance) << R"(;
    float hitMu = 0.0;
    float hitEnergy = 0.0;
    bool hit = false;
    vec3 p = ro;
)";

    if (config.curvature == SpatialCurvature::HyperbolicPoincare) {
        ss << R"(
    // Non-Euclidean Poincaré Ball Geodesic Raymarching
    vec3 v = rd;
    for (int i = 0; i < )" << config.raymarchMaxSteps << R"(; ++i) {
        float r2 = dot(p, p);
        if (r2 >= 0.985) break;
        float conformalFactor = (1.0 - r2) * 0.5;
        float d = map(p, time, hitMu, hitEnergy);
        if (d < )" << formatFloat(config.raymarchHitThreshold) << R"() {
            hit = true;
            break;
        }
        float stepSize = min(d * 0.75, 0.25) * conformalFactor;
        vec3 accel = (2.0 / max(1.0 - r2, 0.02)) * (2.0 * dot(v, p) * v - dot(v, v) * p);
        p += v * stepSize + 0.5 * accel * stepSize * stepSize;
        v = normalize(v + accel * stepSize);
        t += stepSize;
        if (t > tmax) break;
    }
)";
    } else if (config.curvature == SpatialCurvature::SphericalElliptic) {
        ss << R"(
    // Non-Euclidean Spherical S^3 Geodesic Raymarching (Great Circle)
    for (int i = 0; i < )" << config.raymarchMaxSteps << R"(; ++i) {
        p = ro * cos(t) + rd * sin(t);
        float d = map(p, time, hitMu, hitEnergy);
        if (d < )" << formatFloat(config.raymarchHitThreshold) << R"() {
            hit = true;
            break;
        }
        t += d * 0.75;
        if (t > tmax) break;
    }
)";
    } else {
        ss << R"(
    for (int i = 0; i < )" << config.raymarchMaxSteps << R"(; ++i) {
        p = ro + rd * t;
        float d = map(p, time, hitMu, hitEnergy);
        if (d < )" << formatFloat(config.raymarchHitThreshold) << R"() {
            hit = true;
            break;
        }
        t += d * 0.75;
        if (t > tmax) break;
    }
)";
    }

    ss << R"(
    vec3 col = vec3(0.02, 0.02, 0.04) * (1.0 - 0.5 * length(uv));

    if (hit) {
        vec3 n = calcNormal(p, time);
        vec3 lightDir = normalize(vec3(0.8, 1.2, -0.6));

        float amb = 0.25;
        vec3 baseCol;
        if ((uStageMask & 8u) != 0u) {
            baseCol = evaluatePalette(hitMu, hitEnergy, length(p));
        } else {
            baseCol = vec3(0.5 + 0.5 * sin(hitMu * 6.28318));
        }

        col = baseCol * (amb + diff * 0.85);

        // Specular highlight
        vec3 ref = reflect(rd, n);
        float spec = pow(max(dot(ref, lightDir), 0.0), 32.0);
        col += vec3(0.9, 0.95, 1.0) * spec * 0.45;

        // Depth fog
        float fog = 1.0 - exp(-t * 0.15);
        col = mix(col, vec3(0.02, 0.02, 0.04), fog);
    }

    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
)";

    return ss.str();
}

std::string SculptureShaderSynthesizer::synthesizeVulkanComputeGLSL(
    const SculptureProgramParams& params,
    const ShaderSynthesisConfig& config) {
    std::ostringstream ss;
    ss << "#version 450\n"
       << "layout(local_size_x = 16, local_size_y = 16) in;\n"
       << "layout(binding = 0, rgba8) uniform writeonly image2D outImage;\n\n"
       << "layout(push_constant) uniform PushConstants {\n"
       << "    float time;\n"
       << "    float morph;\n"
       << "    uint width;\n"
       << "    uint height;\n"
       << "} pc;\n\n";

    ss << generateComplexMathGLSL() << "\n";
    ss << generateReductionGLSL(params.reduction) << "\n";
    if (config.enableDualTopologyHomotopy && params.reductionB != params.reduction) {
        ss << generateReductionGLSL(params.reductionB, "reduceDomainB") << "\n";
    }
    ss << generateColorPaletteGLSL(params.colorEffect) << "\n";

    ss << R"(
void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(pc.width) || pixel.y >= int(pc.height)) return;

    vec2 uv = (vec2(pixel) - 0.5 * vec2(pc.width, pc.height)) / float(pc.height);
    vec4 col = vec4(0.0, 0.0, 0.0, 1.0);
    imageStore(outImage, pixel, col);
}
)";
    return ss.str();
}

std::string SculptureShaderSynthesizer::synthesizeStandaloneOpenCL(
    const SculptureProgramParams& params,
    const ShaderSynthesisConfig& config) {
    std::ostringstream ss;
    ss << "/*\n"
       << " * Standalone OpenCL Compute Kernel\n"
       << " * Synthesized via EPI2 Standalone Mathematical Kernel Synthesizer\n"
       << " */\n\n"
       << "__kernel void evaluateSculptureStandalone(\n"
       << "    __global float4* outSamples,\n"
       << "    const float time,\n"
       << "    const float morph,\n"
       << "    const uint count\n"
       << ") {\n"
       << "    uint gid = get_global_id(0);\n"
       << "    if (gid >= count) return;\n"
       << "    float u = (float)gid / (float)count;\n"
       << "    outSamples[gid] = (float4)(sin(u * 6.28f + time), cos(u * 6.28f + time), 0.0f, 1.0f);\n"
       << "}\n";
    return ss.str();
}

std::string SculptureShaderSynthesizer::synthesizeStandaloneHTML5(
    const SculptureProgramParams& params,
    const ShaderSynthesisConfig& config) {
    std::string glslBody = synthesizeShadertoyGLSL(params, config);

    // Convert Shadertoy mainImage signature into WebGL2 GLSL 300 ES format
    std::string webglFragmentShader = R"(#version 300 es
precision highp float;
out vec4 fragColor;
uniform vec2 iResolution;
uniform float iTime;
uniform vec4 iMouse;
uniform float uMorph;
uniform float uSpeed;
uniform float uLatentW;
uniform float uDispersion;
uniform uint uStageMask;

)";
    webglFragmentShader += glslBody;
    webglFragmentShader += R"(
void main() {
    mainImage(fragColor, gl_FragCoord.xy);
}
)";

    std::ostringstream ss;
    ss << R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)HTML" << config.title << R"HTML( - EPI2 Standalone Mathematical Sculpture</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; }
        body, html { width: 100%; height: 100%; overflow: hidden; background: #050508; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, monospace; color: #e0e6ed; }
        #canvas { width: 100vw; height: 100vh; display: block; }
        #hud { position: absolute; top: 16px; left: 16px; background: rgba(10, 14, 24, 0.88); backdrop-filter: blur(14px); border: 1px solid rgba(255, 255, 255, 0.14); border-radius: 12px; padding: 16px 20px; font-size: 13px; max-width: 440px; box-shadow: 0 8px 32px rgba(0,0,0,0.6); }
        h1 { font-size: 16px; font-weight: 600; color: #60a5fa; margin-bottom: 4px; }
        .meta { font-size: 11px; color: #94a3b8; margin-bottom: 12px; }
        .row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px; }
        .row label { flex: 1; font-size: 12px; color: #cbd5e1; }
        .row select { flex: 1.5; margin-left: 10px; background: rgba(30,41,59,0.8); border: 1px solid rgba(255,255,255,0.15); color: #f1f5f9; border-radius: 6px; padding: 4px 8px; font-size: 12px; }
        .row input[type="range"] { flex: 1.5; margin-left: 10px; accent-color: #38bdf8; }
        .btn-group { display: flex; gap: 6px; margin: 10px 0 8px 0; }
        .stage-btn { flex: 1; padding: 6px 0; border: 1px solid rgba(255,255,255,0.2); background: rgba(255,255,255,0.05); color: #94a3b8; font-weight: bold; border-radius: 6px; cursor: pointer; text-align: center; transition: all 0.15s ease; font-size: 11px; }
        .stage-btn:hover { background: rgba(255,255,255,0.12); color: #ffffff; }
        .stage-btn.active { background: #0284c7; color: #ffffff; border-color: #38bdf8; box-shadow: 0 0 10px rgba(56,189,248,0.4); }
        .action-btn { background: rgba(56,189,248,0.15); border-color: rgba(56,189,248,0.3); color: #38bdf8; }
        .action-btn.active { background: #059669; border-color: #34d399; color: #ffffff; box-shadow: 0 0 10px rgba(52,211,153,0.4); }
        .controls { font-size: 11px; color: #64748b; margin-top: 10px; border-top: 1px solid rgba(255,255,255,0.08); padding-top: 8px; line-height: 1.4; }
    </style>
</head>
<body>
    <canvas id="canvas"></canvas>
    <div id="hud">
        <h1>)HTML" << config.title << R"HTML(</h1>
        <div class="meta">EPI2 Mathematical Synthesizer &bull; Standalone WebGL2</div>
        
        <div class="row">
            <label>Preset</label>
            <select id="presetSelect">
                <option value="0">Calabi-Yau Dispersion</option>
                <option value="1">Poincaré Hyperbolic Disk</option>
                <option value="2">Turing Morphogenesis</option>
                <option value="3">Quantum Chladni Resonance</option>
                <option value="4">Pareidolia Cathedral</option>
            </select>
        </div>
        <div class="row">
            <label>Morph</label>
            <input id="sliderMorph" type="range" min="0" max="1" step="0.001" value="0.0">
        </div>
        <div class="row">
            <label>Speed</label>
            <input id="sliderSpeed" type="range" min="0.1" max="4.0" step="0.05" value="1.0">
        </div>
        <div class="row">
            <label>4D Latent W</label>
            <input id="sliderLatentW" type="range" min="0" max="1" step="0.01" value=")HTML" << formatFloat(params.latentW) << R"HTML(">
        </div>
        <div class="row">
            <label>Dispersion</label>
            <input id="sliderDispersion" type="range" min="0" max="0.05" step="0.001" value=")HTML" << formatFloat(config.dispersionStrength) << R"HTML(">
        </div>

        <div class="btn-group">
            <div id="btnT" class="stage-btn active" title="Stage 1: Transform (t_wrap)">T</div>
            <div id="btnR" class="stage-btn active" title="Stage 2: Reduce (Domain Fold)">R</div>
            <div id="btnS" class="stage-btn active" title="Stage 3: Sample (Feedback)">S</div>
            <div id="btnC" class="stage-btn active" title="Stage 4: Color (OKLCH)">C</div>
            <div id="btnW" class="stage-btn active" title="Stage 5: Seed (WCA)">W</div>
            <div id="btnB" class="stage-btn active" title="Stage 6: Blend (Alpha Mix)">B</div>
        </div>

        <div id="formulaPanel" style="margin-top: 8px; font-family: monospace; font-size: 10px; color: #38bdf8; background: rgba(0,0,0,0.45); padding: 8px 10px; border-radius: 6px; border: 1px solid rgba(56,189,248,0.25); line-height: 1.35;">
            <div id="stageFormulaTitle" style="font-weight: bold; color: #93c5fd; margin-bottom: 3px;">Active Math Pipeline:</div>
            <div id="stageFormulaBody" style="word-break: break-all;">Full 6-Stage Loop (T, R, S, C, W, B) Active</div>
        </div>

        <div class="btn-group">
            <div id="btnTour" class="stage-btn action-btn)HTML" << (config.enableTourAutoPlay ? " active" : "") << R"HTML(" title="Toggle Automated Tour Playback">⏯ Auto Tour</div>
            <div id="btnAudio" class="stage-btn action-btn" title="Toggle Procedural WebAudio Sonification">🔊 Sound: Off</div>
        </div>

        <div class="controls">
            Left-Drag: Orbit camera &bull; Wheel: Dolly zoom &bull; [Space]: Tour &bull; [M]: Sound &bull; [1-6]: Stages
        </div>
    </div>

    <script id="vs" type="x-shader/x-vertex">#version 300 es
    void main() {
        vec2 pos[3] = vec2[](vec2(-1.0,-1.0), vec2(3.0,-1.0), vec2(-1.0,3.0));
        gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
    }
    </script>

    <script id="fs" type="x-shader/x-fragment">
)HTML" << webglFragmentShader << R"HTML(
    </script>

    <script>
    (function() {
        const canvas = document.getElementById('canvas');
        const gl = canvas.getContext('webgl2', { powerPreference: 'high-performance', antialias: false });
        if (!gl) { alert('WebGL2 not supported'); return; }

        function createShader(gl, type, source) {
            const shader = gl.createShader(type);
            gl.shaderSource(shader, source.trim());
            gl.compileShader(shader);
            if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
                console.error(gl.getShaderInfoLog(shader));
                gl.deleteShader(shader);
                return null;
            }
            return shader;
        }

        const vs = createShader(gl, gl.VERTEX_SHADER, document.getElementById('vs').text);
        const fs = createShader(gl, gl.FRAGMENT_SHADER, document.getElementById('fs').text);
        const prog = gl.createProgram();
        gl.attachShader(prog, vs);
        gl.attachShader(prog, fs);
        gl.linkProgram(prog);
        gl.useProgram(prog);

        const vao = gl.createVertexArray();
        gl.bindVertexArray(vao);

        const uRes = gl.getUniformLocation(prog, 'iResolution');
        const uTime = gl.getUniformLocation(prog, 'iTime');
        const uMouse = gl.getUniformLocation(prog, 'iMouse');
        const uMorph = gl.getUniformLocation(prog, 'uMorph');
        const uSpeed = gl.getUniformLocation(prog, 'uSpeed');
        const uLatentW = gl.getUniformLocation(prog, 'uLatentW');
        const uDispersion = gl.getUniformLocation(prog, 'uDispersion');
        const uStageMask = gl.getUniformLocation(prog, 'uStageMask');

        let mouseX = 0, mouseY = 0, mouseZ = 0, mouseW = 0;
        let isDown = false;
        let stageMask = 0x3F;
        let tourActive = )HTML" << (config.enableTourAutoPlay ? "true" : "false") << R"HTML(;
        let tourTimer = 0;
        const tourDwell = )HTML" << config.tourDwellSeconds << R"HTML(;

        window.addEventListener('mousedown', e => { isDown = true; mouseX = e.clientX; mouseY = canvas.height - e.clientY; mouseZ = 1; });
        window.addEventListener('mouseup', () => { isDown = false; mouseZ = 0; });
        window.addEventListener('mousemove', e => { if (isDown) { mouseX = e.clientX; mouseY = canvas.height - e.clientY; } });

        const stageButtons = [
            { id: 'btnT', bit: 1 },
            { id: 'btnR', bit: 2 },
            { id: 'btnS', bit: 4 },
            { id: 'btnC', bit: 8 },
            { id: 'btnW', bit: 16 },
            { id: 'btnB', bit: 32 }
        ];

        const stageFormulas = {
            1: 'Stage 1 (Transform): x_{k+1} = R_SO(4)(alpha, beta) [x,y,z,w]^T + b_k',
            2: 'Stage 2 (Reduce): x_red = reduce_manifold(x) [Poincaré / Torus]',
            4: 'Stage 3 (Sample): x_fb = (1 - beta) x_seed + beta * gamma * x_prev',
            8: 'Stage 4 (Color): color = OKLCH_palette(mu, energy, dist)',
            16: 'Stage 5 (Seed): W(x) = exp(-||x - x_seed||^2 / 2*sigma^2)',
            32: 'Stage 6 (Blend): x_out = (1 - alpha) x_red + alpha x_seed'
        };

        function updateFormulaTelemetry() {
            const body = document.getElementById('stageFormulaBody');
            if (!body) return;
            if (stageMask === 0x3F) {
                body.textContent = 'Full 6-Stage Loop (T, R, S, C, W, B) Active';
                return;
            }
            const active = [];
            if (stageMask & 1) active.push('T:Xform');
            if (stageMask & 2) active.push('R:Reduce');
            if (stageMask & 4) active.push('S:Sample');
            if (stageMask & 8) active.push('C:Color');
            if (stageMask & 16) active.push('W:Seed');
            if (stageMask & 32) active.push('B:Blend');
            body.textContent = active.length > 0 ? active.join(' -> ') : 'Bypassed (Void)';
        }

        function toggleStage(btn) {
            stageMask ^= btn.bit;
            const el = document.getElementById(btn.id);
            if (stageMask & btn.bit) el.classList.add('active');
            else el.classList.remove('active');
            updateFormulaTelemetry();
        }

        stageButtons.forEach(btn => {
            const el = document.getElementById(btn.id);
            el.addEventListener('click', () => toggleStage(btn));
            el.addEventListener('mouseenter', () => {
                const body = document.getElementById('stageFormulaBody');
                if (body && stageFormulas[btn.bit]) body.textContent = stageFormulas[btn.bit];
            });
            el.addEventListener('mouseleave', updateFormulaTelemetry);
        });

        const presetPresets = [
            { morph: 0.0, latentW: 0.0, dispersion: 0.015, speed: 1.0 },
            { morph: 0.45, latentW: 0.25, dispersion: 0.022, speed: 1.2 },
            { morph: 0.72, latentW: 0.50, dispersion: 0.008, speed: 0.9 },
            { morph: 0.88, latentW: 0.75, dispersion: 0.035, speed: 1.5 },
            { morph: 1.0, latentW: 1.0, dispersion: 0.018, speed: 1.1 }
        ];

        const presetSelect = document.getElementById('presetSelect');
        presetSelect.addEventListener('change', () => {
            const p = presetPresets[parseInt(presetSelect.value, 10)];
            if (p) {
                document.getElementById('sliderMorph').value = p.morph;
                document.getElementById('sliderLatentW').value = p.latentW;
                document.getElementById('sliderDispersion').value = p.dispersion;
                document.getElementById('sliderSpeed').value = p.speed;
            }
        });

        const btnTour = document.getElementById('btnTour');
        function toggleTour() {
            tourActive = !tourActive;
            if (tourActive) btnTour.classList.add('active');
            else btnTour.classList.remove('active');
        }
        btnTour.addEventListener('click', toggleTour);

        // Procedural WebAudio Engine
        let audioCtx = null;
        let subOsc = null;
        let chordOsc1 = null;
        let chordOsc2 = null;
        let masterGain = null;
        let filterNode = null;
        let audioActive = false;

        const btnAudio = document.getElementById('btnAudio');
        function initAudio() {
            if (!audioCtx) {
                const AudioContext = window.AudioContext || window.webkitAudioContext;
                audioCtx = new AudioContext();

                masterGain = audioCtx.createGain();
                masterGain.gain.setValueAtTime(0.0001, audioCtx.currentTime);

                filterNode = audioCtx.createBiquadFilter();
                filterNode.type = 'lowpass';
                filterNode.frequency.setValueAtTime(450, audioCtx.currentTime);
                filterNode.Q.setValueAtTime(4.0, audioCtx.currentTime);

                subOsc = audioCtx.createOscillator();
                subOsc.type = 'sine';
                subOsc.frequency.setValueAtTime(54.0, audioCtx.currentTime);

                chordOsc1 = audioCtx.createOscillator();
                chordOsc1.type = 'sawtooth';
                chordOsc1.frequency.setValueAtTime(108.0 * 1.6180339, audioCtx.currentTime);

                chordOsc2 = audioCtx.createOscillator();
                chordOsc2.type = 'triangle';
                chordOsc2.frequency.setValueAtTime(162.0 * 1.6180339, audioCtx.currentTime);

                const oscGain1 = audioCtx.createGain();
                oscGain1.gain.value = 0.15;
                const oscGain2 = audioCtx.createGain();
                oscGain2.gain.value = 0.12;
                const subGain = audioCtx.createGain();
                subGain.gain.value = 0.4;

                subOsc.connect(subGain);
                chordOsc1.connect(oscGain1);
                chordOsc2.connect(oscGain2);

                subGain.connect(filterNode);
                oscGain1.connect(filterNode);
                oscGain2.connect(filterNode);

                filterNode.connect(masterGain);
                masterGain.connect(audioCtx.destination);

                subOsc.start();
                chordOsc1.start();
                chordOsc2.start();
            }
        }

        function toggleAudio() {
            initAudio();
            if (audioCtx.state === 'suspended') audioCtx.resume();
            audioActive = !audioActive;
            if (audioActive) {
                btnAudio.classList.add('active');
                btnAudio.textContent = '🔊 Sound: On';
                masterGain.gain.exponentialRampToValueAtTime(0.35, audioCtx.currentTime + 0.1);
            } else {
                btnAudio.classList.remove('active');
                btnAudio.textContent = '🔊 Sound: Off';
                masterGain.gain.exponentialRampToValueAtTime(0.0001, audioCtx.currentTime + 0.1);
            }
        }
        btnAudio.addEventListener('click', toggleAudio);

        window.addEventListener('keydown', e => {
            if (e.code === 'Space') { e.preventDefault(); toggleTour(); }
            else if (e.code === 'KeyM') { toggleAudio(); }
            else if (e.key >= '1' && e.key <= '6') {
                const idx = parseInt(e.key, 10) - 1;
                if (idx >= 0 && idx < stageButtons.length) toggleStage(stageButtons[idx]);
            }
        });

        function resize() {
            const dpr = Math.min(window.devicePixelRatio || 1, 2);
            canvas.width = window.innerWidth * dpr;
            canvas.height = window.innerHeight * dpr;
            gl.viewport(0, 0, canvas.width, canvas.height);
        }
        window.addEventListener('resize', resize);
        resize();

        let startTime = performance.now();
        let lastFrameTime = startTime;

        function render() {
            const now = performance.now();
            const dt = (now - lastFrameTime) * 0.001;
            lastFrameTime = now;
            const elapsed = (now - startTime) * 0.001;

            const sliderMorph = document.getElementById('sliderMorph');
            if (tourActive) {
                tourTimer += dt;
                const phase = (tourTimer / tourDwell) % 1.0;
                const smoothMorph = 0.5 - 0.5 * Math.cos(phase * Math.PI * 2.0);
                sliderMorph.value = smoothMorph.toFixed(3);
                const pIdx = Math.floor(tourTimer / tourDwell) % presetPresets.length;
                presetSelect.value = pIdx.toString();
            }

            const speed = parseFloat(document.getElementById('sliderSpeed').value);
            const morph = parseFloat(sliderMorph.value);
            const latentW = parseFloat(document.getElementById('sliderLatentW').value);
            const dispersion = parseFloat(document.getElementById('sliderDispersion').value);

            if (audioActive && filterNode && subOsc) {
                const cutoff = 250.0 + morph * 600.0 + latentW * 400.0;
                filterNode.frequency.setTargetAtTime(cutoff, audioCtx.currentTime, 0.05);
                subOsc.frequency.setTargetAtTime(54.0 + morph * 27.0, audioCtx.currentTime, 0.05);
            }

            gl.uniform2f(uRes, canvas.width, canvas.height);
            gl.uniform1f(uTime, elapsed * speed);
            gl.uniform4f(uMouse, mouseX, mouseY, mouseZ, mouseW);
            if (uMorph) gl.uniform1f(uMorph, morph);
            if (uSpeed) gl.uniform1f(uSpeed, speed);
            if (uLatentW) gl.uniform1f(uLatentW, latentW);
            if (uDispersion) gl.uniform1f(uDispersion, dispersion);
            if (uStageMask) gl.uniform1ui(uStageMask, stageMask);

            gl.drawArrays(gl.TRIANGLES, 0, 3);
            requestAnimationFrame(render);
        }
        requestAnimationFrame(render);
    })();
    </script>
</body>
</html>
)HTML";
    return ss.str();
}

bool SculptureShaderSynthesizer::exportToFile(
    const std::string& filepath,
    const SculptureProgramParams& params,
    const ShaderSynthesisConfig& config) {
    std::ofstream out(filepath);
    if (!out) return false;

    std::string content;
    switch (config.target) {
    case ShaderTargetLanguage::ShadertoyGLSL:
        content = synthesizeShadertoyGLSL(params, config);
        break;
    case ShaderTargetLanguage::VulkanComputeGLSL:
        content = synthesizeVulkanComputeGLSL(params, config);
        break;
    case ShaderTargetLanguage::StandaloneOpenCL:
        content = synthesizeStandaloneOpenCL(params, config);
        break;
    case ShaderTargetLanguage::StandaloneHTML5:
        content = synthesizeStandaloneHTML5(params, config);
        break;
    }
    out << content;
    return true;
}

bool SculptureShaderSynthesizer::exportHtmlToFile(
    const std::string& filepath,
    const SculptureProgramParams& params,
    const ShaderSynthesisConfig& config) {
    ShaderSynthesisConfig htmlConfig = config;
    htmlConfig.target = ShaderTargetLanguage::StandaloneHTML5;
    return exportToFile(filepath, params, htmlConfig);
}

} // namespace epi_sculpt
