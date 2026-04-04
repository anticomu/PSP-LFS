#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("LFS_PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[262144];

// --- STATI DEL GIOCO ---
typedef enum {
    STATE_MENU,
    STATE_CAR_SELECT,
    STATE_SETTINGS,
    STATE_RACING
} GameState;

GameState current_state = STATE_MENU;

// --- STRUTTURE DATI ---
typedef struct {
    char name[32];
    float max_speed;
    float acceleration;
    float handling;
    unsigned int color;
} CarData;

CarData car_db[] = {
    {"XF GTI", 160.0f, 0.15f, 0.95f, 0xFF0000FF}, // Rosso
    {"XR GT",  210.0f, 0.22f, 0.90f, 0xFF00FF00}, // Verde
    {"RB4 GT", 240.0f, 0.30f, 0.85f, 0xFFFF0000}  // Blu
};

typedef struct {
    int abs_enabled;
    int traction_control;
    int auto_gear;
    int graphics_detail; // 0: Low, 1: High
    int volume;
} GameSettings;

GameSettings settings = {1, 1, 1, 1, 80};

typedef struct {
    float x, z, angle, speed;
    int selected_car_idx;
} Player;

Player player = {0.0f, -10.0f, 0.0f, 0.0f, 0};

// --- GESTIONE CALLBACK PSP ---
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

// --- FUNZIONI DI RENDERING E LOGICA ---
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

void draw_menu(SceCtrlData *pad, int *selected_option, int max_options) {
    static int last_buttons = 0;
    if ((pad->Buttons & PSP_CTRL_DOWN) && !(last_buttons & PSP_CTRL_DOWN)) (*selected_option)++;
    if ((pad->Buttons & PSP_CTRL_UP) && !(last_buttons & PSP_CTRL_UP)) (*selected_option)--;
    if (*selected_option < 0) *selected_option = max_options - 1;
    if (*selected_option >= max_options) *selected_option = 0;
    last_buttons = pad->Buttons;

    pspDebugScreenSetXY(15, 5);
    pspDebugScreenPrintf("--- LIVE FOR SPEED PSP ---");
}

int main() {
    setup_callbacks();
    pspDebugScreenInit();
    init_graphics();

    SceCtrlData pad;
    int menu_idx = 0;
    int last_btn = 0;

    while(1) {
        sceCtrlReadBufferPositive(&pad, 1);
        
        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xff222222);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
        
        switch(current_state) {
            case STATE_MENU:
                draw_menu(&pad, &menu_idx, 3);
                pspDebugScreenSetXY(18, 10);
                pspDebugScreenPrintf(menu_idx == 0 ? "> INIZIA GARA" : "  INIZIA GARA");
                pspDebugScreenSetXY(18, 11);
                pspDebugScreenPrintf(menu_idx == 1 ? "> SELEZIONE AUTO" : "  SELEZIONE AUTO");
                pspDebugScreenSetXY(18, 12);
                pspDebugScreenPrintf(menu_idx == 2 ? "> IMPOSTAZIONI" : "  IMPOSTAZIONI");

                if ((pad.Buttons & PSP_CTRL_CROSS) && !(last_btn & PSP_CTRL_CROSS)) {
                    if (menu_idx == 0) current_state = STATE_RACING;
                    if (menu_idx == 1) current_state = STATE_CAR_SELECT;
                    if (menu_idx == 2) current_state = STATE_SETTINGS;
                }
                break;

            case STATE_CAR_SELECT:
                pspDebugScreenSetXY(15, 5);
                pspDebugScreenPrintf("--- SELEZIONE AUTO ---");
                for(int i=0; i<3; i++) {
                    pspDebugScreenSetXY(18, 10+i);
                    pspDebugScreenPrintf(player.selected_car_idx == i ? "> %s" : "  %s", car_db[i].name);
                }
                if ((pad.Buttons & PSP_CTRL_DOWN) && !(last_btn & PSP_CTRL_DOWN)) player.selected_car_idx++;
                if ((pad.Buttons & PSP_CTRL_UP) && !(last_btn & PSP_btn & PSP_CTRL_UP)) player.selected_car_idx--;
                if (player.selected_car_idx > 2) player.selected_car_idx = 0;
                if (player.selected_car_idx < 0) player.selected_car_idx = 2;
                
                if ((pad.Buttons & PSP_CTRL_CIRCLE) && !(last_btn & PSP_CTRL_CIRCLE)) current_state = STATE_MENU;
                break;

            case STATE_SETTINGS:
                pspDebugScreenSetXY(15, 5);
                pspDebugScreenPrintf("--- IMPOSTAZIONI ---");
                pspDebugScreenSetXY(10, 10);
                pspDebugScreenPrintf("ABS: %s", settings.abs_enabled ? "[ON]" : "[OFF]");
                pspDebugScreenSetXY(10, 11);
                pspDebugScreenPrintf("TRACTION CONTROL: %s", settings.traction_control ? "[ON]" : "[OFF]");
                pspDebugScreenSetXY(10, 12);
                pspDebugScreenPrintf("CAMBIO AUTO: %s", settings.auto_gear ? "[ON]" : "[OFF]");
                pspDebugScreenSetXY(10, 14);
                pspDebugScreenPrintf("(O) TORNA AL MENU");
                
                if ((pad.Buttons & PSP_CTRL_CIRCLE) && !(last_btn & PSP_CTRL_CIRCLE)) current_state = STATE_MENU;
                break;

            case STATE_RACING:
                // Fisica
                if (pad.Buttons & PSP_CTRL_CROSS) player.speed += car_db[player.selected_car_idx].acceleration;
                player.speed *= car_db[player.selected_car_idx].handling;
                player.z += player.speed;

                pspDebugScreenSetXY(2, 2);
                pspDebugScreenPrintf("AUTO: %s", car_db[player.selected_car_idx].name);
                pspDebugScreenSetXY(2, 4);
                pspDebugScreenPrintf("VELOCITA': %d KM/H", (int)(player.speed * 100));
                
                if (pad.Buttons & PSP_CTRL_SELECT) current_state = STATE_MENU;
                break;
        }

        last_btn = pad.Buttons;
        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }
    return 0;
}

