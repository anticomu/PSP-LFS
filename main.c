#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

PSP_MODULE_INFO("LFS_PORSCHE", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[262144];

typedef struct {
    float x, y, z;
} Vertex;

Vertex* car_vertices = NULL;
int vertex_count = 0;

// Cubo di test se l'OBJ fallisce
Vertex cube_vertices[] = {
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}, // Front
    {-1,-1,-1}, {-1, 1,-1}, { 1, 1,-1}, { 1,-1,-1}, // Back
};
unsigned short cube_indices[] = {
    0,1,2, 2,3,0, 1,7,6, 6,2,1, 7,4,5, 5,6,7, 4,0,3, 3,5,4, 3,2,6, 6,5,3, 4,7,1, 1,0,4
};
Vertex car_cube[36]; // Triangoli del cubo

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
    pspDebugScreenPrintf("Apertura file: %s...\n", filename);
    FILE* file = fopen(filename, "r");
    if (!file) {
        pspDebugScreenPrintf("ERRORE: File non trovato!\n");
        return;
    }

    char line[256];
    int total_v = 0;
    int total_f = 0;
    
    // First pass: count vertices and faces (including quads)
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') total_v++;
        else if (line[0] == 'f' && line[1] == ' ') {
            int spaces = 0;
            for(int i=2; line[i]; i++) if(line[i] == ' ') spaces++;
            if (spaces >= 3) total_f += 2;
            else total_f += 1;
        }
    }

    pspDebugScreenPrintf("Trovati: %d vertici, %d triangoli potenziali.\n", total_v, total_f);

    if (total_v == 0 || total_f == 0) { 
        pspDebugScreenPrintf("ERRORE: Modello vuoto o non valido.\n");
        fclose(file); 
        return; 
    }

    // SAFE MODE: Limite ancora più stretto per PPSSPP/PSP (stabilità massima)
    if (total_v > 15000) {
        pspDebugScreenPrintf("OTTIMIZZAZIONE: Riduzione a 15k vertici per stabilità.\n");
        total_v = 15000;
    }

    Vertex* temp_v = (Vertex*)malloc(total_v * sizeof(Vertex));
    if (!temp_v) {
        pspDebugScreenPrintf("ERRORE: Memoria RAM insufficiente per i vertici!\n");
        fclose(file);
        return;
    }

    fseek(file, 0, SEEK_SET);

    float min_x = 1e6, max_x = -1e6, min_y = 1e6, max_y = -1e6, min_z = 1e6, max_z = -1e6;

    int v_idx = 0;
    while (fgets(line, sizeof(line), file) && v_idx < total_v) {
        if (line[0] == 'v' && line[1] == ' ') {
            sscanf(line, "v %f %f %f", &temp_v[v_idx].x, &temp_v[v_idx].y, &temp_v[v_idx].z);
            if(temp_v[v_idx].x < min_x) min_x = temp_v[v_idx].x;
            if(temp_v[v_idx].x > max_x) max_x = temp_v[v_idx].x;
            if(temp_v[v_idx].y < min_y) min_y = temp_v[v_idx].y;
            if(temp_v[v_idx].y > max_y) max_y = temp_v[v_idx].y;
            if(temp_v[v_idx].z < min_z) min_z = temp_v[v_idx].z;
            if(temp_v[v_idx].z > max_z) max_z = temp_v[v_idx].z;
            v_idx++;
        }
    }

    float center_x = (min_x + max_x) / 2.0f;
    float center_y = min_y;
    float center_z = (min_z + max_z) / 2.0f;
    float size_x = max_x - min_x;
    float size_y = max_y - min_y;
    float size_z = max_z - min_z;
    float max_size = (size_x > size_y) ? (size_x > size_z ? size_x : size_z) : (size_y > size_z ? size_y : size_z);
    float scale = 4.0f / max_size;

    for(int i=0; i<v_idx; i++) {
        temp_v[i].x = (temp_v[i].x - center_x) * scale;
        temp_v[i].y = (temp_v[i].y - center_y) * scale;
        temp_v[i].z = (temp_v[i].z - center_z) * scale;
    }

    // Limite triangoli per stabilità
    int max_triangles = 20000; 
    if (total_f > max_triangles) total_f = max_triangles;

    vertex_count = total_f * 3;
    car_vertices = (Vertex*)malloc(vertex_count * sizeof(Vertex));
    if (!car_vertices) {
        pspDebugScreenPrintf("ERRORE: Memoria RAM insufficiente per i triangoli!\n");
        free(temp_v);
        fclose(file);
        return;
    }

    fseek(file, 0, SEEK_SET);

    int f_idx = 0;
    int faces_processed = 0;
    while (fgets(line, sizeof(line), file) && faces_processed < total_f) {
        if (line[0] == 'f' && line[1] == ' ') {
            int v_indices[4];
            int found = 0;
            char* ptr = line + 2;
            
            while(*ptr && found < 4) {
                while(*ptr == ' ') ptr++;
                if(!*ptr || *ptr == '\n' || *ptr == '\r') break;
                v_indices[found++] = atoi(ptr);
                while(*ptr && *ptr != ' ') ptr++;
            }
            
            if (found >= 3) {
                for(int j=0; j<3; j++) {
                    int idx = v_indices[j];
                    if (idx < 0) idx = total_v + idx + 1;
                    if (idx > 0 && idx <= total_v) car_vertices[f_idx++] = temp_v[idx-1];
                }
                faces_processed++;
                if (found == 4 && faces_processed < total_f) {
                    int quad_idx[3] = {0, 2, 3};
                    for(int j=0; j<3; j++) {
                        int idx = v_indices[quad_idx[j]];
                        if (idx < 0) idx = total_v + idx + 1;
                        if (idx > 0 && idx <= total_v) car_vertices[f_idx++] = temp_v[idx-1];
                    }
                    faces_processed++;
                }
            }
        }
    }
    
    vertex_count = f_idx;
    free(temp_v);
    fclose(file);
    pspDebugScreenPrintf("Modello pronto! Avvio motore grafico...\n");
    sceKernelDelayThread(1000000); // Aspetta 1 secondo per leggere il messaggio
    pspDebugScreenInit(); // Pulisce lo schermo per il gioco
}

