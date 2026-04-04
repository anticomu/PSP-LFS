#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>

// Informazioni base del modulo
PSP_MODULE_INFO("LFS_RESCUE", 0, 1, 1);
// Attributi minimi
PSP_MAIN_THREAD_ATTR(0);

int main() {
    // Inizializza solo lo schermo per il testo
    pspDebugScreenInit();
    
    // Scrivi subito qualcosa
    pspDebugScreenPrintf("LFS PSP - MODALITA' EMERGENZA\n");
    pspDebugScreenPrintf("Se vedi questo, il codice sta girando!\n");
    pspDebugScreenPrintf("FPS dovrebbero essere > 0");

    // Loop infinito per non far chiudere il gioco
    while(1) {
        // Aspetta il segnale video per non surriscaldare
        sceDisplayWaitVblankStart();
    }

    return 0;
}
