#ifndef __SERVER_METRICS_H__
#define __SERVER_METRICS_H__

#include <inttypes.h>
#include <stddef.h>

typedef struct {
  size_t total_received_messages;
  size_t total_send_mesages;
  size_t total_out_messages;
  size_t total_in_messages;
  size_t total_inp_messages;
  size_t total_rd_messages;
  size_t total_rdp_messages;
  size_t total_rejected_inp_messages;
  size_t total_rejected_rdp_messages;
  size_t total_queued_in_messages;
  size_t total_queued_rd_messages;
  size_t currently_stashed_tuples;
  size_t currently_queued_tuples;
  size_t acc_received_message_length;
  size_t acc_send_message_length;
  size_t total_serialization_issues;
  size_t total_invalid_get_tuples;
  size_t total_invalid_send_tuples;
  size_t total_errors;
} server_metrics_t;

server_metrics_t server_initialize_metrics(void);

void server_print_metrics(const server_metrics_t* metrics);

#endif  // __SERVER_METRICS_H__
