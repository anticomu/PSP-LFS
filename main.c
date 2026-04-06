#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

PSP_MODULE_INFO("LFS_PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[262144];

typedef struct {
    unsigned int color;
    float x, y, z;
} Vertex;

Vertex* car_vertices = NULL;
int vertex_count = 0;

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

// --- OBJ LOADER ---
void load_obj(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return;
    char line[256];
    int total_v = 0, total_f = 0;
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') total_v++;
        else if (line[0] == 'f') total_f++;
    }
    Vertex* temp_v = (Vertex*)malloc(total_v * sizeof(Vertex));
    fseek(file, 0, SEEK_SET);
    int v_idx = 0;
    float min_x = 1e6, max_x = -1e6, min_y = 1e6, max_y = -1e6, min_z = 1e6, max_z = -1e6;
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') {
            temp_v[v_idx].color = 0xFFFFFFFF;
            sscanf(line, "v %f %f %f", &temp_v[v_idx].x, &temp_v[v_idx].y, &temp_v[v_idx].z);
            if(temp_v[v_idx].x < min_x) min_x = temp_v[v_idx].x; if(temp_v[v_idx].x > max_x) max_x = temp_v[v_idx].x;
            if(temp_v[v_idx].y < min_y) min_y = temp_v[v_idx].y; if(temp_v[v_idx].y > max_y) max_y = temp_v[v_idx].y;
            if(temp_v[v_idx].z < min_z) min_z = temp_v[v_idx].z; if(temp_v[v_idx].z > max_z) max_z = temp_v[v_idx].z;
            v_idx++;
        }
    }
    float center_x = (min_x + max_x) / 2.0f, center_y = min_y, center_z = (min_z + max_z) / 2.0f;
    float max_dim = (max_x-min_x > max_z-min_z) ? (max_x-min_x) : (max_z-min_z);
    float scale = 3.0f / max_dim;
    for(int i=0; i<total_v; i++) {
        temp_v[i].x = (temp_v[i].x - center_x) * scale;
        temp_v[i].y = (temp_v[i].y - center_y) * scale;
        temp_v[i].z = (temp_v[i].z - center_z) * scale;
    }
    vertex_count = total_f * 3;
    car_vertices = (Vertex*)memalign(16, vertex_count * sizeof(Vertex));
    fseek(file, 0, SEEK_SET);
    int f_idx = 0;
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'f') {
            int v1, v2, v3;
            if (sscanf(line, "f %d %d %d", &v1, &v2, &v3) == 3) {
                car_vertices[f_idx++] = temp_v[v1-1]; car_vertices[f_idx++] = temp_v[v2-1]; car_vertices[f_idx++] = temp_v[v3-1];
            }
        }
    }
    free(temp_v); fclose(file);
}

int main() {
    setup_callbacks();
    load_obj("car.obj");
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
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    float px = 0, pz = 0, angle = 0, speed = 0;
    SceCtrlData pad;
    while(1) {
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_CROSS) speed += 0.05f;
        if (pad.Buttons & PSP_CTRL_SQUARE) speed -= 0.08f;
        if (pad.Lx < 80) angle -= 0.05f;
        if (pad.Lx > 170) angle += 0.05f;
        speed *= 0.96f;
        px += sinf(angle) * speed; pz += cosf(angle) * speed;

        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xFF66BBFF); // Cielo azzurro
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
        sceGumMatrixMode(GU_PROJECTION); sceGumLoadIdentity(); sceGumPerspective(75.0f, 480.0f/272.0f, 0.5f, 1000.0f);
        sceGumMatrixMode(GU_VIEW); sceGumLoadIdentity();
        ScePspFVector3 cam_p = { px - sinf(angle)*10, 5.0f, pz - cosf(angle)*10 };
        ScePspFVector3 cam_l = { px, 1.0f, pz };
        ScePspFVector3 cam_u = { 0, 1, 0 };
        sceGumLookAt(&cam_p, &cam_l, &cam_u);

        // Erba (Verde)
        sceGumMatrixMode(GU_MODEL); sceGumLoadIdentity();
        sceGuColor(0xFF008800);
        Vertex ground[6] = { {0, -500,0,-500}, {0, 500,0,-500}, {0, 500,0, 500}, {0, 500,0, 500}, {0, -500,0, 500}, {0, -500,0,-500} };
        sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D, 6, 0, ground);

        // Strada (Grigia)
        sceGuColor(0xFF444444);
        Vertex road[6] = { {0, -15,0.01f,-500}, {0, 15,0.01f,-500}, {0, 15,0.01f, 500}, {0, 15,0.01f, 500}, {0, -15,0.01f, 500}, {0, -15,0.01f,-500} };
        sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D, 6, 0, road);

        // Auto (Bianca)
        if (car_vertices) {
            sceGumMatrixMode(GU_MODEL); sceGumLoadIdentity();
            ScePspFVector3 p = { px, 0.1f, pz }; sceGumTranslate(&p); sceGumRotateY(angle);
            sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D, vertex_count, 0, car_vertices);
        }
        sceGuFinish(); sceGuSync(0, 0); sceDisplayWaitVblankStart(); sceGuSwapBuffers();
    }
    return 0;
}
