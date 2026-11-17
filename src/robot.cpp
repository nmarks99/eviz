#include "robot.hpp"
#include "raymath.h"
#include <queue>
#include <stdexcept>
#include <unordered_map>

constexpr float AXES_THICKNESS = 0.0075f;

RobotModel::RobotModel(const std::filesystem::path& urdf_path)
    : robot_(urdf::parse(urdf_path)), root_tf_(MatrixIdentity()) {
    std::unordered_map<std::string, int> link_index;
    for (int i = 0; i < static_cast<int>(robot_.links.size()); i++) {
        link_index[robot_.links[i].name] = i;
    }

    const int njoints = static_cast<int>(robot_.joints.size());
    joint_parent_.resize(njoints);
    joint_child_.resize(njoints);
    joint_origin_tf_.resize(njoints);

    std::vector<bool> is_child(robot_.links.size(), false);
    std::vector<std::vector<int>> children(robot_.links.size());
    for (int j = 0; j < njoints; j++) {
        const urdf::Joint& joint = robot_.joints[j];
        auto parent = link_index.find(joint.parent);
        auto child = link_index.find(joint.child);
        if (parent == link_index.end() || child == link_index.end()) {
            throw std::runtime_error("URDF: joint '" + joint.name + "' references unknown link");
        }
        if (is_child[child->second]) {
            throw std::runtime_error("URDF: link '" + joint.child + "' has multiple parents");
        }
        is_child[child->second] = true;
        joint_parent_[j] = parent->second;
        joint_child_[j] = child->second;
        joint_origin_tf_[j] = urdf::pose_to_matrix(joint.origin);
        children[parent->second].push_back(j);
    }

    for (int i = 0; i < static_cast<int>(robot_.links.size()); i++) {
        if (!is_child[i]) {
            if (root_link_ != -1) {
                throw std::runtime_error("URDF: multiple root links");
            }
            root_link_ = i;
        }
    }
    if (root_link_ == -1) {
        throw std::runtime_error("URDF: no root link");
    }

    std::queue<int> frontier;
    frontier.push(root_link_);
    while (!frontier.empty()) {
        int link = frontier.front();
        frontier.pop();
        for (int j : children[link]) {
            joint_order_.push_back(j);
            frontier.push(joint_child_[j]);
        }
    }

    for (int i = 0; i < static_cast<int>(robot_.links.size()); i++) {
        for (const urdf::Visual& vis : robot_.links[i].visuals) {
            auto model = std::make_unique<RLModel>(vis.mesh, robot_.links[i].name);
            model->load();
            if (!IsModelValid(model->model)) {
                throw std::runtime_error("Failed to load mesh: " + vis.mesh.string());
            }
            Matrix local = MatrixMultiply(MatrixScale(vis.scale.x, vis.scale.y, vis.scale.z),
                                          urdf::pose_to_matrix(vis.origin));
            visuals_.push_back({i, local, std::move(model)});
        }
    }

    q_.assign(njoints, 0.0f);
    link_tf_.assign(robot_.links.size(), MatrixIdentity());
    update();
}

int RobotModel::joint_index(const std::string& name) const {
    for (int j = 0; j < static_cast<int>(robot_.joints.size()); j++) {
        if (robot_.joints[j].name == name) {
            if (robot_.joints[j].type == urdf::JointType::Fixed) {
                throw std::runtime_error("Joint '" + name + "' is fixed");
            }
            return j;
        }
    }
    throw std::runtime_error("No joint named '" + name + "' in robot '" + robot_.name + "'");
}

void RobotModel::update() {
    link_tf_[root_link_] = root_tf_;
    for (int j : joint_order_) {
        const urdf::Joint& joint = robot_.joints[j];
        const float value = q_[j];

        Matrix motion = MatrixIdentity();
        switch (joint.type) {
        case urdf::JointType::Revolute:
        case urdf::JointType::Continuous:
            motion = MatrixRotate(joint.axis, value);
            break;
        case urdf::JointType::Prismatic: {
            Vector3 d = Vector3Scale(Vector3Normalize(joint.axis), value);
            motion = MatrixTranslate(d.x, d.y, d.z);
            break;
        }
        case urdf::JointType::Fixed:
            break;
        }

        link_tf_[joint_child_[j]] =
            MatrixMultiply(MatrixMultiply(motion, joint_origin_tf_[j]), link_tf_[joint_parent_[j]]);
    }

    for (VisualInstance& vis : visuals_) {
        vis.model->model.transform = MatrixMultiply(vis.local_tf, link_tf_[vis.link]);
    }
}

void RobotModel::draw() {
    for (VisualInstance& vis : visuals_) {
        vis.model->draw();
    }
}

void RobotModel::draw_axes() {
    for (const Matrix& tf : link_tf_) {
        draw_axes_3d(AXES_THICKNESS, tf);
    }
}
