#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../libts/unix/tuple_space_unix_alloc.h"
#include "../libts/unix/tuple_space_unix_device.h"
#include "server_log.h"
#include "server_process_requests.h"

#define SERVER_PORT 43532
#define MAX_ACK_REPLIES 5
#define MAX_ACK_AWAIT_TIME 1000

static void server_error_cb(void* user_data) {
  server_data_t* data = (server_data_t*)user_data;
  server_log("Unknown error found during input operation\n");
  ++data->metrics.total_errors;
}

static void server_get_tuple_cb(void* user_data, ts_tuple_field_t* tuple,
                                ts_size_t tuple_size,
                                ts_bool_t respond_when_available,
                                ts_bool_t remove_after_use,
                                ts_ipv4_t sender_ip_address,
                                ts_port_t sender_port_id) {
  server_data_t* data = (server_data_t*)user_data;
  server_log("Received GET TUPLE message from %s and port %d\n",
             ts_unix_ipv4_to_str(sender_ip_address), sender_port_id);
  server_log("Tuple from message:\n");
  ts_print_tuple(tuple, tuple_size);
  data->metrics.acc_received_message_length += tuple_size;
  ++data->metrics.total_received_messages;
  if (remove_after_use && respond_when_available) {
    server_log("Processing IN message\n");
    ++data->metrics.total_in_messages;
    server_process_in(data, tuple, tuple_size, sender_ip_address,
                      sender_port_id);
  } else if (remove_after_use && !respond_when_available) {
    server_log("Processing INP message\n");
    ++data->metrics.total_inp_messages;
    server_process_inp(data, tuple, tuple_size, sender_ip_address,
                       sender_port_id);
  } else if (!remove_after_use && respond_when_available) {
    server_log("Processing RD message\n");
    ++data->metrics.total_rd_messages;
    server_process_rd(data, tuple, tuple_size, sender_ip_address,
                      sender_port_id);
  } else {
    server_log("Processing RDP message\n");
    ++data->metrics.total_rdp_messages;
    server_process_rdp(data, tuple, tuple_size, sender_ip_address,
                       sender_port_id);
  }
}

static void server_get_tuple_invalid_invariant_cb(void* user_data,
                                                  ts_ipv4_t sender_ip_address,
                                                  ts_port_t sender_port_id) {
  server_data_t* data = (server_data_t*)user_data;
  server_log(
      "Received GET TUPLE message from %s and port %d with invalid data\n",
      ts_unix_ipv4_to_str(sender_ip_address), sender_port_id);
  ++data->metrics.total_invalid_get_tuples;
}

static void server_send_tuple_cb(void* user_data, ts_tuple_field_t* tuple,
                                 ts_size_t tuple_size,
                                 ts_ipv4_t sender_ip_address,
                                 ts_port_t sender_port_id) {
  server_data_t* data = (server_data_t*)user_data;
  server_log("Received SEND TUPLE message from %s and port %d\n",
             ts_unix_ipv4_to_str(sender_ip_address), sender_port_id);
  server_log("Received tuple:\n");
  ts_print_tuple(tuple, tuple_size);
  ++data->metrics.total_out_messages;
  data->metrics.acc_received_message_length += tuple_size;
  ++data->metrics.total_received_messages;
  server_log("Processing OUT tuple\n");
  server_process_out(data, tuple, tuple_size, sender_ip_address,
                     sender_port_id);
}

static void server_send_tuple_invalid_invariant_cb(void* user_data,
                                                   ts_ipv4_t sender_ip_address,
                                                   ts_port_t sender_port_id) {
  server_data_t* data = (server_data_t*)user_data;
  server_log(
      "Received SEND TUPLE message from %s and port %d with invalid data\n",
      ts_unix_ipv4_to_str(sender_ip_address), sender_port_id);
  ++data->metrics.total_invalid_send_tuples;
}

static void server_serialization_issue_cb(
    void* user_data, ts_client_to_server_message_type_t message_type,
    ts_ipv4_t sender_ip_address, ts_port_t sender_port_id) {
  server_data_t* data = (server_data_t*)user_data;
  server_log(
      "Received SEND TUPLE message from %s and port %d could not been "
      "serialized\n",
      ts_unix_ipv4_to_str(sender_ip_address), sender_port_id);
  ++data->metrics.total_serialization_issues;
}

