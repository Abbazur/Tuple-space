#include "tuple_space_network.h"

#include <string.h>

#include "tuple_space_serialization.h"

ts_bool_t ts_initialize_server_context(
    ts_server_context_t* server_context, ts_device_context_t device_context,
    ts_server_receiver_callbacks_t server_callbacks) {
  if (device_context.current_state != TS_DEVICE_INITIALIZED) {
    return TS_FALSE;
  }
  memset(server_context, 0, sizeof(ts_server_context_t));
  server_context->device_context = device_context;
  server_context->server_callbacks = server_callbacks;
  return TS_TRUE;
}

ts_bool_t ts_initialize_client_context(
    ts_client_context_t* client_context, ts_device_context_t device_context,
    ts_client_receiver_callbacks_t client_callbacks,
    ts_ipv4_t server_ip_address, ts_port_t server_port_id,
    ts_size_t max_number_of_repeats, ts_uint_t ack_await_time) {
  if (device_context.current_state != TS_DEVICE_INITIALIZED) {
    return TS_FALSE;
  }
  memset(client_context, 0, sizeof(ts_client_context_t));
  client_context->device_context = device_context;
  client_context->server_ip_address = server_ip_address;
  client_context->server_port_id = server_port_id;
  client_context->client_callbacks = client_callbacks;
  client_context->ack_await_time = ack_await_time;
  client_context->max_number_of_repeats = max_number_of_repeats;
  client_context->had_got_delay_message = TS_FALSE;
  return TS_TRUE;
}

static ts_bool_t ts_server_listen_for_message(
    ts_server_context_t* server_context, ts_ipv4_t* sender_ip_address,
    ts_port_t* sender_port_id, ts_byte_t* buffer, ts_size_t* buffer_size,
    ts_client_to_server_message_type_t* message_type) {
  ts_size_t result_size = server_context->device_context.recv_cb(
      &server_context->device_context, sender_ip_address, sender_port_id,
      buffer, *buffer_size);
  if (result_size == 0) {
    return TS_FALSE;
  }
  *buffer_size = result_size;
  *message_type = ts_client_to_server_message_type(buffer, *buffer_size);
  return TS_TRUE;
}

static ts_bool_t ts_client_listen_for_message(
    ts_client_context_t* client_context, ts_ipv4_t* sender_ip_address,
    ts_port_t* sender_port_id, ts_byte_t* buffer, ts_size_t* buffer_size,
    ts_server_to_client_message_type_t* message_type) {
  ts_size_t result_size = client_context->device_context.recv_cb(
      &client_context->device_context, sender_ip_address, sender_port_id,
      buffer, *buffer_size);
  if (result_size == 0) {
    return TS_FALSE;
  }
  *buffer_size = result_size;
  *message_type = ts_server_to_client_message_type(buffer, *buffer_size);
  return TS_TRUE;
}

static void ts_server_process_send_message(ts_server_context_t* server_context,
                                           const ts_byte_t* buffer,
                                           ts_size_t buffer_size,
                                           ts_ipv4_t sender_ip_address,
                                           ts_port_t sender_port_id,
                                           const ts_allocator_t* allocator,
                                           void* user_data) {
  ts_size_t tuple_size = ts_serialized_tuple_size(buffer, buffer_size);
  if (tuple_size == 0) {
    server_context->server_callbacks.serialization_issue_cb(
        user_data, TS_CLIENT_TO_SERVER_MESSAGE_SEND_TUPLE, sender_ip_address,
        sender_port_id);
    return;
  }
  ts_tuple_field_t* tuple = (ts_tuple_field_t*)allocator->allocator_cb(
      allocator->context, sizeof(ts_tuple_field_t) * tuple_size);
  if (!ts_deserialize_tuple(buffer + 1, buffer_size - 1, tuple, tuple_size,
                            allocator)) {
    allocator->deallocator_cb(allocator->context, tuple);
    server_context->server_callbacks.serialization_issue_cb(
        user_data, TS_CLIENT_TO_SERVER_MESSAGE_SEND_TUPLE, sender_ip_address,
        sender_port_id);
    return;
  }
  if (!ts_does_all_fields_of_tuple_contain_data(tuple, tuple_size)) {
    server_context->server_callbacks.send_tuple_invalid_invariant_cb(
        user_data, sender_ip_address, sender_port_id);
    ts_free_tuple(tuple, tuple_size, allocator);
    return;
  }
  server_context->server_callbacks.send_tuple_cb(
      user_data, tuple, tuple_size, sender_ip_address, sender_port_id);
}

