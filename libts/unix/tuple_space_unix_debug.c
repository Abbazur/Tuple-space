#include "tuple_space_unix_debug.h"

#include <stdio.h>

static void ts_print_tuple_field_type(const ts_tuple_field_t* field) {
  printf("Type: ");
  switch (ts_tuple_field_get_type(field)) {
    case TS_FIELD_TYPE_BOOL:
      printf("Bool");
      return;
    case TS_FIELD_TYPE_FLOAT:
      printf("Float");
      return;
    case TS_FIELD_TYPE_INT:
      printf("Int");
      return;
    case TS_FIELD_TYPE_UINT:
      printf("UInt");
      return;
    case TS_FIELD_TYPE_STRING:
      printf("String");
      return;
    default:
      printf("Invalid");
      break;
  }
}

static void ts_print_tuple_field_payload(const ts_tuple_field_t* field) {
  printf("Payload: ");
  switch (ts_tuple_field_get_type(field)) {
    case TS_FIELD_TYPE_BOOL:
      printf(field->data.bool_field ? "True" : "False");
      return;
    case TS_FIELD_TYPE_FLOAT:
      printf("%f", field->data.float_field);
      return;
    case TS_FIELD_TYPE_INT:
      printf("%d", field->data.int_field);
      return;
    case TS_FIELD_TYPE_UINT:
      printf("%u", field->data.uint_field);
      return;
    case TS_FIELD_TYPE_STRING:
      printf("\"%s\"", field->data.string_field);
      return;
    default:
      printf("?");
      break;
  }
}

static void ts_print_tuple_field(const ts_tuple_field_t* field) {
  printf("(");
  ts_print_tuple_field_type(field);
  printf(", ");
  if (ts_tuple_field_contains_data(field)) {
    ts_print_tuple_field_payload(field);
  } else {
    printf("Field does not contain data");
  }
  printf(")");
}

void ts_print_tuple(const ts_tuple_field_t* tuple, ts_size_t tuple_size) {
  printf("{\n");
  for (ts_size_t i = 0; i < tuple_size; ++i) {
    printf("\t");
    ts_print_tuple_field(&tuple[i]);
    printf(",\n");
  }
  printf("}\n");
}
