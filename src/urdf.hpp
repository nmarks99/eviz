#pragma once
#include "raylib.h"
#include <filesystem>
#include <string>
#include <vector>

namespace urdf {

struct Pose {
    Vector3 xyz{0.0f, 0.0f, 0.0f};
    Vector3 rpy{0.0f, 0.0f, 0.0f};
};

struct Visual {
    Pose origin;
    std::filesystem::path mesh;
    Vector3 scale{1.0f, 1.0f, 1.0f};
};

struct Link {
    std::string name;
    std::vector<Visual> visuals;
};

enum class JointType { Fixed, Revolute, Continuous, Prismatic };

struct Joint {
    std::string name;
    JointType type = JointType::Fixed;
    std::string parent;
    std::string child;
    Pose origin;
    Vector3 axis{1.0f, 0.0f, 0.0f};
    float lower = 0.0f;
    float upper = 0.0f;
};

struct Robot {
    std::string name;
    std::vector<Link> links;
    std::vector<Joint> joints;
};

Robot parse(const std::filesystem::path& path);

Matrix pose_to_matrix(const Pose& pose);

} // namespace urdf
