#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <math.h>

PSP_MODULE_INFO("LFS_PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[262144];

// --- CALLBACKS DI SISTEMA ---
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

// --- STRUTTURA VERTICI ---
typedef struct {
    float u, v;
    unsigned int color;
    float x, y, z;
} Vertex;

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
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

int main() {
    setup_callbacks();
    pspDebugScreenInit();
    init_graphics();

    while(1) {
        sceGuStart(GU_DIRECT, list);
        
        // Sfondo Blu (Stile Menu LFS)
        sceGuClearColor(0xFF884400); 
        sceGuClear(GU_COLOR_BUFFER_BIT);

        sceGuFinish();
        sceGuSync(0, 0);

        // Test Testo (Sopra il 3D)
        pspDebugScreenSetXY(10, 10);
        pspDebugScreenPrintf("LIVE FOR SPEED PSP - CARICATO!");
        pspDebugScreenSetXY(10, 12);
        pspDebugScreenPrintf("PREMI IL TASTO HOME PER USCIRE");

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }
    return 0;
}
