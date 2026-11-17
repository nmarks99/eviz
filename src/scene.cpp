#include "scene.hpp"
#include "raymath.h"
#include <pace.hpp>
#include <stdexcept>

namespace {

double units_to_scale(const std::string& units, urdf::JointType type, const std::string& joint) {
    const bool rotational = type == urdf::JointType::Revolute || type == urdf::JointType::Continuous;
    if (rotational && units == "rad") {
        return 1.0;
    }
    if (rotational && units == "deg") {
        return PI / 180.0;
    }
    if (!rotational && units == "m") {
        return 1.0;
    }
    if (!rotational && units == "mm") {
        return 0.001;
    }
    throw std::runtime_error("Joint '" + joint + "': invalid units '" + units + "' for " +
                             (rotational ? "rotational (rad, deg)" : "prismatic (m, mm)") + " joint");
}

} // namespace

Scene::Scene(const SceneConfig& config, pace::Context& ctx) {
    for (const RobotConfig& rc : config.robots) {
        SceneRobot robot;
        robot.model = std::make_unique<RobotModel>(rc.urdf);
        robot.model->set_root_transform(
            MatrixMultiply(urdf::pose_to_matrix(rc.placement), MatrixRotateX(-PI / 2.0f)));

        for (const JointBinding& b : rc.joints) {
            const int joint = robot.model->joint_index(b.joint);
            double scale = b.scale;
            if (!b.units.empty()) {
                scale = units_to_scale(b.units, robot.model->description().joints[joint].type, b.joint);
            }
            auto input = std::make_unique<JointInput>(JointInput{joint, scale, b.offset});
            ctx.connect(b.pv).bind(input->raw);
            robot.inputs.push_back(std::move(input));
        }
        robot.model->update();
        robots_.push_back(std::move(robot));
    }
}

void Scene::update() {
    for (SceneRobot& robot : robots_) {
        for (const auto& in : robot.inputs) {
            robot.model->set_joint_position(in->joint, static_cast<float>(in->raw * in->scale + in->offset));
        }
        robot.model->update();
    }
}

void Scene::draw() {
    for (SceneRobot& robot : robots_) {
        robot.model->draw();
    }
}

void Scene::draw_axes() {
    for (SceneRobot& robot : robots_) {
        robot.model->draw_axes();
    }
}
