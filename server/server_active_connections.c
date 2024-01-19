#include "server_active_connections.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "server_log.h"

void server_add_connection_node(server_active_connections_t* connection_list,
                                ts_ipv4_t receiver_ip, ts_port_t receiver_port,
                                ts_tuple_field_t* data_tuple,
                                ts_size_t tuple_size,
                                ts_bool_t insert_if_rejected,
                                ts_bool_t ping_for_await) {
  server_connection_node_t** root_node = &connection_list->list;
  server_connection_node_t* new_node =
      (server_connection_node_t*)malloc(sizeof(server_connection_node_t));
  memset(new_node, 0, sizeof(server_connection_node_t));
  new_node->data_tuple = data_tuple;
  new_node->receiver_ip = receiver_ip;
  new_node->receiver_port = receiver_port;
  new_node->insertion_time = server_current_time_ms();
  new_node->insert_if_rejected = insert_if_rejected;
  new_node->tuple_size = tuple_size;
  new_node->resend_counter = 0;
  new_node->ping_for_await = ping_for_await;
  new_node->next_node = NULL;
  if (*root_node == NULL) {
    *root_node = new_node;
  } else {
    new_node->next_node = *root_node;
    *root_node = new_node;
  }
}

server_connection_node_t server_remove_connection_node(
    server_active_connections_t* connection_list, ts_ipv4_t receiver_ip,
    ts_port_t receiver_port) {
  server_connection_node_t result;
  memset(&result, 0, sizeof(server_connection_node_t));
  server_connection_node_t** node = &connection_list->list;
  for (; *node != NULL;
       node = (server_connection_node_t**)&(*node)->next_node) {
    if (((*node)->receiver_ip == receiver_ip) &&
        ((*node)->receiver_port == receiver_port)) {
      ts_tuple_field_t* data_tuple = (*node)->data_tuple;
      server_connection_node_t* old_node = *node;
      result = *old_node;
      *node = (*node)->next_node;
      free(old_node);
      return result;
    }
  }
  return result;
}

server_connection_node_t* server_find_connection_node(
    server_active_connections_t* connection_list, ts_ipv4_t receiver_ip,
    ts_port_t receiver_port) {
  server_connection_node_t* node = connection_list->list;
  for (; node != NULL; node = (server_connection_node_t*)node->next_node) {
    if ((node->receiver_ip == receiver_ip) &&
        (node->receiver_port == receiver_ip)) {
      return node;
    }
  }
  return NULL;
}
