#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

PSP_MODULE_INFO("LFS_EMERGENCY", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[262144];

typedef struct {
    unsigned int color;
    float x, y, z;
} Vertex;

// Triangolo 2D che DEVE apparire in alto a sinistra
Vertex test_2d[3] __attribute__((aligned(16))) = {
    { 0xFF0000FF, 10, 10, 0 },
    { 0xFF00FF00, 200, 10, 0 },
    { 0xFFFF0000, 100, 150, 0 }
};

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

int main() {
    setup_callbacks();
    
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
    sceGuDispBuffer(480, 272, (void*)0x88000, 512);
    sceGuOffset(2048 - (480/2), 2048 - (272/2));
    sceGuViewport(2048, 2048, 480, 272);
    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    while(1) {
        sceGuStart(GU_DIRECT, list);
        // SFONDO ROSSO PER CAPIRE SE IL CODICE E' NUOVO
        sceGuClearColor(0xFF0000FF); 
        sceGuClear(GU_COLOR_BUFFER_BIT);

        // DISEGNA TRIANGOLO 2D (Senza telecamera, senza 3D)
        sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D, 3, 0, test_2d);

        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }
    return 0;
}
