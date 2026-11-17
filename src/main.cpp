#include "raylib.h"
#include "raymath.h"
#include "rl_utils.hpp"
#include "robot.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>

inline std::filesystem::path get_resource_dir() {
    if (const char* env = std::getenv("EVIZ_RESOURCE_DIR")) {
        return std::filesystem::path(env);
    }
    return EVIZ_RESOURCE_DIR;
}

int main(int argc, char* argv[]) {
    const std::filesystem::path urdf_path =
        argc > 1 ? std::filesystem::path(argv[1]) : get_resource_dir() / "ur5e" / "ur5e.urdf";

    // initialize the window
    RLWindow window(1120, 760, "eviz");

    // Set up 3D camera
    RLCamera3D cam;

    std::unique_ptr<RobotModel> robot_ptr;
    try {
        robot_ptr = std::make_unique<RobotModel>(urdf_path);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    RobotModel& robot = *robot_ptr;
    robot.set_root_transform(MatrixRotateX(-PI / 2.0f));
    robot.update({
        {"shoulder_pan_joint", 0.0f},
        {"shoulder_lift_joint", -PI / 2.0f},
        {"elbow_joint", PI / 2.0f},
        {"wrist_1_joint", -PI / 2.0f},
        {"wrist_2_joint", -PI / 2.0f},
        {"wrist_3_joint", 0.0f},
    });

    while (!WindowShouldClose()) {
        // UPDATE ///////////////////////////////////////////////
        cam.update();
        // UPDATE ///////////////////////////////////////////////

        // DRAW /////////////////////////////////////////////////
        BeginDrawing();
        ClearBackground(Color{42, 42, 42});

        // 3D ---------------------------------------------------
        BeginMode3D(cam.camera);
        DrawGrid(10, 0.1f);
        robot.draw();
        EndMode3D();
        // 3D ---------------------------------------------------

        EndDrawing();
        // DRAW /////////////////////////////////////////////////
    }

    return 0;
}
