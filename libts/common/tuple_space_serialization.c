#include "tuple_space_serialization.h"

#include <inttypes.h>
#include <string.h>

static ts_bool_t ts_is_little_endian(void) {
  union {
    uint16_t element_short;
    uint8_t element_byte;
  } data;
  data.element_short = 1;
  return data.element_byte == 1;
}

static void ts_endianaware_memcpy(void* dst, const void* src, ts_size_t size) {
  if (ts_is_little_endian()) {
    memcpy(dst, src, size);
  } else {
    for (size_t i = 0; i < size; ++i) {
      ((ts_byte_t*)dst)[i] = ((ts_byte_t*)src)[size - i - 1];
    }
  }
}

void ts_tuple_span(ts_tuple_field_t** tuple_span, ts_tuple_field_t* tuple,
                   ts_size_t tuple_size) {
  for (ts_size_t i = 0; i < tuple_size; ++i) {
    tuple_span[i] = &tuple[i];
  }
}

static ts_size_t ts_serialize_tuple_field_data(const ts_tuple_field_t* field,
                                               ts_byte_t** buffer,
                                               ts_size_t* buffer_size,
                                               ts_size_t data_size) {
  if (*buffer_size < data_size) {
    return 0;
  }
  ts_endianaware_memcpy(*buffer, &field->data, data_size);
  (*buffer) += data_size;
  (*buffer_size) -= data_size;
  return data_size;
}

static ts_size_t ts_serialize_tuple_field_string(const ts_tuple_field_t* field,
                                                 ts_byte_t** buffer,
                                                 ts_size_t* buffer_size) {
  const ts_ushort_t string_length = strlen(field->data.string_field);
  const ts_size_t total_field_len = string_length + 2;
  if (total_field_len > *buffer_size) {
    return 0;
  }
  ts_endianaware_memcpy(*buffer, &string_length, 2);
  memcpy(*buffer + 2, field->data.string_field, string_length);
  (*buffer) += total_field_len;
  (*buffer_size) -= total_field_len;
  return total_field_len;
}

static ts_size_t ts_serialize_tuple_field(const ts_tuple_field_t* field,
                                          ts_byte_t** buffer,
                                          ts_size_t* buffer_size) {
  if (*buffer_size == 0) {
    return 0;
  }
  (*buffer)[0] = field->flags;
  ++(*buffer);
  --(*buffer_size);
  if (!ts_tuple_field_contains_data(field)) {
    return 1;
  }
  ts_size_t data_length = 0;
  switch (ts_tuple_field_get_type(field)) {
    case TS_FIELD_TYPE_UINT:
    case TS_FIELD_TYPE_INT:
    case TS_FIELD_TYPE_FLOAT:
      data_length =
          ts_serialize_tuple_field_data(field, buffer, buffer_size, 4);
      break;
    case TS_FIELD_TYPE_STRING:
      data_length = ts_serialize_tuple_field_string(field, buffer, buffer_size);
      break;
    case TS_FIELD_TYPE_BOOL:
      *(*buffer - 1) =
          (field->flags & 0xbf) | (field->data.bool_field ? 0x40 : 0x00);
      return 1;
    default:
      break;
  }
  return data_length == 0 ? 0 : 1 + data_length;
}

ts_size_t ts_serialize_tuple(const ts_tuple_field_t* tuple,
                             ts_size_t tuple_size,
                             ts_byte_t* serialization_buffer,
                             ts_size_t buffer_size) {
  if (tuple_size > TS_MAX_TUPLE_SIZE) {
    return 0;
  }
  ts_size_t total_size = 0;
  for (ts_size_t i = 0; i < tuple_size; ++i) {
    const ts_tuple_field_t* current_field = &tuple[i];
    ts_size_t serialized_field_size = ts_serialize_tuple_field(
        current_field, &serialization_buffer, &buffer_size);
    if (serialized_field_size == 0) {
      return 0;
    }
    total_size += serialized_field_size;
  }
  return total_size;
}

ts_size_t ts_serialize_tuple_span(ts_tuple_field_t** tuple,
                                  ts_size_t tuple_size,
                                  ts_byte_t* serialization_buffer,
                                  ts_size_t buffer_size) {
  if (tuple_size > TS_MAX_TUPLE_SIZE) {
    return 0;
  }
  ts_size_t total_size = 0;
  for (ts_size_t i = 0; i < tuple_size; ++i) {
    const ts_tuple_field_t* current_field = tuple[i];
    ts_size_t serialized_field_size = ts_serialize_tuple_field(
        current_field, &serialization_buffer, &buffer_size);
    if (serialized_field_size == 0) {
      return 0;
    }
    total_size += serialized_field_size;
  }
  return total_size;
}

ts_size_t ts_serialized_tuple_size(const ts_byte_t* message_buffer,
                                   ts_size_t buffer_size) {
  return buffer_size == 0 ? 0 : (1 + (message_buffer[0] >> 4));
}

