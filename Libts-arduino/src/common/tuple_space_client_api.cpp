#include "tuple_space_client_api.h"

#include <string.h>

typedef enum {
  TS_APPLICATION_RESPONSE_NO_RESPONSE = 0,
  TS_APPLICATION_RESPONSE_LACK_OF_TUPLE = 1,
  TS_APPLICATION_RESPONSE_TUPLE = 2
} ts_application_response_t;

typedef struct {
  ts_tuple_field_t* tuple;
  ts_size_t tuple_size;
  ts_byte_t error;     // ts_application_error_t
  ts_byte_t response;  // ts_application_response_t
} ts_application_local_context_t;

static void ts_application_internal_error_cb(void* user_data) {
  ts_application_local_context_t* context =
      (ts_application_local_context_t*)user_data;
  context->error = TS_APPLICATION_INTERNAL_ERROR;
}

static void ts_application_lack_of_tuple_cb(void* user_data) {
  ts_application_local_context_t* context =
      (ts_application_local_context_t*)user_data;
  context->response = TS_APPLICATION_RESPONSE_LACK_OF_TUPLE;
}

static void ts_application_tuple_cb(void* user_data, ts_tuple_field_t* tuple,
                                    ts_size_t tuple_size) {
  ts_application_local_context_t* context =
      (ts_application_local_context_t*)user_data;
  context->tuple = tuple;
  context->tuple_size = tuple_size;
  context->response = TS_APPLICATION_RESPONSE_TUPLE;
}

static void ts_application_invalid_tuple_invariant_cb(void* user_data) {
  ts_application_local_context_t* context =
      (ts_application_local_context_t*)user_data;
  context->error = TS_APPLICATION_INVALID_TUPLE;
}

static void ts_application_unknown_sender_cb(void* user_data,
                                             ts_ipv4_t sender_ip_address,
                                             ts_port_t sender_port_id) {
  ts_application_local_context_t* context =
      (ts_application_local_context_t*)user_data;
  context->error = TS_APPLICATION_UNKNOWN_SENDER;
}

static ts_client_receiver_callbacks_t ts_application_create_callbacks(void) {
  ts_client_receiver_callbacks_t callbacks;
  memset(&callbacks, 0, sizeof(ts_client_receiver_callbacks_t));

  callbacks.error_cb = ts_application_internal_error_cb;
  callbacks.lack_of_tuple_cb = ts_application_lack_of_tuple_cb;
  callbacks.tuple_cb = ts_application_tuple_cb;
  callbacks.tuple_invalid_invariant_cb =
      ts_application_invalid_tuple_invariant_cb;
  callbacks.unknown_sender_cb = ts_application_unknown_sender_cb;
  return callbacks;
}

ts_bool_t ts_initialize_application_context(
    ts_application_context_t* application_context,
    ts_device_context_t device_context, ts_ipv4_t server_ip_address,
    ts_port_t server_port_id, ts_size_t max_number_of_repeats,
    ts_uint_t ack_await_time) {
  memset(application_context, 0, sizeof(ts_application_context_t));
  return ts_initialize_client_context(
      &application_context->client_context, device_context,
      ts_application_create_callbacks(), server_ip_address, server_port_id,
      max_number_of_repeats, ack_await_time);
}

ts_application_connection_t ts_application_create_connection(
    ts_application_context_t* context, ts_size_t connection_id) {
  ts_application_connection_t connection;
  memset(&connection, 0, sizeof(ts_application_connection_t));
  connection.app_context = context;
  connection.app_id = connection_id;
  return connection;
}

static ts_bool_t ts_application_send_template(
    ts_application_connection_t* connection, ts_tuple_field_t* template_tuple,
    ts_size_t template_size, const ts_allocator_t* allocator,
    ts_application_in_result_t* result, ts_bool_t is_blocking,
    ts_bool_t is_removing, ts_application_local_context_t* context) {
  ts_tuple_field_t first_field =
      ts_tuple_field_set_uint(connection->app_id, TS_TRUE);
  ts_tuple_field_t** tuple_span = allocator->allocator_cb(
      allocator->context, sizeof(ts_tuple_field_t*) * (template_size + 1));
  tuple_span[0] = &first_field;
  ts_tuple_span(tuple_span + 1, template_tuple, template_size);

  ts_client_to_server_error_t status =
      ts_client_send_client_to_server_get_tuple_span(
          &connection->app_context->client_context, tuple_span,
          template_size + 1, is_removing, is_blocking, context, allocator);

  allocator->deallocator_cb(allocator->context, tuple_span);

  if (status = TS_CLIENT_TO_SERVER_NO_ERROR) {
    result->error = TS_APPLICATION_INTERNAL_ERROR;
    return TS_FALSE;
  }
  return TS_TRUE;
}

