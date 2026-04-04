#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <math.h>
#include <stdio.h>

PSP_MODULE_INFO("LFS_PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[262144];

// --- GESTIONE CALLBACK PSP (Obbligatoria) ---
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
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
    return thid;
}

// --- STRUTTURA VERTICI ---
typedef struct {
    float x, y, z;
} Vertex;

// Simulazione di un modello 3D (un piano per la pista e un'auto semplificata)
Vertex pista_vertices[] = {
    {-50, 0, -50}, {50, 0, -50}, {-50, 0, 50}, {50, 0, 50}
};

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
    sceGuEnable(GU_CULL_FACE);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

int main() {
    setup_callbacks();
    pspDebugScreenInit();
    init_graphics();

    float car_x = 0, car_z = -10, car_angle = 0;
    SceCtrlData pad;

    while(1) {
        sceCtrlReadBufferPositive(&pad, 1);
        
        // Comandi base
        if (pad.Buttons & PSP_CTRL_CROSS) {
            car_x += sinf(car_angle) * 0.5f;
            car_z += cosf(car_angle) * 0.5f;
        }
        if (pad.Lx < 80) car_angle -= 0.05f;
        if (pad.Lx > 170) car_angle += 0.05f;

        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xff333333); // Grigio asfalto
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        // Telecamera
        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(75.0f, 480.0f/272.0f, 0.5f, 1000.0f);

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        ScePspFVector3 cam_pos = { car_x - sinf(car_angle)*10, 5.0f, car_z - cosf(car_angle)*10 };
        ScePspFVector3 cam_look = { car_x, 0, car_z };
        ScePspFVector3 cam_up = { 0, 1, 0 };
        sceGumLookAt(&cam_pos, &cam_look, &cam_up);

        // Disegna Pista (Griglia)
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
        sceGuColor(0xffaaaaaa);
        sceGuDrawArray(GU_TRIANGLE_STRIP, GU_VERTEX_32BITF|GU_TRANSFORM_3D, 4, 0, pista_vertices);

        sceGuFinish();
        sceGuSync(0, 0);

        pspDebugScreenSetXY(2, 2);
        pspDebugScreenPrintf("LFS PSP - 3D ENGINE READY");
        pspDebugScreenSetXY(2, 4);
        pspDebugScreenPrintf("X: %.2f Z: %.2f", car_x, car_z);

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }
    return 0;
}