static void ts_server_process_get_message(ts_server_context_t* server_context,
                                          const ts_byte_t* buffer,
                                          ts_size_t buffer_size,
                                          ts_ipv4_t sender_ip_address,
                                          ts_port_t sender_port_id,
                                          const ts_allocator_t* allocator,
                                          void* user_data) {
  ts_size_t tuple_size = ts_serialized_tuple_size(buffer, buffer_size);
  if (tuple_size < 2) {
    server_context->server_callbacks.serialization_issue_cb(
        user_data, TS_CLIENT_TO_SERVER_MESSAGE_GET_TUPLE, sender_ip_address,
        sender_port_id);
    return;
  }
  ts_tuple_field_t* tuple = (ts_tuple_field_t*)allocator->allocator_cb(
      allocator->context, sizeof(ts_tuple_field_t) * tuple_size);
  ts_byte_t remove_flag = (buffer[1] & 0x01) > 0;
  ts_byte_t respond_flag = (buffer[1] & 0x02) > 0;
  if (!ts_deserialize_tuple(buffer + 2, buffer_size - 2, tuple, tuple_size,
                            allocator)) {
    allocator->deallocator_cb(allocator->context, tuple);
    server_context->server_callbacks.serialization_issue_cb(
        user_data, TS_CLIENT_TO_SERVER_MESSAGE_GET_TUPLE, sender_ip_address,
        sender_port_id);
    return;
  }
  if (ts_does_all_fields_of_tuple_contain_data(tuple, tuple_size)) {
    server_context->server_callbacks.get_tuple_invalid_invariant_cb(
        user_data, sender_ip_address, sender_port_id);
    ts_free_tuple(tuple, tuple_size, allocator);
    return;
  }
  server_context->server_callbacks.get_tuple_cb(
      user_data, tuple, tuple_size, respond_flag, remove_flag,
      sender_ip_address, sender_port_id);
}

void ts_server_get_message(ts_server_context_t* server_context, void* user_data,
                           const ts_allocator_t* allocator) {
  ts_byte_t buffer[TS_BUFFER_SIZE];
  ts_size_t buffer_size = TS_BUFFER_SIZE;
  ts_ipv4_t sender_ip_address;
  ts_port_t sender_port_id;
  ts_client_to_server_message_type_t message_type;
  if (!ts_server_listen_for_message(server_context, &sender_ip_address,
                                    &sender_port_id, buffer, &buffer_size,
                                    &message_type)) {
    return;
  }
  switch (message_type) {
    case TS_CLIENT_TO_SERVER_MESSAGE_SEND_TUPLE:
      return ts_server_process_send_message(server_context, buffer, buffer_size,
                                            sender_ip_address, sender_port_id,
                                            allocator, user_data);
    case TS_CLIENT_TO_SERVER_MESSAGE_GET_TUPLE:
      return ts_server_process_get_message(server_context, buffer, buffer_size,
                                           sender_ip_address, sender_port_id,
                                           allocator, user_data);
    case TS_CLIENT_TO_SERVER_MESSAGE_RECEIVED_MSG:
      return server_context->server_callbacks.ack_cb(
          user_data, sender_ip_address, sender_port_id);
    case TS_CLIENT_TO_SERVER_MESSAGE_INVALID:
    default:
      server_context->server_callbacks.error_cb(user_data);
      return;
  }
}

