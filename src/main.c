#include <stdio.h>
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"

// Definições de OpenToken (declaradas em opentoken.h)
#include "opentoken.h"
#include "ctap2_engine.h"
#include "ccid_engine.h"

// Funções de callback do TinyUSB (placeholder)
void tud_mount_cb(void) {
    printf("OpenToken: Dispositivo USB montado.\n");
}

void tud_umount_cb(void) {
    printf("OpenToken: Dispositivo USB desmontado.\n");
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    printf("OpenToken: Suspenso.\n");
}

void tud_resume_cb(void) {
    printf("OpenToken: Resumido.\n");
}

// Loop principal do firmware
int main() {
    // Inicialização básica do Pico SDK
    stdio_init_all();
    board_init();

    // Inicialização do TinyUSB
    tusb_init();

    printf("OpenToken Firmware (RP2350) - Iniciado.\n");
    printf("Interfaces USB: HID (FIDO2/CTAP2) e CCID (OATH/OpenPGP).\n");

    while (1) {
        // Tarefas do TinyUSB
        tud_task();

        // Lógica de aplicação do OpenToken (placeholder)
        // Aqui seria o loop principal para processar comandos CTAP2 e APDU
        sleep_ms(10);
    }

    return 0;
}
