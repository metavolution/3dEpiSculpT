#include "SculptureSpatialGestureEngine.h"
#include <algorithm>

namespace epi_sculpt {

SculptureVec3 Quaternion4D::rotate(const SculptureVec3& v) const noexcept {
    // q * v * q^*
    // v_rot = v + 2 * cross(q.xyz, cross(q.xyz, v) + q.w * v)
    const float qx = x, qy = y, qz = z, qw = w;
    const float tx = 2.0f * (qy * v.z - qz * v.y);
    const float ty = 2.0f * (qz * v.x - qx * v.z);
    const float tz = 2.0f * (qx * v.y - qy * v.x);

    return SculptureVec3{
        v.x + qw * tx + (qy * tz - qz * ty),
        v.y + qw * ty + (qz * tx - qx * tz),
        v.z + qw * tz + (qx * ty - qy * tx)
    };
}

Quaternion4D Quaternion4D::slerp(const Quaternion4D& q1, const Quaternion4D& q2, float t) noexcept {
    const float u = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
    float dot = q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;

    Quaternion4D q2Target = q2;
    if (dot < 0.0f) {
        dot = -dot;
        q2Target = {-q2.w, -q2.x, -q2.y, -q2.z};
    }

    if (dot > 0.9995f) {
        // NLERP for nearly identical orientations
        const float rw = q1.w + u * (q2Target.w - q1.w);
        const float rx = q1.x + u * (q2Target.x - q1.x);
        const float ry = q1.y + u * (q2Target.y - q1.y);
        const float rz = q1.z + u * (q2Target.z - q1.z);
        const float len = std::sqrt(rw * rw + rx * rx + ry * ry + rz * rz);
        if (len > 1e-6f) {
            return {rw / len, rx / len, ry / len, rz / len};
        }
        return q1;
    }

    const float theta0 = std::acos(std::clamp(dot, -1.0f, 1.0f));
    const float theta = theta0 * u;
    const float sinTheta = std::sin(theta);
    const float sinTheta0 = std::sin(theta0);

    const float s0 = std::cos(theta) - dot * sinTheta / sinTheta0;
    const float s1 = sinTheta / sinTheta0;

    return {
        s0 * q1.w + s1 * q2Target.w,
        s0 * q1.x + s1 * q2Target.x,
        s0 * q1.y + s1 * q2Target.y,
        s0 * q1.z + s1 * q2Target.z
    };
}

void SculptureSpatialGestureEngine::processGestures(
    SculptureProgramParams& params,
    std::vector<float>& spatialBuffer,
    float deltaSeconds
) {
    gestureScaleDelta_ = 0.0f;
    gestureRotationDelta_ = 0.0f;
    gestureVortexIntensity_ = 0.0f;

    // 0. Update 2nd-order critically damped spatial tracker filter
    filteredHands_ = gestureFilter_.update(leftHand_, rightHand_, deltaSeconds);
    if (filteredHands_.leftHand.valid) {
        leftHand_ = filteredHands_.leftHand;
    }
    if (filteredHands_.rightHand.valid) {
        rightHand_ = filteredHands_.rightHand;
    }

    const bool leftActive = leftHand_.valid && (leftHand_.grip > 0.3f || leftHand_.trigger > 0.3f);
    const bool rightActive = rightHand_.valid && (rightHand_.grip > 0.3f || rightHand_.trigger > 0.3f);

    if (leftActive && rightActive) {
        // ---------------------------------------------------------------------
        // Two-Handed Conformal Metric Pinch / Rotate
        // ---------------------------------------------------------------------
        activeGesture_ = GestureType::ConformalPinchRotate;

        const float dx = rightHand_.position.x - leftHand_.position.x;
        const float dy = rightHand_.position.y - leftHand_.position.y;
        const float dz = rightHand_.position.z - leftHand_.position.z;
        const float currentDist = std::max(std::sqrt(dx * dx + dy * dy + dz * dz), 0.01f);
        const float currentAngle = std::atan2(dy, dx);

        if (!gestureAnchorActive_) {
            initialHandDistance_ = currentDist;
            initialHandAngle_ = currentAngle;
            gestureAnchorActive_ = true;
        } else {
            // Distance ratio modulates scale
            const float distRatio = currentDist / std::max(initialHandDistance_, 0.01f);
            gestureScaleDelta_ = distRatio - 1.0f;
            params.worldScale = std::clamp(params.worldScale * (1.0f + gestureScaleDelta_ * deltaSeconds * 2.0f), 0.1f, 10.0f);

            // Angle difference modulates complex affine phase rotation
            float angleDelta = currentAngle - initialHandAngle_;
            while (angleDelta > 3.14159265f) angleDelta -= 6.2831853f;
            while (angleDelta < -3.14159265f) angleDelta += 6.2831853f;
            gestureRotationDelta_ = angleDelta;

            // Rotate pre-affine transform
            const float r = std::sqrt(params.pre.scaleReal * params.pre.scaleReal + params.pre.scaleImag * params.pre.scaleImag);
            const float theta = std::atan2(params.pre.scaleImag, params.pre.scaleReal) + angleDelta * deltaSeconds;
            params.pre.scaleReal = r * std::cos(theta);
            params.pre.scaleImag = r * std::sin(theta);
        }
    } else if (rightActive && rightHand_.trigger > 0.5f) {
        // ---------------------------------------------------------------------
        // Right Hand Localized Vortex Injection into Fluid Buffer
        // ---------------------------------------------------------------------
        activeGesture_ = GestureType::VortexInject;
        gestureAnchorActive_ = false;

        if (!spatialBuffer.empty()) {
            const double countD = static_cast<double>(spatialBuffer.size());
            const std::uint32_t N = static_cast<std::uint32_t>(std::round(std::cbrt(countD)));
            if (N >= 2U) {
                // Map hand position to normalized voxel coordinates [0, N-1]
                const float handNormX = std::clamp((rightHand_.position.x + 1.0f) * 0.5f, 0.0f, 1.0f) * (N - 1);
                const float handNormY = std::clamp((rightHand_.position.y + 1.0f) * 0.5f, 0.0f, 1.0f) * (N - 1);
                const float handNormZ = std::clamp((rightHand_.position.z + 1.0f) * 0.5f, 0.0f, 1.0f) * (N - 1);

                const int cx = static_cast<int>(handNormX);
                const int cy = static_cast<int>(handNormY);
                const int cz = static_cast<int>(handNormZ);
                const int radius = std::max(2, static_cast<int>(N / 8));

                gestureVortexIntensity_ = rightHand_.trigger;

                for (int z = std::max(0, cz - radius); z <= std::min(static_cast<int>(N - 1), cz + radius); ++z) {
                    for (int y = std::max(0, cy - radius); y <= std::min(static_cast<int>(N - 1), cy + radius); ++y) {
                        for (int x = std::max(0, cx - radius); x <= std::min(static_cast<int>(N - 1), cx + radius); ++x) {
                            const float r2 = static_cast<float>((x - cx)*(x - cx) + (y - cy)*(y - cy) + (z - cz)*(z - cz));
                            if (r2 <= static_cast<float>(radius * radius)) {
                                const std::size_t idx = static_cast<std::size_t>(x + y * N + z * N * N);
                                if (idx < spatialBuffer.size()) {
                                    const float strength = (1.0f - std::sqrt(r2) / static_cast<float>(radius)) * rightHand_.trigger * 0.8f;
                                    spatialBuffer[idx] = std::clamp(spatialBuffer[idx] + strength, 0.0f, 1.0f);
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (leftActive && leftHand_.grip > 0.5f) {
        // ---------------------------------------------------------------------
        // Left Hand Topology Sculpt / Foliation Modulation
        // ---------------------------------------------------------------------
        activeGesture_ = GestureType::TopologySculpt;
        gestureAnchorActive_ = false;

        // Modulate 4D latent coordinate and topology blend from hand height & tilt
        params.latentW = std::clamp(params.latentW + leftHand_.position.y * deltaSeconds * 0.5f, 0.0f, 1.0f);
        params.topologyBlend = std::clamp(leftHand_.trigger, 0.0f, 1.0f);
    } else {
        activeGesture_ = GestureType::Idle;
        gestureAnchorActive_ = false;
    }

    // -------------------------------------------------------------------------
    // Biological Wavelet Extraction (Human-Environment Morphogenic Coupling)
    // -------------------------------------------------------------------------
    // The VR headset naturally bobs with the user's heartbeat and respiration.
    // We extract this vertical delta and use it as an oscillator phase driver.
    if (headPose_.valid && prevHeadPose_.valid) {
        float headDeltaY = headPose_.position.y - prevHeadPose_.position.y;
        
        // Accumulate and low-pass filter the micro-movements (simulating respiration drive)
        accumulatedHeadDelta_ = accumulatedHeadDelta_ * 0.95f + headDeltaY * 0.05f;
        
        // Drive a biological phase oscillator
        respirationPhase_ += (0.2f + std::abs(accumulatedHeadDelta_) * 100.0f) * deltaSeconds;
        if (respirationPhase_ > 6.2831853f) respirationPhase_ -= 6.2831853f;
        
        // Map the biological phase directly into the OpenCL BZ reaction-diffusion feed/kill parameters
        // We blend the bio-wavelet with the existing audio-spectrum values so BOTH the room audio 
        // and the user's biological respiration drive the morphogens symbiotically!
        params.audioSpectrum[3] = std::clamp(params.audioSpectrum[3] + 0.5f * std::sin(respirationPhase_), 0.0f, 1.0f); // BZ audio_feed (w)
        params.audioSpectrum[0] = std::clamp(params.audioSpectrum[0] + 0.5f * std::cos(respirationPhase_ * 1.5f), 0.0f, 1.0f); // BZ audio_kill (x)
    }

    prevLeftHand_ = leftHand_;
    prevRightHand_ = rightHand_;
    prevHeadPose_ = headPose_;
}

void SculptureSpatialGestureEngine::applySpatialGestureTo4DLatent(
    SculptureProgramParams& params,
    float deltaSeconds
) noexcept {
    if (!leftHand_.valid && !rightHand_.valid) return;

    // Dual-hand spatial velocity steering on 4D latent coordinate and rotation
    if (leftHand_.valid && rightHand_.valid) {
        const float vxAvg = 0.5f * (filteredHands_.leftVelocity.x + filteredHands_.rightVelocity.x);
        const float vyAvg = 0.5f * (filteredHands_.leftVelocity.y + filteredHands_.rightVelocity.y);
        const float vzAvg = 0.5f * (filteredHands_.leftVelocity.z + filteredHands_.rightVelocity.z);

        // Map vertical kinetic hand motion to 4D coordinate latentW
        params.latentW = std::clamp(params.latentW + vyAvg * deltaSeconds * 0.35f, -2.5f, 2.5f);

        // Map lateral kinetic motion to pattern speed / double-isoclinic rotation rate
        params.patternSpeed = std::clamp(params.patternSpeed + (vxAvg + vzAvg * 0.5f) * deltaSeconds * 0.25f, -5.0f, 5.0f);
    } else if (rightHand_.valid) {
        params.latentW = std::clamp(params.latentW + filteredHands_.rightVelocity.y * deltaSeconds * 0.25f, -2.5f, 2.5f);
    } else if (leftHand_.valid) {
        params.latentW = std::clamp(params.latentW + filteredHands_.leftVelocity.y * deltaSeconds * 0.25f, -2.5f, 2.5f);
    }
}

HapticFeedbackPulse SculptureSpatialGestureEngine::computeHapticFeedback(
    const GeometricAudioFeatures& audioFeatures,
    bool isLeftHand
) const noexcept {
    HapticFeedbackPulse pulse{};
    if (activeGesture_ == GestureType::ConformalPinchRotate) {
        pulse.amplitude = std::clamp(0.2f + audioFeatures.spectralEnergy * 0.5f, 0.0f, 1.0f);
        pulse.frequencyHz = 160.0f + audioFeatures.lyapunovIndex * 40.0f;
        pulse.durationSeconds = 0.02f;
    } else if (activeGesture_ == GestureType::VortexInject && !isLeftHand) {
        pulse.amplitude = std::clamp(gestureVortexIntensity_ * 0.8f, 0.0f, 1.0f);
        pulse.frequencyHz = 240.0f;
        pulse.durationSeconds = 0.03f;
    } else if (activeGesture_ == GestureType::TopologySculpt && isLeftHand) {
        pulse.amplitude = std::clamp(audioFeatures.surfaceCurvature * 0.5f, 0.0f, 1.0f);
        pulse.frequencyHz = 110.0f;
        pulse.durationSeconds = 0.02f;
    }
    return pulse;
}

void SculptureSpatialGestureEngine::startRecording(std::string_view trackName) noexcept {
    isRecording_ = true;
    recordingTimer_ = 0.0f;
    recordedTrajectory_.name = std::string(trackName);
    recordedTrajectory_.keyframes.clear();
    recordedTrajectory_.totalDurationSeconds = 0.0f;

    // Record initial frame at t = 0
    recordedTrajectory_.keyframes.push_back(GestureKeyframe{
        .timestampSeconds = 0.0f,
        .leftHand = leftHand_,
        .rightHand = rightHand_,
        .gesture = activeGesture_,
        .customParamA = gestureScaleDelta_,
        .customParamB = gestureRotationDelta_
    });
}

void SculptureSpatialGestureEngine::stopRecording() noexcept {
    isRecording_ = false;
    recordedTrajectory_.totalDurationSeconds = recordingTimer_;
}

void SculptureSpatialGestureEngine::recordFrame(float deltaSeconds) noexcept {
    if (!isRecording_) return;
    const float dt = std::max(0.0f, deltaSeconds);
    recordingTimer_ += dt;

    recordedTrajectory_.keyframes.push_back(GestureKeyframe{
        .timestampSeconds = recordingTimer_,
        .leftHand = leftHand_,
        .rightHand = rightHand_,
        .gesture = activeGesture_,
        .customParamA = gestureScaleDelta_,
        .customParamB = gestureRotationDelta_
    });
    recordedTrajectory_.totalDurationSeconds = recordingTimer_;
}

void SculptureSpatialGestureEngine::setRecordedTrajectory(GestureTrajectory trajectory) noexcept {
    recordedTrajectory_ = std::move(trajectory);
}

void SculptureSpatialGestureEngine::startPlayback(TrajectoryPlaybackMode mode, float speed) noexcept {
    if (recordedTrajectory_.keyframes.size() < 2) return;
    isPlaying_ = true;
    isPlaybackPaused_ = false;
    playbackTime_ = 0.0f;
    playbackPingPongForward_ = true;
    recordedTrajectory_.mode = mode;
    recordedTrajectory_.speed = std::max(0.01f, speed);
}

void SculptureSpatialGestureEngine::stopPlayback() noexcept {
    isPlaying_ = false;
    isPlaybackPaused_ = false;
    playbackTime_ = 0.0f;
}

void SculptureSpatialGestureEngine::updatePlayback(float deltaSeconds) noexcept {
    if (!isPlaying_ || isPlaybackPaused_ || recordedTrajectory_.keyframes.size() < 2) return;
    const float totalDur = std::max(0.01f, recordedTrajectory_.totalDurationSeconds);
    const float step = deltaSeconds * recordedTrajectory_.speed;

    switch (recordedTrajectory_.mode) {
    case TrajectoryPlaybackMode::Once:
        playbackTime_ += step;
        if (playbackTime_ >= totalDur) {
            playbackTime_ = totalDur;
            isPlaying_ = false;
        }
        break;
    case TrajectoryPlaybackMode::Loop:
        playbackTime_ = std::fmod(playbackTime_ + step, totalDur);
        if (playbackTime_ < 0.0f) playbackTime_ += totalDur;
        break;
    case TrajectoryPlaybackMode::PingPong:
        if (playbackPingPongForward_) {
            playbackTime_ += step;
            if (playbackTime_ >= totalDur) {
                playbackTime_ = totalDur;
                playbackPingPongForward_ = false;
            }
        } else {
            playbackTime_ -= step;
            if (playbackTime_ <= 0.0f) {
                playbackTime_ = 0.0f;
                playbackPingPongForward_ = true;
            }
        }
        break;
    }

    const auto sampled = sampleTrajectory(playbackTime_);
    leftHand_ = sampled.leftHand;
    rightHand_ = sampled.rightHand;
    activeGesture_ = sampled.gesture;
}

SculptureVec3 SculptureSpatialGestureEngine::hermiteInterpolate(
    const SculptureVec3& p0, const SculptureVec3& m0,
    const SculptureVec3& p1, const SculptureVec3& m1,
    float t
) noexcept {
    const float u = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
    const float u2 = u * u;
    const float u3 = u2 * u;

    const float h00 = 2.0f * u3 - 3.0f * u2 + 1.0f;
    const float h10 = u3 - 2.0f * u2 + u;
    const float h01 = -2.0f * u3 + 3.0f * u2;
    const float h11 = u3 - u2;

    return SculptureVec3{
        h00 * p0.x + h10 * m0.x + h01 * p1.x + h11 * m1.x,
        h00 * p0.y + h10 * m0.y + h01 * p1.y + h11 * m1.y,
        h00 * p0.z + h10 * m0.z + h01 * p1.z + h11 * m1.z
    };
}

Pose6DOF SculptureSpatialGestureEngine::interpolatePose(
    const Pose6DOF& p0, const SculptureVec3& m0,
    const Pose6DOF& p1, const SculptureVec3& m1,
    float t
) noexcept {
    const float u = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
    return Pose6DOF{
        .position = hermiteInterpolate(p0.position, m0, p1.position, m1, u),
        .orientation = Quaternion4D::slerp(p0.orientation, p1.orientation, u),
        .trigger = p0.trigger + (p1.trigger - p0.trigger) * u,
        .grip = p0.grip + (p1.grip - p0.grip) * u,
        .valid = p0.valid || p1.valid
    };
}

GestureKeyframe SculptureSpatialGestureEngine::sampleTrajectory(float timeSeconds) const noexcept {
    if (recordedTrajectory_.keyframes.empty()) {
        return GestureKeyframe{};
    }
    if (recordedTrajectory_.keyframes.size() == 1 || timeSeconds <= recordedTrajectory_.keyframes.front().timestampSeconds) {
        return recordedTrajectory_.keyframes.front();
    }
    if (timeSeconds >= recordedTrajectory_.keyframes.back().timestampSeconds) {
        return recordedTrajectory_.keyframes.back();
    }

    const auto& kfs = recordedTrajectory_.keyframes;
    std::size_t idx = 0;
    for (std::size_t i = 0; i < kfs.size() - 1; ++i) {
        if (timeSeconds >= kfs[i].timestampSeconds && timeSeconds <= kfs[i + 1].timestampSeconds) {
            idx = i;
            break;
        }
    }

    const auto& k0 = kfs[idx];
    const auto& k1 = kfs[idx + 1];
    const float dt = std::max(1e-5f, k1.timestampSeconds - k0.timestampSeconds);
    const float u = (timeSeconds - k0.timestampSeconds) / dt;

    // Catmull-Rom tangents for left hand
    SculptureVec3 left_m0{0.0f, 0.0f, 0.0f};
    SculptureVec3 left_m1{0.0f, 0.0f, 0.0f};
    if (idx > 0) {
        const auto& km1 = kfs[idx - 1];
        const float dt_prev = std::max(1e-5f, k1.timestampSeconds - km1.timestampSeconds);
        left_m0 = {
            (k1.leftHand.position.x - km1.leftHand.position.x) / dt_prev * dt,
            (k1.leftHand.position.y - km1.leftHand.position.y) / dt_prev * dt,
            (k1.leftHand.position.z - km1.leftHand.position.z) / dt_prev * dt
        };
    } else {
        left_m0 = {
            (k1.leftHand.position.x - k0.leftHand.position.x),
            (k1.leftHand.position.y - k0.leftHand.position.y),
            (k1.leftHand.position.z - k0.leftHand.position.z)
        };
    }

    if (idx + 2 < kfs.size()) {
        const auto& kp2 = kfs[idx + 2];
        const float dt_next = std::max(1e-5f, kp2.timestampSeconds - k0.timestampSeconds);
        left_m1 = {
            (kp2.leftHand.position.x - k0.leftHand.position.x) / dt_next * dt,
            (kp2.leftHand.position.y - k0.leftHand.position.y) / dt_next * dt,
            (kp2.leftHand.position.z - k0.leftHand.position.z) / dt_next * dt
        };
    } else {
        left_m1 = {
            (k1.leftHand.position.x - k0.leftHand.position.x),
            (k1.leftHand.position.y - k0.leftHand.position.y),
            (k1.leftHand.position.z - k0.leftHand.position.z)
        };
    }

    // Catmull-Rom tangents for right hand
    SculptureVec3 right_m0{0.0f, 0.0f, 0.0f};
    SculptureVec3 right_m1{0.0f, 0.0f, 0.0f};
    if (idx > 0) {
        const auto& km1 = kfs[idx - 1];
        const float dt_prev = std::max(1e-5f, k1.timestampSeconds - km1.timestampSeconds);
        right_m0 = {
            (k1.rightHand.position.x - km1.rightHand.position.x) / dt_prev * dt,
            (k1.rightHand.position.y - km1.rightHand.position.y) / dt_prev * dt,
            (k1.rightHand.position.z - km1.rightHand.position.z) / dt_prev * dt
        };
    } else {
        right_m0 = {
            (k1.rightHand.position.x - k0.rightHand.position.x),
            (k1.rightHand.position.y - k0.rightHand.position.y),
            (k1.rightHand.position.z - k0.rightHand.position.z)
        };
    }

    if (idx + 2 < kfs.size()) {
        const auto& kp2 = kfs[idx + 2];
        const float dt_next = std::max(1e-5f, kp2.timestampSeconds - k0.timestampSeconds);
        right_m1 = {
            (kp2.rightHand.position.x - k0.rightHand.position.x) / dt_next * dt,
            (kp2.rightHand.position.y - k0.rightHand.position.y) / dt_next * dt,
            (kp2.rightHand.position.z - k0.rightHand.position.z) / dt_next * dt
        };
    } else {
        right_m1 = {
            (k1.rightHand.position.x - k0.rightHand.position.x),
            (k1.rightHand.position.y - k0.rightHand.position.y),
            (k1.rightHand.position.z - k0.rightHand.position.z)
        };
    }

    return GestureKeyframe{
        .timestampSeconds = timeSeconds,
        .leftHand = interpolatePose(k0.leftHand, left_m0, k1.leftHand, left_m1, u),
        .rightHand = interpolatePose(k0.rightHand, right_m0, k1.rightHand, right_m1, u),
        .gesture = (u < 0.5f) ? k0.gesture : k1.gesture,
        .customParamA = k0.customParamA + (k1.customParamA - k0.customParamA) * u,
        .customParamB = k0.customParamB + (k1.customParamB - k0.customParamB) * u
    };
}

} // namespace epi_sculpt
