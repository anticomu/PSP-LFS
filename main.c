#include "raylib.h"
#include <pspkernel.h>
#include <pspdebug.h>
#include <math.h>

PSP_MODULE_INFO("LFS_PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

int exit_callback(int arg1, int arg2, void *common) { sceKernelExitGame(); return 0; }
int callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}
int setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
    return thid;
}

int main(void) {
    setup_callbacks();
    InitWindow(480, 272, "LFS PSP Pro");

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model car = LoadModel("car.obj");
    Model track = LoadModel("track.obj");

    Vector3 carPosition = { 0.0f, 0.0f, 0.0f };
    float carRotation = 0.0f;
    float carSpeed = 0.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_UP)) carSpeed += 0.1f;
        if (IsKeyDown(KEY_DOWN)) carSpeed -= 0.1f;
        if (IsKeyDown(KEY_LEFT)) carRotation += 2.0f;
        if (IsKeyDown(KEY_RIGHT)) carRotation -= 2.0f;

        carSpeed *= 0.98f;
        carPosition.x += carSpeed * sinf(carRotation * DEG2RAD);
        carPosition.z += carSpeed * cosf(carRotation * DEG2RAD);

        camera.target = carPosition;
        camera.position.x = carPosition.x - 10.0f * sinf(carRotation * DEG2RAD);
        camera.position.z = carPosition.z - 10.0f * cosf(carRotation * DEG2RAD);
        camera.position.y = 5.0f;

        BeginDrawing();
            ClearBackground(SKYBLUE);
            BeginMode3D(camera);
                if (track.meshCount == 0) DrawGrid(20, 1.0f);
                else DrawModel(track, (Vector3){ 0, 0, 0 }, 1.0f, WHITE);
                DrawModelEx(car, carPosition, (Vector3){ 0, 1, 0 }, carRotation, (Vector3){ 1, 1, 1 }, WHITE);
            EndMode3D();
            DrawFPS(10, 10);
        EndDrawing();
    }
    UnloadModel(car); UnloadModel(track); CloseWindow();
    return 0;
}
