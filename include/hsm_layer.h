#ifndef HSM_LAYER_H
#define HSM_LAYER_H

#include <stdbool.h>
#include <stdint.h>

// Maximum number of PIN retry attempts before lockout
#define HSM_PIN_MAX_RETRIES 3

// PIN verification result codes
typedef enum {
  HSM_PIN_SUCCESS = 0,
  HSM_PIN_INCORRECT = 1,
  HSM_PIN_LOCKED = 2,
  HSM_PIN_ERROR = 3
} hsm_pin_result_t;

// Key storage slot identifiers
typedef enum {
  HSM_KEY_SLOT_FIDO2_MASTER = 0,
  HSM_KEY_SLOT_OPENPGP_SIGN = 1,
  HSM_KEY_SLOT_OPENPGP_DECRYPT = 2,
  HSM_KEY_SLOT_OPENPGP_AUTH = 3,
  HSM_KEY_SLOT_MAX = 4
} hsm_key_slot_t;

// Definições de tipos para chaves e assinaturas (simplificadas)
typedef struct {
  uint8_t x[32]; // Coordenada X da chave pública ECC P-256
  uint8_t y[32]; // Coordenada Y da chave pública ECC P-256
} hsm_pubkey_t;

typedef struct {
  hsm_pubkey_t pub;
  uint8_t priv[32]; // Private key (d) - NEVER exported from HSM
} hsm_keypair_t;

// Funções de interface do HSM (Estrutura real de funções)
// O corpo dessas funções será implementado com bibliotecas criptográficas reais
// (ex: TinyCrypt, mbedTLS)

// Initialize HSM layer
void hsm_init(void);

// Get cryptographically secure random bytes
bool hsm_get_random(uint8_t *out, size_t len);

// Operações de Chave (FIDO2/OpenPGP)
// Generate ECC P-256 keypair and store in secure slot
bool hsm_generate_key_ecc(hsm_key_slot_t slot, hsm_pubkey_t *pubkey_out);

// Load public key from secure storage slot
bool hsm_load_pubkey(hsm_key_slot_t slot, hsm_pubkey_t *pubkey_out);

// Sign hash using private key from secure slot (private key never leaves HSM)
bool hsm_sign_ecc_slot(hsm_key_slot_t slot, const uint8_t *hash_in,
                       uint16_t hash_len, uint8_t *signature_out,
                       uint16_t *signature_len);

// Legacy functions for backward compatibility - DEPRECATED
bool hsm_generate_key_ecc_legacy(hsm_keypair_t *keypair_out);
bool hsm_sign_ecc(const uint8_t *priv_key, const uint8_t *hash_in,
                  uint16_t hash_len, uint8_t *signature_out,
                  uint16_t *signature_len);

// Operações de PIN/Verificação (OpenPGP/OATH)
// Verify PIN with retry counter management
hsm_pin_result_t hsm_verify_pin_secure(const uint8_t *pin_in, uint16_t pin_len);

// Get current PIN retry counter
uint8_t hsm_get_pin_retries_remaining(void);

// Reset PIN retry counter (admin function)
bool hsm_reset_pin_counter(const uint8_t *admin_pin, uint16_t admin_pin_len);

// Legacy PIN verification - DEPRECATED
bool hsm_verify_pin(const uint8_t *pin_in, uint16_t pin_len);

// Operações OATH
bool hsm_calculate_oath(const uint8_t *challenge_in, uint16_t challenge_len,
                        uint8_t *code_out, uint16_t *code_len);

// Key management functions
bool hsm_key_exists(hsm_key_slot_t slot);
bool hsm_delete_key(hsm_key_slot_t slot);

#endif // HSM_LAYER_H
