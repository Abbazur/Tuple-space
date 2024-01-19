#include "server_tuple_space.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void server_insert_data_tuple(server_tuple_space_t* tuple_space,
                              ts_tuple_field_t* tuple, ts_size_t tuple_size) {
  server_tuple_space_node_t** root_node = &tuple_space->nodes[tuple_size - 1];
  server_tuple_space_node_t* new_node =
      (server_tuple_space_node_t*)malloc(sizeof(server_tuple_space_node_t));
  memset(new_node, 0, sizeof(server_tuple_space_node_t));
  new_node->tuple = tuple;
  new_node->next_node = NULL;
  if (*root_node == NULL) {
    *root_node = new_node;
  } else {
    new_node->next_node = *root_node;
    *root_node = new_node;
  }
}

void server_insert_template_tuple(server_tuple_queue_t* tuple_space,
                                  ts_tuple_field_t* tuple, ts_size_t tuple_size,
                                  ts_ipv4_t sender_ip_address,
                                  ts_port_t sender_port_id,
                                  ts_bool_t remove_matching) {
  server_tuple_queue_node_t** root_node = &tuple_space->nodes[tuple_size - 1];
  server_tuple_queue_node_t* new_node =
      (server_tuple_queue_node_t*)malloc(sizeof(server_tuple_queue_node_t));
  memset(new_node, 0, sizeof(server_tuple_queue_node_t));
  new_node->entry.tuple = tuple;
  new_node->entry.sender_ip_address = sender_ip_address;
  new_node->entry.sender_port_id = sender_port_id;
  new_node->entry.remove_matching = remove_matching;
  new_node->next_node = NULL;
  if (*root_node == NULL) {
    *root_node = new_node;
  } else {
    new_node->next_node = *root_node;
    *root_node = new_node;
  }
}

server_tuple_queue_entry_t server_get_template_node(
    server_tuple_queue_t* tuple_space, ts_tuple_field_t* data_tuple,
    ts_size_t tuple_size, ts_bool_t remove_when_found) {
  server_tuple_queue_node_t** node = &tuple_space->nodes[tuple_size - 1];
  server_tuple_queue_entry_t entry;
  entry.tuple = NULL;
  for (; *node != NULL;
       node = (server_tuple_queue_node_t**)&(*node)->next_node) {
    if (ts_is_matching_tuple((*node)->entry.tuple, data_tuple, tuple_size)) {
      entry = (*node)->entry;
      if (remove_when_found) {
        server_tuple_queue_node_t* old_node = *node;
        *node = (*node)->next_node;
        free(old_node);
      }
      return entry;
    }
  }
  return entry;
}

ts_tuple_field_t* server_get_data_node(server_tuple_space_t* tuple_space,
                                       ts_tuple_field_t* template_tuple,
                                       ts_size_t tuple_size,
                                       ts_bool_t remove_when_found) {
  server_tuple_space_node_t** node = &tuple_space->nodes[tuple_size - 1];
  ts_tuple_field_t* tuple = NULL;
  for (; *node != NULL;
       node = (server_tuple_space_node_t**)&(*node)->next_node) {
    if (ts_is_matching_tuple(template_tuple, (*node)->tuple, tuple_size)) {
      tuple = (*node)->tuple;
      if (remove_when_found) {
        server_tuple_space_node_t* old_node = *node;
        *node = (*node)->next_node;
        free(old_node);
      }
      return tuple;
    }
  }
  return tuple;
}