static void ts_client_process_tuple_message(ts_client_context_t* client_context,
                                            const ts_byte_t* buffer,
                                            ts_size_t buffer_size,
                                            const ts_allocator_t* allocator,
                                            void* user_data) {
  ts_size_t tuple_size = ts_serialized_tuple_size(buffer, buffer_size);
  if (tuple_size == 0) {
    return client_context->client_callbacks.tuple_invalid_invariant_cb(
        user_data);
  }
  ts_tuple_field_t* tuple = (ts_tuple_field_t*)allocator->allocator_cb(
      allocator->context, sizeof(ts_tuple_field_t) * tuple_size);
  if (!ts_deserialize_tuple(buffer + 1, buffer_size - 1, tuple, tuple_size,
                            allocator)) {
    allocator->deallocator_cb(allocator->context, tuple);
    return client_context->client_callbacks.tuple_invalid_invariant_cb(
        user_data);
  }
  if (!ts_does_all_fields_of_tuple_contain_data(tuple, tuple_size)) {
    client_context->client_callbacks.tuple_invalid_invariant_cb(user_data);
    return ts_free_tuple(tuple, tuple_size, allocator);
  }
  client_context->client_callbacks.tuple_cb(user_data, tuple, tuple_size);
}

static void ts_client_process_lack_of_tuple_message(
    ts_client_context_t* client_context, void* user_data) {
  client_context->client_callbacks.lack_of_tuple_cb(user_data);
}

ts_bool_t ts_client_get_message(ts_client_context_t* client_context,
                                void* user_data,
                                const ts_allocator_t* allocator) {
  ts_byte_t buffer[TS_BUFFER_SIZE];
  ts_size_t buffer_size = TS_BUFFER_SIZE;
  ts_ipv4_t sender_ip_address;
  ts_port_t sender_port_id;
  ts_server_to_client_message_type_t message_type;
  if (!ts_client_listen_for_message(client_context, &sender_ip_address,
                                    &sender_port_id, buffer, &buffer_size,
                                    &message_type)) {
    return TS_FALSE;
  }
  if (client_context->device_context.can_receive_ip &&
      ((sender_ip_address != client_context->server_ip_address) ||
       (sender_port_id != client_context->server_port_id))) {
    client_context->client_callbacks.unknown_sender_cb(
        user_data, sender_ip_address, sender_port_id);
    return TS_TRUE;
  }
  switch (message_type) {
    case TS_SERVER_TO_CLIENT_MESSAGE_TUPLE:
      ts_client_process_tuple_message(client_context, buffer, buffer_size,
                                      allocator, user_data);
      break;
    case TS_SERVER_TO_CLIENT_MESSAGE_LACK_OF_TUPLE:
      ts_client_process_lack_of_tuple_message(client_context, user_data);
      break;
    case TS_SERVER_TO_CLIENT_MESSAGE_AWAITING_TUPLE:
      client_context->had_got_delay_message = TS_TRUE;
      break;
    case TS_SERVER_TO_CLIENT_MESSAGE_INVALID:
    default:
      client_context->client_callbacks.error_cb(user_data);
      break;
  }
  return TS_TRUE;
}

void ts_close_server_context(ts_server_context_t* server_context) {
  server_context->device_context.destructor_cb(&server_context->device_context);
}

void ts_close_client_context(ts_client_context_t* client_context) {
  client_context->device_context.destructor_cb(&client_context->device_context);
}

void ts_free_tuple(ts_tuple_field_t* tuple, ts_size_t tuple_size,
                   const ts_allocator_t* allocator) {
  ts_deallocate_tuple(tuple, tuple_size, allocator);
  allocator->deallocator_cb(allocator->context, tuple);
}

ts_bool_t ts_server_send_server_to_client_tuple(
    ts_server_context_t* server_context, ts_tuple_field_t* tuple,
    ts_size_t tuple_size, ts_ipv4_t client_ip_address,
    ts_port_t client_port_id) {
  ts_byte_t buffer[TS_BUFFER_SIZE];
  if (!ts_server_to_client_encode_message_type(
          TS_SERVER_TO_CLIENT_MESSAGE_TUPLE, buffer, TS_BUFFER_SIZE)) {
    return TS_FALSE;
  }
  if (!ts_encode_tuple_size(tuple_size, buffer, TS_BUFFER_SIZE)) {
    return TS_FALSE;
  }
  ts_size_t buffer_size =
      ts_serialize_tuple(tuple, tuple_size, buffer + 1, TS_BUFFER_SIZE - 1);
  if (buffer_size == 0) {
    return TS_FALSE;
  }
  return server_context->device_context.send_cb(
      &server_context->device_context, client_ip_address, client_port_id,
      buffer, buffer_size + 1);
}

