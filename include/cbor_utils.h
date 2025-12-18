#ifndef CBOR_UTILS_H
#define CBOR_UTILS_H

#include <stdbool.h>
#include <stdint.h>


// Simple CBOR Encoder/Decoder for CTAP2

typedef struct {
  uint8_t *buffer;
  uint16_t size;
  uint16_t offset;
} cbor_encoder_t;

typedef struct {
  const uint8_t *buffer;
  uint16_t size;
  uint16_t offset;
} cbor_decoder_t;

// Encoder
void cbor_encoder_init(cbor_encoder_t *enc, uint8_t *buffer, uint16_t size);
bool cbor_encode_uint(cbor_encoder_t *enc, uint32_t val);
bool cbor_encode_int(cbor_encoder_t *enc, int32_t val);
bool cbor_encode_bstr(cbor_encoder_t *enc, const uint8_t *data, uint16_t len);
bool cbor_encode_tstr(cbor_encoder_t *enc, const char *str);
bool cbor_encode_map_start(cbor_encoder_t *enc, uint8_t num_pairs);

// Decoder
void cbor_decoder_init(cbor_decoder_t *dec, const uint8_t *buffer,
                       uint16_t size);
bool cbor_decode_uint(cbor_decoder_t *dec, uint32_t *val);
bool cbor_peek_type(cbor_decoder_t *dec, uint8_t *type); // Returns major type
// ... Expanded as needed for parsing complex maps

#endif // CBOR_UTILS_H
