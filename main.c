#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspdebug.h>

PSP_MODULE_INFO("LFS_PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

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

    sceGuInit();
    sceGuStart(GU_DIRECT, (void*)0);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
    sceGuDispBuffer(480, 272, (void*)0x88000, 512);
    sceGuDisplay(GU_TRUE);
    sceGuFinish();
    sceGuSync(0, 0);

    while(1) {
        sceGuStart(GU_DIRECT, (void*)0);
        sceGuClearColor(0xff222222);
        sceGuClear(GU_COLOR_BUFFER_BIT);
        
        pspDebugScreenSetXY(15, 10);
        pspDebugScreenPrintf("LIVE FOR SPEED PSP - ISO OK!");
        
        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }
    return 0;
}
