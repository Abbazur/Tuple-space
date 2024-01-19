#ifndef __TUPLE_SPACE_PROTOCOL_H__
#define __TUPLE_SPACE_PROTOCOL_H__

#include "tuple_space.h"
#include "tuple_space_serialization.h"

#define TS_CLIENT_TO_SERVER_ACK_SIZE 1
#define TS_SERVER_TO_CLIENT_ACK_SIZE 1

typedef enum {
  TS_SERVER_TO_CLIENT_MESSAGE_INVALID = 0,
  TS_SERVER_TO_CLIENT_MESSAGE_TUPLE = 1,
  TS_SERVER_TO_CLIENT_MESSAGE_LACK_OF_TUPLE = 2,
  TS_SERVER_TO_CLIENT_MESSAGE_RECEIVED_MSG = 3,
  TS_SERVER_TO_CLIENT_MESSAGE_AWAITING_TUPLE = 4
} ts_server_to_client_message_type_t;

typedef enum {
  TS_CLIENT_TO_SERVER_MESSAGE_INVALID = 0,
  TS_CLIENT_TO_SERVER_MESSAGE_SEND_TUPLE = 1,
  TS_CLIENT_TO_SERVER_MESSAGE_GET_TUPLE = 2,
  TS_CLIENT_TO_SERVER_MESSAGE_RECEIVED_MSG = 3
} ts_client_to_server_message_type_t;

ts_client_to_server_message_type_t ts_client_to_server_message_type(
    const ts_byte_t* buffer, ts_size_t buffer_size);

ts_server_to_client_message_type_t ts_server_to_client_message_type(
    const ts_byte_t* buffer, ts_size_t buffer_size);

ts_bool_t ts_server_to_client_encode_message_type(
    ts_server_to_client_message_type_t message_type, ts_byte_t* buffer,
    ts_size_t buffer_size);

ts_bool_t ts_client_to_server_encode_message_type(
    ts_client_to_server_message_type_t message_type, ts_byte_t* buffer,
    ts_size_t buffer_size);

ts_bool_t ts_client_to_server_encode_received_msg(
    ts_client_to_server_message_type_t message_type, ts_size_t id,
    ts_byte_t* buffer, ts_size_t buffer_size);

ts_bool_t ts_encode_tuple_size(ts_size_t tuple_size, ts_byte_t* buffer,
                               ts_size_t buffer_size);

typedef struct {
  ts_tuple_field_t* tuple;
  ts_size_t tuple_size;
} ts_client_to_server_set_tuple_message_t;

ts_client_to_server_set_tuple_message_t ts_client_to_server_set_tuple_message(
    ts_byte_t* message_buffer, ts_size_t buffer_size,
    const ts_allocator_t* allocator);

typedef struct {
  ts_tuple_field_t* tuple;
  ts_size_t tuple_size;
  ts_bool_t respond_when_available;
  ts_bool_t remove_after_use;
} ts_client_to_server_get_tuple_message_t;

ts_client_to_server_get_tuple_message_t ts_client_to_server_get_tuple_message(
    ts_byte_t* message_buffer, ts_size_t buffer_size,
    const ts_allocator_t* allocator);

#endif  // __TUPLE_SPACE_PROTOCOL_H__
