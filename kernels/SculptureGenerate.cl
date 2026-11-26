#include "SculptureGpuAbi.clh"
#include "EpiSampleHistory.clh"
#include "EpiLearnMode.clh"
#include "EpiMath.clh"
#include "EpiColorComponent.clh"
#include "EpiTopology.clh"
#include "EpiFractal.clh"
#include "EpiVectorFields.clh"
#include "EpiAtomic.clh"
#include "EpiMorphogen.clh"
#include "EpiFluid.clh"

// OpenCL Main Entry points

__kernel void mark_macro_blocks(
    __constant const SculptureProgramParamsGpu* params,
    __constant const SculptureFrameGpu* frame,
    __global uchar* active_blocks,
    uint samples_offset, // Retained ABI argument; private history starts at zero.
    __global const uchar* prev_output_buffer_raw,
    __global float4* spatial_buffer,
    __global const float4* prev_spatial_buffer,
    __read_only image3d_t prev_spatial_image) {
    (void)samples_offset;
    const uint index = get_global_id(0);
    if (index >= EPI_SCULPT_VOLUME_VOXEL_COUNT) return;

    if (index < EPI_SCULPT_VOLUME_MACRO_BLOCK_COUNT) {
        const uint b = index;
        const uint bx = b % 5u;
        const uint by = (b / 5u) % 5u;
        const uint bz = b / 25u;
        const uint center_index = (bx * 5u + 2u) + (by * 5u + 2u) * 25u + (bz * 5u + 2u) * 625u;
        
        const float u = convert_float(center_index) / convert_float(16384u);
        const float3 seed = epi_seed_point(center_index, 16384u, frame->time, params);
        
        float3 prev_feedback = seed;
        SculptureSourceSample previous;
        if (params->output_scale.z > 0.0f
            && params->output_scale.w == 0.0f
            && epi_source_history_read(prev_output_buffer_raw, center_index, &previous)) {
            prev_feedback = previous.position_valid.xyz;
        }
        
        float orbit_energy;
        float final_seed_weight;
        float orbit_trap;
        float3 feedback = epi_evaluate_fractal_loop(
            seed, u, params, frame, prev_feedback,
            &orbit_energy, &final_seed_weight, &orbit_trap);
            
        const float shaped_morph = epi_smoothstep(0.0f, 1.0f, clamp(frame->morph, 0.0f, 1.0f));
        const float3 domain = mix(seed, feedback, shaped_morph);
        const float magnitude = length(domain);
        
        if (fabs(magnitude - 1.0f) < 0.9f) {
            active_blocks[b] = 1;
        }
    }
    
    // 2. Fluid Diffusion via Trilinear Advection
    const float fx_idx = convert_float(index % EPI_SCULPT_VOLUME_GRID_SIZE);
    const float fy_idx = convert_float((index / EPI_SCULPT_VOLUME_GRID_SIZE) % EPI_SCULPT_VOLUME_GRID_SIZE);
    const float fz_idx = convert_float(index / (EPI_SCULPT_VOLUME_GRID_SIZE * EPI_SCULPT_VOLUME_GRID_SIZE));
    // Modularized: Trilinear Advection & Biological Morphogenesis
    float4 current_fluid = epi_evaluate_fluid_diffusion(
        index, fx_idx, fy_idx, fz_idx, params, frame, prev_spatial_buffer, prev_spatial_image);
    

    // Modularized: Volumetric Fractal Epimorphism Injection
    current_fluid = epi_evaluate_morphogen_injection(
        fx_idx, fy_idx, fz_idx, current_fluid, params, frame);
    // (We no longer overwrite xyz with analytic velocity, they are now biological morphogens!)
    
    if (spatial_buffer) spatial_buffer[index] = current_fluid;
    
    if (current_fluid.w > 0.05f) {
        const uint b = (convert_uint(fx_idx) / 10u) + (convert_uint(fy_idx) / 10u) * 5u + (convert_uint(fz_idx) / 10u) * 25u;
        active_blocks[b] = 1;
    }
}

