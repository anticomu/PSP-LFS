#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

PSP_MODULE_INFO("LFS_TEST", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[262144];

typedef struct {
    unsigned int color;
    float x, y, z;
} Vertex;

Vertex* car_vertices = NULL;
int vertex_count = 0;

// Cubo colorato super-visibile
Vertex cube_v[] __attribute__((aligned(16))) = {
    {0xFF0000FF, -1,-1, 1}, {0xFF00FF00,  1,-1, 1}, {0xFFFF0000,  1, 1, 1}, {0xFFFFFFFF, -1, 1, 1},
    {0xFF0000FF, -1,-1,-1}, {0xFF00FF00, -1, 1,-1}, {0xFFFF0000,  1, 1,-1}, {0xFFFFFFFF,  1,-1,-1},
};
unsigned short cube_i[] = { 0,1,2, 2,3,0, 1,7,6, 6,2,1, 7,4,5, 5,6,7, 4,0,3, 3,5,4, 3,2,6, 6,5,3, 4,7,1, 1,0,4 };
Vertex car_cube[36] __attribute__((aligned(16)));

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

void load_obj(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return;
    // Caricamento semplificato per il test
    char line[256];
    int total_v = 0, total_f = 0;
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') total_v++;
        else if (line[0] == 'f') total_f++;
    }
    if (total_v > 5000) total_v = 5000;
    Vertex* temp_v = malloc(total_v * sizeof(Vertex));
    fseek(file, 0, SEEK_SET);
    int v_idx = 0;
    while (fgets(line, sizeof(line), file) && v_idx < total_v) {
        if (line[0] == 'v' && line[1] == ' ') {
            temp_v[v_idx].color = 0xFFFFFFFF;
            sscanf(line, "v %f %f %f", &temp_v[v_idx].x, &temp_v[v_idx].y, &temp_v[v_idx].z);
            v_idx++;
        }
    }
    vertex_count = (total_f > 2000 ? 2000 : total_f) * 3;
    car_vertices = memalign(16, vertex_count * sizeof(Vertex));
    fseek(file, 0, SEEK_SET);
    int f_idx = 0;
    while (fgets(line, sizeof(line), file) && f_idx < vertex_count) {
        if (line[0] == 'f') {
            int v1, v2, v3;
            sscanf(line, "f %d %d %d", &v1, &v2, &v3);
            if (v1 <= total_v && v2 <= total_v && v3 <= total_v) {
                car_vertices[f_idx++] = temp_v[v1-1];
                car_vertices[f_idx++] = temp_v[v2-1];
                car_vertices[f_idx++] = temp_v[v3-1];
            }
        }
    }
    free(temp_v);
    fclose(file);
}

void init_graphics() {
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
    sceGuDispBuffer(480, 272, (void*)0x88000, 512);
    sceGuDepthBuffer((void*)0x110000, 512);
    sceGuOffset(2048 - (480/2), 2048 - (272/2));
    sceGuViewport(2048, 2048, 480, 272);
    sceGuDepthRange(0, 65535);
    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuShadeModel(GU_SMOOTH);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

int main() {
    setup_callbacks();
    for(int i=0; i<36; i++) car_cube[i] = cube_v[cube_i[i]];
    load_obj("car.obj");
    init_graphics();

    while(1) {
        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xFF443322);
        sceGuClear(GU_COLOR_BUFFER_BIT);

        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(75.0f, 480.0f/272.0f, 0.1f, 1000.0f);

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        ScePspFVector3 cam_pos = { 0, 5, -15 };
        ScePspFVector3 cam_look = { 0, 0, 0 };
        ScePspFVector3 cam_up = { 0, 1, 0 };
        sceGumLookAt(&cam_pos, &cam_look, &cam_up);

        // Cubo Test
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
        static float rot = 0; rot += 0.05f;
        sceGumRotateY(rot);
        sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D, 36, 0, car_cube);

        // Auto
        if (car_vertices) {
            sceGumMatrixMode(GU_MODEL);
            sceGumLoadIdentity();
            ScePspFVector3 p = { -5, 0, 0 };
            sceGumTranslate(&p);
            sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D, vertex_count, 0, car_vertices);
        }

        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }
    return 0;
}
