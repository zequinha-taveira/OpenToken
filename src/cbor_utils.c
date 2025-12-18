#include "cbor_utils.h"
#include <string.h>

void cbor_encoder_init(cbor_encoder_t *enc, uint8_t *buffer, uint16_t size) {
  enc->buffer = buffer;
  enc->size = size;
  enc->offset = 0;
}

static bool write_byte(cbor_encoder_t *enc, uint8_t b) {
  if (enc->offset >= enc->size)
    return false;
  enc->buffer[enc->offset++] = b;
  return true;
}

static bool write_bytes(cbor_encoder_t *enc, const uint8_t *data,
                        uint16_t len) {
  if (enc->offset + len > enc->size)
    return false;
  memcpy(enc->buffer + enc->offset, data, len);
  enc->offset += len;
  return true;
}

// Helper for generic Initial Byte + Length
static bool encode_type_val(cbor_encoder_t *enc, uint8_t major, uint32_t val) {
  major <<= 5;
  if (val < 24) {
    return write_byte(enc, major | val);
  } else if (val <= 0xFF) {
    if (!write_byte(enc, major | 24))
      return false;
    return write_byte(enc, (uint8_t)val);
  } else if (val <= 0xFFFF) {
    if (!write_byte(enc, major | 25))
      return false;
    write_byte(enc, (val >> 8) & 0xFF);
    return write_byte(enc, val & 0xFF);
  } else {
    if (!write_byte(enc, major | 26))
      return false;
    write_byte(enc, (val >> 24) & 0xFF);
    write_byte(enc, (val >> 16) & 0xFF);
    write_byte(enc, (val >> 8) & 0xFF);
    return write_byte(enc, val & 0xFF);
  }
}

bool cbor_encode_uint(cbor_encoder_t *enc, uint32_t val) {
  return encode_type_val(enc, 0, val);
}

bool cbor_encode_int(cbor_encoder_t *enc, int32_t val) {
  if (val >= 0)
    return cbor_encode_uint(enc, val);
  return encode_type_val(enc, 1, -1 - val);
}

bool cbor_encode_bstr(cbor_encoder_t *enc, const uint8_t *data, uint16_t len) {
  if (!encode_type_val(enc, 2, len))
    return false;
  return write_bytes(enc, data, len);
}

bool cbor_encode_tstr(cbor_encoder_t *enc, const char *str) {
  uint16_t len = strlen(str);
  if (!encode_type_val(enc, 3, len))
    return false;
  return write_bytes(enc, (const uint8_t *)str, len);
}

bool cbor_encode_map_start(cbor_encoder_t *enc, uint8_t num_pairs) {
  return encode_type_val(enc, 5, num_pairs);
}

// Decoder Implementation (Minimal)
void cbor_decoder_init(cbor_decoder_t *dec, const uint8_t *buffer,
                       uint16_t size) {
  dec->buffer = buffer;
  dec->size = size;
  dec->offset = 0;
}

bool cbor_decode_uint(cbor_decoder_t *dec, uint32_t *val) {
  if (dec->offset >= dec->size)
    return false;
  uint8_t initial = dec->buffer[dec->offset++];
  uint8_t major = initial >> 5;
  uint8_t additional = initial & 0x1F;

  if (major != 0)
    return false; // Not unsigned int

  if (additional < 24) {
    *val = additional;
  } else if (additional == 24) {
    if (dec->offset >= dec->size)
      return false;
    *val = dec->buffer[dec->offset++];
  } else if (additional == 25) {
    if (dec->offset + 2 > dec->size)
      return false;
    *val = (dec->buffer[dec->offset] << 8) | dec->buffer[dec->offset + 1];
    dec->offset += 2;
  } else if (additional == 26) {
    // 32-bit
    if (dec->offset + 4 > dec->size)
      return false;
    *val = (dec->buffer[dec->offset] << 24) |
           (dec->buffer[dec->offset + 1] << 16) |
           (dec->buffer[dec->offset + 2] << 8) | dec->buffer[dec->offset + 3];
    dec->offset += 4;
  } else {
    return false; // Not supported (64-bit or reserved)
  }
  return true;
}
