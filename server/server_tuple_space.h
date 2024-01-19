#ifndef __SERVER_TUPLE_SPACE_H__
#define __SERVER_TUPLE_SPACE_H__

#include "../libts/common/tuple_space.h"
#include "../libts/common/tuple_space_network.h"

typedef struct {
  ts_tuple_field_t* tuple;
  void* next_node;
} server_tuple_space_node_t;

typedef struct {
  ts_tuple_field_t* tuple;
  ts_ipv4_t sender_ip_address;
  ts_port_t sender_port_id;
  ts_bool_t remove_matching;
} server_tuple_queue_entry_t;

typedef struct {
  server_tuple_queue_entry_t entry;
  void* next_node;
} server_tuple_queue_node_t;

typedef struct {
  server_tuple_space_node_t* nodes[TS_MAX_TUPLE_SIZE];
} server_tuple_space_t;

typedef struct {
  server_tuple_queue_node_t* nodes[TS_MAX_TUPLE_SIZE];
} server_tuple_queue_t;

void server_insert_data_tuple(server_tuple_space_t* tuple_space,
                              ts_tuple_field_t* tuple, ts_size_t tuple_size);

void server_insert_template_tuple(server_tuple_queue_t* tuple_space,
                                  ts_tuple_field_t* tuple, ts_size_t tuple_size,
                                  ts_ipv4_t sender_ip_address,
                                  ts_port_t sender_port_id,
                                  ts_bool_t remove_matching);

server_tuple_queue_entry_t server_get_template_node(
    server_tuple_queue_t* tuple_space, ts_tuple_field_t* data_tuple,
    ts_size_t tuple_size, ts_bool_t remove_when_found);

ts_tuple_field_t* server_get_data_node(server_tuple_space_t* tuple_space,
                                       ts_tuple_field_t* template_tuple,
                                       ts_size_t tuple_size,
                                       ts_bool_t remove_when_found);

#endif  // __SERVER_TUPLE_SPACE_H__
