// Interaction-specific persistence adapters.
//
// Core preset/world serialization intentionally lives in SculptureSerializer.cpp
// and depends only on the authored-state model. Gesture trajectories and macro
// engines belong to optional live-control domains, so their concrete type
// dependencies stay in this separate translation unit.

#include "SculptureSerializer.h"

#include "SculptureMacroAutomation.h"
#include "SculptureSpatialGestureEngine.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace epi_sculpt {

std::string SculptureSerializer::serializeMacroEngine(const SculptureMacroAutomationEngine& engine) {
    return engine.serializeJson();
}

bool SculptureSerializer::deserializeMacroEngine(const std::string& content, SculptureMacroAutomationEngine& engine) {
    return engine.deserializeJson(content);
}

std::string SculptureSerializer::serializeGestureTrajectory(const GestureTrajectory& trajectory) {
    std::ostringstream out;
    out << "# Epi2 Gesture Trajectory\n";
    out << "version=1\n";
    out << "name=" << trajectory.name << "\n";
    out << "totalDuration=" << trajectory.totalDurationSeconds << "\n";
    out << "mode=" << static_cast<int>(trajectory.mode) << "\n";
    out << "speed=" << trajectory.speed << "\n";
    out << "keyframeCount=" << trajectory.keyframes.size() << "\n";

    for (const auto& kf : trajectory.keyframes) {
        out << "kf=" << kf.timestampSeconds << ";"
            << kf.leftHand.position.x << "," << kf.leftHand.position.y << "," << kf.leftHand.position.z << ";"
            << kf.leftHand.orientation.w << "," << kf.leftHand.orientation.x << "," << kf.leftHand.orientation.y << "," << kf.leftHand.orientation.z << ";"
            << kf.leftHand.trigger << "," << kf.leftHand.grip << "," << (kf.leftHand.valid ? 1 : 0) << ";"
            << kf.rightHand.position.x << "," << kf.rightHand.position.y << "," << kf.rightHand.position.z << ";"
            << kf.rightHand.orientation.w << "," << kf.rightHand.orientation.x << "," << kf.rightHand.orientation.y << "," << kf.rightHand.orientation.z << ";"
            << kf.rightHand.trigger << "," << kf.rightHand.grip << "," << (kf.rightHand.valid ? 1 : 0) << ";"
            << static_cast<int>(kf.gesture) << ";"
            << kf.customParamA << "," << kf.customParamB << "\n";
    }
    return out.str();
}

std::optional<GestureTrajectory> SculptureSerializer::deserializeGestureTrajectory(const std::string& payload) {
    if (payload.empty()) return std::nullopt;
    std::istringstream in(payload);
    std::string line;

    GestureTrajectory traj{};

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        const auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        const std::string key = line.substr(0, eqPos);
        const std::string val = line.substr(eqPos + 1);

        if (key == "name") {
            traj.name = val;
        } else if (key == "totalDuration") {
            try { traj.totalDurationSeconds = std::stof(val); } catch (...) {}
        } else if (key == "mode") {
            try { traj.mode = static_cast<TrajectoryPlaybackMode>(std::stoi(val)); } catch (...) {}
        } else if (key == "speed") {
            try { traj.speed = std::stof(val); } catch (...) {}
        } else if (key == "kf") {
            std::stringstream ss(val);
            std::string section;
            std::vector<std::string> sections;
            while (std::getline(ss, section, ';')) {
                sections.push_back(section);
            }

            if (sections.size() >= 7) {
                GestureKeyframe kf{};
                try {
                    kf.timestampSeconds = std::stof(sections[0]);

                    // Lpos
                    std::stringstream s_lpos(sections[1]);
                    std::string t;
                    if (std::getline(s_lpos, t, ',')) kf.leftHand.position.x = std::stof(t);
                    if (std::getline(s_lpos, t, ',')) kf.leftHand.position.y = std::stof(t);
                    if (std::getline(s_lpos, t, ',')) kf.leftHand.position.z = std::stof(t);

                    // Lori
                    std::stringstream s_lori(sections[2]);
                    if (std::getline(s_lori, t, ',')) kf.leftHand.orientation.w = std::stof(t);
                    if (std::getline(s_lori, t, ',')) kf.leftHand.orientation.x = std::stof(t);
                    if (std::getline(s_lori, t, ',')) kf.leftHand.orientation.y = std::stof(t);
                    if (std::getline(s_lori, t, ',')) kf.leftHand.orientation.z = std::stof(t);

                    // Lctrl
                    std::stringstream s_lctrl(sections[3]);
                    if (std::getline(s_lctrl, t, ',')) kf.leftHand.trigger = std::stof(t);
                    if (std::getline(s_lctrl, t, ',')) kf.leftHand.grip = std::stof(t);
                    if (std::getline(s_lctrl, t, ',')) kf.leftHand.valid = (std::stoi(t) != 0);

                    // Rpos
                    std::stringstream s_rpos(sections[4]);
                    if (std::getline(s_rpos, t, ',')) kf.rightHand.position.x = std::stof(t);
                    if (std::getline(s_rpos, t, ',')) kf.rightHand.position.y = std::stof(t);
                    if (std::getline(s_rpos, t, ',')) kf.rightHand.position.z = std::stof(t);

                    // Rori
                    std::stringstream s_rori(sections[5]);
                    if (std::getline(s_rori, t, ',')) kf.rightHand.orientation.w = std::stof(t);
                    if (std::getline(s_rori, t, ',')) kf.rightHand.orientation.x = std::stof(t);
                    if (std::getline(s_rori, t, ',')) kf.rightHand.orientation.y = std::stof(t);
                    if (std::getline(s_rori, t, ',')) kf.rightHand.orientation.z = std::stof(t);

                    // Rctrl
                    std::stringstream s_rctrl(sections[6]);
                    if (std::getline(s_rctrl, t, ',')) kf.rightHand.trigger = std::stof(t);
                    if (std::getline(s_rctrl, t, ',')) kf.rightHand.grip = std::stof(t);
                    if (std::getline(s_rctrl, t, ',')) kf.rightHand.valid = (std::stoi(t) != 0);

                    if (sections.size() >= 8) {
                        kf.gesture = static_cast<GestureType>(std::stoi(sections[7]));
                    }
                    if (sections.size() >= 9) {
                        std::stringstream s_cust(sections[8]);
                        if (std::getline(s_cust, t, ',')) kf.customParamA = std::stof(t);
                        if (std::getline(s_cust, t, ',')) kf.customParamB = std::stof(t);
                    }
                    traj.keyframes.push_back(kf);
                } catch (...) {
                    continue;
                }
            }
        }
    }

    if (traj.keyframes.empty()) return std::nullopt;
    if (traj.totalDurationSeconds <= 0.0f) {
        traj.totalDurationSeconds = traj.keyframes.back().timestampSeconds;
    }
    return traj;
}

bool SculptureSerializer::saveGestureTrajectoryToFile(const std::string& filepath, const GestureTrajectory& trajectory) {
    try {
        std::filesystem::path p(filepath);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        std::ofstream f(filepath);
        if (!f.is_open()) return false;
        f << serializeGestureTrajectory(trajectory);
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<GestureTrajectory> SculptureSerializer::loadGestureTrajectoryFromFile(const std::string& filepath) {
    try {
        std::ifstream f(filepath);
        if (!f.is_open()) return std::nullopt;
        std::stringstream buffer;
        buffer << f.rdbuf();
        return deserializeGestureTrajectory(buffer.str());
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace epi_sculpt
