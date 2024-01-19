#ifndef __TUPLE_SPACE_NETWORK_H__
#define __TUPLE_SPACE_NETWORK_H__

#include "tuple_space.h"
#include "tuple_space_protocol.h"

#define TS_BUFFER_SIZE 256

typedef uint32_t ts_ipv4_t;
typedef uint16_t ts_port_t;

typedef ts_size_t (*ts_data_recv_cb_t)(void* device_context,
                                       ts_ipv4_t* sender_ip_address,
                                       ts_port_t* sender_port_id,
                                       ts_byte_t* buffer,
                                       ts_size_t buffer_size);
typedef ts_bool_t (*ts_data_send_cb_t)(void* device_context,
                                       ts_ipv4_t target_ip_address,
                                       ts_port_t target_port_id,
                                       const ts_byte_t* buffer,
                                       ts_size_t buffer_size);
typedef ts_uint_t (*ts_clock_ms_cb_t)(void);
typedef void (*ts_device_context_destructor_cb_t)(void* device_context);

#define TS_DEVICE_CONTEXT_SIZE 50

typedef enum {
  TS_DEVICE_UNINITIALIZED = 0,
  TS_DEVICE_INITIALIZED = 1
} ts_device_state_t;

typedef struct {
  ts_data_recv_cb_t recv_cb;
  ts_data_send_cb_t send_cb;
  ts_clock_ms_cb_t clock_cb;
  ts_device_context_destructor_cb_t destructor_cb;
  ts_byte_t device_context[TS_DEVICE_CONTEXT_SIZE];
  ts_byte_t current_state;  // ts_device_state_t
  ts_byte_t can_receive_ip;
} ts_device_context_t;

typedef void (*ts_client_to_server_send_tuple_cb_t)(void* user_data,
                                                    ts_tuple_field_t* tuple,
                                                    ts_size_t tuple_size,
                                                    ts_ipv4_t sender_ip_address,
                                                    ts_port_t sender_port_id);

typedef void (*ts_client_to_server_get_tuple_cb_t)(
    void* user_data, ts_tuple_field_t* tuple, ts_size_t tuple_size,
    ts_bool_t respond_when_available, ts_bool_t remove_after_use,
    ts_ipv4_t sender_ip_address, ts_port_t sender_port_id);

typedef void (*ts_client_to_server_ack_cb_t)(void* user_data,
                                             ts_ipv4_t sender_ip_address,
                                             ts_port_t sender_port_id);

typedef void (*ts_client_to_server_serialization_issue_t)(
    void* user_data, ts_client_to_server_message_type_t message_type,
    ts_ipv4_t sender_ip_address, ts_port_t sender_port_id);

typedef void (*ts_client_to_server_error_cb_t)(void* user_data);

typedef void (*ts_client_to_server_processing_error_cb_t)(
    void* user_data, ts_ipv4_t sender_ip_address, ts_port_t sender_port_id);

typedef struct {
  ts_client_to_server_send_tuple_cb_t send_tuple_cb;
  ts_client_to_server_get_tuple_cb_t get_tuple_cb;
  ts_client_to_server_ack_cb_t ack_cb;
  ts_client_to_server_error_cb_t error_cb;
  ts_client_to_server_processing_error_cb_t send_tuple_invalid_invariant_cb;
  ts_client_to_server_processing_error_cb_t get_tuple_invalid_invariant_cb;
  ts_client_to_server_serialization_issue_t serialization_issue_cb;
} ts_server_receiver_callbacks_t;

typedef struct {
  ts_device_context_t device_context;
  ts_server_receiver_callbacks_t server_callbacks;
} ts_server_context_t;

typedef void (*ts_server_to_client_lack_of_tuple_cb_t)(void* user_data);
typedef void (*ts_server_to_client_tuple_cb_t)(void* user_data,
                                               ts_tuple_field_t* tuple,
                                               ts_size_t tuple_size);
typedef void (*ts_server_to_client_error_cb_t)(void* user_data);
typedef void (*ts_server_to_client_unknown_sender_cb_t)(
    void* user_data, ts_ipv4_t sender_ip_address, ts_port_t sender_port_id);

