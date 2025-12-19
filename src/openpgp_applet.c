#include "openpgp_applet.h"
#include "hsm_layer.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>

// OpenPGP AID (Standard: D2 76 00 01 24 01)
const uint8_t OPENPGP_AID[OPENPGP_AID_LEN] = {0xD2, 0x76, 0x00, 0x01, 0x24, 0x01};

static bool is_selected = false;
static openpgp_card_state_t card_state = {0};

// Helper macro for creating status word response
#define SET_SW(sw)                                                             \
  do {                                                                         \
    response[*response_len] = (uint8_t)((sw) >> 8);                            \
    response[*response_len + 1] = (uint8_t)((sw) & 0xFF);                      \
    *response_len += 2;                                                        \
  } while (0)

//--------------------------------------------------------------------+
// OPENPGP APPLET INITIALIZATION
//--------------------------------------------------------------------+
void openpgp_applet_init(void) {
    printf("OpenPGP Applet: Initializing\n");
    
    // Initialize card state
    memset(&card_state, 0, sizeof(card_state));
    card_state.pin_retries = 3;
    card_state.admin_pin_retries = 3;
    
    // Check if keys already exist in HSM slots
    card_state.sign_key_generated = hsm_key_exists(HSM_KEY_SLOT_OPENPGP_SIGN);
    card_state.decrypt_key_generated = hsm_key_exists(HSM_KEY_SLOT_OPENPGP_DECRYPT);
    card_state.auth_key_generated = hsm_key_exists(HSM_KEY_SLOT_OPENPGP_AUTH);
    
    printf("OpenPGP Applet: Initialized (Sign:%d, Decrypt:%d, Auth:%d)\n",
           card_state.sign_key_generated, card_state.decrypt_key_generated, card_state.auth_key_generated);
}

//--------------------------------------------------------------------+
// OPENPGP APPLET SELECTION
//--------------------------------------------------------------------+
bool openpgp_applet_select(const uint8_t *aid, uint8_t len) {
    if (!aid || len != OPENPGP_AID_LEN) {
        return false;
    }

    if (memcmp(aid, OPENPGP_AID, OPENPGP_AID_LEN) == 0) {
        is_selected = true;
        printf("OpenPGP Applet: Selected successfully\n");
        return true;
    }

    return false;
}

//--------------------------------------------------------------------+
// PIN VERIFICATION HELPER
//--------------------------------------------------------------------+
static bool verify_pin_internal(uint8_t pin_type, const uint8_t *pin_data, uint8_t pin_len) {
    hsm_pin_result_t result = hsm_verify_pin_secure(pin_data, pin_len);
    
    switch (result) {
        case HSM_PIN_SUCCESS:
            if (pin_type == OPENPGP_PIN_USER) {
                card_state.pin_verified = true;
                card_state.pin_retries = 3;
            } else if (pin_type == OPENPGP_PIN_ADMIN) {
                card_state.admin_pin_verified = true;
                card_state.admin_pin_retries = 3;
            }
            return true;
            
        case HSM_PIN_INCORRECT:
            if (pin_type == OPENPGP_PIN_USER) {
                card_state.pin_retries = hsm_get_pin_retries_remaining();
            } else if (pin_type == OPENPGP_PIN_ADMIN) {
                card_state.admin_pin_retries = hsm_get_pin_retries_remaining();
            }
            return false;
            
        case HSM_PIN_LOCKED:
            if (pin_type == OPENPGP_PIN_USER) {
                card_state.pin_retries = 0;
            } else if (pin_type == OPENPGP_PIN_ADMIN) {
                card_state.admin_pin_retries = 0;
            }
            return false;
            
        default:
            return false;
    }
}

