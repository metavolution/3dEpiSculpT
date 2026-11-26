// -----------------------------------------------------------------------------
// Educational Annotation: OpenCL 3D Epimorphic SDF Chamber Raymarcher
// -----------------------------------------------------------------------------
// This kernel generates high-resolution screen-space raymarched visuals directly
// on the GPU by sphere-tracing through the epimorphic signed distance field.
//
// For each viewport pixel (x, y):
// 1. Synthesize camera ray from normalized device coordinates [-1, 1]^2.
// 2. March ray through epimorphic folded domain and enclosed chamber walls.
// 3. Accumulate volumetric participating media (Henyey-Greenstein scattering).
// 4. On surface hit: calculate tetrahedral normal, Blinn-Phong/PBR specular,
//    ambient occlusion, and Cauchy chromatic dispersion reflections.
// 5. Write non-negative linear HDR color + linear ray depth. Display tone
//    mapping remains Vulkan's responsibility; offline exporters do it explicitly.
// -----------------------------------------------------------------------------

#include "SculptureGpuAbi.clh"
#include "EpiMath.clh"
#include "EpiColor.clh"
#include "EpiTopology.clh"
#include "EpiFractal.clh"
#include "EpiSdf.clh"
#include "EpiAtmosphere.clh"
#include "EpiGeodesic.clh"

