#include "server_process_requests.h"

#include <stdio.h>

#include "../libts/unix/tuple_space_unix_debug.h"
#include "../libts/unix/tuple_space_unix_device.h"
#include "server_log.h"

void server_send_single_ack(server_data_t* data, ts_ipv4_t sender_ip_address,
                            ts_port_t sender_port_id) {
  ts_server_send_server_to_client_ack(&data->server_context, sender_ip_address,
                                      sender_port_id);
}

static ts_bool_t server_process_in_send_tuple_to_client(
    server_data_t* data, ts_tuple_field_t* data_tuple, ts_size_t tuple_size,
    ts_ipv4_t sender_ip_address, ts_port_t sender_port_id) {
  ts_bool_t status = ts_server_send_server_to_client_tuple(
      &data->server_context, data_tuple, tuple_size, sender_ip_address,
      sender_port_id);
  server_add_connection_node(&data->active_connections, sender_ip_address,
                             sender_port_id, data_tuple, tuple_size, TS_TRUE,
                             TS_FALSE);
  return status;
}

static ts_bool_t server_process_in_await_for_tuple(server_data_t* data,
                                                   ts_ipv4_t sender_ip_address,
                                                   ts_port_t sender_port_id) {
  ts_bool_t status = ts_server_send_server_to_client_await_for_tuple(
      &data->server_context, sender_ip_address, sender_port_id);
  server_add_connection_node(&data->active_connections, sender_ip_address,
                             sender_port_id, NULL, 1, TS_FALSE, TS_TRUE);
  return status;
}

void server_process_in(server_data_t* data, ts_tuple_field_t* tuple,
                       ts_size_t tuple_size, ts_ipv4_t sender_ip_address,
                       ts_port_t sender_port_id) {
  ts_tuple_field_t* data_tuple =
      server_get_data_node(&data->tuple_stash, tuple, tuple_size, TS_TRUE);
  if (data_tuple != NULL) {
    --data->metrics.currently_stashed_tuples;
    server_log("Found matching tuple:\n");
    ts_print_tuple(data_tuple, tuple_size);
    ts_bool_t status = server_process_in_send_tuple_to_client(
        data, data_tuple, tuple_size, sender_ip_address, sender_port_id);
    server_log(
        "Responded to the client at %s:%d with a given tuple with "
        "status %d - removing tuple from stash\n",
        ts_unix_ipv4_to_str(sender_ip_address), sender_port_id, status);
    ++data->metrics.total_send_mesages;
    data->metrics.acc_send_message_length += tuple_size;
    data->allocator.deallocator_cb(data->allocator.context, tuple);
    return;
  }
  ++data->metrics.currently_queued_tuples;
  ++data->metrics.total_queued_in_messages;
  server_log(
      "No matching tuples has been found - queueing tuple for a match\n");
  server_insert_template_tuple(&data->tuple_queue, tuple, tuple_size,
                               sender_ip_address, sender_port_id, TS_TRUE);
  server_process_in_await_for_tuple(data, sender_ip_address, sender_port_id);
}

static ts_bool_t server_process_inp_send_tuple_to_client(
    server_data_t* data, ts_tuple_field_t* data_tuple, ts_size_t tuple_size,
    ts_ipv4_t sender_ip_address, ts_port_t sender_port_id) {
  ts_bool_t status = ts_server_send_server_to_client_tuple(
      &data->server_context, data_tuple, tuple_size, sender_ip_address,
      sender_port_id);
  server_add_connection_node(&data->active_connections, sender_ip_address,
                             sender_port_id, data_tuple, tuple_size, TS_TRUE,
                             TS_FALSE);
  return status;
}

static ts_bool_t server_process_inp_await_for_tuple(server_data_t* data,
                                                    ts_ipv4_t sender_ip_address,
                                                    ts_port_t sender_port_id) {
  ts_bool_t status = ts_server_send_server_to_client_await_for_tuple(
      &data->server_context, sender_ip_address, sender_port_id);
  server_add_connection_node(&data->active_connections, sender_ip_address,
                             sender_port_id, NULL, 0, TS_FALSE, TS_TRUE);
  return status;
}

