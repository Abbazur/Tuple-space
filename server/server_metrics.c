#include "server_metrics.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "server_log.h"

server_metrics_t server_initialize_metrics(void) {
  server_metrics_t metrics;
  memset(&metrics, 0, sizeof(server_metrics_t));
  return metrics;
}

void server_print_metrics(const server_metrics_t* metrics) {
  char time_buf[SERVER_LOG_BUFFER_SIZE];
  memset(time_buf, 0, SERVER_LOG_BUFFER_SIZE);
  server_current_date(time_buf, SERVER_LOG_BUFFER_SIZE);
  printf("==================================================\n");
  printf("Current time: %s\n", time_buf);
  printf("Total received messages: %lu\n", metrics->total_received_messages);
  printf("Total send messages: %lu\n", metrics->total_send_mesages);
  printf("Total out messages: %lu\n", metrics->total_out_messages);
  printf("Total in messages: %lu\n", metrics->total_in_messages);
  printf("Total inp messages: %lu\n", metrics->total_inp_messages);
  printf("Total rd messages: %lu\n", metrics->total_rd_messages);
  printf("Total rdp messages: %lu\n", metrics->total_rdp_messages);
  printf("Total rejected inp messages: %lu\n",
         metrics->total_rejected_inp_messages);
  printf("Total accepted inp messages: %lu\n",
         metrics->total_inp_messages - metrics->total_rejected_inp_messages);
  printf("Total rejected rdp messages: %lu\n",
         metrics->total_rejected_rdp_messages);
  printf("Total accepted rdp messages: %lu\n",
         metrics->total_rdp_messages - metrics->total_rejected_rdp_messages);
  printf("Total queued in messages: %lu\n", metrics->total_queued_in_messages);
  printf("Total responend-on-spot in messages: %lu\n",
         metrics->total_in_messages - metrics->total_queued_in_messages);
  printf("Total queued rd messages: %lu\n", metrics->total_queued_rd_messages);
  printf("Total responsed-on-spot rd messages: %lu\n",
         metrics->total_rd_messages - metrics->total_queued_rd_messages);
  printf("Currently stashed tuples: %lu\n", metrics->currently_stashed_tuples);
  printf("Currently queued tuples: %lu\n", metrics->currently_queued_tuples);
  printf("Accumulated received messages length: %lu\n",
         metrics->acc_received_message_length);
  printf("Accumulated send messages length: %lu\n",
         metrics->acc_send_message_length);
  if (metrics->total_received_messages) {
    printf("Average received message length: %f\n",
           (float)metrics->acc_received_message_length /
               metrics->total_received_messages);
  } else {
    printf("Average received message length: N/A\n");
  }
  if (metrics->total_send_mesages) {
    printf(
        "Average send message length: %f\n",
        (float)metrics->acc_send_message_length / metrics->total_send_mesages);
  } else {
    printf("Average send message length: N/A\n");
  }

  printf("Total invalid GET messages: %lu\n",
         metrics->total_invalid_get_tuples);
  printf("Total invalid SEND messages: %lu\n",
         metrics->total_invalid_send_tuples);
  printf("Total serialization issues: %lu\n",
         metrics->total_serialization_issues);
  printf("Total errors: %lu\n", metrics->total_errors);
  printf("==================================================\n");
}