ts_bool_t ts_server_send_server_to_client_tuple_span(
    ts_server_context_t* server_context, ts_tuple_field_t** tuple,
    ts_size_t tuple_size, ts_ipv4_t client_ip_address,
    ts_port_t client_port_id) {
  ts_byte_t buffer[TS_BUFFER_SIZE];
  if (!ts_server_to_client_encode_message_type(
          TS_SERVER_TO_CLIENT_MESSAGE_TUPLE, buffer, TS_BUFFER_SIZE)) {
    return TS_FALSE;
  }
  if (!ts_encode_tuple_size(tuple_size, buffer, TS_BUFFER_SIZE)) {
    return TS_FALSE;
  }
  ts_size_t buffer_size = ts_serialize_tuple_span(tuple, tuple_size, buffer + 1,
                                                  TS_BUFFER_SIZE - 1);
  if (buffer_size == 0) {
    return TS_FALSE;
  }
  return server_context->device_context.send_cb(
      &server_context->device_context, client_ip_address, client_port_id,
      buffer, buffer_size + 1);
}

static ts_bool_t ts_server_send_server_to_client_payloadless_msg(
    ts_server_context_t* server_context, ts_ipv4_t client_ip_address,
    ts_port_t client_port_id, ts_server_to_client_message_type_t message_type) {
  ts_byte_t buffer[1];
  if (!ts_server_to_client_encode_message_type(message_type, buffer, 1)) {
    return TS_FALSE;
  }
  return server_context->device_context.send_cb(&server_context->device_context,
                                                client_ip_address,
                                                client_port_id, buffer, 1);
}

ts_bool_t ts_server_send_server_to_client_lack_of_tuple(
    ts_server_context_t* server_context, ts_ipv4_t client_ip_address,
    ts_port_t client_port_id) {
  return ts_server_send_server_to_client_payloadless_msg(
      server_context, client_ip_address, client_port_id,
      TS_SERVER_TO_CLIENT_MESSAGE_LACK_OF_TUPLE);
}

ts_bool_t ts_server_send_server_to_client_ack(
    ts_server_context_t* server_context, ts_ipv4_t client_ip_address,
    ts_port_t client_port_id) {
  return ts_server_send_server_to_client_payloadless_msg(
      server_context, client_ip_address, client_port_id,
      TS_SERVER_TO_CLIENT_MESSAGE_RECEIVED_MSG);
}

ts_bool_t ts_server_send_server_to_client_await_for_tuple(
    ts_server_context_t* server_context, ts_ipv4_t client_ip_address,
    ts_port_t client_port_id) {
  return ts_server_send_server_to_client_payloadless_msg(
      server_context, client_ip_address, client_port_id,
      TS_SERVER_TO_CLIENT_MESSAGE_AWAITING_TUPLE);
}

static ts_bool_t ts_client_send_client_to_server_receive_ack(
    ts_client_context_t* client_context) {
  ts_byte_t buffer[TS_BUFFER_SIZE];
  ts_port_t port_id;
  ts_ipv4_t ip_address;
  ts_size_t recv_size = client_context->device_context.recv_cb(
      &client_context->device_context, &ip_address, &port_id, buffer,
      TS_BUFFER_SIZE);
  if (recv_size == 0) {
    return TS_FALSE;
  }
  if ((port_id != client_context->server_port_id) ||
      (ip_address != client_context->server_ip_address)) {
    return TS_FALSE;
  }
  return ts_server_to_client_message_type(buffer, recv_size) ==
         TS_SERVER_TO_CLIENT_MESSAGE_RECEIVED_MSG;
}

static ts_bool_t ts_client_send_client_to_server_send_tuple_protocol(
    ts_client_context_t* client_context, const ts_byte_t* buffer,
    ts_size_t buffer_size) {
  ts_size_t repeats = client_context->max_number_of_repeats;
  do {
    if (client_context->device_context.send_cb(
            &client_context->device_context, client_context->server_ip_address,
            client_context->server_port_id, buffer, buffer_size) == 0) {
      return TS_FALSE;
    }
    const ts_uint_t send_time = client_context->device_context.clock_cb();
    if (client_context->ack_await_time == 0) {
      return TS_TRUE;
    }
    ts_uint_t current_time = send_time;
    while ((send_time + client_context->ack_await_time) > current_time) {
      if (ts_client_send_client_to_server_receive_ack(client_context)) {
        return TS_TRUE;
      }
      current_time = client_context->device_context.clock_cb();
    }
  } while ((repeats == 0) || (--repeats));
  return TS_FALSE;
}

