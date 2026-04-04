#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <math.h>
#include <stdio.h>

PSP_MODULE_INFO("LFS_PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[262144];

typedef struct {
    float x, z, angle, speed, rpm;
    int gear;
} Car;

Car lfs_car = {0.0f, -10.0f, 0.0f, 0.0f, 1000.0f, 0};

void init_graphics() {
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
    sceGuDispBuffer(480, 272, (void*)0x88000, 512);
    sceGuDepthBuffer((void*)0x110000, 512);
    sceGuOffset(2048 - (480/2), 2048 - (272/2));
    sceGuViewport(2048, 2048, 480, 272);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuShadeModel(GU_SMOOTH);
    sceGuFrontFace(GU_CCW);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void update_game() {
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);

    // Controlli
    if (pad.Buttons & PSP_CTRL_CROSS) lfs_car.speed += 0.01f;
    if (pad.Buttons & PSP_CTRL_SQUARE) lfs_car.speed -= 0.02f;
    if (pad.Lx < 80) lfs_car.angle -= 0.03f;
    if (pad.Lx > 170) lfs_car.angle += 0.03f;

    // Fisica base
    lfs_car.speed *= 0.98f;
    lfs_car.x += sinf(lfs_car.angle) * lfs_car.speed;
    lfs_car.z += cosf(lfs_car.angle) * lfs_car.speed;
    lfs_car.rpm = 1000.0f + (fabsf(lfs_car.speed) * 5000.0f);
}

int main() {
    pspDebugScreenInit();
    init_graphics();

    while(1) {
        update_game();

        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xff444444);
        sceGuClearDepth(0);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
        sceGuFinish();
        sceGuSync(0, 0);

        pspDebugScreenSetXY(2, 2);
        pspDebugScreenPrintf("LIVE FOR SPEED PSP - PROTOTIPO");
        pspDebugScreenSetXY(2, 4);
        pspDebugScreenPrintf("VELOCITA': %d KM/H", (int)(fabsf(lfs_car.speed) * 200));
        pspDebugScreenSetXY(2, 5);
        pspDebugScreenPrintf("GIRI MOTORE: %d RPM", (int)lfs_car.rpm);

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }
    return 0;
}
