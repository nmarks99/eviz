#include "raylib.h"
#include "rl_utils.hpp"

int main(int argc, char *argv[]) {

    // initialize the window
    RLWindow window(1120, 760, "eviz");

    // Set up 3D camera
    RLCamera3D cam;

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
        EndMode3D();
        // 3D ---------------------------------------------------

        EndDrawing();
        // DRAW /////////////////////////////////////////////////
    }

    return 0;
}
