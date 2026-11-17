#pragma once
#include "urdf.hpp"
#include <filesystem>
#include <string>
#include <vector>

struct JointBinding {
    std::string joint;
    std::string pv;
    std::string units;
    double scale = 1.0;
    double offset = 0.0;
};

struct RobotConfig {
    std::filesystem::path urdf;
    urdf::Pose placement;
    std::vector<JointBinding> joints;
};

struct SceneConfig {
    std::vector<RobotConfig> robots;
};

SceneConfig load_scene_config(const std::filesystem::path& path);
