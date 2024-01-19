#ifndef __SERVER_PROCESS_REQUESTS_H__
#define __SERVER_PROCESS_REQUESTS_H__

#include "../libts/common/tuple_space_network.h"
#include "../libts/unix/tuple_space_unix_debug.h"
#include "server_active_connections.h"
#include "server_metrics.h"
#include "server_tuple_space.h"

typedef struct {
  server_metrics_t metrics;
  ts_server_context_t server_context;
  server_tuple_space_t tuple_stash;
  server_tuple_queue_t tuple_queue;
  server_active_connections_t active_connections;
  ts_allocator_t allocator;
} server_data_t;

void server_send_single_ack(server_data_t* data, ts_ipv4_t sender_ip_address,
                            ts_port_t sender_port_id);

void server_process_in(server_data_t* data, ts_tuple_field_t* tuple,
                       ts_size_t tuple_size, ts_ipv4_t sender_ip_address,
                       ts_port_t sender_port_id);

void server_process_inp(server_data_t* data, ts_tuple_field_t* tuple,
                        ts_size_t tuple_size, ts_ipv4_t sender_ip_address,
                        ts_port_t sender_port_id);

void server_process_rd(server_data_t* data, ts_tuple_field_t* tuple,
                       ts_size_t tuple_size, ts_ipv4_t sender_ip_address,
                       ts_port_t sender_port_id);

void server_process_rdp(server_data_t* data, ts_tuple_field_t* tuple,
                        ts_size_t tuple_size, ts_ipv4_t sender_ip_address,
                        ts_port_t sender_port_id);

void server_process_out(server_data_t* data, ts_tuple_field_t* tuple,
                        ts_size_t tuple_size, ts_ipv4_t sender_ip_address,
                        ts_port_t sender_port_id);

#endif  // __SERVER_PROCESS_REQUESTS_H__
