#include "config.hpp"
#include "raylib.h"
#include "rl_utils.hpp"
#include "scene.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>

#include <pace.hpp>

inline std::filesystem::path get_resource_dir() {
    if (const char* env = std::getenv("EVIZ_RESOURCE_DIR")) {
        return std::filesystem::path(env);
    }
    return EVIZ_RESOURCE_DIR;
}

int main(int argc, char* argv[]) {
    const std::filesystem::path scene_path =
        argc > 1 ? std::filesystem::path(argv[1]) : get_resource_dir() / "ur5e" / "ur5e.toml";

    pace::Context ctx("pva");

    // initialize the window
    RLWindow window(1120, 760, "eviz");

    // Set up 3D camera
    RLCamera3D cam;

    std::unique_ptr<Scene> scene;
    try {
        scene = std::make_unique<Scene>(load_scene_config(scene_path), ctx);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    while (!WindowShouldClose()) {
        // UPDATE ///////////////////////////////////////////////
        cam.update();
        if (ctx.sync()) {
            scene->update();
        }
        // UPDATE ///////////////////////////////////////////////

        // DRAW /////////////////////////////////////////////////
        BeginDrawing();
        ClearBackground(Color{42, 42, 42});

        // 3D ---------------------------------------------------
        BeginMode3D(cam.camera);
        DrawGrid(10, 0.2f);
        scene->draw();
        EndMode3D();
        // 3D ---------------------------------------------------

        EndDrawing();
        // DRAW /////////////////////////////////////////////////
    }

    return 0;
}
