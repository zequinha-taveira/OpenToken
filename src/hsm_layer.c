#include "hsm_layer.h"
#include <stdio.h>
#include <string.h>

// mbedTLS Includes
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/md.h"
#include "mbedtls/platform.h"


// Static Contexts (to avoid stack overflow on small micros)
// In a real generic implem, these should be managed more carefully.
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;
static bool is_init = false;

static void ensure_init(void) {
  if (is_init)
    return;
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  // Seed with some fixed string + timer if possible (in Pico use standard RNG)
  // Pico SDK pico_mbedtls handles this if using standard platform setup,
  // but here we just do a basic seed.
  const char *pers = "opentoken";
  mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                        (const unsigned char *)pers, strlen(pers));
  is_init = true;
}

// Global PIN
static const uint8_t USER_PIN[] = {0x31, 0x32, 0x33,
                                   0x34, 0x35, 0x36}; // "123456"

bool hsm_verify_pin(const uint8_t *pin_in, uint16_t pin_len) {
  printf("HSM: Verifying PIN.\n");
  if (pin_len != sizeof(USER_PIN))
    return false;
  uint8_t diff = 0;
  for (uint16_t i = 0; i < pin_len; i++)
    diff |= (pin_in[i] ^ USER_PIN[i]);
  return (diff == 0);
}

bool hsm_calculate_oath(const uint8_t *challenge_in, uint16_t challenge_len,
                        uint8_t *code_out, uint16_t *code_len) {
  printf("HSM: OATH Calc (mbedTLS HMAC-SHA1).\n");
  // HARDCODED SECRET for Demo: "abba" (0x61 0x62 0x62 0x61)
  // Matches the default manual entry in walkthrough.
  uint8_t secret[] = "abba";

  uint8_t hmac_result[20]; // SHA1 is 20 bytes

  const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);

  if (mbedtls_md_hmac(md_info, secret, 4, challenge_in, challenge_len,
                      hmac_result) != 0) {
    return false;
  }

  // Truncate
  uint8_t offset = hmac_result[19] & 0x0F;
  uint32_t bin_code = (hmac_result[offset] & 0x7F) << 24 |
                      (hmac_result[offset + 1] & 0xFF) << 16 |
                      (hmac_result[offset + 2] & 0xFF) << 8 |
                      (hmac_result[offset + 3] & 0xFF);

  // Convert to 6 digits? Usually OATH returns the full code (uint32) and app
  // truncates. Spec says: "The R response Data field contains the 4 bytes ...
  // of the truncated value" So we return the big-endian 4-byte integer.

  // However, Yk codes are usually standard TOTP arithmetic.
  // The previous stub was returning raw bytes.
  // Let's pass the 4-byte truncated value.

  code_out[0] = (bin_code >> 24) & 0xFF;
  code_out[1] = (bin_code >> 16) & 0xFF;
  code_out[2] = (bin_code >> 8) & 0xFF;
  code_out[3] = bin_code & 0xFF;
  *code_len = 4;

  return true;
}

bool hsm_generate_key_ecc(hsm_pubkey_t *pubkey_out) {
  ensure_init();
  printf("HSM: Generating ECC P-256 Key...\n");

  mbedtls_ecp_keypair key;
  mbedtls_ecp_keypair_init(&key);

  // Gen Key
  if (mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, &key,
                          mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
    printf("HSM: Key Gen Failed\n");
    mbedtls_ecp_keypair_free(&key);
    return false;
  }

  // Export Public Point X/Y
  // mbedtls stores in MPI. We need raw bytes.
  // X and Y are 32 bytes each for P-256.
  mbedtls_mpi_write_binary(&key.Q.X, pubkey_out->x, 32);
  mbedtls_mpi_write_binary(&key.Q.Y, pubkey_out->y, 32);

  // TODO: Store Private Key (d) in Flash securely.
  // This function signature doesn't take an ID/Handle.
  // Just a demo for now.

  mbedtls_ecp_keypair_free(&key);
  return true;
}

bool hsm_sign_ecc(const uint8_t *hash_in, uint16_t hash_len,
                  uint8_t *signature_out, uint16_t *signature_len) {
  ensure_init();
  printf("HSM: Signing ECC...\n");

  // Regenerate a key just to sign (Dummy Logic because we didn't save the
  // private key above) REAL WORLD: Load private key from secure storage.
  mbedtls_ecp_keypair key;
  mbedtls_ecp_keypair_init(&key);
  mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, &key, mbedtls_ctr_drbg_random,
                      &ctr_drbg);

  mbedtls_mpi r, s;
  mbedtls_mpi_init(&r);
  mbedtls_mpi_init(&s);

  if (mbedtls_ecdsa_sign(&key.grp, &r, &s, &key.d, hash_in, hash_len,
                         mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
    mbedtls_ecp_keypair_free(&key);
    return false;
  }

  // Write ASN.1 Signature (Sequence { r, s }) OR Raw format?
  // WebAuthn usually expects ASN.1 DER.
  // We'll output simple Raw concatenation for now (64 bytes) if requested by
  // the pointer size? Or doing a manual DER encoding. Let's assume the caller
  // processes DER if needed, BUT the stub returned 64 bytes (Raw R|S). FIDO U2F
  // raw is 64 bytes. FIDO2 often uses COSE which wraps raw. We will return Raw
  // R (32) | S (32).

  mbedtls_mpi_write_binary(&r, signature_out, 32);
  mbedtls_mpi_write_binary(&s, signature_out + 32, 32);
  *signature_len = 64;

  mbedtls_mpi_free(&r);
  mbedtls_mpi_free(&s);
  mbedtls_ecp_keypair_free(&key);

  return true;
}
