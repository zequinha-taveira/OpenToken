#ifndef OPENPGP_APPLET_H
#define OPENPGP_APPLET_H

#include <stdint.h>
#include <stdbool.h>

// Application Identifier (AID) for OpenPGP Card (Standard: D2 76 00 01 24 01)
#define OPENPGP_AID_LEN 6
extern const uint8_t OPENPGP_AID[OPENPGP_AID_LEN];

// OpenPGP APDU Commands (INS - Instruction Byte)
#define OPENPGP_INS_SELECT              0xA4
#define OPENPGP_INS_VERIFY              0x20
#define OPENPGP_INS_GET_DATA            0xCA
#define OPENPGP_INS_PUT_DATA            0xDA
#define OPENPGP_INS_PSO                 0x2A // Perform Security Operation
#define OPENPGP_INS_GENERATE_KEYPAIR    0x47 // Generate Asymmetric Key Pair
#define OPENPGP_INS_GET_CHALLENGE       0x84
#define OPENPGP_INS_INTERNAL_AUTH       0x88

// PIN References (P2 values for VERIFY command)
#define OPENPGP_PIN_USER                0x81 // User PIN (PIN 1)
#define OPENPGP_PIN_ADMIN               0x83 // Admin PIN (PIN 3)

// PSO Operation Types (P1 P2 values)
#define OPENPGP_PSO_COMPUTE_SIGNATURE   0x9E9A
#define OPENPGP_PSO_DECIPHER            0x8086

// Key References for key generation
#define OPENPGP_KEY_SIGN                0xB6
#define OPENPGP_KEY_DECRYPT             0xB8
#define OPENPGP_KEY_AUTH                0xA4

// Status Words (SW1 SW2)
#define OPENPGP_SW_OK                           0x9000
#define OPENPGP_SW_FILE_NOT_FOUND               0x6A82
#define OPENPGP_SW_WRONG_P1P2                   0x6A86
#define OPENPGP_SW_SECURITY_STATUS_NOT_SATISFIED 0x6982
#define OPENPGP_SW_VERIFICATION_FAILED          0x63C0 // 0x63Cx where x is remaining attempts
#define OPENPGP_SW_WRONG_LENGTH                 0x6700
#define OPENPGP_SW_INSTRUCTION_NOT_SUPPORTED    0x6D00
#define OPENPGP_SW_CONDITIONS_NOT_SATISFIED     0x6985

// Data Object Tags for GET DATA / PUT DATA
#define OPENPGP_TAG_AID                 0x004F
#define OPENPGP_TAG_CARDHOLDER_DATA     0x0065
#define OPENPGP_TAG_LOGIN_DATA          0x005E
#define OPENPGP_TAG_HISTORICAL_BYTES    0x5F52
#define OPENPGP_TAG_PUBKEY_SIGN         0x7F49
#define OPENPGP_TAG_PUBKEY_DECRYPT      0x7F49
#define OPENPGP_TAG_PUBKEY_AUTH         0x7F49
#define OPENPGP_TAG_EXTENDED_CAPABILITIES 0x00C0
#define OPENPGP_TAG_ALGORITHM_ATTRIBUTES_SIGN 0x00C1
#define OPENPGP_TAG_ALGORITHM_ATTRIBUTES_DECRYPT 0x00C2
#define OPENPGP_TAG_ALGORITHM_ATTRIBUTES_AUTH 0x00C3

// OpenPGP Card Data Structure
typedef struct {
    bool pin_verified;
    bool admin_pin_verified;
    uint8_t pin_retries;
    uint8_t admin_pin_retries;
    bool sign_key_generated;
    bool decrypt_key_generated;
    bool auth_key_generated;
} openpgp_card_state_t;

// Function declarations
bool openpgp_applet_select(const uint8_t *aid, uint8_t len);
void openpgp_applet_process_apdu(const uint8_t *apdu, uint16_t len, uint8_t *response, uint16_t *response_len);
void openpgp_applet_init(void);

#endif // OPENPGP_APPLET_H
