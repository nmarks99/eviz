#pragma once
#include "config.hpp"
#include "markers.hpp"
#include "robot.hpp"
#include <memory>
#include <vector>

namespace pace {
class Context;
}

class Scene {
  public:
    Scene(const SceneConfig& config, pace::Context& ctx);

    void update();
    void draw();
    void draw_axes();

  private:
    struct JointInput {
        int joint;
        double scale;
        double offset;
        double raw = 0.0;
    };

    struct SceneRobot {
        std::unique_ptr<RobotModel> model;
        std::vector<std::unique_ptr<JointInput>> inputs;
    };

    std::vector<SceneRobot> robots_;
    std::vector<std::unique_ptr<MarkerDisplay>> markers_;
};