typedef struct {
  ts_server_to_client_lack_of_tuple_cb_t lack_of_tuple_cb;
  ts_server_to_client_tuple_cb_t tuple_cb;
  ts_server_to_client_error_cb_t error_cb;
  ts_server_to_client_unknown_sender_cb_t unknown_sender_cb;
  ts_server_to_client_error_cb_t tuple_invalid_invariant_cb;
} ts_client_receiver_callbacks_t;

typedef struct {
  ts_device_context_t device_context;
  ts_client_receiver_callbacks_t client_callbacks;
  ts_ipv4_t server_ip_address;
  ts_port_t server_port_id;
  ts_size_t max_number_of_repeats;  // if 0 then inf
  ts_uint_t ack_await_time;         // if 0 then dont
  ts_bool_t had_got_delay_message;
} ts_client_context_t;

ts_bool_t ts_initialize_server_context(
    ts_server_context_t* server_context, ts_device_context_t device_context,
    ts_server_receiver_callbacks_t server_callbacks);

ts_bool_t ts_initialize_client_context(
    ts_client_context_t* client_context, ts_device_context_t device_context,
    ts_client_receiver_callbacks_t client_callbacks,
    ts_ipv4_t server_ip_address, ts_port_t server_port_id,
    ts_size_t max_number_of_repeats, ts_uint_t ack_await_time);

void ts_server_get_message(ts_server_context_t* server_context, void* user_data,
                           const ts_allocator_t* allocator);

ts_bool_t ts_client_get_message(ts_client_context_t* client_context,
                                void* user_data,
                                const ts_allocator_t* allocator);

ts_bool_t ts_server_send_server_to_client_tuple(
    ts_server_context_t* server_context, ts_tuple_field_t* tuple,
    ts_size_t tuple_size, ts_ipv4_t client_ip_address,
    ts_port_t client_port_id);

ts_bool_t ts_server_send_server_to_client_tuple_span(
    ts_server_context_t* server_context, ts_tuple_field_t** tuple,
    ts_size_t tuple_size, ts_ipv4_t client_ip_address,
    ts_port_t client_port_id);

ts_bool_t ts_server_send_server_to_client_lack_of_tuple(
    ts_server_context_t* server_context, ts_ipv4_t client_ip_address,
    ts_port_t client_port_id);

ts_bool_t ts_server_send_server_to_client_ack(
    ts_server_context_t* server_context, ts_ipv4_t client_ip_address,
    ts_port_t client_port_id);

ts_bool_t ts_server_send_server_to_client_await_for_tuple(
    ts_server_context_t* server_context, ts_ipv4_t client_ip_address,
    ts_port_t client_port_id);

ts_bool_t ts_client_send_client_to_server_send_tuple(
    ts_client_context_t* client_context, ts_tuple_field_t* tuple,
    ts_size_t tuple_size);

ts_bool_t ts_client_send_client_to_server_send_tuple_span(
    ts_client_context_t* client_context, ts_tuple_field_t** tuple,
    ts_size_t tuple_size);

typedef enum {
  TS_CLIENT_TO_SERVER_NO_ERROR = 0,
  TS_CLIENT_TO_SERVER_INTERNAL_ERROR = 1,
  TS_CLIENT_TO_SERVER_INVALID_TUPLE = 2,
  TS_CLIENT_TO_SERVER_UNKNOWN_SENDER = 3,
  TS_CLIENT_TO_SERVER_INVALID_RESPONSE_PROTOCOL = 4
} ts_client_to_server_error_t;

ts_client_to_server_error_t ts_client_send_client_to_server_get_tuple(
    ts_client_context_t* client_context, ts_tuple_field_t* tuple,
    ts_size_t tuple_size, ts_bool_t remove_after_use_flag,
    ts_bool_t respond_when_available_flag, void* user_data,
    const ts_allocator_t* allocator);

ts_client_to_server_error_t ts_client_send_client_to_server_get_tuple_span(
    ts_client_context_t* client_context, ts_tuple_field_t** tuple,
    ts_size_t tuple_size, ts_bool_t remove_after_use_flag,
    ts_bool_t respond_when_available_flag, void* user_data,
    const ts_allocator_t* allocator);

void ts_close_server_context(ts_server_context_t* server_context);

void ts_close_client_context(ts_client_context_t* client_context);

void ts_free_tuple(ts_tuple_field_t* tuple, ts_size_t tuple_size,
                   const ts_allocator_t* allocator);

#endif  // __TUPLE_SPACE_NETWORK_H__