ts_bool_t ts_client_send_client_to_server_send_tuple(
    ts_client_context_t* client_context, ts_tuple_field_t* tuple,
    ts_size_t tuple_size) {
  ts_byte_t buffer[TS_BUFFER_SIZE];
  if (!ts_client_to_server_encode_message_type(
          TS_CLIENT_TO_SERVER_MESSAGE_SEND_TUPLE, buffer, TS_BUFFER_SIZE)) {
    return TS_FALSE;
  }
  if (!ts_encode_tuple_size(tuple_size, buffer, TS_BUFFER_SIZE)) {
    return TS_FALSE;
  }
  ts_size_t buffer_size =
      ts_serialize_tuple(tuple, tuple_size, buffer + 1, TS_BUFFER_SIZE - 1);
  if (buffer_size == 0) {
    return TS_FALSE;
  }
  return ts_client_send_client_to_server_send_tuple_protocol(
      client_context, buffer, buffer_size + 1);
}

ts_bool_t ts_client_send_client_to_server_send_tuple_span(
    ts_client_context_t* client_context, ts_tuple_field_t** tuple,
    ts_size_t tuple_size) {
  ts_byte_t buffer[TS_BUFFER_SIZE];
  if (!ts_client_to_server_encode_message_type(
          TS_CLIENT_TO_SERVER_MESSAGE_SEND_TUPLE, buffer, TS_BUFFER_SIZE)) {
    return TS_FALSE;
  }
  if (!ts_encode_tuple_size(tuple_size, buffer, TS_BUFFER_SIZE)) {
    return TS_FALSE;
  }
  ts_size_t buffer_size = ts_serialize_tuple_span(tuple, tuple_size, buffer + 1,
                                                  TS_BUFFER_SIZE - 1);
  if (buffer_size == 0) {
    return TS_FALSE;
  }
  return ts_client_send_client_to_server_send_tuple_protocol(
      client_context, buffer, buffer_size + 1);
}

static ts_bool_t ts_client_to_server_resolve_reponse_ack(
    ts_client_context_t* client_context) {
  const ts_uint_t send_time = client_context->device_context.clock_cb();
  ts_uint_t current_time = send_time;
  while ((send_time + client_context->ack_await_time) > current_time) {
    ts_byte_t recv_buf[TS_SERVER_TO_CLIENT_ACK_SIZE];
    ts_ipv4_t sender_ip;
    ts_port_t sender_port;
    if (client_context->device_context.recv_cb(
            &client_context->device_context, &sender_ip, &sender_port, recv_buf,
            TS_SERVER_TO_CLIENT_ACK_SIZE)) {
      if ((sender_ip == client_context->server_ip_address) &&
          (sender_port == client_context->server_port_id) &&
          (ts_server_to_client_message_type(recv_buf,
                                            TS_SERVER_TO_CLIENT_ACK_SIZE) ==
           TS_SERVER_TO_CLIENT_MESSAGE_RECEIVED_MSG)) {
        return TS_TRUE;
      }
    }
    current_time = client_context->device_context.clock_cb();
  }
  return TS_FALSE;
}

static void ts_client_to_server_resolve_ack(
    ts_client_context_t* client_context) {
  ts_byte_t ack_buffer[TS_CLIENT_TO_SERVER_ACK_SIZE];
  if (!ts_client_to_server_encode_message_type(
          TS_CLIENT_TO_SERVER_MESSAGE_RECEIVED_MSG, ack_buffer,
          TS_CLIENT_TO_SERVER_ACK_SIZE)) {
    return;
  }
  ts_size_t repeats = client_context->max_number_of_repeats;
  do {
    if (client_context->device_context.send_cb(
            &client_context->device_context, client_context->server_ip_address,
            client_context->server_port_id, ack_buffer,
            TS_CLIENT_TO_SERVER_ACK_SIZE) == 0) {
      return;
    }
    if (ts_client_to_server_resolve_reponse_ack(client_context)) {
      return;
    }
  } while ((repeats == 0) || (--repeats));
}

