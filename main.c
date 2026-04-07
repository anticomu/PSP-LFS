#include "raylib.h"
#include <pspkernel.h>
#include <pspdebug.h>
#include <math.h>

/* PSP Module Info */
PSP_MODULE_INFO("LFS_PSP_RAYLIB", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/* PSP Callbacks */
int exit_callback(int arg1, int arg2, void *common) {
    sceKernelExitGame();
    return 0;
}

int callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }
    return thid;
}

int main(void) {
    setup_callbacks();

    // Initialization
    const int screenWidth = 480;
    const int screenHeight = 272;

    InitWindow(screenWidth, screenHeight, "LFS PSP - Raylib Edition");

    // Define the camera to look into our 3d world
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera mode type

    // Load car model
    Model car = LoadModel("car.obj");
    Vector3 carPosition = { 0.0f, 0.0f, 0.0f };
    float carRotation = 0.0f;
    float carSpeed = 0.0f;

    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose()) {
        // Update
        if (IsKeyDown(KEY_UP)) carSpeed += 0.1f;
        if (IsKeyDown(KEY_DOWN)) carSpeed -= 0.1f;
        if (IsKeyDown(KEY_LEFT)) carRotation += 2.0f;
        if (IsKeyDown(KEY_RIGHT)) carRotation -= 2.0f;

        // Friction
        carSpeed *= 0.98f;

        // Move car
        carPosition.x += carSpeed * sinf(carRotation * DEG2RAD);
        carPosition.z += carSpeed * cosf(carRotation * DEG2RAD);

        // Update Camera to follow car
        camera.target = carPosition;
        camera.position.x = carPosition.x - 10.0f * sinf(carRotation * DEG2RAD);
        camera.position.z = carPosition.z - 10.0f * cosf(carRotation * DEG2RAD);
        camera.position.y = 5.0f;

        // Draw
        BeginDrawing();
            ClearBackground(SKYBLUE);

            BeginMode3D(camera);
                // Draw ground
                DrawGrid(20, 1.0f);
                
                // Draw car model
                DrawModelEx(car, carPosition, (Vector3){ 0, 1, 0 }, carRotation, (Vector3){ 1, 1, 1 }, WHITE);
                
            EndMode3D();

            DrawFPS(10, 10);
            DrawText("LFS PSP - Raylib Professional Engine", 10, 30, 20, BLACK);
            DrawText(TextFormat("Speed: %.2f", carSpeed), 10, 60, 20, MAROON);

        EndDrawing();
    }

    // De-Initialization
    UnloadModel(car);
    CloseWindow();

    return 0;
}