//--------------------------------------------------------------------+
// KEY GENERATION HELPER
//--------------------------------------------------------------------+
static bool generate_key_pair(uint8_t key_ref, uint8_t *response, uint16_t *response_len) {
    hsm_key_slot_t slot;
    
    // Map key reference to HSM slot
    switch (key_ref) {
        case OPENPGP_KEY_SIGN:
            slot = HSM_KEY_SLOT_OPENPGP_SIGN;
            break;
        case OPENPGP_KEY_DECRYPT:
            slot = HSM_KEY_SLOT_OPENPGP_DECRYPT;
            break;
        case OPENPGP_KEY_AUTH:
            slot = HSM_KEY_SLOT_OPENPGP_AUTH;
            break;
        default:
            return false;
    }
    
    // Generate key pair in HSM
    hsm_pubkey_t pubkey;
    if (!hsm_generate_key_ecc(slot, &pubkey)) {
        printf("OpenPGP Applet: Key generation failed for slot %d\n", slot);
        return false;
    }
    
    // Update card state
    switch (key_ref) {
        case OPENPGP_KEY_SIGN:
            card_state.sign_key_generated = true;
            break;
        case OPENPGP_KEY_DECRYPT:
            card_state.decrypt_key_generated = true;
            break;
        case OPENPGP_KEY_AUTH:
            card_state.auth_key_generated = true;
            break;
    }
    
    // Format public key response (simplified format)
    // In real OpenPGP card, this would be a proper DER/TLV encoded public key
    response[0] = 0x7F; // Public key template tag
    response[1] = 0x49;
    response[2] = 66;   // Length (64 bytes for coordinates + 2 bytes header)
    response[3] = 0x86; // Public key tag
    response[4] = 64;   // Length of key data
    memcpy(response + 5, pubkey.x, 32);
    memcpy(response + 37, pubkey.y, 32);
    *response_len = 69;
    
    printf("OpenPGP Applet: Key pair generated for reference 0x%02X\n", key_ref);
    return true;
}

//--------------------------------------------------------------------+
// SIGNATURE OPERATION HELPER
//--------------------------------------------------------------------+
static bool perform_signature(const uint8_t *hash_data, uint8_t hash_len, 
                             uint8_t *response, uint16_t *response_len) {
    if (!card_state.pin_verified) {
        printf("OpenPGP Applet: PIN not verified for signature\n");
        return false;
    }
    
    if (!card_state.sign_key_generated) {
        printf("OpenPGP Applet: No signing key available\n");
        return false;
    }
    
    // Use HSM to sign with the signing key
    return hsm_sign_ecc_slot(HSM_KEY_SLOT_OPENPGP_SIGN, hash_data, hash_len, 
                            response, response_len);
}

//--------------------------------------------------------------------+
// GET PUBLIC KEY HELPER
//--------------------------------------------------------------------+
static bool get_public_key(uint8_t key_ref, uint8_t *response, uint16_t *response_len) {
    hsm_key_slot_t slot;
    
    // Map key reference to HSM slot
    switch (key_ref) {
        case OPENPGP_KEY_SIGN:
            if (!card_state.sign_key_generated) return false;
            slot = HSM_KEY_SLOT_OPENPGP_SIGN;
            break;
        case OPENPGP_KEY_DECRYPT:
            if (!card_state.decrypt_key_generated) return false;
            slot = HSM_KEY_SLOT_OPENPGP_DECRYPT;
            break;
        case OPENPGP_KEY_AUTH:
            if (!card_state.auth_key_generated) return false;
            slot = HSM_KEY_SLOT_OPENPGP_AUTH;
            break;
        default:
            return false;
    }
    
    hsm_pubkey_t pubkey;
    if (!hsm_load_pubkey(slot, &pubkey)) {
        return false;
    }
    
    // Format public key response (simplified format)
    response[0] = 0x7F; // Public key template tag
    response[1] = 0x49;
    response[2] = 66;   // Length
    response[3] = 0x86; // Public key tag
    response[4] = 64;   // Length of key data
    memcpy(response + 5, pubkey.x, 32);
    memcpy(response + 37, pubkey.y, 32);
    *response_len = 69;
    
    return true;
}

