#pragma once
#include "raylib.h"
#include "rl_utils.hpp"
#include "urdf.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class RobotModel {
  public:
    explicit RobotModel(const std::filesystem::path& urdf_path);

    int joint_index(const std::string& name) const;
    void set_joint_position(int index, float value) { q_.at(index) = value; }
    float joint_position(int index) const { return q_.at(index); }

    void update();
    void draw();
    void draw_axes();

    void set_root_transform(Matrix tf) { root_tf_ = tf; }
    const urdf::Robot& description() const { return robot_; }

  private:
    struct VisualInstance {
        int link;
        Matrix local_tf;
        std::unique_ptr<RLModel> model;
    };

    urdf::Robot robot_;
    int root_link_ = -1;
    std::vector<int> joint_order_;
    std::vector<int> joint_child_;
    std::vector<int> joint_parent_;
    std::vector<Matrix> joint_origin_tf_;
    std::vector<float> q_;
    std::vector<Matrix> link_tf_;
    std::vector<VisualInstance> visuals_;
    Matrix root_tf_;
};
