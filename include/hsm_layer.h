#ifndef HSM_LAYER_H
#define HSM_LAYER_H

#include <stdint.h>
#include <stdbool.h>

// Definições de tipos para chaves e assinaturas (simplificadas)
typedef struct {
    uint8_t x[32]; // Coordenada X da chave pública ECC P-256
    uint8_t y[32]; // Coordenada Y da chave pública ECC P-256
} hsm_pubkey_t;

// Funções de interface do HSM (Estrutura real de funções)
// O corpo dessas funções será implementado com bibliotecas criptográficas reais (ex: TinyCrypt, mbedTLS)

// Operações de Chave (FIDO2/OpenPGP)
bool hsm_generate_key_ecc(hsm_pubkey_t *pubkey_out);
bool hsm_sign_ecc(const uint8_t *hash_in, uint16_t hash_len, uint8_t *signature_out, uint16_t *signature_len);

// Operações de PIN/Verificação (OpenPGP/OATH)
bool hsm_verify_pin(const uint8_t *pin_in, uint16_t pin_len);

// Operações OATH
bool hsm_calculate_oath(const uint8_t *challenge_in, uint16_t challenge_len, uint8_t *code_out, uint16_t *code_len);

#endif // HSM_LAYER_H