//--------------------------------------------------------------------+
// OPENPGP APDU PROCESSING
//--------------------------------------------------------------------+
void openpgp_applet_process_apdu(const uint8_t *apdu, uint16_t len, 
                                 uint8_t *response, uint16_t *response_len) {
    if (!apdu || len < 4 || !response || !response_len) {
        *response_len = 0;
        SET_SW(OPENPGP_SW_WRONG_P1P2);
        return;
    }

    // Check if applet is selected
    if (!is_selected) {
        *response_len = 0;
        SET_SW(OPENPGP_SW_FILE_NOT_FOUND);
        return;
    }

    // Parse APDU header
    uint8_t cla = apdu[0];
    uint8_t ins = apdu[1];
    uint8_t p1 = apdu[2];
    uint8_t p2 = apdu[3];
    uint8_t lc = (len > 4) ? apdu[4] : 0;
    const uint8_t *data = (len > 5) ? apdu + 5 : NULL;

    *response_len = 0;

    switch (ins) {
        case OPENPGP_INS_VERIFY:
            printf("OpenPGP Applet: VERIFY command (PIN 0x%02X)\n", p2);

            if (!data || lc == 0) {
                SET_SW(OPENPGP_SW_WRONG_LENGTH);
                break;
            }

            if (verify_pin_internal(p2, data, lc)) {
                SET_SW(OPENPGP_SW_OK);
            } else {
                uint8_t retries = (p2 == OPENPGP_PIN_USER) ? card_state.pin_retries : card_state.admin_pin_retries;
                SET_SW(OPENPGP_SW_VERIFICATION_FAILED | retries);
            }
            break;

        case OPENPGP_INS_GENERATE_KEYPAIR:
            printf("OpenPGP Applet: GENERATE ASYMMETRIC KEY PAIR (Key 0x%02X)\n", p1);

            if (!card_state.admin_pin_verified) {
                SET_SW(OPENPGP_SW_SECURITY_STATUS_NOT_SATISFIED);
                break;
            }

            if (generate_key_pair(p1, response, response_len)) {
                SET_SW(OPENPGP_SW_OK);
            } else {
                SET_SW(OPENPGP_SW_WRONG_P1P2);
            }
            break;

        case OPENPGP_INS_PSO:
            printf("OpenPGP Applet: PSO (Perform Security Operation) P1P2=0x%02X%02X\n", p1, p2);

            uint16_t pso_op = (p1 << 8) | p2;
            
            if (pso_op == OPENPGP_PSO_COMPUTE_SIGNATURE) {
                if (!data || lc == 0) {
                    SET_SW(OPENPGP_SW_WRONG_LENGTH);
                    break;
                }

                if (perform_signature(data, lc, response, response_len)) {
                    SET_SW(OPENPGP_SW_OK);
                } else {
                    SET_SW(OPENPGP_SW_SECURITY_STATUS_NOT_SATISFIED);
                }
            } else {
                SET_SW(OPENPGP_SW_WRONG_P1P2);
            }
            break;

        case OPENPGP_INS_GET_DATA:
            printf("OpenPGP Applet: GET DATA P1=0x%02X P2=0x%02X\n", p1, p2);

            uint16_t tag = (p1 << 8) | p2;
            
            switch (tag) {
                case OPENPGP_TAG_AID:
                    memcpy(response, OPENPGP_AID, OPENPGP_AID_LEN);
                    *response_len = OPENPGP_AID_LEN;
                    SET_SW(OPENPGP_SW_OK);
                    break;
                    
                case OPENPGP_TAG_CARDHOLDER_DATA:
                    // Return minimal cardholder data
                    response[0] = 0x5B; // Name tag
                    response[1] = 0x0C; // Length
                    memcpy(response + 2, "OpenToken User", 12);
                    *response_len = 14;
                    SET_SW(OPENPGP_SW_OK);
                    break;
                    
                case OPENPGP_TAG_LOGIN_DATA:
                    {
                        const char *login = "opentoken";
                        memcpy(response, login, strlen(login));
                        *response_len = strlen(login);
                        SET_SW(OPENPGP_SW_OK);
                    }
                    break;
                    
                case OPENPGP_TAG_HISTORICAL_BYTES:
                    {
                        uint8_t hist[] = {0x00, 0x31, 0xC5, 0x73, 0xC0, 0x01, 0x40, 0x05, 0x90, 0x00};
                        memcpy(response, hist, sizeof(hist));
                        *response_len = sizeof(hist);
                        SET_SW(OPENPGP_SW_OK);
                    }
                    break;
                    
                case OPENPGP_TAG_PUBKEY_SIGN:
                    if (get_public_key(OPENPGP_KEY_SIGN, response, response_len)) {
                        SET_SW(OPENPGP_SW_OK);
                    } else {
                        SET_SW(OPENPGP_SW_FILE_NOT_FOUND);
                    }
                    break;
                    
                default:
                    SET_SW(OPENPGP_SW_FILE_NOT_FOUND);
                    break;
            }
            break;

        case OPENPGP_INS_GET_CHALLENGE:
            printf("OpenPGP Applet: GET CHALLENGE\n");
            
            // Generate random challenge (simplified - use HSM random if available)
            for (int i = 0; i < 8; i++) {
                response[i] = (uint8_t)(0x42 + i); // Simple pattern for demo
            }
            *response_len = 8;
            SET_SW(OPENPGP_SW_OK);
            break;

        default:
            printf("OpenPGP Applet: Unsupported INS 0x%02X\n", ins);
            SET_SW(OPENPGP_SW_INSTRUCTION_NOT_SUPPORTED);
            break;
    }
}
