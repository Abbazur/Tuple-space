#ifndef __TUPLE_SPACE_CLIENT_API_H__
#define __TUPLE_SPACE_CLIENT_API_H__

#include "tuple_space_network.h"

typedef struct {
  ts_client_context_t client_context;
} ts_application_context_t;

typedef struct {
  ts_application_context_t* app_context;
  ts_size_t app_id;
} ts_application_connection_t;

ts_bool_t ts_initialize_application_context(
    ts_application_context_t* application_context,
    ts_device_context_t device_context, ts_ipv4_t server_ip_address,
    ts_port_t server_port_id, ts_size_t max_number_of_repeats,
    ts_uint_t ack_await_time);

ts_application_connection_t ts_application_create_connection(
    ts_application_context_t* context, ts_size_t connection_id);

typedef enum {
  TS_APPLICATION_NO_ERROR = 0,
  TS_APPLICATION_INTERNAL_ERROR = 1,
  TS_APPLICATION_INVALID_TUPLE = 2,
  TS_APPLICATION_UNKNOWN_SENDER = 3,
  TS_APPLICATION_INVALID_TEMPLATE = 4,
  TS_APPLICATION_INVALID_RESPONSE_PROTOCOL = 5
} ts_application_error_t;

typedef struct {
  ts_tuple_field_t* tuple;
  ts_size_t tuple_size;
  ts_application_error_t error;
} ts_application_in_result_t;

ts_application_in_result_t ts_application_in(
    ts_application_connection_t* connection, ts_tuple_field_t* template_tuple,
    ts_size_t template_size, const ts_allocator_t* allocator);

ts_application_in_result_t ts_application_inp(
    ts_application_connection_t* connection, ts_tuple_field_t* template_tuple,
    ts_size_t template_size, const ts_allocator_t* allocator);

ts_application_in_result_t ts_application_rd(
    ts_application_connection_t* connection, ts_tuple_field_t* template_tuple,
    ts_size_t template_size, const ts_allocator_t* allocator);

ts_application_in_result_t ts_application_rdp(
    ts_application_connection_t* connection, ts_tuple_field_t* template_tuple,
    ts_size_t template_size, const ts_allocator_t* allocator);

ts_application_error_t ts_application_out(
    ts_application_connection_t* connection, ts_tuple_field_t* template_tuple,
    ts_size_t template_size, const ts_allocator_t* allocator);

void ts_free_application_result_tuple(ts_tuple_field_t* tuple, ts_size_t size,
                                      const ts_allocator_t* allocator);

#endif  // __TUPLE_SPACE_CLIENT_API_H__