static ts_bool_t ts_deserialize_tuple_field_data(
    ts_tuple_field_t* current_field, const ts_byte_t** buffer,
    ts_size_t* remaining_size, ts_size_t field_size) {
  if (*remaining_size < field_size) {
    return TS_FALSE;
  }
  ts_endianaware_memcpy(&current_field->data, *buffer, field_size);
  *remaining_size -= field_size;
  *buffer += field_size;
  return TS_TRUE;
}

static ts_bool_t ts_deserialize_tuple_field_string(
    ts_tuple_field_t* current_field, const ts_byte_t** buffer,
    ts_size_t* remaining_size, const ts_allocator_t* allocator) {
  if (*remaining_size < 2) {
    return TS_FALSE;
  }
  ts_ushort_t string_size;
  ts_endianaware_memcpy(&string_size, *buffer, 2);
  *remaining_size -= 2;
  *buffer += 2;
  if (*remaining_size < string_size) {
    return TS_FALSE;
  }
  char* string_buffer =
      (char*)allocator->allocator_cb(allocator->context, string_size + 1);
  if (string_buffer == NULL) {
    return TS_FALSE;
  }
  memcpy(string_buffer, *buffer, string_size);
  string_buffer[string_size] = '\0';
  current_field->data.string_field = string_buffer;
  *remaining_size -= string_size;
  *buffer += string_size;
  return TS_TRUE;
}

static ts_bool_t ts_deserialize_tuple_field(ts_tuple_field_t* current_field,
                                            const ts_byte_t** buffer,
                                            ts_size_t* remaining_size,
                                            const ts_allocator_t* allocator) {
  if (*remaining_size == 0) {
    return TS_FALSE;
  }
  current_field->flags = (*buffer)[0];
  --(*remaining_size);
  ++(*buffer);
  if (!ts_tuple_field_contains_data(current_field)) {
    return TS_TRUE;
  }
  switch (ts_tuple_field_get_type(current_field)) {
    case TS_FIELD_TYPE_UINT:
    case TS_FIELD_TYPE_INT:
    case TS_FIELD_TYPE_FLOAT:
      return ts_deserialize_tuple_field_data(current_field, buffer,
                                             remaining_size, 4);
    case TS_FIELD_TYPE_STRING:
      return ts_deserialize_tuple_field_string(current_field, buffer,
                                               remaining_size, allocator);
    case TS_FIELD_TYPE_BOOL:
      current_field->data.bool_field = (current_field->flags & 0x40) >> 6;
      return TS_TRUE;
    default:
      return TS_FALSE;
  }
  return TS_FALSE;
}

ts_bool_t ts_deserialize_tuple(const ts_byte_t* message_buffer,
                               ts_size_t buffer_size, ts_tuple_field_t* tuple,
                               ts_size_t tuple_size,
                               const ts_allocator_t* allocator) {
  for (ts_size_t i = 0; i < tuple_size; ++i) {
    if (!ts_deserialize_tuple_field(&tuple[i], &message_buffer, &buffer_size,
                                    allocator)) {
      ts_deallocate_tuple(tuple, tuple_size, allocator);
      return TS_FALSE;
    }
  }
  return TS_TRUE;
}

ts_bool_t ts_deserialize_tuple_span(const ts_byte_t* message_buffer,
                                    ts_size_t buffer_size,
                                    ts_tuple_field_t** tuple,
                                    ts_size_t tuple_size,
                                    const ts_allocator_t* allocator) {
  for (ts_size_t i = 0; i < tuple_size; ++i) {
    if (!ts_deserialize_tuple_field(tuple[i], &message_buffer, &buffer_size,
                                    allocator)) {
      ts_deallocate_tuple_span(tuple, tuple_size, allocator);
      return TS_FALSE;
    }
  }
  return TS_TRUE;
}

void ts_deallocate_tuple(ts_tuple_field_t* tuple, ts_size_t tuple_size,
                         const ts_allocator_t* allocator) {
  for (ts_size_t i = 0; i < tuple_size; ++i) {
    ts_tuple_field_t* current_field = &tuple[i];
    if ((ts_tuple_field_get_type(current_field) == TS_FIELD_TYPE_STRING) &&
        ts_tuple_field_contains_data(current_field)) {
      allocator->deallocator_cb(allocator->context,
                                (char*)current_field->data.string_field);
    }
  }
}

void ts_deallocate_tuple_span(ts_tuple_field_t** tuple, ts_size_t tuple_size,
                              const ts_allocator_t* allocator) {
  for (ts_size_t i = 0; i < tuple_size; ++i) {
    ts_tuple_field_t* current_field = tuple[i];
    if ((ts_tuple_field_get_type(current_field) == TS_FIELD_TYPE_STRING) &&
        ts_tuple_field_contains_data(current_field)) {
      allocator->deallocator_cb(allocator->context,
                                (char*)current_field->data.string_field);
    }
  }
}
