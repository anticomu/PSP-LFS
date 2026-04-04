#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspgu.h>

PSP_MODULE_INFO("LFS_PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

// --- CALLBACK DI SISTEMA (Indispensabili per PPSSPP) ---
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

// --- MAIN ---
int main() {
    setup_callbacks();
    pspDebugScreenInit();

    // Inizializzazione Video Minima
    sceGuInit();
    sceGuStart(GU_DIRECT, (void*)0); // Non usiamo liste per questo test
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
    sceGuDispBuffer(480, 272, (void*)0x88000, 512);
    sceGuDisplay(GU_TRUE);
    sceGuFinish();
    sceGuSync(0, 0);

    while(1) {
        // Pulizia schermo
        sceGuStart(GU_DIRECT, (void*)0);
        sceGuClearColor(0xff444444); // Grigio scuro
        sceGuClear(GU_COLOR_BUFFER_BIT);
        sceGuFinish();
        sceGuSync(0, 0);

        // Scritta di test
        pspDebugScreenSetXY(2, 2);
        pspDebugScreenPrintf("--- LIVE FOR SPEED PSP ---");
        pspDebugScreenSetXY(2, 4);
        pspDebugScreenPrintf("STATO: FUNZIONANTE (FPS OK)");

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    return 0;
}
