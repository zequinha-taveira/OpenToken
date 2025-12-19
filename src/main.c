#include <stdio.h>
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"

// OpenToken includes
#include "opentoken.h"
#include "ctap2_engine.h"
#include "ccid_engine.h"
#include "hsm_layer.h"
#include "storage.h"
#include "openpgp_applet.h"
#include "yubikey_mgmt.h"
#include "error_handling.h"

// Helper function for tusb_init (which is a macro in modern TinyUSB)
// to satisfy retry_operation's function pointer requirement
bool opentoken_usb_init(void) {
    return tusb_init();
}

// External descriptor declarations (defined in usb_descriptors.c)
extern tusb_desc_device_t const desc_device;
extern uint8_t const desc_configuration[];

//--------------------------------------------------------------------+
// USB DEVICE CALLBACKS
//--------------------------------------------------------------------+
void tud_mount_cb(void) {
    printf("OpenToken: USB device mounted - HID+CCID composite device ready\n");
    
    // Update USB stability tracking
    usb_stability_update_state(USB_STATE_CONNECTED);
    
    // Handle YubiKey Manager compatibility layer reconnection
    yubikey_mgmt_handle_reconnection();
}

void tud_umount_cb(void) {
    printf("OpenToken: USB device unmounted\n");
    
    // Update USB stability tracking
    usb_stability_update_state(USB_STATE_DISCONNECTED);
    
    // Cleanup resources on disconnect
    error_cleanup_resources();
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    printf("OpenToken: USB suspended\n");
    
    // Update USB stability tracking
    usb_stability_update_state(USB_STATE_SUSPENDED);
}

void tud_resume_cb(void) {
    printf("OpenToken: USB resumed\n");
    
    // Update USB stability tracking
    usb_stability_update_state(USB_STATE_CONNECTED);
}

//--------------------------------------------------------------------+
// DEVICE DESCRIPTORS CALLBACKS
//--------------------------------------------------------------------+
uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index; // For multiple configurations
    return desc_configuration;
}

//--------------------------------------------------------------------+
// MAIN FUNCTION
//--------------------------------------------------------------------+
int main() {
    // Initialize Pico SDK
    stdio_init_all();
    board_init();

    // Initialize error handling and stability system FIRST
    error_handling_init();
    
    printf("OpenToken Firmware v1.0 - RP2350\n");
    printf("USB Composite Device: HID (FIDO2/CTAP2) + CCID (OATH/OpenPGP)\n");
    printf("VID:PID = %04X:%04X\n", OPENTOKEN_VID, OPENTOKEN_PID);

    // Initialize storage and HSM layer with error handling
    if (!retry_operation((bool (*)(void))storage_init, &RETRY_CONFIG_STORAGE)) {
        ERROR_REPORT_CRITICAL(ERROR_STORAGE_WRITE_FAILED, "Storage initialization failed");
        system_enter_safe_mode();
    }
    
    if (!retry_operation((bool (*)(void))hsm_init, &RETRY_CONFIG_CRYPTO)) {
        ERROR_REPORT_CRITICAL(ERROR_CRYPTO_KEY_GENERATION, "HSM initialization failed");
        system_enter_safe_mode();
    }
    
    // Initialize protocol engines
    ctap2_engine_init();
    ccid_engine_init();
    
    // Initialize YubiKey Manager compatibility layer
    yubikey_mgmt_init_compatibility_layer();
    
    // Initialize applets
    openpgp_applet_init();

    // Initialize TinyUSB composite device with retry
    usb_stability_update_state(USB_STATE_CONNECTING);
    if (!retry_operation((bool (*)(void))opentoken_usb_init, &RETRY_CONFIG_USB)) {
        ERROR_REPORT_CRITICAL(ERROR_USB_ENUMERATION_FAILED, "USB initialization failed");
        system_enter_safe_mode();
    }

    printf("OpenToken: All systems initialized successfully\n");
    printf("OpenToken: Error handling and stability features active\n");

    uint32_t last_health_check = 0;
    
    while (1) {
        // TinyUSB device task - handles USB enumeration and communication
        tud_task();

        // Periodic system health monitoring
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_health_check > 5000) { // Every 5 seconds
            system_health_check();
            last_health_check = now;
        }
        
        tight_loop_contents();
    }

    return 0;
}
