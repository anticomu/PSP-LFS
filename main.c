#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <math.h>

PSP_MODULE_INFO("LFS_PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[262144];

// --- GESTIONE USCITA (OBBLIGATORIA) ---
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

// --- LOGICA AUTO ---
typedef struct {
    float x, z, angle, speed;
} Car;

Car lfs_car = {0.0f, -10.0f, 0.0f, 0.0f};

int main() {
    setup_callbacks(); // <--- QUESTO SBLOCCA LO SCHERMO NERO!
    
    pspDebugScreenInit();
    
    // Inizializzazione Grafica minima
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
    sceGuDispBuffer(480, 272, (void*)0x88000, 512);
    sceGuDisplay(GU_TRUE);
    sceGuFinish();
    sceGuSync(0, 0);

    SceCtrlData pad;

    while(1) {
        sceCtrlReadBufferPositive(&pad, 1);

        // Fisica ultra-semplice per test
        if (pad.Buttons & PSP_CTRL_CROSS) lfs_car.speed += 0.05f;
        lfs_car.speed *= 0.95f;
        lfs_car.z += lfs_car.speed;

        // Inizio disegno
        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xff444444); // Grigio LFS
        sceGuClear(GU_COLOR_BUFFER_BIT);
        
        // Debug testo
        pspDebugScreenSetXY(2, 2);
        pspDebugScreenPrintf("LIVE FOR SPEED PSP - TEST OK!");
        pspDebugScreenSetXY(2, 4);
        pspDebugScreenPrintf("VELOCITA': %d", (int)lfs_car.speed);
        
        sceGuFinish();
        sceGuSync(0, 0);
        
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    return 0;
}
