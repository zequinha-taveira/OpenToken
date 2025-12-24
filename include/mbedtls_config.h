#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

#warning "Using custom mbedtls_config.h from OpenToken"

// System support
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C

// Allow private access for mbedTLS 3.x compatibility
#define MBEDTLS_ALLOW_PRIVATE_ACCESS

// mbed TLS modules
#define MBEDTLS_AES_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_MD_C
#define MBEDTLS_OID_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_HMAC_DRBG_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_GCM_C

// Prerequisites for Entropy and DRBG
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT

// Curves
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED

// Optimization
#define MBEDTLS_ECP_NIST_OPTIM

#endif /* MBEDTLS_CONFIG_H */
