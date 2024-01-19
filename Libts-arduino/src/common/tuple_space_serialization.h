#ifndef __TUPLE_SPACE_SERIALIZATION_H__
#define __TUPLE_SPACE_SERIALIZATION_H__

#include "tuple_space.h"

typedef void* (*ts_allocator_cb_t)(void* allocator_context,
                                   ts_size_t allocated_size);
typedef void (*ts_deallocator_cb_t)(void* allocator_context, void* memory_ptr);

typedef struct {
  ts_allocator_cb_t allocator_cb;
  ts_deallocator_cb_t deallocator_cb;
  void* context;
} ts_allocator_t;

void ts_tuple_span(ts_tuple_field_t** tuple_span, ts_tuple_field_t* tuple,
                   ts_size_t tuple_size);

ts_size_t ts_serialize_tuple(const ts_tuple_field_t* tuple,
                             ts_size_t tuple_size,
                             ts_byte_t* serialization_buffer,
                             ts_size_t buffer_size);

ts_size_t ts_serialize_tuple_span(ts_tuple_field_t** tuple,
                                  ts_size_t tuple_size,
                                  ts_byte_t* serialization_buffer,
                                  ts_size_t buffer_size);

ts_size_t ts_serialized_tuple_size(const ts_byte_t* message_buffer,
                                   ts_size_t buffer_size);

ts_bool_t ts_deserialize_tuple(const ts_byte_t* message_buffer,
                               ts_size_t buffer_size, ts_tuple_field_t* tuple,
                               ts_size_t tuple_size,
                               const ts_allocator_t* allocator);

ts_bool_t ts_deserialize_tuple_span(const ts_byte_t* message_buffer,
                                    ts_size_t buffer_size,
                                    ts_tuple_field_t** tuple,
                                    ts_size_t tuple_size,
                                    const ts_allocator_t* allocator);

void ts_deallocate_tuple_span(ts_tuple_field_t** tuple, ts_size_t tuple_size,
                              const ts_allocator_t* allocator);

void ts_deallocate_tuple(ts_tuple_field_t* tuple, ts_size_t tuple_size,
                         const ts_allocator_t* allocator);

#endif  // __TUPLE_SPACE_SERIALIZATION_H__