void server_process_inp(server_data_t* data, ts_tuple_field_t* tuple,
                        ts_size_t tuple_size, ts_ipv4_t sender_ip_address,
                        ts_port_t sender_port_id) {
  ts_tuple_field_t* data_tuple =
      server_get_data_node(&data->tuple_stash, tuple, tuple_size, TS_TRUE);
  if (data_tuple != NULL) {
    --data->metrics.currently_stashed_tuples;
    server_log("Found matching tuple:\n");
    ts_print_tuple(data_tuple, tuple_size);
    ts_bool_t status = server_process_inp_send_tuple_to_client(
        data, data_tuple, tuple_size, sender_ip_address, sender_port_id);
    server_log(
        "Responded to the client at %s:%d with a given tuple with "
        "status %d - removing tuple from stash\n",
        ts_unix_ipv4_to_str(sender_ip_address), sender_port_id, status);
    ++data->metrics.total_send_mesages;
    data->metrics.acc_send_message_length += tuple_size;
    data->allocator.deallocator_cb(data->allocator.context, tuple);
    return;
  }
  ++data->metrics.total_rejected_inp_messages;
  data->allocator.deallocator_cb(data->allocator.context, tuple);
  ts_bool_t status = ts_server_send_server_to_client_lack_of_tuple(
      &data->server_context, sender_ip_address, sender_port_id);
  server_process_inp_await_for_tuple(data, sender_ip_address, sender_port_id);
  server_log(
      "No matching tuples has been found - responding to a client about it "
      "with status %d\n",
      status);
}

static ts_bool_t server_process_rd_send_tuple_to_client(
    server_data_t* data, ts_tuple_field_t* data_tuple, ts_size_t tuple_size,
    ts_ipv4_t sender_ip_address, ts_port_t sender_port_id) {
  ts_bool_t status = ts_server_send_server_to_client_tuple(
      &data->server_context, data_tuple, tuple_size, sender_ip_address,
      sender_port_id);
  server_add_connection_node(&data->active_connections, sender_ip_address,
                             sender_port_id, data_tuple, tuple_size, TS_FALSE,
                             TS_FALSE);
  return status;
}

static ts_bool_t server_process_rd_await_for_tuple(server_data_t* data,
                                                   ts_ipv4_t sender_ip_address,
                                                   ts_port_t sender_port_id) {
  ts_bool_t status = ts_server_send_server_to_client_await_for_tuple(
      &data->server_context, sender_ip_address, sender_port_id);
  server_add_connection_node(&data->active_connections, sender_ip_address,
                             sender_port_id, NULL, 1, TS_FALSE, TS_FALSE);
  return status;
}

void server_process_rd(server_data_t* data, ts_tuple_field_t* tuple,
                       ts_size_t tuple_size, ts_ipv4_t sender_ip_address,
                       ts_port_t sender_port_id) {
  ts_tuple_field_t* data_tuple =
      server_get_data_node(&data->tuple_stash, tuple, tuple_size, TS_FALSE);
  if (data_tuple != NULL) {
    server_log("Found matching tuple:\n");
    ts_print_tuple(data_tuple, tuple_size);
    ts_bool_t status = server_process_rd_send_tuple_to_client(
        data, data_tuple, tuple_size, sender_ip_address, sender_port_id);
    server_log(
        "Responded to the client at %s:%d with a given tuple with "
        "status %d\n",
        ts_unix_ipv4_to_str(sender_ip_address), sender_port_id, status);
    ++data->metrics.total_send_mesages;
    data->metrics.acc_send_message_length += tuple_size;
    data->allocator.deallocator_cb(data->allocator.context, tuple);
    return;
  }
  ++data->metrics.currently_queued_tuples;
  ++data->metrics.total_queued_rd_messages;
  server_log(
      "No matching tuples has been found - queueing tuple for a match\n");
  server_process_rd_await_for_tuple(data, sender_ip_address, sender_port_id);
  server_insert_template_tuple(&data->tuple_queue, tuple, tuple_size,
                               sender_ip_address, sender_port_id, TS_FALSE);
}

static ts_bool_t server_process_rdp_send_tuple_to_client(
    server_data_t* data, ts_tuple_field_t* data_tuple, ts_size_t tuple_size,
    ts_ipv4_t sender_ip_address, ts_port_t sender_port_id) {
  ts_bool_t status = ts_server_send_server_to_client_tuple(
      &data->server_context, data_tuple, tuple_size, sender_ip_address,
      sender_port_id);
  server_add_connection_node(&data->active_connections, sender_ip_address,
                             sender_port_id, data_tuple, tuple_size, TS_FALSE,
                             TS_FALSE);
  return status;
}

static ts_bool_t server_process_rdp_await_for_tuple(server_data_t* data,
                                                    ts_ipv4_t sender_ip_address,
                                                    ts_port_t sender_port_id) {
  ts_bool_t status = ts_server_send_server_to_client_await_for_tuple(
      &data->server_context, sender_ip_address, sender_port_id);
  server_add_connection_node(&data->active_connections, sender_ip_address,
                             sender_port_id, NULL, 0, TS_FALSE, TS_FALSE);
  return status;
}