void init_graphics() {
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
    sceGuDispBuffer(480, 272, (void*)0x88000, 512);
    sceGuDepthBuffer((void*)0x110000, 512);
    sceGuOffset(2048 - (480/2), 2048 - (272/2));
    sceGuViewport(2048, 2048, 480, 272);
    sceGuDepthRange(65535, 0); // Standard PSP
    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL); // Standard PSP
    sceGuEnable(GU_CULL_FACE);
    sceGuFrontFace(GU_CCW);
    sceGuShadeModel(GU_SMOOTH);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

int main() {
    setup_callbacks();
    pspDebugScreenInit();
    
    // Inizializza il cubo di test (se l'OBJ non viene caricato)
    for(int i=0; i<36; i++) {
        car_cube[i] = cube_vertices[cube_indices[i]];
    }

    load_obj("car.obj");
    
    init_graphics();

    float car_x = 0, car_z = 0, car_angle = 0, speed = 0;
    SceCtrlData pad;

    while(1) {
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_CROSS) speed += 0.05f;
        if (pad.Lx < 80) car_angle -= 0.04f;
        if (pad.Lx > 170) car_angle += 0.04f;
        speed *= 0.96f;
        car_x += sinf(car_angle) * speed;
        car_z += cosf(car_angle) * speed;

        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xFF554433); // BLU NOTTE
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(75.0f, 480.0f/272.0f, 0.5f, 1000.0f);

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        // Camera un po' più vicina e alta
        ScePspFVector3 cam_pos = { car_x - sinf(car_angle)*8, 4.0f, car_z - cosf(car_angle)*8 };
        ScePspFVector3 cam_look = { car_x, 1.0f, car_z };
        ScePspFVector3 cam_up = { 0, 1, 0 };
        sceGumLookAt(&cam_pos, &cam_look, &cam_up);

        // Terreno Verde Matrix
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
        sceGuColor(0xFF00FF00);
        for(int i = -200; i <= 200; i += 20) {
            Vertex line_v[2];
            line_v[0] = (Vertex){(float)i, 0, -200}; line_v[1] = (Vertex){(float)i, 0, 200};
            sceGuDrawArray(GU_LINES, GU_VERTEX_32BITF|GU_TRANSFORM_3D, 2, 0, line_v);
            line_v[0] = (Vertex){-200, 0, (float)i}; line_v[1] = (Vertex){200, 0, (float)i};
            sceGuDrawArray(GU_LINES, GU_VERTEX_32BITF|GU_TRANSFORM_3D, 2, 0, line_v);
        }

        // Cubo Rosso di Test
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
        ScePspFVector3 cube_pos = { car_x + 3, 1, car_z };
        sceGumTranslate(&cube_pos);
        sceGumRotateY(car_angle);
        sceGuColor(0xFF0000FF);
        sceGuDrawArray(GU_TRIANGLES, GU_VERTEX_32BITF|GU_TRANSFORM_3D, 36, 0, car_cube);

        // Auto Bianca
        if (car_vertices != NULL) {
            sceGumMatrixMode(GU_MODEL);
            sceGumLoadIdentity();
            ScePspFVector3 car_pos = { car_x, 0, car_z };
            sceGumTranslate(&car_pos);
            sceGumRotateY(car_angle);
            sceGuColor(0xFFFFFFFF);
            sceGuDrawArray(GU_TRIANGLES, GU_VERTEX_32BITF|GU_TRANSFORM_3D, vertex_count, 0, car_vertices);
        }

        sceGuFinish();
        sceGuSync(0, 0);
        // Tolto pspDebugScreen nel loop per non creare conflitti
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }
    return 0;
}
