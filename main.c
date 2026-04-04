#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <math.h>
#include <stdio.h>

PSP_MODULE_INFO("LFS_PRO", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[262144];

// --- CALLBACKS ---
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

typedef struct { float x, y, z; } Vertex;

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
    sceGuFrontFace(GU_CCW);
    sceGuShadeModel(GU_SMOOTH);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

int main() {
    setup_callbacks();
    pspDebugScreenInit();
    init_graphics();

    float car_x = 0, car_z = 0, car_angle = 0, speed = 0;
    SceCtrlData pad;

    while(1) {
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_CROSS) speed += 0.05f;
        if (pad.Lx < 80) car_angle -= 0.04f;
        if (pad.Lx > 170) car_angle += 0.04f;
        speed *= 0.96f;
        car_x += sinf(car_angle) * speed;
        car_z += cosf(car_angle) * speed;

        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xff222222);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        // Setup Proiezione 3D
        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(75.0f, 480.0f/272.0f, 0.5f, 1000.0f);

        // Setup Telecamera (Inseguimento)
        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        ScePspFVector3 cam_pos = { car_x - sinf(car_angle)*8, 4.0f, car_z - cosf(car_angle)*8 };
        ScePspFVector3 cam_look = { car_x, 1.0f, car_z };
        ScePspFVector3 cam_up = { 0, 1, 0 };
        sceGumLookAt(&cam_pos, &cam_look, &cam_up);

        sceGuFinish();
        sceGuSync(0, 0);

        pspDebugScreenSetXY(2, 2);
        pspDebugScreenPrintf("LFS PSP - 3D READY");
        pspDebugScreenSetXY(2, 4);
        pspDebugScreenPrintf("SPEED: %d KM/H | ANGLE: %.2f", (int)(speed*20), car_angle);

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }
    return 0;
}