void server_process_rdp(server_data_t* data, ts_tuple_field_t* tuple,
                        ts_size_t tuple_size, ts_ipv4_t sender_ip_address,
                        ts_port_t sender_port_id) {
  ts_tuple_field_t* data_tuple =
      server_get_data_node(&data->tuple_stash, tuple, tuple_size, TS_FALSE);
  if (data_tuple != NULL) {
    server_log("Found matching tuple:\n");
    ts_print_tuple(data_tuple, tuple_size);
    ts_bool_t status = server_process_rdp_send_tuple_to_client(
        data, data_tuple, tuple_size, sender_ip_address, sender_port_id);
    server_add_connection_node(&data->active_connections, sender_ip_address,
                               sender_port_id, data_tuple, tuple_size, TS_FALSE,
                               TS_FALSE);
    server_log(
        "Responded to the client at %s:%d with a given tuple with "
        "status %d - removing tuple from stash\n",
        ts_unix_ipv4_to_str(sender_ip_address), sender_port_id, status);
    ++data->metrics.total_send_mesages;
    data->metrics.acc_send_message_length += tuple_size;
    data->allocator.deallocator_cb(data->allocator.context, tuple);
    return;
  }
  ++data->metrics.total_rejected_rdp_messages;
  data->allocator.deallocator_cb(data->allocator.context, tuple);
  server_add_connection_node(&data->active_connections, sender_ip_address,
                             sender_port_id, NULL, 0, TS_FALSE, TS_FALSE);
  ts_bool_t status = ts_server_send_server_to_client_lack_of_tuple(
      &data->server_context, sender_ip_address, sender_port_id);
  server_log(
      "No matching tuples has been found - responding to a client about it "
      "with status %d\n",
      status);
  server_process_rdp_await_for_tuple(data, sender_ip_address, sender_port_id);
}

static void server_process_out_transform_await(server_data_t* data,
                                               ts_tuple_field_t* tuple,
                                               ts_size_t tuple_size,
                                               ts_ipv4_t sender_ip_address,
                                               ts_port_t sender_port_id,
                                               ts_bool_t do_remove) {
  server_connection_node_t* node = server_find_connection_node(
      &data->active_connections, sender_ip_address, sender_port_id);
  if (!node) {
    return server_add_connection_node(&data->active_connections,
                                      sender_ip_address, sender_port_id, tuple,
                                      tuple_size, !do_remove, TS_FALSE);
  }
  node->ping_for_await = TS_FALSE;
  node->insert_if_rejected = !do_remove;
  node->tuple_size = tuple_size;
  node->data_tuple = tuple;
}

void server_process_out(server_data_t* data, ts_tuple_field_t* tuple,
                        ts_size_t tuple_size, ts_ipv4_t sender_ip_address,
                        ts_port_t sender_port_id) {
  server_tuple_queue_entry_t entry =
      server_get_template_node(&data->tuple_queue, tuple, tuple_size, TS_TRUE);
  while (entry.tuple != NULL) {
    --data->metrics.currently_queued_tuples;
    ts_bool_t status = ts_server_send_server_to_client_tuple(
        &data->server_context, tuple, tuple_size, entry.sender_ip_address,
        entry.sender_port_id);
    server_process_out_transform_await(data, tuple, tuple_size,
                                       sender_ip_address, sender_port_id,
                                       entry.remove_matching);
    server_log(
        "Responded to the awaiting client at %s:%d with a given tuple with "
        "status %d\n",
        ts_unix_ipv4_to_str(entry.sender_ip_address), entry.sender_port_id,
        status);
    ++data->metrics.total_send_mesages;
    data->metrics.acc_send_message_length += tuple_size;
    data->allocator.deallocator_cb(data->allocator.context, entry.tuple);
    if (entry.remove_matching) {
      server_log(
          "Tuple has been redirected to the awaiting client - not saving\n");
      data->allocator.deallocator_cb(data->allocator.context, tuple);
      return;
    }
    entry = server_get_template_node(&data->tuple_queue, tuple, tuple_size,
                                     TS_TRUE);
  }
  server_log("Saving tuple on the stash\n");
  ++data->metrics.currently_stashed_tuples;
  server_insert_data_tuple(&data->tuple_stash, tuple, tuple_size);
  server_send_single_ack(data, sender_ip_address, sender_port_id);
}
