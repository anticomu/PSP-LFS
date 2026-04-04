#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("LFS_PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[262144];

typedef enum { STATE_MENU, STATE_CAR_SELECT, STATE_RACING } GameState;
GameState current_state = STATE_MENU;

typedef struct { float x, y, z; unsigned int color; } Vertex;
typedef struct { char name[32]; float accel; unsigned int color; } CarData;

CarData car_db[] = {
    {"XF GTI", 0.05f, 0xFF0000FF}, // Rosso
    {"XR GT",  0.08f, 0xFF00FF00}, // Verde
    {"RB4 GT", 0.12f, 0xFFFF0000}  // Blu
};

struct { float x, z, angle, speed; int car_idx; } player = {0, -10, 0, 0, 0};

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

void draw_cube(unsigned int color) {
    Vertex* vertices = (Vertex*)sceGuGetMemory(36 * sizeof(Vertex));
    // Dati cubo semplificati per test
    for(int i=0; i<36; i++) { vertices[i].color = color; }
    sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D, 36, 0, vertices);
}

int main() {
    setup_callbacks();
    pspDebugScreenInit();
    init_graphics();

    SceCtrlData pad;
    int menu_idx = 0, last_btn = 0;

    while(1) {
        sceCtrlReadBufferPositive(&pad, 1);
        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xff222222);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        if (current_state == STATE_MENU) {
            pspDebugScreenSetXY(15, 5); pspDebugScreenPrintf("--- LIVE FOR SPEED PSP ---");
            pspDebugScreenSetXY(18, 10); pspDebugScreenPrintf(menu_idx==0?"> GARA":"  GARA");
            pspDebugScreenSetXY(18, 11); pspDebugScreenPrintf(menu_idx==1?"> AUTO":"  AUTO");
            if((pad.Buttons & PSP_CTRL_DOWN) && !(last_btn & PSP_CTRL_DOWN)) menu_idx++;
            if((pad.Buttons & PSP_CTRL_UP) && !(last_btn & PSP_CTRL_UP)) menu_idx--;
            if(menu_idx > 1) menu_idx = 0; if(menu_idx < 0) menu_idx = 1;
            if((pad.Buttons & PSP_CTRL_CROSS) && !(last_btn & PSP_CTRL_CROSS)) {
                if(menu_idx==0) current_state = STATE_RACING;
                else current_state = STATE_CAR_SELECT;
            }
        } 
        else if (current_state == STATE_CAR_SELECT) {
            pspDebugScreenSetXY(15, 5); pspDebugScreenPrintf("--- SELEZIONA AUTO ---");
            for(int i=0; i<3; i++) {
                pspDebugScreenSetXY(18, 10+i);
                pspDebugScreenPrintf(player.car_idx==i?"> %s":"  %s", car_db[i].name);
            }
            if((pad.Buttons & PSP_CTRL_DOWN) && !(last_btn & PSP_CTRL_DOWN)) player.car_idx++;
            if((pad.Buttons & PSP_CTRL_UP) && !(last_btn & PSP_CTRL_UP)) player.car_idx--;
            if(player.car_idx > 2) player.car_idx = 0; if(player.car_idx < 0) player.car_idx = 2;
            if((pad.Buttons & PSP_CTRL_CIRCLE)) current_state = STATE_MENU;
        }
        else if (current_state == STATE_RACING) {
            if(pad.Buttons & PSP_CTRL_CROSS) player.speed += car_db[player.car_idx].accel;
            if(pad.Lx < 80) player.angle -= 0.05f; if(pad.Lx > 170) player.angle += 0.05f;
            player.speed *= 0.97f;
            player.x += sinf(player.angle) * player.speed;
            player.z += cosf(player.angle) * player.speed;

            sceGumMatrixMode(GU_PROJECTION); sceGumLoadIdentity(); sceGumPerspective(75.0f, 480.0f/272.0f, 0.5f, 1000.0f);
            sceGumMatrixMode(GU_VIEW); sceGumLoadIdentity();
            ScePspFVector3 pos = { player.x - sinf(player.angle)*5, 2.0f, player.z - cosf(player.angle)*5 };
            ScePspFVector3 look = { player.x, 0, player.z };
            ScePspFVector3 up = { 0, 1, 0 };
            sceGumLookAt(&pos, &look, &up);

            sceGumMatrixMode(GU_MODEL); sceGumLoadIdentity();
            ScePspFVector3 car_pos = { player.x, 0, player.z };
            sceGumTranslate(&car_pos);
            sceGumRotateY(player.angle);
            draw_cube(car_db[player.car_idx].color);

            pspDebugScreenSetXY(2, 2); pspDebugScreenPrintf("KM/H: %d", (int)(player.speed*200));
            if(pad.Buttons & PSP_CTRL_SELECT) current_state = STATE_MENU;
        }

        last_btn = pad.Buttons;
        sceGuFinish(); sceGuSync(0, 0);
        sceDisplayWaitVblankStart(); sceGuSwapBuffers();
    }
    return 0;
}