static ts_application_in_result_t ts_application_in_impl(
    ts_application_connection_t* connection, ts_tuple_field_t* template_tuple,
    ts_size_t template_size, const ts_allocator_t* allocator,
    ts_bool_t is_blocking, ts_bool_t is_removing) {
  ts_application_in_result_t result;
  memset(&result, 0, sizeof(ts_application_in_result_t));
  if ((template_size > 15) || (ts_does_all_fields_of_tuple_contain_data(
                                  template_tuple, template_size))) {
    result.error = TS_APPLICATION_INVALID_TEMPLATE;
    return result;
  }

  ts_application_local_context_t context;
  memset(&context, 0, sizeof(ts_application_local_context_t));
  if (!ts_application_send_template(connection, template_tuple, template_size,
                                    allocator, &result, is_blocking,
                                    is_removing, &context)) {
    return result;
  }

  if (context.error != TS_APPLICATION_NO_ERROR) {
    result.error = context.error;
    return result;
  }
  if (!is_blocking &&
      (context.response == TS_APPLICATION_RESPONSE_LACK_OF_TUPLE)) {
    return result;
  } else if (context.response != TS_APPLICATION_RESPONSE_TUPLE) {
    result.error = TS_APPLICATION_INVALID_RESPONSE_PROTOCOL;
    return result;
  }

  ts_uint_t id;
  if ((ts_tuple_field_get_uint(context.tuple, &id) != TS_OPERATION_SUCCESS) ||
      (id != connection->app_id)) {
    result.error = TS_APPLICATION_INVALID_RESPONSE_PROTOCOL;
    return result;
  }

  result.tuple = context.tuple + 1;
  result.tuple_size = context.tuple_size - 1;
  return result;
}

ts_application_in_result_t ts_application_in(
    ts_application_connection_t* connection, ts_tuple_field_t* template_tuple,
    ts_size_t template_size, const ts_allocator_t* allocator) {
  return ts_application_in_impl(connection, template_tuple, template_size,
                                allocator, TS_TRUE, TS_TRUE);
}

ts_application_in_result_t ts_application_inp(
    ts_application_connection_t* connection, ts_tuple_field_t* template_tuple,
    ts_size_t template_size, const ts_allocator_t* allocator) {
  return ts_application_in_impl(connection, template_tuple, template_size,
                                allocator, TS_FALSE, TS_TRUE);
}

ts_application_in_result_t ts_application_rd(
    ts_application_connection_t* connection, ts_tuple_field_t* template_tuple,
    ts_size_t template_size, const ts_allocator_t* allocator) {
  return ts_application_in_impl(connection, template_tuple, template_size,
                                allocator, TS_TRUE, TS_FALSE);
}

ts_application_in_result_t ts_application_rdp(
    ts_application_connection_t* connection, ts_tuple_field_t* template_tuple,
    ts_size_t template_size, const ts_allocator_t* allocator) {
  return ts_application_in_impl(connection, template_tuple, template_size,
                                allocator, TS_FALSE, TS_FALSE);
}

ts_application_error_t ts_application_out(
    ts_application_connection_t* connection, ts_tuple_field_t* template_tuple,
    ts_size_t template_size, const ts_allocator_t* allocator) {
  if (!ts_does_all_fields_of_tuple_contain_data(template_tuple,
                                                template_size)) {
    return TS_APPLICATION_INVALID_TUPLE;
  }
  ts_tuple_field_t first_field =
      ts_tuple_field_set_uint(connection->app_id, TS_TRUE);
  ts_tuple_field_t** tuple_span = allocator->allocator_cb(
      allocator->context, sizeof(ts_tuple_field_t*) * (template_size + 1));
  tuple_span[0] = &first_field;
  ts_tuple_span(tuple_span + 1, template_tuple, template_size);

  ts_bool_t status = ts_client_send_client_to_server_send_tuple_span(
      &connection->app_context->client_context, tuple_span, template_size + 1);

  allocator->deallocator_cb(allocator->context, tuple_span);

  return status ? TS_APPLICATION_NO_ERROR : TS_APPLICATION_INTERNAL_ERROR;
}

void ts_free_application_result_tuple(ts_tuple_field_t* tuple, ts_size_t size,
                                      const ts_allocator_t* allocator) {
  ts_deallocate_tuple(tuple - 1, size + 1, allocator);
  allocator->deallocator_cb(allocator->context, tuple - 1);
}