// Educational Annotation: Mono / Stereoscopic SBS Raymarching
// Zero eye separation is an explicit mono contract and uses the full image
// width. A positive IPD opts into two equal side-by-side eye viewports.
__kernel void raymarch_sculpture_chamber(
    __constant const SculptureProgramParamsGpu* params,
    __constant const SculptureFrameGpu* frame,
    __write_only image2d_t out_color_image,
    __write_only image2d_t out_depth_image,
    __write_only image2d_t out_normal_image,
    __write_only image2d_t out_velocity_image,
    uint width,
    uint height,
    float4 cam_pos_raw,
    float4 cam_target_raw,
    float4 cam_up_raw,
    float fov_radians,
    float4 key_light_dir,
    float4 key_light_radiance,
    uint max_steps,
    float hit_epsilon,
    float max_dist,
    float eye_separation // Inter-Pupillary Distance (IPD)
) {
    const uint px = get_global_id(0);
    const uint py = get_global_id(1);
    if (px >= width || py >= height) return;
    const uint pixel_index = py * width + px;

    float3 cam_pos = cam_pos_raw.xyz;
    const float3 cam_target = cam_target_raw.xyz;
    const float3 cam_up = cam_up_raw.xyz;

    // Build camera coordinate frame
    float3 forward = normalize(cam_target - cam_pos);
    float3 right = normalize(cross(forward, cam_up));
    float3 up = cross(right, forward);
    
    const bool stereo = eye_separation > 1.0e-6f;
    const uint eye_width = stereo ? width / 2u : width;
    const bool right_eye = stereo && px >= eye_width;
    const float eye_offset = stereo
        ? (right_eye ? eye_separation * 0.5f : -eye_separation * 0.5f)
        : 0.0f;
    cam_pos += right * eye_offset;

    const uint local_px = right_eye ? px - eye_width : px;

    // Compute Ray Direction in Camera Frustum
    const float aspect = convert_float(eye_width) / convert_float(height);
    const float ndcX = (2.0f * (convert_float(local_px) + 0.5f) / convert_float(eye_width) - 1.0f) * aspect * tan(fov_radians * 0.5f);
    const float ndcY = (1.0f - 2.0f * (convert_float(py) + 0.5f) / convert_float(height)) * tan(fov_radians * 0.5f);
    const float3 ray_dir = normalize(forward + right * ndcX + up * ndcY);

    // ─── Sphere Tracing Loop ─────────────────────────────────────────────────
    float t = 0.05f;
    float hit_dist = max_dist;
    bool hit = false;
    bool captured_by_horizon = false;
    float orbit_energy = 0.0f;
    uint steps_taken = 0u;

    const float geometric_mass = fmax(params->attractor_strength, 0.0f);
    const bool geodesic_enabled = geometric_mass > 1.0e-4f;
    const float horizon_radius = epi_schwarzschild_radius(geometric_mass);
    const float photon_sphere_radius = epi_photon_sphere_radius(geometric_mass);
    float closest_radius = max_dist;
    float accumulated_bend = 0.0f;

    const float world_scale = fmax(params->output_scale.y, 0.1f);
    const float box_bound = world_scale * 3.60f;
    float march_max_dist = max_dist;

    // Macro AABB clipping and empty-space leap-forward for straight rays
    if (!geodesic_enabled) {
        const float3 inv_dir = (float3)(
            fabs(ray_dir.x) > 1e-6f ? 1.0f / ray_dir.x : (ray_dir.x >= 0.0f ? 1e6f : -1e6f),
            fabs(ray_dir.y) > 1e-6f ? 1.0f / ray_dir.y : (ray_dir.y >= 0.0f ? 1e6f : -1e6f),
            fabs(ray_dir.z) > 1e-6f ? 1.0f / ray_dir.z : (ray_dir.z >= 0.0f ? 1e6f : -1e6f)
        );
        const float3 t0 = (-box_bound - cam_pos) * inv_dir;
        const float3 t1 = ( box_bound - cam_pos) * inv_dir;
        const float3 tmin_v = fmin(t0, t1);
        const float3 tmax_v = fmax(t0, t1);
        const float t_box_enter = fmax(fmax(tmin_v.x, tmin_v.y), tmin_v.z);
        const float t_box_exit  = fmin(fmin(tmax_v.x, tmax_v.y), tmax_v.z);

        if (t_box_enter > t_box_exit || t_box_exit < 0.05f) {
            // Ray completely misses the chamber volume; write celestial background (or black for diagnostics)
            uint diagnostic_stage = (params->stageMask >> 8) & 0x0Fu;
            const float3 bg = (diagnostic_stage == 0u)
                ? epi_lensed_deep_space(ray_dir, frame->time)
                : (float3)(0.0f);
            write_imagef(out_color_image, (int2)(px, py), (float4)(bg, 1.0f));
            write_imagef(out_depth_image, (int2)(px, py), (float4)(max_dist, 0.0f, 0.0f, 0.0f));
            write_imagef(out_normal_image, (int2)(px, py), (float4)(0.5f, 0.5f, 1.0f, 1.0f));
            write_imagef(out_velocity_image, (int2)(px, py), (float4)(0.0f, 0.0f, 0.0f, 0.0f));
            return;
        }

        // Camera is outside chamber: jump directly to the chamber boundary
        if (t_box_enter > 0.05f) {
            t = t_box_enter;
        }
        march_max_dist = fmin(max_dist, t_box_exit);
    }

    // Multi-spectral volumetric in-scattering parameters
    float3 transmittance = (float3)(1.0f, 1.0f, 1.0f);
    float3 inscattering = (float3)(0.0f, 0.0f, 0.0f);
    const float3 light_dir = normalize(key_light_dir.xyz);
    // Dual Phase function blending Rayleigh & Henyey-Greenstein Mie aerosol
    const float g = params->optical_volumetric.y;
    const float3 base_scattering = (float3)(0.75f, 0.85f, 0.95f);
    const float3 base_absorption = (float3)(0.12f, 0.08f, 0.05f);

    float3 p = cam_pos + ray_dir * t;
    float3 current_ray_dir = ray_dir;
    float step = 0.05f;

    for (uint i = 0u; i < max_steps; ++i) {
        steps_taken++;

        const float radius = length(p);
        closest_radius = fmin(closest_radius, radius);
        if (geodesic_enabled && radius <= horizon_radius + 0.006f) {
            captured_by_horizon = true;
            hit_dist = t;
            break;
        }

        float d = 0.0f;
        if (params->enable4D > 0u || params->reduction_id != 3u) {
            d = epi_sdf_epimorphic_chamber(p, params, frame->time, frame->morph, &orbit_energy);
        } else {
            d = epi_sdf_pareidolia_cathedral(p, params, frame, &orbit_energy);
        }

        if (d < hit_epsilon) {
            hit = true;
            hit_dist = t;
            break;
        }

        step = fmin(fmax(d * 0.78f, 0.012f), march_max_dist - t);
        if (geodesic_enabled) {
            step = epi_geodesic_safe_step(
                step, p, current_ray_dir, geometric_mass);
        }

        // Atmospheric extinction is integrated over the exact same curved
        // affine segment as the photon path. Its phase is recomputed after
        // each bend, so forward scattering follows the lensed ray naturally.
        const float fog_density = 0.020f * params->optical_volumetric.x * (1.0f + 0.55f * orbit_energy);
        const float3 sigma_s = base_scattering * fog_density;
        const float3 sigma_a = base_absorption * fog_density;
        const float3 sigma_t = sigma_s + sigma_a;
        const float phase = epi_dual_phase(
            dot(current_ray_dir, light_dir), g, 0.75f);
        const float3 step_transmittance = epi_spectral_transmittance(
            sigma_t, step);
        inscattering += key_light_radiance.xyz * sigma_s * step * phase
            * transmittance;
        transmittance *= step_transmittance;

        if (geodesic_enabled) {
            accumulated_bend += epi_integrate_null_geodesic_midpoint(
                &p, &current_ray_dir, step, geometric_mass);
        } else {
            p += current_ray_dir * step;
        }
        t += step;
        if (t >= march_max_dist) break;
    }

    // ─── Surface Shading & Material Coloration ──────────────────────────────
    // Sampling the background with the final geodesic direction makes stars
    // and the galactic ribbon shear into genuine lensing arcs.
    float3 final_color = epi_lensed_deep_space(current_ray_dir, frame->time);

    float3 nrm = (float3)(0.0f, 0.0f, 1.0f);
    if (hit) {
        float3 hit_pos = p;
        nrm = epi_sdf_normal_tetrahedral(frame, hit_pos, params, frame->time, frame->morph, hit_epsilon * 1.5f);

        // Diffuse & Specular Lighting
        float NdotL = fmax(dot(nrm, light_dir), 0.0f);
        float3 half_vec = normalize(light_dir - current_ray_dir);
        float NdotH = fmax(dot(nrm, half_vec), 0.0f);
        float specular = pow(NdotH, 32.0f) * 0.65f;

        // Material Color from Epimorphic Orbit & Color Effect
        float materialCoord = epi_positive_fmod(orbit_energy * 0.75f + params->pulse_material.z + frame->time * 0.05f, 1.0f);
        float3 albedo = epi_palette_evaluate(
            (uint)params->color_effect_id,
            materialCoord,
            orbit_energy,
            length(hit_pos),
            frame->time,
            params->dynamics_seed.x,
            params->dynamics_seed.y,
            params->pulse_material.z
        );

        // Thin-film wave interference physical reflection modulation (Airy formulation)
        if (params->color_effect_id == 9u) {
            float cosThetaI = fmax(dot(-current_ray_dir, nrm), 0.0f);
            float filmThicknessNm = params->optical_volumetric.z + 250.0f * materialCoord;
            float3 thinFilmRefl = epi_airy_thin_film_reflectance(cosThetaI, filmThicknessNm, 1.38f, params->optical_volumetric.w);
            albedo = mix(albedo, thinFilmRefl, 0.65f);
            specular *= (1.0f + (thinFilmRefl.x + thinFilmRefl.y + thinFilmRefl.z) * 0.5f);
        }

        // Chromatic dispersion Cauchy refraction highlight (mode 10)
        if (params->color_effect_id == 10u) {
            float cosThetaI = fmax(dot(-current_ray_dir, nrm), 0.0f);
            float3 dispRefract = epi_cauchy_dispersion_refract(current_ray_dir, nrm, 1.50f, 0.0045f);
            float dispersionFresnel = pow(1.0f - cosThetaI, 3.0f);
            albedo = mix(albedo, dispRefract * 0.5f + 0.5f, dispersionFresnel * 0.5f);
        }

        // Bioluminescence self-emission glow (mode 15)
        float3 emission = (float3)(0.0f);
        if (params->color_effect_id == 15u) {
            float pulse = 0.5f + 0.5f * sin(frame->time * 4.0f + materialCoord * 10.0f);
            emission = albedo * pow(pulse, 2.5f) * 1.8f;
        }

        // Ambient Occlusion estimate from step count
        float ao = clamp(1.0f - convert_float(steps_taken) / convert_float(max_steps) * 1.2f, 0.15f, 1.0f);

        final_color = albedo * (0.15f + NdotL * key_light_radiance.xyz) * ao + (float3)(specular) * key_light_radiance.xyz + emission;
    }

    if (geodesic_enabled) {
        const float ring_width = fmax(photon_sphere_radius * 0.16f, 0.045f);
        const float photon_ring = exp(
            -fabs(closest_radius - photon_sphere_radius) / ring_width)
            * clamp(accumulated_bend * 2.4f, 0.0f, 1.0f);
        const float doppler_side = 0.5f + 0.5f * dot(
            normalize((float3)(-current_ray_dir.z, 0.0f, current_ray_dir.x)),
            (float3)(1.0f, 0.0f, 0.0f));
        const float3 ring_color = mix(
            (float3)(0.20f, 0.55f, 1.35f),
            (float3)(1.80f, 0.48f, 0.08f), doppler_side);
        final_color += ring_color * photon_ring * 2.2f;
    }

    // The horizon absorbs surface/background radiance. A small amount of
    // foreground participating-media scattering remains physically visible.
    final_color = captured_by_horizon
        ? inscattering * 0.08f
        : final_color * transmittance + inscattering;

    // Vulkan's HDR/post chain owns display tone mapping. Keeping this buffer
    // scene-linear avoids compressing highlights twice during live rendering.
    
    // -------------------------------------------------------------------------
    // Educational Annotation: 3D Learn Mode Visualizer
    // -------------------------------------------------------------------------
    // Overrides the final linear color with pedagogical diagnostic layers.
    uint diagnostic_stage = (params->stageMask >> 8) & 0x0Fu;
    if (diagnostic_stage != 0u) {
        if (diagnostic_stage == 1u) {
            // Stage 1: Transform (Iteration Heatmap)
            // Displays SDF step count. Brightness maps to sphere-tracing effort,
            // teaching the user how non-linear folded geometry impacts raymarch efficiency.
            float heatmap = convert_float(steps_taken) / convert_float(max_steps);
            final_color = (float3)(heatmap, heatmap * 0.4f, heatmap * 0.1f);
        } else if (diagnostic_stage == 2u) {
            // Stage 2: Reduce (Raw Surface Normals)
            // Visualizes the tetrahedral SDF gradient, removing all lighting/materials.
            // Teaches how the folded domain affects local surface curvature.
            final_color = hit ? (nrm * 0.5f + 0.5f) : (float3)(0.0f);
        } else if (diagnostic_stage == 3u) {
            // Stage 3: SeedWindow (Volumetric In-Scattering Only)
            // Isolates the participating media (fog) integration, hiding the hard surfaces.
            final_color = inscattering;
        } else if (diagnostic_stage == 4u) {
            // Stage 4: MaterialCoordinate (Raw Albedo Palette)
            // Shows the unlit, unshadowed base color derived from the orbit energy.
            float materialCoord = epi_positive_fmod(orbit_energy * 0.75f + params->pulse_material.z + frame->time * 0.05f, 1.0f);
            final_color = hit ? epi_palette_evaluate(
                (uint)params->color_effect_id, materialCoord, orbit_energy,
                length(p), frame->time, params->dynamics_seed.x,
                params->dynamics_seed.y, params->pulse_material.z
            ) : (float3)(0.0f);
        } else if (diagnostic_stage == 5u) {
            // Stage 5: OrbitEnergy (Orbit Energy Field)
            // Maps the raw kinetic/orbit energy escaped from the fractal directly to grayscale.
            float energy_norm = clamp(orbit_energy * 0.5f, 0.0f, 1.0f);
            final_color = (float3)(energy_norm);
        } else if (diagnostic_stage == 6u) {
            // Stage 6: OrbitTrap (Ambient Occlusion Only)
            // Shows the geometry self-shadowing approximation derived from the SDF ray bounds.
            float ao = clamp(1.0f - convert_float(steps_taken) / convert_float(max_steps) * 1.2f, 0.15f, 1.0f);
            final_color = hit ? (float3)(ao) : (float3)(0.0f);
        }
    }

    final_color = fmax(final_color, (float3)(0.0f));
    
    write_imagef(out_color_image, (int2)(px, py), 
        (float4)(final_color.x, final_color.y, final_color.z, 1.0f));
        
    // Images cannot be conditionally written via pointers the same way, but 
    // the producer always passes them now.
    write_imagef(out_depth_image, (int2)(px, py), (float4)(hit_dist, 0.0f, 0.0f, 0.0f));
    
    // Auxiliaries are opt-in. The live fullscreen compositor currently only
    // samples color, so null buffer arguments suppress both writes and the
    // otherwise unnecessary normal packing work.
    float3 out_normal = hit ? (nrm * 0.5f + 0.5f)
                            : (float3)(0.5f, 0.5f, 1.0f);
    write_imagef(out_normal_image, (int2)(px, py), (float4)(out_normal.x, out_normal.y, out_normal.z, 1.0f));
    
    // Placeholder for true re-projection velocity (would require previous frame cam_pos)
    write_imagef(out_velocity_image, (int2)(px, py), (float4)(0.0f, 0.0f, 0.0f, 0.0f));
}
