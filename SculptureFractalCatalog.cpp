#include "SculptureFractalCatalog.h"
#include <algorithm>
#include <array>

namespace epi_sculpt {

struct FractalDef {
    std::uint32_t id;
    std::string_view name;
    std::string_view clCode;
};

static constexpr std::array<FractalDef, 44> kInnerDefs = {{
    {0, "identity", "return z;"},
    {1, "square", "return epi_complex_mul(z,z);"},
    {2, "cube", "return epi_complex_mul(epi_complex_mul(z,z),z);"},
    {3, "sin", "return (float2)(sin(z.x)*cosh(z.y), cos(z.x)*sinh(z.y));"},
    {4, "cos", "return (float2)(cos(z.x)*cosh(z.y), -sin(z.x)*sinh(z.y));"},
    {5, "sinh", "return (float2)(sinh(z.x)*cos(z.y), cosh(z.x)*sin(z.y));"},
    {6, "cosh", "return (float2)(cosh(z.x)*cos(z.y), sinh(z.x)*sin(z.y));"},
    {7, "tanh", "return epi_complex_div((float2)(sinh(z.x)*cos(z.y), cosh(z.x)*sin(z.y)), (float2)(cosh(z.x)*cos(z.y), sinh(z.x)*sin(z.y)));"},
    {8, "exp", "float m = exp(z.x); return m*(float2)(cos(z.y), sin(z.y));"},
    {9, "invert", "float m = fmax(dot(z,z), 1e-5f); return (float2)(z.x/m, -z.y/m);"},
    {10, "kaleidoscope", "float r = length(z); float th = atan2(z.y, z.x); float w = fmod(th+EPI_SCULPT_PI, 2.0f*EPI_SCULPT_PI/6.0f); if(w<0.0f) w+=2.0f*EPI_SCULPT_PI/6.0f; float f = fabs(w-EPI_SCULPT_PI/6.0f)-EPI_SCULPT_PI; return (float2)(r*cos(f), r*sin(f));"},
    {11, "log", "return (float2)(log(fmax(length(z), 1e-5f)), atan2(z.y, z.x));"},
    {12, "henon", "return (float2)(1.0f - 1.4f*z.x*z.x + z.y, 0.3f*z.x);"},
    {13, "clifford", "return (float2)(sin(-1.4f*z.y) + 1.0f*cos(-1.4f*z.x), sin(1.6f*z.x) + 0.7f*cos(1.6f*z.y));"},
    {14, "lissajous", "return (float2)(sin(1.5f*z.x+0.5f), sin(2.0f*z.y));"},
    {15, "mandelbox", "float x=z.x; float y=z.y; if(x>1.0f) x=2.0f-x; else if(x<-1.0f) x=-2.0f-x; if(y>1.0f) y=2.0f-y; else if(y<-1.0f) y=-2.0f-y; float mag=x*x+y*y; if(mag<0.25f){x*=4.0f; y*=4.0f;} else if(mag<1.0f&&mag>0.0001f){x/=mag;y/=mag;} return (float2)(x*2.0f, y*2.0f);"},
    {16, "gyroid", "return (float2)(sin(z.x)*cos(z.y) + sin(z.y)*cos(z.x), sin(z.x)*sin(z.y) - cos(z.x)*cos(z.y));"},
    {17, "drop", "float r = length(z); float th = atan2(z.y, z.x); float d = 1.0f+r*r; return (float2)(r*cos(th)/d, r*sin(th)/d);"},
    {18, "mobius", "float2 num=(float2)(z.x-1.0f, z.y); float2 den=(float2)(z.x+1.0f, z.y); return epi_complex_div(num, den);"},
    {19, "quat_julia", "return epi_quat_square((float4)(z.x, z.y, 0.0f, 0.0f)).xy;"},
    {20, "quat_mandel", "return epi_quat_exp((float4)(z.x, z.y, 0.0f, 0.0f)).xy;"},
    {21, "octonion", "float4 lo = (float4)(z.x, z.y, 0.25f*z.x, -0.15f*z.y); float4 hi = (float4)(0.1f*z.y, 0.05f*z.x, -0.08f*z.y, 0.12f*z.x); float4 sq_lo, sq_hi, exp_lo, exp_hi; epi_oct_square(lo, hi, &sq_lo, &sq_hi); epi_oct_exp(lo*0.35f, hi*0.35f, &exp_lo, &exp_hi); return epi_oct_project_2d(sq_lo + exp_lo, sq_hi + exp_hi);"},
    {22, "poincare", "float r2 = dot(z,z); if (r2 >= 1.0f) { z = (float2)(z.x/r2, -z.y/r2); r2 = dot(z,z); } float r = sqrt(fmax(r2, 1e-8f)); float th = atan2(z.y, z.x); float d = 0.5f*log((1.0f + clamp(r, 1e-4f, 0.999f))/(1.0f - clamp(r, 1e-4f, 0.999f))); float r_out = tanh(1.5f * d); return (float2)(r_out * cos(th), r_out * sin(th));"},
    {23, "klein", "float r2 = dot(z,z); float d = 1.0f + r2; float2 zk = 2.0f * z / d; float kr2 = clamp(dot(zk, zk), 0.0f, 0.98f); float f = 1.0f / (1.0f + sqrt(1.0f - kr2)); return zk * f;"},
    {24, "calabi_yau", "float r = length(z); float th = atan2(z.y, z.x); float r5 = pow(clamp(r, 0.0f, 2.5f), 5.0f); float2 z5 = (float2)(r5*cos(5.0f*th), r5*sin(5.0f*th)); float2 w = (float2)(1.0f - z5.x, -z5.y); float rw = length(w); float phi_w = atan2(w.y, w.x); float out_r = pow(fmax(rw, 1e-6f), 0.2f); return (float2)(out_r * cos(phi_w * 0.2f), out_r * sin(phi_w * 0.2f));"},
    {25, "octonion_cayley", "float4 a_lo = (float4)(z.x, z.y, 0.1f, -0.1f); float4 a_hi = (float4)(0.05f, -0.05f, 0.02f, -0.02f); float4 b_lo = (float4)(0.2f, -0.1f, z.x*0.5f, z.y*0.5f); float4 b_hi = (float4)(0.1f, 0.0f, -0.1f, 0.0f); float4 c_lo = (float4)(0.1f, 0.2f, -0.1f, 0.0f); float4 c_hi = (float4)(z.y*0.3f, z.x*0.3f, 0.05f, -0.05f); float4 bc_lo, bc_hi, prod_lo, prod_hi; epi_oct_mul(b_lo, b_hi, c_lo, c_hi, &bc_lo, &bc_hi); epi_oct_mul(a_lo, a_hi, bc_lo, bc_hi, &prod_lo, &prod_hi); return epi_oct_project_2d(prod_lo, prod_hi);"},
    {26, "clifford_hopf", "float r1_sq = z.x*z.x + z.y*z.y; float r2_sq = 1.0f - clamp(r1_sq, 0.0f, 0.95f); float2 z2 = (float2)(sqrt(r2_sq)*cos(1.5f*z.x), sqrt(r2_sq)*sin(1.5f*z.y)); float den = fmax(r1_sq + r2_sq, 1e-5f); return (float2)(2.0f*(z.x*z2.x + z.y*z2.y)/den, 2.0f*(z.y*z2.x - z.x*z2.y)/den);"},
    {27, "lorenz", "float dx=10.0f*(z.y-z.x); float dy=z.x*(28.0f-(z.x*z.x+z.y*z.y)*0.04f)-z.y; float2 nz = z + 0.02f*(float2)(dx,dy); float m=length(nz); return (m>4.0f)?(nz*(4.0f/m)):nz;"},
    {28, "rossler", "float dx=-z.y-(z.x*z.x+z.y*z.y)*0.08f; float dy=z.x+0.2f*z.y; float2 nz = z + 0.025f*(float2)(dx,dy); float m=length(nz); return (m>4.0f)?(nz*(4.0f/m)):nz;"},
    {29, "aizawa", "float r2=z.x*z.x+z.y*z.y; float dx=(z.y-0.7f)*z.x-3.5f*z.y; float dy=3.5f*z.x+(z.y-0.7f)*z.y-r2*0.15f; float2 nz = z + 0.015f*(float2)(dx,dy); float m=length(nz); return (m>4.0f)?(nz*(4.0f/m)):nz;"},
    {30, "chen", "float dx=3.5f*(z.y-z.x); float dy=-0.7f*z.x-z.x*(z.x*z.x+z.y*z.y)*0.003f+2.8f*z.y; float2 nz = z + 0.015f*(float2)(dx,dy); float m=length(nz); return (m>4.0f)?(nz*(4.0f/m)):nz;"},
    {31, "turing_rd", "float x=z.x; float y=z.y; float r2=x*x+y*y; float u=clamp(1.0f-0.5f*exp(-r2*2.0f)*cos(4.0f*x)*cos(4.0f*y), 0.0f, 1.0f); float v=clamp(0.25f*exp(-r2*2.0f)*(1.0f+sin(3.0f*x+y)), 0.0f, 1.0f); float lapU=-16.0f*(u-0.5f)*cos(3.0f*x)*cos(3.0f*y); float lapV=-16.0f*v*cos(3.0f*x)*cos(3.0f*y); float uvv=u*v*v; float du=0.18f*lapU-uvv+0.035f*(1.0f-u); float dv=0.09f*lapV+uvv-(0.035f+0.065f)*v; float u_next=clamp(u+0.8f*du,0.0f,1.0f); float v_next=clamp(v+0.8f*dv,0.0f,1.0f); float ang=atan2(y,x)+(v_next-u_next)*2.5f; float rad=sqrt(r2)*(0.85f+0.35f*v_next); return (float2)(rad*cos(ang), rad*sin(ang));"},
    {32, "mandelbox_fold", "float2 p=z; p.x=clamp(p.x,-1.0f,1.0f)*2.0f-p.x; p.y=clamp(p.y,-1.0f,1.0f)*2.0f-p.y; float r2=dot(p,p); if(r2<0.25f){p*=4.0f;}else if(r2<1.0f){p*=(1.0f/r2);} p=p*1.85f+(float2)(0.35f,-0.25f); float m=length(p); return (m>5.0f)?(p*(5.0f/m)):p;"},
    {33, "homotopy_sinh_exp", "float2 z_sh = (float2)(sinh(z.x)*cos(z.y), cosh(z.x)*sin(z.y)); float m = exp(z.x); float2 z_ex = m*(float2)(cos(z.y), sin(z.y)); float2 h = 0.5f*z_sh + 0.5f*z_ex; float len = length(h); return (len > 6.0f) ? (h * (6.0f / len)) : h;"},
    {34, "octonion_associator", "float4 x_lo = (float4)(z.x, z.y, 0.35f*z.x, -0.25f*z.y); float4 x_hi = (float4)(0.15f*z.y, 0.10f*z.x, -0.05f*z.y, 0.08f*z.x); float4 y_lo = (float4)(0.25f, -0.15f, z.x*0.4f, z.y*0.4f); float4 y_hi = (float4)(0.05f, -0.05f, 0.12f, -0.08f); float4 z_lo = (float4)(0.15f, 0.20f, -0.10f, 0.05f); float4 z_hi = (float4)(z.y*0.25f, z.x*0.25f, 0.04f, -0.04f); float4 rotX_lo, rotX_hi; epi_oct_rotate_g2(x_lo, x_hi, (z.x + z.y)*0.5f, &rotX_lo, &rotX_hi); float4 assoc_lo, assoc_hi; epi_oct_associator(rotX_lo, rotX_hi, y_lo, y_hi, z_lo, z_hi, &assoc_lo, &assoc_hi); float4 prod_lo, prod_hi; epi_oct_mul(rotX_lo, rotX_hi, y_lo, y_hi, &prod_lo, &prod_hi); return epi_oct_project_2d(prod_lo + assoc_lo*0.75f, prod_hi + assoc_hi*0.75f);"},
    {35, "holomorphic_potential", "return epi_holomorphic_potential(z);"},
    {36, "hyperbolic_honeycomb", "return epi_hyperbolic_honeycomb(z);"},
    {37, "kuramoto_lattice", "float th=atan2(z.y,z.x); float c=0.65f*sin(3.0f*th)+0.35f*sin(5.0f*th); float r=tanh(1.2f*length(z)); return (float2)(r*cos(th+0.45f*c), r*sin(th+0.45f*c));"},
    {38, "quantum_wave", "float r2=dot(z,z); float env=exp(-0.35f*r2); return env*(float2)((2.0f*z.x*z.x-1.0f)*cos(2.5f*z.y), (2.0f*z.y*z.y-1.0f)*sin(2.5f*z.x));"},
    {39, "jacobi_sn_cn", "return (float2)(sin(2.0f*z.x)*cos(1.5f*z.y), cos(2.0f*z.y)*sin(1.5f*z.x));"},
    {40, "chebyshev_poly", "float cx=clamp(z.x,-1.0f,1.0f); float cy=clamp(z.y,-1.0f,1.0f); float t3=4.0f*cx*cx*cx-3.0f*cx; float y3=cy*cy*cy; float t5=16.0f*y3*cy*cy-20.0f*y3+5.0f*cy; float2 p=(float2)(t3,t5); float m=length(p); return (m>2.5f)?(p*(2.5f/m)):p;"},
    {41, "penrose_tiling_4d", "float th1 = 3.141592f*0.4f; float th2 = 3.141592f*0.8f; float2 v1 = (float2)(cos(th1)*z.x + sin(th1)*z.y, cos(th2)*z.y - sin(th2)*z.x); float2 v2 = (float2)(cos(th2)*z.x + sin(th2)*z.y, cos(th1)*z.y - sin(th1)*z.x); float2 p = (float2)(sin(v1.x)*cosh(v2.y), cos(v2.x)*sinh(v1.y)); float m = length(p); return (m>3.0f)?(p*(3.0f/m)):p;"},
    {42, "siphonophore_attractor", "float r2=dot(z,z); float ang=atan2(z.y, z.x); float twist=ang+1.5f*sin(r2*4.0f); float rad=0.5f*log(1.0f+r2)+0.15f*cos(3.0f*ang); return (float2)(rad*cos(twist), rad*sin(twist));"},
    {43, "kali_set_orbit", "float2 p = z; p.x = fabs(p.x); p.y = fabs(p.y); float m = dot(p,p); if(m < 0.25f) p*=4.0f; else if(m < 1.0f) p*=(1.0f/m); p = p*1.5f - (float2)(0.5f, 0.5f); return p;"}
}};

static constexpr std::array<FractalDef, 21> kSeedDefs = {{
    {0, "border", "return (1.0f - epi_smoothstep(w, 2.0f*w, dist))*(1.0f - epi_smoothstep(w, 2.0f*w, dist));"},
    {1, "wave_ring", "return epi_smoothstep(0.7f,1.0f,cos(25.0f*dist-time*5.0f))*(1.0f-epi_smoothstep(0.0f,3.0f*w,dist));"},
    {2, "cross", "return 1.0f - epi_smoothstep(w*0.1f, w*0.5f, min(min(fabs(diff.x), fabs(diff.y)), fabs(diff.z)));"},
    {3, "dipole", "float r = fmax(dist, 0.001f); float cos_th = diff.y/r; float sin2_th = 1.0f-cos_th*cos_th; return clamp(sin2_th*cos(15.0f*r-time*6.0f)/(1.0f+r*2.0f), 0.0f, 1.0f);"},
    {4, "airy", "float r = fmax(dist, 0.0001f); float x = 15.0f*r; float airy = 2.0f*(sin(x)/(x*x) - cos(x)/x)/x; return clamp(airy*airy, 0.0f, 1.0f);"},
    {5, "lissajous", "float3 lissa = (float3)(sin(3.0f*time+reduced.x*5.0f)*0.5f, sin(2.0f*time+reduced.y*5.0f+1.57079632f)*0.5f, sin(4.0f*time+reduced.z*5.0f)*0.5f); return 1.0f - smoothstep(0.0f, w, length(reduced - lissa));"},
    {6, "attractor", "float dx=10.0f*(reduced.y-reduced.x); float dy=reduced.x*(28.0f-reduced.z)-reduced.y; float dz=reduced.x*reduced.y-2.666f*reduced.z; return 1.0f - smoothstep(0.0f, w*2.0f, fabs(sqrt(dx*dx+dy*dy+dz*dz)*0.05f-1.0f));"},
    {7, "hopf", "float r_xy = length(reduced.xy); float d_core = fabs(r_xy - 1.0f); float d_hopf = sqrt(d_core*d_core + reduced.z*reduced.z); return 1.0f - smoothstep(0.0f, w*2.5f, fabs(d_hopf - 0.35f));"},
    {8, "hypersphere_4d", "float r4 = sqrt(dist*dist + 0.04f); return 1.0f - epi_smoothstep(w, 2.0f*w, fabs(r4 - 1.0f));"},
    {9, "cross_polytope_4d", "float m4 = fabs(diff.x) + fabs(diff.y) + fabs(diff.z); return 1.0f - epi_smoothstep(w, 2.0f*w, fabs(m4 - 1.2f));"},
    {10, "torus_knot_pq", "float phi = atan2(reduced.y, reduced.x); float r_xy = length(reduced.xy); float knot_r = 1.0f + 0.35f*cos(3.0f*phi + time*0.5f); float knot_z = 0.35f*sin(2.0f*phi + time*0.5f); float knot_dist = sqrt((r_xy - knot_r)*(r_xy - knot_r) + (reduced.z - knot_z)*(reduced.z - knot_z)); return 1.0f - smoothstep(0.0f, w*2.2f, knot_dist);"},
    {11, "chladni_cymatics", "float pi=EPI_SCULPT_PI; float drift=time*0.25f; float psi = cos(3.0f*pi*reduced.x+drift)*cos(2.0f*pi*reduced.y-drift*0.5f)*cos(4.0f*pi*reduced.z) - sin(2.0f*pi*reduced.x-drift)*sin(3.0f*pi*reduced.y+drift*0.5f)*sin(4.0f*pi*reduced.z); return 1.0f - smoothstep(0.0f, w*2.0f, fabs(psi));"},
    {12, "turing_perturbation", "float spotWave = cos(8.0f*reduced.x)*cos(8.0f*reduced.y)*cos(8.0f*reduced.z); float vConc = clamp(0.5f+0.5f*spotWave-dist*0.5f, 0.0f, 1.0f); float turingMask = 1.0f - epi_smoothstep(0.2f, 0.7f, fabs(vConc - 0.45f)); return clamp(turingMask*(1.0f - epi_smoothstep(0.0f, w*2.5f, dist)), 0.0f, 1.0f);"},
    {13, "julia_slice", "float4 q=(float4)(reduced.x,reduced.y,reduced.z,0.35f); float4 q2=(float4)(q.x*q.x-q.y*q.y-q.z*q.z-q.w*q.w, 2.0f*q.x*q.y, 2.0f*q.x*q.z, 2.0f*q.x*q.w)+(float4)(-0.4f,0.6f,0.2f,-0.2f); return 1.0f - smoothstep(0.0f, w*2.2f, fabs(length(q2)-1.2f));"},
    {14, "octonion_fibration_s7", "float r_xy = length(reduced.xy); float d8 = sqrt(r_xy*r_xy + reduced.z*reduced.z + 0.05f); return 1.0f - smoothstep(0.0f, w*2.2f, fabs(d8 - 0.85f));"},
    {15, "fibonacci_phyllotaxis", "float r = length(reduced); float theta = atan2(reduced.y, reduced.x); float phi = acos(reduced.z/r); return 1.0f - smoothstep(0.0f, w*1.5f, fabs(sin(13.0f*theta+21.0f*phi)-0.5f));"},
    {16, "kuramoto_synchronization", "float r = fmax(length(reduced), 1e-4f); float theta = atan2(reduced.y, reduced.x); float phi = acos(clamp(reduced.z/r, -1.0f, 1.0f)); float gm = time * 2.5f; return 1.0f - smoothstep(0.0f, w*2.0f, fabs(sin(3.0f*theta-gm)+sin(5.0f*phi-gm*1.618f)-0.5f));"},
    {17, "navier_stokes_vortex", "float2 u_vort = (float2)(-reduced.y, reduced.x) / (dot(reduced.xy, reduced.xy) + 0.1f); float vort_mag = length(u_vort); return 1.0f - smoothstep(0.0f, w*2.5f, fabs(vort_mag - 1.5f - 0.2f*sin(time*2.0f)));"},
    {18, "hyperbolic_tessellation", "float r = length(reduced.xy); float phi = atan2(reduced.y, reduced.x); float kr = tanh(r*2.0f); float poincare = kr * cos(7.0f*phi + time); return 1.0f - smoothstep(0.0f, w*2.0f, fabs(poincare - 0.5f));"},
    {19, "calabi_yau_manifold", "float z_c = reduced.x*reduced.x + reduced.y*reduced.y - reduced.z*reduced.z; float w_c = 2.0f*reduced.x*reduced.y; float manifold = pow(z_c*z_c + w_c*w_c, 0.2f); return 1.0f - smoothstep(0.0f, w*3.0f, fabs(manifold - 0.85f));"},
    {20, "quantum_probability_cloud", "float r = length(reduced); float rho = r * exp(-r*0.5f); float Y_lm = fabs(0.5f * sqrt(15.0f/(EPI_SCULPT_PI)) * reduced.x * reduced.y / (r*r+1e-5f)); float psi2 = rho * rho * Y_lm * Y_lm; return clamp(psi2 * 10.0f, 0.0f, 1.0f) * (1.0f - smoothstep(w*1.5f, w*3.0f, r));"}
}};

std::string_view SculptureFractalCatalog::getInnerName(std::uint32_t id) noexcept {
    for (const auto& def : kInnerDefs) {
        if (def.id == id) return def.name;
    }
    return "unknown";
}

std::string_view SculptureFractalCatalog::getInnerClCode(std::uint32_t id) noexcept {
    for (const auto& def : kInnerDefs) {
        if (def.id == id) return def.clCode;
    }
    return "return z;";
}

std::string_view SculptureFractalCatalog::getSeedName(std::uint32_t id) noexcept {
    for (const auto& def : kSeedDefs) {
        if (def.id == id) return def.name;
    }
    return "unknown";
}

std::string_view SculptureFractalCatalog::getSeedClCode(std::uint32_t id) noexcept {
    for (const auto& def : kSeedDefs) {
        if (def.id == id) return def.clCode;
    }
    return kSeedDefs[0].clCode;
}

static constexpr std::array<FractalDef, 7> kReductionDefs = {{
    {0, "torus fold", "return (float3)(fmod(fabs(p.x)+1.0f, 2.0f)-1.0f, fmod(fabs(p.y)+1.0f, 2.0f)-1.0f, fmod(fabs(p.z)+1.0f, 2.0f)-1.0f);"},
    {1, "grid wrap", "return (float3)(clamp(p.x, -1.0f, 1.0f), clamp(p.y, -1.0f, 1.0f), clamp(p.z, -1.0f, 1.0f));"},
    {2, "clamp", "return clamp(p, (float3)(-1.0f, -1.0f, -1.0f), (float3)(1.0f, 1.0f, 1.0f));"},
    {3, "pareidolia cathedral", "float3 cp = fabs(p); return (float3)(fmin(cp.x, 1.0f), fmin(cp.y, 1.0f), fmin(cp.z, 1.0f));"},
    {4, "ammann beenker", "float3 ab = p; float k = 0.70710678f; ab.xy = (float2)(k*(ab.x+ab.y), k*(ab.y-ab.x)); return clamp(ab, -1.0f, 1.0f);"},
    {5, "poincare hyperbolic", "float r2 = dot(p,p); float r = sqrt(fmax(r2, 1e-6f)); return (r >= 1.0f) ? (p * (tanh(r) / (r * r))) : p;"},
    {6, "clifford foliation", "float2 a = (float2)(cos(3.14159f*p.x), sin(3.14159f*p.x)); float2 b = (float2)(cos(3.14159f*p.y), sin(3.14159f*p.y)); float den = fmax(1.0f - 0.5f*b.y, 0.25f); return (float3)(a.x/den, a.y/den, b.x/den);"}
}};

static constexpr std::array<FractalDef, 18> kColorEffectDefs = {{
    {0, "standard_hsv", "return epi_hsv_to_rgb((float3)(fmod(hue, 1.0f), sat, val));"},
    {1, "distance_hsv", "return epi_hsv_to_rgb((float3)(fmod(hue + dist * 0.25f, 1.0f), sat, val));"},
    {2, "channel_swap", "float3 c = epi_hsv_to_rgb((float3)(fmod(hue, 1.0f), sat, val)); return (float3)(c.z, c.x, c.y);"},
    {3, "glitch", "float3 c = epi_hsv_to_rgb((float3)(fmod(hue + (dist > 0.8f ? 0.33f : 0.0f), 1.0f), sat, val)); return c;"},
    {4, "thermal_hologram", "float t = clamp(dist * 0.5f, 0.0f, 1.0f); return (float3)(t*t, sin(t*3.14159f), 1.0f - t);"},
    {5, "vaporwave", "float t = fmod(hue, 1.0f); return mix((float3)(0.05f, 0.85f, 0.95f), (float3)(0.95f, 0.15f, 0.65f), t);"},
    {6, "golden_ratio", "float phi = 0.61803398875f; return epi_hsv_to_rgb((float3)(fmod(hue + phi, 1.0f), sat, val));"},
    {7, "hopf_fibration", "float phase = atan2(p.y, p.x) / 6.2831853f; return epi_hsv_to_rgb((float3)(fmod(phase + hue, 1.0f), 0.85f, val));"},
    {8, "quaternion_curvature", "float k = length(p); return epi_hsv_to_rgb((float3)(fmod(k * 0.35f + hue, 1.0f), sat, val));"},
    {9, "thin_film_iridescence", "float cosTheta = clamp(fabs(p.z) / fmax(length(p), 1e-4f), 0.1f, 1.0f); float opd = 2.0f * 1.45f * 480.0f * cosTheta; float3 irid = (float3)(0.5f + 0.5f*cos(6.28318f * opd / 650.0f), 0.5f + 0.5f*cos(6.28318f * opd / 532.0f), 0.5f + 0.5f*cos(6.28318f * opd / 450.0f)); return mix(epi_hsv_to_rgb((float3)(fmod(hue, 1.0f), sat*0.5f, val)), irid * val, 0.85f);"},
    {10, "chromatic_dispersion", "float r = length(p); float disp = 0.05f * sin(r * 12.0f - time * 3.0f); float3 base = epi_hsv_to_rgb((float3)(fmod(hue, 1.0f), sat, val)); return (float3)(base.x * (1.0f + disp), base.y, base.z * (1.0f - disp));"},
    {11, "phenomenological_palette", "float t = 0.5f + 0.5f * sin(p.x * 2.0f + hue * 6.28318f); float3 colA = (float3)(0.12f, 0.58f, 0.95f); float3 colB = (float3)(0.98f, 0.42f, 0.12f); return mix(colA, colB, t) * val;"},
    {12, "op_art_isoluminance", "float stripe = 0.5f + 0.5f * sin(20.0f * (p.x + p.y)); float h = fmod(hue + (stripe > 0.5f ? 0.5f : 0.0f), 1.0f); return epi_hsv_to_rgb((float3)(h, sat, val));"},
    {13, "kinetic_velocity", "float v = length(p); float3 c = epi_hsv_to_rgb((float3)(fmod(hue + v * 0.15f, 1.0f), sat, val)); return c * (0.6f + 0.4f * sin(v * 4.0f));"},
    {15, "bioluminescence", "float pulse = 0.5f + 0.5f * sin(time * 3.0f + dist * 5.0f); float3 bio = (float3)(0.08f, 0.95f, 0.65f); return mix(epi_hsv_to_rgb((float3)(fmod(hue, 1.0f), sat, val)), bio * val * 1.5f, pulse * 0.7f);"},
    {16, "quantum_interference", "float psi = sin(8.0f * p.x) * cos(8.0f * p.y) * sin(8.0f * p.z); float prob = psi * psi; return epi_hsv_to_rgb((float3)(fmod(hue + prob * 0.5f, 1.0f), sat, val * (0.4f + 0.6f * prob)));"},
    {17, "chladni_acoustic", "float nodal = fabs(cos(3.0f * EPI_SCULPT_PI * p.x) * cos(2.0f * EPI_SCULPT_PI * p.y) - sin(2.0f * EPI_SCULPT_PI * p.x) * sin(3.0f * EPI_SCULPT_PI * p.y)); return mix((float3)(0.05f, 0.10f, 0.25f), (float3)(0.95f, 0.85f, 0.35f), 1.0f - smoothstep(0.0f, 0.25f, nodal)) * val;"},
    {21, "living_palette_agent", "Spiraling phenomenological agent mapped via spatial angles and logarithmic radius."}
}};

std::string_view SculptureFractalCatalog::getReductionName(std::uint32_t id) noexcept {
    for (const auto& def : kReductionDefs) {
        if (def.id == id) return def.name;
    }
    return "unknown";
}

std::string_view SculptureFractalCatalog::getReductionClCode(std::uint32_t id) noexcept {
    for (const auto& def : kReductionDefs) {
        if (def.id == id) return def.clCode;
    }
    return kReductionDefs[0].clCode;
}

std::string_view SculptureFractalCatalog::getColorEffectName(std::uint32_t id) noexcept {
    for (const auto& def : kColorEffectDefs) {
        if (def.id == id) return def.name;
    }
    return "unknown";
}

std::string_view SculptureFractalCatalog::getColorEffectClCode(std::uint32_t id) noexcept {
    for (const auto& def : kColorEffectDefs) {
        if (def.id == id) return def.clCode;
    }
    return kColorEffectDefs[0].clCode;
}

epi::Complexf SculptureFractalCatalog::evaluateHomotopy(epi::Complexf zA, epi::Complexf zB, epi::Complexf zOrig, float t) noexcept {
    const float clampedT = std::clamp(t, 0.0f, 1.0f);
    const float eps = 4.0f * clampedT * (1.0f - clampedT); // 0 at t=0 and t=1, peak 1.0 at t=0.5
    epi::Complexf linearMix = {zA.re * (1.0f - clampedT) + zB.re * clampedT,
                               zA.im * (1.0f - clampedT) + zB.im * clampedT};
    if (eps <= 1e-4f) return linearMix;

    // Cross product / phase difference indicator
    float crossIm = zA.re * zB.im - zA.im * zB.re;
    float dotRe = zA.re * zB.re + zA.im * zB.im;
    float magA = zA.magnitude();
    float magB = zB.magnitude();
    float avgMag = 0.5f * (magA + magB);

    // Homotopy phase regularization term
    float phaseSign = (crossIm >= 0.0f) ? 1.0f : -1.0f;
    float phaseDiff = (std::abs(crossIm) < 1e-6f && dotRe < 0.0f) ? 3.14159265358979323846f : std::atan2(std::abs(crossIm), dotRe);
    float regularizerScale = 0.5f * eps * std::sin(phaseDiff * 0.5f) * avgMag;

    float zOrigMag = std::max(zOrig.magnitude(), 1e-4f);
    epi::Complexf rotZ = {-zOrig.im / zOrigMag, zOrig.re / zOrigMag}; // i * z / |z|

    epi::Complexf regularized = {
        linearMix.re + phaseSign * regularizerScale * rotZ.re,
        linearMix.im + phaseSign * regularizerScale * rotZ.im
    };

    // Bounded envelope safeguard preventing division-by-zero / pole blowup
    float maxMag = std::max(magA, magB) * (1.0f + 0.25f * eps);
    float regMag = regularized.magnitude();
    if (regMag > maxMag && regMag > 1e-5f) {
        float scale = maxMag / regMag;
        regularized.re *= scale;
        regularized.im *= scale;
    }
    return regularized;
}

} // namespace epi_sculpt
