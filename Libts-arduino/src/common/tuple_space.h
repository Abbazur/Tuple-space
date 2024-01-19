#ifndef __TUPLE_SPACE_H__
#define __TUPLE_SPACE_H__

#include <inttypes.h>

typedef unsigned char ts_bool_t;
typedef uint32_t ts_uint_t;
typedef int32_t ts_int_t;
typedef const char* ts_string_t;
typedef float ts_float_t;
typedef uint32_t ts_size_t;
typedef int32_t ts_ssize_t;
typedef uint16_t ts_ushort_t;
typedef unsigned char ts_byte_t;

typedef struct {
  uint8_t flags;
  union {
    ts_bool_t bool_field;
    ts_uint_t uint_field;
    ts_int_t int_field;
    ts_float_t float_field;
    ts_string_t string_field;
  } data;
} ts_tuple_field_t;

#define __TS_TUPLE_FACTORY(tuple_size)   \
  typedef struct {                       \
    ts_tuple_field_t fields[tuple_size]; \
  } ts_tuple_##tuple_size##_t;

__TS_TUPLE_FACTORY(1);
__TS_TUPLE_FACTORY(2);
__TS_TUPLE_FACTORY(3);
__TS_TUPLE_FACTORY(4);
__TS_TUPLE_FACTORY(5);
__TS_TUPLE_FACTORY(6);
__TS_TUPLE_FACTORY(7);
__TS_TUPLE_FACTORY(8);
__TS_TUPLE_FACTORY(9);
__TS_TUPLE_FACTORY(10);
__TS_TUPLE_FACTORY(11);
__TS_TUPLE_FACTORY(12);
__TS_TUPLE_FACTORY(13);
__TS_TUPLE_FACTORY(14);
__TS_TUPLE_FACTORY(15);
__TS_TUPLE_FACTORY(16);

#undef __TS_TUPLE_FACTORY

#define TS_MAX_TUPLE_SIZE 16

typedef enum {
  TS_OPERATION_SUCCESS = 0,
  TS_OPERATION_FAILURE = 1
} ts_operation_status_t;

typedef enum {
  TS_FIELD_TYPE_INVALID = 0,
  TS_FIELD_TYPE_UINT = 1,
  TS_FIELD_TYPE_INT = 2,
  TS_FIELD_TYPE_FLOAT = 3,
  TS_FIELD_TYPE_STRING = 4,
  TS_FIELD_TYPE_BOOL = 5
} ts_tuple_field_type_t;

typedef enum { TS_FALSE = 0, TS_TRUE = 1 } ts_bool_value_t;

ts_tuple_field_t ts_tuple_field_set_uint(ts_uint_t value,
                                         ts_bool_t contains_data);
ts_tuple_field_t ts_tuple_field_set_int(ts_int_t value,
                                        ts_bool_t contains_data);
ts_tuple_field_t ts_tuple_field_set_float(ts_float_t value,
                                          ts_bool_t contains_data);
ts_tuple_field_t ts_tuple_field_set_bool(ts_bool_t value,
                                         ts_bool_t contains_data);
ts_tuple_field_t ts_tuple_field_set_string(ts_string_t value,
                                           ts_bool_t contains_data);

ts_tuple_field_type_t ts_tuple_field_get_type(const ts_tuple_field_t* field);

ts_bool_t ts_tuple_field_contains_data(const ts_tuple_field_t* field);

ts_operation_status_t ts_tuple_field_get_uint(const ts_tuple_field_t* field,
                                              ts_uint_t* value);
ts_operation_status_t ts_tuple_field_get_int(const ts_tuple_field_t* field,
                                             ts_int_t* value);
ts_operation_status_t ts_tuple_field_get_float(const ts_tuple_field_t* field,
                                               ts_float_t* value);
ts_operation_status_t ts_tuple_field_get_bool(const ts_tuple_field_t* field,
                                              ts_bool_t* value);
ts_operation_status_t ts_tuple_field_get_string(const ts_tuple_field_t* field,
                                                ts_string_t* value);

ts_bool_t ts_does_all_fields_of_tuple_contain_data(
    const ts_tuple_field_t* tuple, ts_size_t size);

ts_bool_t ts_does_all_fields_of_tuple_span_contain_data(
    ts_tuple_field_t** tuple, ts_size_t size);

ts_bool_t ts_are_equal_tuples(const ts_tuple_field_t* left_tuple,
                              const ts_tuple_field_t* right_tuple,
                              ts_size_t tuple_size);

ts_bool_t ts_is_matching_tuple(const ts_tuple_field_t* template_tuple,
                               const ts_tuple_field_t* data_tuple,
                               ts_size_t tuple_size);

#endif  // __TUPLE_SPACE_H__
