#include "tuple_space_protocol.h"

ts_client_to_server_message_type_t ts_client_to_server_message_type(
    const ts_byte_t* buffer, ts_size_t buffer_size) {
  if (buffer_size < 1) {
    return TS_CLIENT_TO_SERVER_MESSAGE_INVALID;
  }
  switch (buffer[0] & 0x0f) {
    case TS_CLIENT_TO_SERVER_MESSAGE_SEND_TUPLE:
      return TS_CLIENT_TO_SERVER_MESSAGE_SEND_TUPLE;
    case TS_CLIENT_TO_SERVER_MESSAGE_GET_TUPLE:
      return TS_CLIENT_TO_SERVER_MESSAGE_GET_TUPLE;
    case TS_CLIENT_TO_SERVER_MESSAGE_RECEIVED_MSG:
      return TS_CLIENT_TO_SERVER_MESSAGE_RECEIVED_MSG;
    default:
      break;
  }
  return TS_CLIENT_TO_SERVER_MESSAGE_INVALID;
}

ts_server_to_client_message_type_t ts_server_to_client_message_type(
    const ts_byte_t* buffer, ts_size_t buffer_size) {
  if (buffer_size < 1) {
    return TS_SERVER_TO_CLIENT_MESSAGE_INVALID;
  }
  switch (buffer[0] & 0x0f) {
    case TS_SERVER_TO_CLIENT_MESSAGE_TUPLE:
      return TS_SERVER_TO_CLIENT_MESSAGE_TUPLE;
    case TS_SERVER_TO_CLIENT_MESSAGE_LACK_OF_TUPLE:
      return TS_SERVER_TO_CLIENT_MESSAGE_LACK_OF_TUPLE;
    case TS_SERVER_TO_CLIENT_MESSAGE_RECEIVED_MSG:
      return TS_SERVER_TO_CLIENT_MESSAGE_RECEIVED_MSG;
    case TS_SERVER_TO_CLIENT_MESSAGE_AWAITING_TUPLE:
      return TS_SERVER_TO_CLIENT_MESSAGE_AWAITING_TUPLE;
    default:
      break;
  }
  return TS_SERVER_TO_CLIENT_MESSAGE_INVALID;
}

ts_bool_t ts_server_to_client_encode_message_type(
    ts_server_to_client_message_type_t message_type, ts_byte_t* buffer,
    ts_size_t buffer_size) {
  if (buffer_size == 0) {
    return TS_FALSE;
  }
  *buffer = (*buffer & 0xf0) | (message_type & 0x0f);
  return TS_TRUE;
}

ts_bool_t ts_client_to_server_encode_message_type(
    ts_client_to_server_message_type_t message_type, ts_byte_t* buffer,
    ts_size_t buffer_size) {
  if (buffer_size == 0) {
    return TS_FALSE;
  }
  *buffer = (*buffer & 0xf0) | (message_type & 0x0f);
  return TS_TRUE;
}

ts_bool_t ts_encode_tuple_size(ts_size_t tuple_size, ts_byte_t* buffer,
                               ts_size_t buffer_size) {
  if ((buffer_size == 0) || (tuple_size > 16) || (tuple_size == 0)) {
    return TS_FALSE;
  }
  *buffer = (*buffer & 0x0f) | (((tuple_size - 1) << 4) & 0xf0);
  return TS_TRUE;
}
