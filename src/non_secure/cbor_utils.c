#include "cbor_utils.h"
#include <stdbool.h>
#include <stdint.h>
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

bool cbor_encode_type_val(cbor_encoder_t *enc, uint8_t major, uint32_t val) {
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
  return cbor_encode_type_val(enc, 0, val);
}

bool cbor_encode_int(cbor_encoder_t *enc, int32_t val) {
  if (val >= 0)
    return cbor_encode_uint(enc, (uint32_t)val);
  return cbor_encode_type_val(enc, 1, (uint32_t)(-1 - val));
}

bool cbor_encode_bstr(cbor_encoder_t *enc, const uint8_t *data, uint16_t len) {
  if (!cbor_encode_type_val(enc, 2, len))
    return false;
  return write_bytes(enc, data, len);
}

bool cbor_encode_tstr(cbor_encoder_t *enc, const char *str) {
  uint16_t len = (uint16_t)strlen(str);
  if (!cbor_encode_type_val(enc, 3, len))
    return false;
  return write_bytes(enc, (const uint8_t *)str, len);
}

bool cbor_encode_map_start(cbor_encoder_t *enc, uint32_t num_pairs) {
  return cbor_encode_type_val(enc, 5, num_pairs);
}

bool cbor_encode_array_start(cbor_encoder_t *enc, uint32_t num_elements) {
  return cbor_encode_type_val(enc, 4, num_elements);
}

// Decoder Implementation
void cbor_decoder_init(cbor_decoder_t *dec, const uint8_t *buffer,
                       uint16_t size) {
  dec->buffer = buffer;
  dec->size = size;
  dec->offset = 0;
}

static bool decode_type_val(cbor_decoder_t *dec, uint8_t *major_out,
                            uint32_t *val_out) {
  if (dec->offset >= dec->size)
    return false;
  uint8_t initial = dec->buffer[dec->offset++];
  *major_out = initial >> 5;
  uint8_t additional = initial & 0x1F;

  if (additional < 24) {
    *val_out = additional;
  } else if (additional == 24) {
    if (dec->offset >= dec->size)
      return false;
    *val_out = dec->buffer[dec->offset++];
  } else if (additional == 25) {
    if (dec->offset + 2 > dec->size)
      return false;
    *val_out = (dec->buffer[dec->offset] << 8) | dec->buffer[dec->offset + 1];
    dec->offset += 2;
  } else if (additional == 26) {
    if (dec->offset + 4 > dec->size)
      return false;
    *val_out = (dec->buffer[dec->offset] << 24) |
               (dec->buffer[dec->offset + 1] << 16) |
               (dec->buffer[dec->offset + 2] << 8) |
               dec->buffer[dec->offset + 3];
    dec->offset += 4;
  } else {
    return false; // 64-bit or reserved
  }
  return true;
}

bool cbor_decode_uint(cbor_decoder_t *dec, uint32_t *val) {
  uint8_t major;
  if (!decode_type_val(dec, &major, val))
    return false;
  return major == 0;
}

bool cbor_decode_int(cbor_decoder_t *dec, int32_t *val) {
  uint8_t major;
  uint32_t uval;
  if (!decode_type_val(dec, &major, &uval))
    return false;
  if (major == 0) {
    *val = (int32_t)uval;
    return true;
  }
  if (major == 1) {
    *val = -1 - (int32_t)uval;
    return true;
  }
  return false;
}

bool cbor_decode_bstr(cbor_decoder_t *dec, const uint8_t **data,
                      uint16_t *len) {
  uint8_t major;
  uint32_t ulen;
  if (!decode_type_val(dec, &major, &ulen))
    return false;
  if (major != 2)
    return false;
  if (dec->offset + ulen > dec->size)
    return false;
  *data = dec->buffer + dec->offset;
  *len = (uint16_t)ulen;
  dec->offset += (uint16_t)ulen;
  return true;
}

bool cbor_decode_tstr(cbor_decoder_t *dec, const char **str, uint16_t *len) {
  uint8_t major;
  uint32_t ulen;
  if (!decode_type_val(dec, &major, &ulen))
    return false;
  if (major != 3)
    return false;
  if (dec->offset + ulen > dec->size)
    return false;
  *str = (const char *)(dec->buffer + dec->offset);
  *len = (uint16_t)ulen;
  dec->offset += (uint16_t)ulen;
  return true;
}

bool cbor_decode_map_start(cbor_decoder_t *dec, uint32_t *num_pairs) {
  uint8_t major;
  if (!decode_type_val(dec, &major, num_pairs))
    return false;
  return major == 5;
}

bool cbor_decode_array_start(cbor_decoder_t *dec, uint32_t *num_elements) {
  uint8_t major;
  if (!decode_type_val(dec, &major, num_elements))
    return false;
  return major == 4;
}

bool cbor_peek_type(cbor_decoder_t *dec, uint8_t *type) {
  if (dec->offset >= dec->size)
    return false;
  *type = dec->buffer[dec->offset] >> 5;
  return true;
}

bool cbor_skip_item(cbor_decoder_t *dec) {
  if (dec->offset >= dec->size)
    return false;
  uint8_t initial = dec->buffer[dec->offset];
  uint8_t major = initial >> 5;
  uint32_t val;
  uint8_t m;

  uint16_t saved_offset = dec->offset;
  if (!decode_type_val(dec, &m, &val))
    return false;

  if (major == 2 || major == 3) { // Bstr or Tstr
    if (dec->offset + val > dec->size)
      return false;
    dec->offset += (uint16_t)val;
  } else if (major == 4) { // Array
    for (uint32_t i = 0; i < val; i++)
      if (!cbor_skip_item(dec))
        return false;
  } else if (major == 5) { // Map
    for (uint32_t i = 0; i < val * 2; i++)
      if (!cbor_skip_item(dec))
        return false;
  }
  return true;
}
