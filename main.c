#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <math.h>
#include <stdio.h>

PSP_MODULE_INFO("LFS_PSP_COMPLETE", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

// --- Costanti e Definizioni ---
#define BUF_WIDTH 512
#define SCR_WIDTH 480
#define SCR_HEIGHT 272
#define PI 3.14159265f

static unsigned int __attribute__((aligned(16))) list[262144];

// --- Strutture del Gioco ---
typedef struct {
    float x, y, z;
    float angle;
    float speed;
    float rpm;
    int gear; // -1: R, 0: N, 1-6: Gears
    float steer_angle;
    float throttle;
    float brake;
} Car;

typedef struct {
    float x, y, z;
    unsigned int color;
} Vertex;

// --- Stato del Gioco ---
Car lfs_car = {0.0f, 0.0f, -10.0f, 0.0f, 0.0f, 1000.0f, 0, 0.0f, 0.0f, 0.0f};
float gear_ratios[] = {-3.5f, 0.0f, 3.8f, 2.4f, 1.8f, 1.4f, 1.1f, 0.9f}; // R, N, 1, 2, 3, 4, 5, 6
float final_drive = 4.1f;

// --- Funzioni di Supporto ---
void init_graphics() {
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
    sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)0x88000, BUF_WIDTH);
    sceGuDepthBuffer((void*)0x110000, BUF_WIDTH);
    sceGuOffset(2048 - (SCR_WIDTH/2), 2048 - (SCR_HEIGHT/2));
    sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuShadeModel(GU_SMOOTH);
    sceGuFrontFace(GU_CCW)
    sceGuEnable(GU_CULL_FACE);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void update_physics(SceCtrlData *pad) {
    // 1. Input - Acceleratore e Freno
    lfs_car.throttle = (pad->Buttons & PSP_CTRL_CROSS) ? 1.0f : 0.0f;
    lfs_car.brake = (pad->Buttons & PSP_CTRL_SQUARE) ? 1.0f : 0.0f;

    // 2. Cambio Marce (Triangolo = Su, Cerchio = Giu')
    static int last_buttons = 0;
    if ((pad->Buttons & PSP_CTRL_TRIANGLE) && !(last_buttons & PSP_CTRL_TRIANGLE)) {
        if (lfs_car.gear < 6) lfs_car.gear++;
    }
    if ((pad->Buttons & PSP_CTRL_CIRCLE) && !(last_buttons & PSP_CTRL_CIRCLE)) {
        if (lfs_car.gear > -1) lfs_car.gear--;
    }
    last_buttons = pad->Buttons;

    // 3. Sterzo (Levetta)
    float target_steer = (pad->Lx - 128) / 128.0f;
    lfs_car.steer_angle = target_steer * 0.5f; // Max 30 gradi circa

    // 4. Motore e RPM
    if (lfs_car.gear != 0) {
        float wheel_speed = lfs_car.speed * 100.0f; // Semplificazione
        lfs_car.rpm = 1000.0f + (fabsf(wheel_speed) * fabsf(gear_ratios[lfs_car.gear + 1]) * final_drive);
        if (lfs_car.rpm > 8000.0f) lfs_car.rpm = 8000.0f;
        
        float torque = (lfs_car.throttle * 500.0f) - (lfs_car.brake * 1000.0f);
        float force = torque * gear_ratios[lfs_car.gear + 1] * final_drive;
        lfs_car.speed += force * 0.0001f;
    } else {
        lfs_car.rpm = 1000.0f + (lfs_car.throttle * 6000.0f);
    }

    // 5. Attrito e Movimento
    lfs_car.speed *= 0.99f; // Attrito aria/rotolamento
    lfs_car.angle += lfs_car.steer_angle * (lfs_car.speed * 0.1f);
    
    lfs_car.x += sinf(lfs_car.angle) * lfs_car.speed;
    lfs_car.z += cosf(lfs_car.angle) * lfs_car.speed;
}

void draw_ui() {
    pspDebugScreenSetXY(2, 2);
    pspDebugScreenPrintf("LFS PSP - PROTO TYPE");
    pspDebugScreenSetXY(2, 4);
    pspDebugScreenPrintf("SPEED: %d KM/H", (int)(fabsf(lfs_car.speed) * 360));
    pspDebugScreenSetXY(2, 5);
    const char* gear_name = (lfs_car.gear == -1) ? "R" : (lfs_car.gear == 0) ? "N" : "123456" + (lfs_car.gear - 1);
    if (lfs_car.gear > 0) {
        char g[2] = { '0' + lfs_car.gear, '\0' };
        pspDebugScreenPrintf("GEAR:  %s", g);
    } else {
        pspDebugScreenPrintf("GEAR:  %s", gear_name);
    }
    pspDebugScreenSetXY(2, 6);
    pspDebugScreenPrintf("RPM:   %d", (int)lfs_car.rpm);
}

int main() {
    pspDebugScreenInit();
    init_graphics();
    SceCtrlData pad;

    while(1) {
        sceCtrlReadBufferPositive(&pad, 1);
        update_physics(&pad);

        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xff444444); // Grigio Asfalto
        sceGuClearDepth(0);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        // [Rendering 3D della pista e auto andrebbe qui]
        
        sceGuFinish();
        sceGuSync(0, 0);
        
        draw_ui();

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    return 0;
}
