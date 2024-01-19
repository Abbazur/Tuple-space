#include "tuple_space.h"

#include <string.h>

static void ts_tuple_field_set_contains_data_flag(ts_tuple_field_t* field,
                                                  ts_bool_t contains_data) {
  if (contains_data) {
    field->flags |= 0x80;
  } else {
    field->flags &= 0x7f;
  }
}

static void ts_tuple_field_set_type_flag(ts_tuple_field_t* field,
                                         ts_tuple_field_type_t field_type) {
  field->flags &= 0x80;
  field->flags |= field_type & 0x3f;
}

ts_tuple_field_t ts_tuple_field_set_uint(ts_uint_t value,
                                         ts_bool_t contains_data) {
  ts_tuple_field_t tuple_field;
  memset(&tuple_field, 0, sizeof(ts_tuple_field_t));
  ts_tuple_field_set_contains_data_flag(&tuple_field, contains_data);
  ts_tuple_field_set_type_flag(&tuple_field, TS_FIELD_TYPE_UINT);
  tuple_field.data.uint_field = value;
  return tuple_field;
}

ts_tuple_field_t ts_tuple_field_set_int(ts_int_t value,
                                        ts_bool_t contains_data) {
  ts_tuple_field_t tuple_field;
  memset(&tuple_field, 0, sizeof(ts_tuple_field_t));
  ts_tuple_field_set_contains_data_flag(&tuple_field, contains_data);
  ts_tuple_field_set_type_flag(&tuple_field, TS_FIELD_TYPE_INT);
  tuple_field.data.int_field = value;
  return tuple_field;
}

ts_tuple_field_t ts_tuple_field_set_float(ts_float_t value,
                                          ts_bool_t contains_data) {
  ts_tuple_field_t tuple_field;
  memset(&tuple_field, 0, sizeof(ts_tuple_field_t));
  ts_tuple_field_set_contains_data_flag(&tuple_field, contains_data);
  ts_tuple_field_set_type_flag(&tuple_field, TS_FIELD_TYPE_FLOAT);
  tuple_field.data.float_field = value;
  return tuple_field;
}

ts_tuple_field_t ts_tuple_field_set_bool(ts_bool_t value,
                                         ts_bool_t contains_data) {
  ts_tuple_field_t tuple_field;
  memset(&tuple_field, 0, sizeof(ts_tuple_field_t));
  ts_tuple_field_set_contains_data_flag(&tuple_field, contains_data);
  ts_tuple_field_set_type_flag(&tuple_field, TS_FIELD_TYPE_BOOL);
  tuple_field.data.bool_field = value;
  return tuple_field;
}

ts_tuple_field_t ts_tuple_field_set_string(ts_string_t value,
                                           ts_bool_t contains_data) {
  ts_tuple_field_t tuple_field;
  memset(&tuple_field, 0, sizeof(ts_tuple_field_t));
  ts_tuple_field_set_contains_data_flag(&tuple_field, contains_data);
  ts_tuple_field_set_type_flag(&tuple_field, TS_FIELD_TYPE_STRING);
  tuple_field.data.string_field = value;
  return tuple_field;
}

ts_tuple_field_type_t ts_tuple_field_get_type(const ts_tuple_field_t* field) {
  return field->flags & 0x3f;
}

ts_bool_t ts_tuple_field_contains_data(const ts_tuple_field_t* field) {
  return (field->flags & 0x80) != 0;
}

ts_operation_status_t ts_tuple_field_get_uint(const ts_tuple_field_t* field,
                                              ts_uint_t* value) {
  if (ts_tuple_field_get_type(field) != TS_FIELD_TYPE_UINT) {
    return TS_OPERATION_FAILURE;
  }
  *value = field->data.uint_field;
  return TS_OPERATION_SUCCESS;
}

ts_operation_status_t ts_tuple_field_get_int(const ts_tuple_field_t* field,
                                             ts_int_t* value) {
  if (ts_tuple_field_get_type(field) != TS_FIELD_TYPE_INT) {
    return TS_OPERATION_FAILURE;
  }
  *value = field->data.int_field;
  return TS_OPERATION_SUCCESS;
}