static ts_client_to_server_error_t ts_client_send_client_to_server_portocol(
    ts_client_context_t* client_context, ts_byte_t* buffer,
    ts_size_t buffer_size, void* user_data, const ts_allocator_t* allocator) {
  ts_size_t repeats = client_context->max_number_of_repeats;
  do {
    if (client_context->device_context.send_cb(
            &client_context->device_context, client_context->server_ip_address,
            client_context->server_port_id, buffer, buffer_size) == 0) {
      return TS_CLIENT_TO_SERVER_INTERNAL_ERROR;
    }
    const ts_uint_t send_time = client_context->device_context.clock_cb();

    ts_uint_t current_time = send_time;
    while ((send_time + client_context->ack_await_time) > current_time) {
      if (ts_client_get_message(client_context, user_data, allocator)) {
        if (client_context->had_got_delay_message) {
          client_context->had_got_delay_message = TS_FALSE;
          if (repeats) {
            ++repeats;
          }
        } else {
          ts_client_to_server_resolve_ack(client_context);
          return TS_CLIENT_TO_SERVER_NO_ERROR;
        }
      }
      current_time = client_context->device_context.clock_cb();
    }
  } while ((repeats == 0) || (--repeats));
  return TS_CLIENT_TO_SERVER_INTERNAL_ERROR;
}

ts_client_to_server_error_t ts_client_send_client_to_server_get_tuple(
    ts_client_context_t* client_context, ts_tuple_field_t* tuple,
    ts_size_t tuple_size, ts_bool_t remove_after_use_flag,
    ts_bool_t respond_when_available_flag, void* user_data,
    const ts_allocator_t* allocator) {
  ts_byte_t buffer[TS_BUFFER_SIZE];
  if (!ts_client_to_server_encode_message_type(
          TS_CLIENT_TO_SERVER_MESSAGE_GET_TUPLE, buffer, TS_BUFFER_SIZE)) {
    return TS_CLIENT_TO_SERVER_INTERNAL_ERROR;
  }
  if (!ts_encode_tuple_size(tuple_size, buffer, TS_BUFFER_SIZE)) {
    return TS_CLIENT_TO_SERVER_INTERNAL_ERROR;
  }
  ts_size_t buffer_size =
      ts_serialize_tuple(tuple, tuple_size, buffer + 2, TS_BUFFER_SIZE - 2);
  if (buffer_size == 0) {
    return TS_CLIENT_TO_SERVER_INTERNAL_ERROR;
  }
  buffer[1] = (remove_after_use_flag ? 0x01 : 0x00) |
              (respond_when_available_flag ? 0x02 : 0x00);
  return ts_client_send_client_to_server_portocol(
      client_context, buffer, buffer_size + 2, user_data, allocator);
}

ts_client_to_server_error_t ts_client_send_client_to_server_get_tuple_span(
    ts_client_context_t* client_context, ts_tuple_field_t** tuple,
    ts_size_t tuple_size, ts_bool_t remove_after_use_flag,
    ts_bool_t respond_when_available_flag, void* user_data,
    const ts_allocator_t* allocator) {
  ts_byte_t buffer[TS_BUFFER_SIZE];
  if (!ts_client_to_server_encode_message_type(
          TS_CLIENT_TO_SERVER_MESSAGE_GET_TUPLE, buffer, TS_BUFFER_SIZE)) {
    return TS_CLIENT_TO_SERVER_INTERNAL_ERROR;
  }
  if (!ts_encode_tuple_size(tuple_size, buffer, TS_BUFFER_SIZE)) {
    return TS_CLIENT_TO_SERVER_INTERNAL_ERROR;
  }
  ts_size_t buffer_size = ts_serialize_tuple_span(tuple, tuple_size, buffer + 2,
                                                  TS_BUFFER_SIZE - 2);
  if (buffer_size == 0) {
    return TS_CLIENT_TO_SERVER_INTERNAL_ERROR;
  }
  buffer[1] = (remove_after_use_flag ? 0x01 : 0x00) |
              (respond_when_available_flag ? 0x02 : 0x00);
  return ts_client_send_client_to_server_portocol(
      client_context, buffer, buffer_size + 2, user_data, allocator);
}