__kernel void generate_sculpture(
    __constant const SculptureProgramParamsGpu* params,
    __constant const SculptureFrameGpu* frame,
    volatile __global SculptureAnalyticsGpu* analytics,
    __global uchar* output_buffer_raw,
    __global const uchar* active_blocks,
    uint samples_offset, // Current delivery layout only; history starts at zero.
    __global const uchar* prev_output_buffer_raw,
    __global float4* spatial_buffer,
    __global SculptureSourceSample* next_source_history) {

    __global SculptureSampleGpu* samples = (__global SculptureSampleGpu*)(output_buffer_raw + samples_offset);

    const uint index = get_global_id(0);
    uint scale = params->auxiliary_resolution_scale;
    if (scale == 0) scale = 1u;
    
    uint full_index = index;
    if (scale > 1u) {
        if (params->output_mode == 1u && params->output_modeB == 1u) {
            uint dim = 50u / scale;
            uint x = index % dim;
            uint y = (index / dim) % dim;
            uint z = index / (dim * dim);
            full_index = (x * scale) + (y * scale) * 50u + (z * scale) * 2500u;
        } else if (params->output_mode == 2u) {
            uint dim = 128u / scale;
            uint u_idx = index % dim;
            uint v_idx = index / dim;
            full_index = (u_idx * scale) + (v_idx * scale) * 128u;
        } else {
            full_index = index * (scale * scale);
        }
    }

    const uint sample_count = min(params->sample_count, frame->sample_count);
    const uint eval_count = (params->output_mode == 1u && params->output_modeB == 1u) ? EPI_SCULPT_VOLUME_VOXEL_COUNT : sample_count;
    const uint scaled_eval_count = (params->output_mode == 1u && params->output_modeB == 1u) ? (eval_count / (scale * scale * scale)) : (eval_count / (scale * scale));
    if (index >= scaled_eval_count
        || !epi_sculpt_program_abi_valid(params)
        || !epi_sculpt_frame_abi_valid(frame)) {
        return;
    }

    const float u = convert_float(full_index) / convert_float(sample_count);
    
    if ((params->output_mode == 1u && params->output_modeB == 1u) && full_index < EPI_SCULPT_VOLUME_VOXEL_COUNT) {
        const uint bx = (full_index % EPI_SCULPT_VOLUME_GRID_SIZE) / 10u;
        const uint by = ((full_index / EPI_SCULPT_VOLUME_GRID_SIZE) % EPI_SCULPT_VOLUME_GRID_SIZE) / 10u;
        const uint bz = (full_index / (EPI_SCULPT_VOLUME_GRID_SIZE * EPI_SCULPT_VOLUME_GRID_SIZE)) / 10u;
        const uint b = bx + by * 5u + bz * 25u;
        if (active_blocks[b] == 0) return;
    }
    // =========================================================================
    // STAGE 5: SEED (Base Coordinate Generation)
    // =========================================================================
    float3 seed = epi_seed_point(full_index, sample_count, frame->time, params);
    
    // =========================================================================
    // STAGE 3: SAMPLE (History Feedback Fetch)
    // =========================================================================
    float3 prev_feedback = seed;
    SculptureSourceSample previous;
    const int previous_valid = params->output_scale.w == 0.0f &&
        epi_source_history_read(prev_output_buffer_raw, full_index, &previous);
    if (previous_valid && params->output_scale.z > 0.0f) {
        prev_feedback = previous.position_valid.xyz;
    }
    
    // =========================================================================
    // STAGE 1 & 2 & 5 (Mask): TRANSFORM, REDUCE, and SEED WINDOW
    // (Delegated to the canonical fractal loop engine)
    // =========================================================================
    float orbit_energy;
    float final_seed_weight;
    float orbit_trap;
    float3 feedback = epi_evaluate_fractal_loop(
        seed, u, params, frame, prev_feedback, &orbit_energy, &final_seed_weight, &orbit_trap);

    // =========================================================================
    // STAGE 6: BLEND (Morphological Crossfade)
    // =========================================================================
    const float shaped_morph = epi_smoothstep(
        0.0f, 1.0f, clamp(frame->morph, 0.0f, 1.0f));
    const float3 domain = mix(seed, feedback, shaped_morph);
    const float magnitude = length(domain);
    
    // =========================================================================
    // STAGE 4: COLOR (Material Coordinate Derivation)
    // =========================================================================
    const float angle = atan2(domain.z, domain.x)
                      / (2.0f * EPI_SCULPT_PI);
    const float material_coordinate_value =
        angle + params->pulse_material.z
        + params->pulse_material.w * frame->time
        + 0.12f * log1p(magnitude) + 0.08f * orbit_energy;
    float material_coordinate = material_coordinate_value
                              - floor(material_coordinate_value);
    if (!epi_stage_enabled(params->stageMask, EPI_STAGE_COLOR)) material_coordinate = 0.0f;

    float emit_scale = 1.0f;
    uint evalStage = epi_diagnostic_stage(params->stageMask);
    if (evalStage == EPI_DIAGNOSTIC_FULL && (params->output_mode == 1u && params->output_modeB == 1u)) {
        if (fabs(magnitude - 1.0f) > 0.12f) {
            emit_scale = 0.0f;
        }
        if (full_index >= EPI_SCULPT_VOLUME_VOXEL_COUNT) {
            emit_scale = 0.0f;
        }
    }

    if (emit_scale > 0.0f) {
        const float world_scale = clamp(params->output_scale.x, 0.25f, 2.0f) * emit_scale;
        const float radius_scale = clamp(params->output_scale.y, 0.25f, 2.0f) * emit_scale;

        float3 N = (float3)(0.0f, 1.0f, 0.0f);
        if ((params->output_mode == 1u && params->output_modeB == 1u)) {
            const float eps = 0.005f;
            float dummy1, dummy2, dummy3;
            float3 fb_px = epi_evaluate_fractal_loop(seed + (float3)(eps, 0.0f, 0.0f), u, params, frame, seed + (float3)(eps, 0.0f, 0.0f), &dummy1, &dummy2, &dummy3);
            float3 fb_mx = epi_evaluate_fractal_loop(seed - (float3)(eps, 0.0f, 0.0f), u, params, frame, seed - (float3)(eps, 0.0f, 0.0f), &dummy1, &dummy2, &dummy3);
            float3 fb_py = epi_evaluate_fractal_loop(seed + (float3)(0.0f, eps, 0.0f), u, params, frame, seed + (float3)(0.0f, eps, 0.0f), &dummy1, &dummy2, &dummy3);
            float3 fb_my = epi_evaluate_fractal_loop(seed - (float3)(0.0f, eps, 0.0f), u, params, frame, seed - (float3)(0.0f, eps, 0.0f), &dummy1, &dummy2, &dummy3);
            float3 fb_pz = epi_evaluate_fractal_loop(seed + (float3)(0.0f, 0.0f, eps), u, params, frame, seed + (float3)(0.0f, 0.0f, eps), &dummy1, &dummy2, &dummy3);
            float3 fb_mz = epi_evaluate_fractal_loop(seed - (float3)(0.0f, 0.0f, eps), u, params, frame, seed - (float3)(0.0f, 0.0f, eps), &dummy1, &dummy2, &dummy3);
            float v_px = length(mix(seed + (float3)(eps, 0.0f, 0.0f), fb_px, shaped_morph)) - 1.0f;
            float v_mx = length(mix(seed - (float3)(eps, 0.0f, 0.0f), fb_mx, shaped_morph)) - 1.0f;
            float v_py = length(mix(seed + (float3)(0.0f, eps, 0.0f), fb_py, shaped_morph)) - 1.0f;
            float v_my = length(mix(seed - (float3)(0.0f, eps, 0.0f), fb_my, shaped_morph)) - 1.0f;
            float v_pz = length(mix(seed + (float3)(0.0f, 0.0f, eps), fb_pz, shaped_morph)) - 1.0f;
            float v_mz = length(mix(seed - (float3)(0.0f, 0.0f, eps), fb_mz, shaped_morph)) - 1.0f;
            
            float3 grad = (float3)(v_px - v_mx, v_py - v_my, v_pz - v_mz);
            if (length(grad) > 0.0f) {
                N = normalize(grad);
            }
        }

        float fluid = 0.0f;
        float morph_u = 0.0f;
        float morph_v = 0.0f;
        if (spatial_buffer && full_index < EPI_SCULPT_VOLUME_VOXEL_COUNT) {
            // Morphogen injection is now cleanly decoupled into mark_macro_blocks.
            // STAGE 6 (BLEND) here simply samples the resulting fluid field.
            float4 read_fluid = spatial_buffer[full_index];
            fluid = read_fluid.w;
            morph_u = read_fluid.x;
            morph_v = read_fluid.y;
        }

        float trap_density = clamp(1.0f - orbit_trap * 1.5f, 0.0f, 1.0f);

        // RK2 Flow-Field Kinetic Advection
        float3 flow_vel = (float3)(0.0f);
        float3 advected_domain = domain;
        float advect_speed = params->kinetic_flow.x * (1.0f + params->audio_spectrum.x * 3.0f);
        float curl_strength = params->kinetic_flow.y * (1.0f + params->audio_spectrum.z * 2.0f);
        
        if (advect_speed > 0.0f || curl_strength > 0.0f) {
            float3 curl_v = epi_curl_field(domain);
            float3 fb_disp = feedback - seed;
            flow_vel = curl_v * curl_strength + fb_disp * advect_speed;
            
            const float dt = 0.05f;
            float3 mid_p = domain + flow_vel * (0.5f * dt);
            float3 mid_v = epi_curl_field(mid_p) * curl_strength + (feedback - seed) * advect_speed;
            advected_domain = domain + mid_v * dt;
            flow_vel = mid_v;
        }

    // =========================================================================
    // POST-PROCESSING: Payload Construction & Velocity Advection
    // =========================================================================
        SculptureSampleGpu sample;
        float optical_expansion = (scale > 1u) ? sqrt(convert_float(scale)) : 1.0f;
        float audio_radius_boost = params->audio_spectrum.w * 1.5f;
        sample.position_radius = (float4)(
            world_scale * 2.45f * advected_domain.x,
            world_scale * 2.15f * advected_domain.y,
            world_scale * 2.45f * advected_domain.z,
            radius_scale * optical_expansion * clamp(
                0.58f + 0.34f * final_seed_weight + 0.18f * orbit_energy + fluid * 0.45f + trap_density * 0.22f + audio_radius_boost,
                0.48f, 3.5f));


        sample.normal_vector = (float4)(N.x, N.y, N.z, 0.0f);
        
        float4 vel = (float4)(0.0f);
        float dt = (params->temporal_control.y > 0.0f) ? params->temporal_control.y : frame->delta_seconds;
        if (dt > 1e-5f && previous_valid) {
            if (length(flow_vel) > 0.0f) {
                float visc = clamp(params->kinetic_flow.w, 0.0f, 1.0f);
                float decay = params->kinetic_flow.z;
                vel.xyz = (1.0f - visc) * flow_vel +
                    visc * previous.velocity.xyz * decay;
            } else {
                vel.xyz = (sample.position_radius.xyz - previous.position_valid.xyz) / dt;
            }
        } else if (length(flow_vel) > 0.0f) {
            vel.xyz = flow_vel;
        }
        sample.velocity = vel;
        sample.materialRole = (length(vel.xyz) > 0.5f) ? 2u : 1u; // Simple kinetic role assignment
        sample.reserved0 = 0u;
        sample.reserved1 = 0u;
        sample.reserved2 = 0u;
            
        // Store by source identity before atomic compaction, including candidates
        // that exceed the compact publication capacity.
        epi_source_history_store(next_source_history, full_index,
                                 sample.position_radius.xyz, sample.velocity.xyz);
        uint write_index = atomic_inc(&analytics->activeInstanceCount);
        if (write_index < sample_count) {
            samples[write_index] = sample;
            
            float speed = length(sample.velocity.xyz);
            
            if (write_index < 100) {
                atomic_add_f_global(&analytics->totalOrbit, sample.position_radius.w);
                atomic_add_f_global(&analytics->totalKineticEnergy, speed);
            }
            
            if (write_index % 64 == 0) {
                atomic_min_f_global(&analytics->boundsMin[0], sample.position_radius.x);
                atomic_min_f_global(&analytics->boundsMin[1], sample.position_radius.y);
                atomic_min_f_global(&analytics->boundsMin[2], sample.position_radius.z);
                
                atomic_max_f_global(&analytics->boundsMax[0], sample.position_radius.x);
                atomic_max_f_global(&analytics->boundsMax[1], sample.position_radius.y);
                atomic_max_f_global(&analytics->boundsMax[2], sample.position_radius.z);
                
                atomic_add_f_global(&analytics->averageSpeed, speed);
            }
            
            // Diagnostic stages expose intermediate mathematical fields directly to the color channels.
            if (evalStage == EPI_DIAGNOSTIC_TRANSFORM) {
                float3 tr_rgb = clamp((advected_domain + (float3)(1.0f)) * 0.5f, 0.0f, 1.0f);
                samples[write_index].color_tint = (float4)(tr_rgb.x, tr_rgb.y, tr_rgb.z, 1.0f);
            } else if (evalStage == EPI_DIAGNOSTIC_REDUCE) {
                float3 red_rgb = clamp((feedback + (float3)(1.0f)) * 0.5f, 0.0f, 1.0f);
                samples[write_index].color_tint = (float4)(red_rgb.x, red_rgb.y, red_rgb.z, 1.0f);
            } else if (evalStage == EPI_DIAGNOSTIC_SEED_WINDOW) {
                float seed_val = clamp(final_seed_weight, 0.0f, 1.0f);
                samples[write_index].color_tint = (float4)(seed_val, seed_val, seed_val, 1.0f);
            } else if (evalStage == EPI_DIAGNOSTIC_MATERIAL) {
                samples[write_index].color_tint = (float4)(material_coordinate, material_coordinate, material_coordinate, 1.0f);
            } else if (evalStage == EPI_DIAGNOSTIC_ORBIT_ENERGY) {
                float norm_energy = clamp(orbit_energy * 0.5f, 0.0f, 1.0f);
                samples[write_index].color_tint = (float4)(norm_energy, norm_energy, norm_energy, 1.0f);
            } else if (evalStage == EPI_DIAGNOSTIC_ORBIT_TRAP) {
                samples[write_index].color_tint = (float4)(trap_density, trap_density, trap_density, 1.0f);
            } else if (epi_stage_enabled(params->stageMask, EPI_STAGE_COLOR)) {
                if (params->color_effect_id == 13U && length(flow_vel) > 0.0f) { // KineticVelocity
                    // Map fluid velocity vector direction to RGB color (XYZ -> RGB)
                    // and velocity magnitude to luminosity.
                    float3 vel_n = (length(flow_vel) > 1e-5f) ? normalize(flow_vel) : (float3)(0.0f, 1.0f, 0.0f);
                    float3 rgb = vel_n * 0.5f + 0.5f;
                    float v_mag = clamp(length(flow_vel) * 2.0f, 0.0f, 1.0f);
                    rgb = mix((float3)(0.1f), rgb, v_mag);
                    samples[write_index].color_tint = (float4)(rgb.x, rgb.y, rgb.z, 1.0f);
                } else if (params->color_effect_id != 0U) {
                    float3 rgb = epi_compact_palette_evaluate(
                        params->color_effect_id,
                        material_coordinate,
                        orbit_energy,
                        magnitude,
                        frame->time,
                        params->chroma_breath,
                        params->luma_drift,
                        params->pulse_material.z
                    );
                    samples[write_index].color_tint = (float4)(rgb.x, rgb.y, rgb.z, material_coordinate);
                } else {
                    samples[write_index].color_tint = (float4)(material_coordinate, orbit_energy, magnitude, u);
                }
            } else {
                samples[write_index].color_tint = (float4)(material_coordinate, orbit_energy, magnitude, u);
            }
            samples[write_index].normal_vector.w = final_seed_weight;
            float flow_speed = length(vel.xyz);
            samples[write_index].velocity.w = (final_seed_weight > 0.8f) ? 2.0f : flow_speed;
        }
    }
}