ts_operation_status_t ts_tuple_field_get_float(const ts_tuple_field_t* field,
                                               ts_float_t* value) {
  if (ts_tuple_field_get_type(field) != TS_FIELD_TYPE_FLOAT) {
    return TS_OPERATION_FAILURE;
  }
  *value = field->data.float_field;
  return TS_OPERATION_SUCCESS;
}

ts_operation_status_t ts_tuple_field_get_bool(const ts_tuple_field_t* field,
                                              ts_bool_t* value) {
  if (ts_tuple_field_get_type(field) != TS_FIELD_TYPE_BOOL) {
    return TS_OPERATION_FAILURE;
  }
  *value = field->data.bool_field;
  return TS_OPERATION_SUCCESS;
}

ts_operation_status_t ts_tuple_field_get_string(const ts_tuple_field_t* field,
                                                ts_string_t* value) {
  if (ts_tuple_field_get_type(field) != TS_FIELD_TYPE_STRING) {
    return TS_OPERATION_FAILURE;
  }
  *value = field->data.string_field;
  return TS_OPERATION_SUCCESS;
}

ts_bool_t ts_does_all_fields_of_tuple_contain_data(
    const ts_tuple_field_t* tuple, ts_size_t size) {
  ts_bool_t status = TS_TRUE;
  for (ts_size_t i = 0; i < size; ++i) {
    status &= ts_tuple_field_contains_data(&tuple[i]);
  }
  return status;
}

ts_bool_t ts_does_all_fields_of_tuple_span_contain_data(
    ts_tuple_field_t** tuple, ts_size_t size) {
  ts_bool_t status = TS_TRUE;
  for (ts_size_t i = 0; i < size; ++i) {
    status &= ts_tuple_field_contains_data(tuple[i]);
  }
  return status;
}

static ts_bool_t ts_compare_tuple_fields(const ts_tuple_field_t* left_tuple,
                                         const ts_tuple_field_t* right_tuple) {
  switch (ts_tuple_field_get_type(left_tuple)) {
    case TS_FIELD_TYPE_BOOL:
      return left_tuple->data.bool_field == right_tuple->data.bool_field;
    case TS_FIELD_TYPE_FLOAT:
      return left_tuple->data.float_field == right_tuple->data.float_field;
    case TS_FIELD_TYPE_INT:
      return left_tuple->data.int_field == right_tuple->data.int_field;
    case TS_FIELD_TYPE_UINT:
      return left_tuple->data.uint_field == right_tuple->data.uint_field;
    case TS_FIELD_TYPE_STRING:
      return strcmp(left_tuple->data.string_field,
                    right_tuple->data.string_field) == 0;
    default:
      return TS_FALSE;
  }
  return TS_FALSE;
}

ts_bool_t ts_are_equal_tuples(const ts_tuple_field_t* left_tuple,
                              const ts_tuple_field_t* right_tuple,
                              ts_size_t tuple_size) {
  for (ts_size_t i = 0; i < tuple_size; ++i) {
    if (ts_tuple_field_get_type(&left_tuple[i]) !=
        ts_tuple_field_get_type(&right_tuple[i])) {
      return TS_FALSE;
    }
    if (!ts_compare_tuple_fields(&left_tuple[i], &right_tuple[i])) {
      return TS_FALSE;
    }
  }
  return TS_TRUE;
}

ts_bool_t ts_is_matching_tuple(const ts_tuple_field_t* template_tuple,
                               const ts_tuple_field_t* data_tuple,
                               ts_size_t tuple_size) {
  for (ts_size_t i = 0; i < tuple_size; ++i) {
    if (ts_tuple_field_get_type(&template_tuple[i]) !=
        ts_tuple_field_get_type(&data_tuple[i])) {
      return TS_FALSE;
    }
    if (!ts_tuple_field_contains_data(&template_tuple[i])) {
      continue;
    }
    if (!ts_compare_tuple_fields(&template_tuple[i], &data_tuple[i])) {
      return TS_FALSE;
    }
  }
  return TS_TRUE;
}
