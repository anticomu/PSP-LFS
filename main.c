#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspctrl.h>
#include <math.h>

PSP_MODULE_INFO("LFS_STABLE", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

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

int main() {
    setup_callbacks();
    pspDebugScreenInit();

    // Inizializzazione Grafica
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
    sceGuDispBuffer(480, 272, (void*)0x88000, 512);
    sceGuDisplay(GU_TRUE);
    sceGuFinish();
    sceGuSync(0, 0);

    float speed = 0.0f;
    float rpm = 1000.0f;
    SceCtrlData pad;

    while(1) {
        sceCtrlReadBufferPositive(&pad, 1);

        // Fisica Auto
        if (pad.Buttons & PSP_CTRL_CROSS) speed += 0.05f; // Accelera
        speed *= 0.98f; // Attrito
        rpm = 1000.0f + (speed * 50.0f);

        // Rendering
        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xff444444); // Asfalto
        sceGuClear(GU_COLOR_BUFFER_BIT);
        sceGuFinish();
        sceGuSync(0, 0);

        // Interfaccia (HUD)
        pspDebugScreenSetXY(2, 2);
        pspDebugScreenPrintf("LFS PSP - ENGINE RUNNING");
        pspDebugScreenSetXY(2, 4);
        pspDebugScreenPrintf("VELOCITA': %d KM/H", (int)(speed * 10));
        pspDebugScreenSetXY(2, 5);
        pspDebugScreenPrintf("MOTORE:    %d RPM", (int)rpm);
        
        if (pad.Buttons & PSP_CTRL_SELECT) break;

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    sceKernelExitGame();
    return 0;
}
