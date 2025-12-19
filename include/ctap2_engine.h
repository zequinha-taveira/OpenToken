#ifndef CTAP2_ENGINE_H
#define CTAP2_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "opentoken.h"
#include "hsm_layer.h"

// CTAP2 Error Codes
#define CTAP2_OK                    0x00
#define CTAP2_ERR_INVALID_COMMAND   0x01
#define CTAP2_ERR_INVALID_PARAMETER 0x02
#define CTAP2_ERR_INVALID_LENGTH    0x03
#define CTAP2_ERR_INVALID_SEQ       0x04
#define CTAP2_ERR_TIMEOUT           0x05
#define CTAP2_ERR_CHANNEL_BUSY      0x06
#define CTAP2_ERR_LOCK_REQUIRED     0x07
#define CTAP2_ERR_INVALID_CHANNEL   0x08
#define CTAP2_ERR_CBOR_UNEXPECTED_TYPE 0x11
#define CTAP2_ERR_INVALID_CBOR      0x12
#define CTAP2_ERR_MISSING_PARAMETER 0x14
#define CTAP2_ERR_LIMIT_EXCEEDED    0x15
#define CTAP2_ERR_UNSUPPORTED_EXTENSION 0x16
#define CTAP2_ERR_CREDENTIAL_EXCLUDED 0x19
#define CTAP2_ERR_PROCESSING        0x21
#define CTAP2_ERR_INVALID_CREDENTIAL 0x22
#define CTAP2_ERR_USER_ACTION_PENDING 0x23
#define CTAP2_ERR_OPERATION_PENDING 0x24
#define CTAP2_ERR_NO_OPERATIONS     0x25
#define CTAP2_ERR_UNSUPPORTED_ALGORITHM 0x26
#define CTAP2_ERR_OPERATION_DENIED  0x27
#define CTAP2_ERR_KEY_STORE_FULL    0x28
#define CTAP2_ERR_NO_OPERATION_PENDING 0x2A
#define CTAP2_ERR_UNSUPPORTED_OPTION 0x2B
#define CTAP2_ERR_INVALID_OPTION    0x2C
#define CTAP2_ERR_KEEPALIVE_CANCEL  0x2D
#define CTAP2_ERR_NO_CREDENTIALS    0x2E
#define CTAP2_ERR_USER_ACTION_TIMEOUT 0x2F
#define CTAP2_ERR_NOT_ALLOWED       0x30
#define CTAP2_ERR_PIN_INVALID       0x31
#define CTAP2_ERR_PIN_BLOCKED       0x32
#define CTAP2_ERR_PIN_AUTH_INVALID  0x33
#define CTAP2_ERR_PIN_AUTH_BLOCKED  0x34
#define CTAP2_ERR_PIN_NOT_SET       0x35
#define CTAP2_ERR_PIN_REQUIRED      0x36
#define CTAP2_ERR_PIN_POLICY_VIOLATION 0x37
#define CTAP2_ERR_PIN_TOKEN_EXPIRED 0x38
#define CTAP2_ERR_REQUEST_TOO_LARGE 0x39
#define CTAP2_ERR_ACTION_TIMEOUT    0x3A
#define CTAP2_ERR_UP_REQUIRED       0x3B

// CTAP2 Commands
#define CTAP2_MAKE_CREDENTIAL       0x01
#define CTAP2_GET_ASSERTION         0x02
#define CTAP2_GET_INFO              0x04
#define CTAP2_CLIENT_PIN            0x06
#define CTAP2_RESET                 0x07
#define CTAP2_GET_NEXT_ASSERTION    0x08

// CTAP2 State Machine States
typedef enum {
    CTAP2_STATE_IDLE,
    CTAP2_STATE_PROCESSING,
    CTAP2_STATE_WAITING_USER_PRESENCE,
    CTAP2_STATE_ERROR
} ctap2_state_t;

// CTAP2 Engine Context
typedef struct {
    ctap2_state_t state;
    uint32_t current_cid;
    uint8_t current_command;
    bool user_presence_required;
    bool user_verification_required;
} ctap2_context_t;

// CTAP2 Credential Structure
typedef struct {
    uint8_t credential_id[64];
    uint16_t credential_id_len;
    uint8_t user_id[64];
    uint16_t user_id_len;
    uint8_t rp_id_hash[32];
    hsm_keypair_t keypair;
    uint32_t counter;
    bool resident_key;
} ctap2_credential_t;

// Main CTAP2 processing function
void opentoken_process_ctap2_command(uint8_t *buffer, uint16_t len);

// CTAP2 Engine initialization
void ctap2_engine_init(void);

// CTAP2 Command handlers
uint8_t ctap2_handle_make_credential(const uint8_t *cbor_data, uint16_t cbor_len, 
                                   uint8_t *response, uint16_t *response_len);
uint8_t ctap2_handle_get_assertion(const uint8_t *cbor_data, uint16_t cbor_len,
                                 uint8_t *response, uint16_t *response_len);
uint8_t ctap2_handle_get_info(uint8_t *response, uint16_t *response_len);

// Utility functions
bool ctap2_verify_user_presence(void);
bool ctap2_verify_user_verification(void);
uint8_t ctap2_generate_credential_id(const uint8_t *rp_id_hash, 
                                   const uint8_t *user_id, uint16_t user_id_len,
                                   uint8_t *cred_id_out, uint16_t *cred_id_len_out);

#endif // CTAP2_ENGINE_H