static void server_ack_cb(void* user_data, ts_ipv4_t sender_ip_address,
                          ts_port_t sender_port_id) {
  server_data_t* data = (server_data_t*)user_data;
  server_log("Received ACK message from %s and port %d\n",
             ts_unix_ipv4_to_str(sender_ip_address), sender_port_id);
  server_connection_node_t node = server_remove_connection_node(
      &data->active_connections, sender_ip_address, sender_port_id);
  ts_free_tuple(node.data_tuple, node.tuple_size, &data->allocator);
  server_send_single_ack(data, sender_ip_address, sender_port_id);
}

static ts_server_receiver_callbacks_t server_initialize_callbacks(void) {
  ts_server_receiver_callbacks_t callbacks;
  memset(&callbacks, 0, sizeof(ts_server_receiver_callbacks_t));
  callbacks.error_cb = server_error_cb;
  callbacks.get_tuple_cb = server_get_tuple_cb;
  callbacks.get_tuple_invalid_invariant_cb =
      server_get_tuple_invalid_invariant_cb;
  callbacks.send_tuple_cb = server_send_tuple_cb;
  callbacks.send_tuple_invalid_invariant_cb =
      server_send_tuple_invalid_invariant_cb;
  callbacks.ack_cb = server_ack_cb;
  callbacks.serialization_issue_cb = server_serialization_issue_cb;
  return callbacks;
}

static void server_resend_tuple_remove_node(server_data_t* server_data,
                                            server_connection_node_t** node) {
  server_connection_node_t* old_node = *node;
  *node = old_node->next_node;
  if (old_node->insert_if_rejected) {
    server_insert_data_tuple(&server_data->tuple_stash, old_node->data_tuple,
                             old_node->tuple_size);
  } else {
    ts_free_tuple(old_node->data_tuple, old_node->tuple_size,
                  &server_data->allocator);
  }
  free(old_node);
}

static void server_resend_tuple_for_connections(server_data_t* server_data,
                                                ts_size_t max_resend_count,
                                                ts_uint_t resend_interval) {
  server_connection_node_t** node = &server_data->active_connections.list;
  ts_uint_t current_time = server_current_time_ms();
  for (; (*node) != NULL;
       node = (server_connection_node_t**)&(*node)->next_node) {
    if ((*node)->insertion_time + resend_interval < current_time) {
      if (!(*node)->ping_for_await &&
          (*node)->resend_counter++ == max_resend_count) {
        server_resend_tuple_remove_node(server_data, node);
      } else if (!(*node)->ping_for_await) {
        ts_server_send_server_to_client_tuple(
            &server_data->server_context, (*node)->data_tuple,
            (*node)->tuple_size, (*node)->receiver_ip, (*node)->receiver_port);
        (*node)->insertion_time = current_time;
      } else if ((*node)->tuple_size) {
        ts_server_send_server_to_client_await_for_tuple(
            &server_data->server_context, (*node)->receiver_ip,
            (*node)->receiver_port);
      } else {
        ts_server_send_server_to_client_lack_of_tuple(
            &server_data->server_context, (*node)->receiver_ip,
            (*node)->receiver_port);
      }
    }
  }
}

static volatile ts_bool_t IsWorking = TS_TRUE;

static void server_sigint_handler(int) { IsWorking = TS_FALSE; }

static size_t server_current_time_s(void) {
  time_t time_ = time(NULL);
  return localtime(&time_)->tm_sec;
}

int main() {
  signal(SIGINT, server_sigint_handler);
  server_data_t server_data;
  memset(&server_data, 0, sizeof(server_data_t));
  server_data.metrics = server_initialize_metrics();
  server_data.allocator = ts_initialize_unix_allocator();

  if (!ts_initialize_server_context(
          &server_data.server_context,
          ts_initialize_unix_device_context(SERVER_PORT),
          server_initialize_callbacks())) {
    return -1;
  }

  size_t last_message_time_s = server_current_time_s();

  while (IsWorking) {
    ts_server_get_message(&server_data.server_context, &server_data,
                          &server_data.allocator);
    size_t current_time_s = server_current_time_s();
    if (last_message_time_s != current_time_s) {
      server_print_metrics(&server_data.metrics);
      last_message_time_s = current_time_s;
    }
    server_resend_tuple_for_connections(&server_data, MAX_ACK_REPLIES,
                                        MAX_ACK_AWAIT_TIME);
  }

  ts_close_server_context(&server_data.server_context);

  return 0;
}
