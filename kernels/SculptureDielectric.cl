// Dielectric: bounded optical accumulation, not a signed-distance surface.
// The compact reference leaves z/O uninitialized and f4(t.xy,-t) ambiguous.
// We choose z=0.05, O=0, and four poles (x,y,-x,-y), as in the older
// dielectric experiment. Every procedural operation remains in OpenCL.
#include "SculptureGpuAbi.clh"

// Same image ABI as SculptureRaymarch.cl. Lighting/auxiliaries are retained
// for transport compatibility, not used to manufacture opaque surface hits.
__kernel void raymarch_dielectric(
    __constant const SculptureProgramParamsGpu* params,
    __constant const SculptureFrameGpu* frame,
    __write_only image2d_t out_color,
    __write_only image2d_t out_depth,
    __write_only image2d_t out_normal,
    __write_only image2d_t out_velocity,
    uint width, uint height,
    float4 camera_position, float4 camera_target, float4 camera_up,
    float fov_radians,
    float4 key_light_direction, float4 key_light_radiance,
    uint max_steps, float hit_epsilon, float max_distance,
    float eye_separation)
{
    const uint x = get_global_id(0), y = get_global_id(1);
    if (x >= width || y >= height) return;
    const int2 pixel = (int2)(x, y);

    const bool stereo = eye_separation > 1.0e-6f;
    const uint eye_width = stereo ? width / 2u : width;
    const bool right_eye = stereo && x >= eye_width;
    const uint local_x = right_eye ? x - eye_width : x;
    const float3 forward = normalize(camera_target.xyz - camera_position.xyz);
    const float3 right = normalize(cross(forward, camera_up.xyz));
    const float3 up = cross(right, forward);
    const float eye_offset = stereo
        ? (right_eye ? 0.5f : -0.5f) * eye_separation : 0.0f;
    const float2 uv = (float2)(
        (2.0f * (local_x + 0.5f) - eye_width) / (float)height,
        1.0f - 2.0f * (y + 0.5f) / (float)height);
    const float3 ray = normalize(forward + tan(0.5f * fov_radians)
        * (uv.x * right + uv.y * up));

    // Existing serialized controls form a demo-specific lens vocabulary:
    // pre scale/offset = base frequency, advance, pole width, cross-channel mix;
    // seed width/alpha = cylinder radius, gain; pulse frequency = 9/z resonance.
    const float scale = clamp(params->output_scale.x, 0.1f, 8.0f);
    const float3 origin = (camera_position.xyz + eye_offset * right) / scale;
    const float phase = frame->time * params->dynamics_seed.x;
    const uint octaves = clamp(params->feedback_iterations, 1u, 8u);
    const uint steps = clamp(max_steps, 1u, 80u);
    const float base_frequency = clamp(params->pre_affine.x, 0.5f, 12.0f);
    const float advance = clamp(params->pre_affine.y, 0.05f, 0.5f);
    const float pole_width = clamp(params->pre_affine.z, 0.001f, 0.1f);
    const float coupling = clamp(params->pre_affine.w, 0.0f, 0.3f);
    const float radius = clamp(params->dynamics_seed.y, 0.25f, 3.0f);
    const float gain = 0.002f * clamp(params->dynamics_seed.z, 0.0f, 8.0f);
    const float resonance = clamp(params->pulse_material.x, 0.1f, 40.0f);
    const float warp = clamp(params->pulse_material.y, 0.0f, 2.0f);
    const float max_travel = fmin(max_distance / scale, 30.0f);
    float travel = 0.05f;
    float4 energy = (float4)(0.0f);

    for (uint step = 0u; step < steps && travel < max_travel; ++step) {
        const float3 sample_position = origin + travel * ray;
        float3 p = sample_position;
        float frequency = base_frequency;
        // Frequencies double; amplitudes halve. Each octave sees the preceding
        // warp, giving coupled, nested ripples rather than independent noise.
        for (uint octave = 0u; octave < octaves; ++octave) {
            frequency *= 2.0f;
            p += warp * sin(p.zxy * frequency + travel - phase) / frequency;
        }
        // This is a heuristic advance near a rippled cylinder, NOT a safe SDF
        // bound. No hit/normal/AO computation or early opaque termination.
        travel += fmax(fabs(radius - length(p.xy)) * advance, 0.001f);
        const float4 poles = (float4)(sample_position.xy, -sample_position.xy);
        const float4 denominator = fmax(fabs(poles - radius), (float4)(pole_width));
        const float pulse = 1.0f + cos(resonance / travel - phase);
        energy += gain * pulse / denominator;
    }

    // tanh is part of the authored optical response, not an HDR display mode.
    const float4 response = tanh(energy + coupling * length(energy));
    const float3 spectral = response.x * (float3)(1.0f, 0.30f, 0.08f)
        + response.y * (float3)(0.08f, 1.0f, 0.45f)
        + response.z * (float3)(0.12f, 0.50f, 1.0f)
        + response.w * (float3)(0.95f, 0.15f, 0.85f);
    const float palette_mix = clamp(params->pulse_material.z, 0.0f, 1.0f);
    const float3 color = clamp(mix(response.xyz, spectral, palette_mix), 0.0f, 1.0f);
    write_imagef(out_color, pixel, (float4)(color, 1.0f));
    // Explicit miss values: this field has no unique surface or motion vector.
    write_imagef(out_depth, pixel, (float4)(max_distance, 0.0f, 0.0f, 0.0f));
    write_imagef(out_normal, pixel, (float4)(0.5f, 0.5f, 1.0f, 0.0f));
    write_imagef(out_velocity, pixel, (float4)(0.0f));
}
