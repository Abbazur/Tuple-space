#ifndef __SERVER_ACTIVE_CONNECTIONS_H__
#define __SERVER_ACTIVE_CONNECTIONS_H__

#include "../libts/common/tuple_space_network.h"

typedef struct {
  void* next_node;
  ts_ipv4_t receiver_ip;
  ts_port_t receiver_port;
  ts_tuple_field_t* data_tuple;
  ts_size_t tuple_size;
  ts_size_t resend_counter;
  ts_uint_t insertion_time;
  ts_bool_t insert_if_rejected;
  ts_bool_t ping_for_await;
} server_connection_node_t;

typedef struct {
  server_connection_node_t* list;
} server_active_connections_t;

void server_add_connection_node(server_active_connections_t* connection_list,
                                ts_ipv4_t receiver_ip, ts_port_t receiver_port,
                                ts_tuple_field_t* data_tuple,
                                ts_size_t tuple_size,
                                ts_bool_t insert_if_rejected,
                                ts_bool_t ping_for_await);

server_connection_node_t server_remove_connection_node(
    server_active_connections_t* connection_list, ts_ipv4_t receiver_ip,
    ts_port_t receiver_port);

server_connection_node_t* server_find_connection_node(
    server_active_connections_t* connection_list, ts_ipv4_t receiver_ip,
    ts_port_t receiver_port);

#endif  // __SERVER_ACTIVE_CONNECTIONS_H__
