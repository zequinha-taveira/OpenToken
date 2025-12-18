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
bool cbor_encode_map_start(cbor_encoder_t *enc, uint32_t num_pairs);
bool cbor_encode_array_start(cbor_encoder_t *enc, uint32_t num_elements);
bool cbor_encode_type_val(cbor_encoder_t *enc, uint8_t major, uint32_t val);

// Decoder
void cbor_decoder_init(cbor_decoder_t *dec, const uint8_t *buffer,
                       uint16_t size);
bool cbor_decode_uint(cbor_decoder_t *dec, uint32_t *val);
bool cbor_decode_int(cbor_decoder_t *dec, int32_t *val);
bool cbor_decode_bstr(cbor_decoder_t *dec, const uint8_t **data, uint16_t *len);
bool cbor_decode_tstr(cbor_decoder_t *dec, const char **str, uint16_t *len);
bool cbor_decode_map_start(cbor_decoder_t *dec, uint32_t *num_pairs);
bool cbor_decode_array_start(cbor_decoder_t *dec, uint32_t *num_elements);
bool cbor_peek_type(cbor_decoder_t *dec, uint8_t *type); // Returns major type
bool cbor_skip_item(cbor_decoder_t *dec);

#endif // CBOR_UTILS_H
