#include "server_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

static int32_t server_log_message(const char* message) {
  char buffer[SERVER_LOG_BUFFER_SIZE];
  memset(buffer, 0, SERVER_LOG_BUFFER_SIZE);
  server_current_date(buffer, SERVER_LOG_BUFFER_SIZE);
  return printf("[%s] %s\n", buffer, message);
}

uint64_t server_current_time_ms(void) {
  struct timeval timev;
  gettimeofday(&timev, NULL);

  return timev.tv_usec / 1000;
}

int32_t server_current_date(char* buffer, size_t size) {
  time_t rawtime;
  memset(buffer, 0, size);
  time(&rawtime);
  int32_t offst =
      strftime(buffer, size, "%Y/%m/%d %H:%M:%S.", localtime(&rawtime));
  if (offst > 0) {
    offst += sprintf(buffer + offst, "%04lu", server_current_time_ms());
  }
  return offst;
}

int32_t server_log(const char* format_string, ...) {
  char buffer[SERVER_LOG_BUFFER_SIZE + 1];
  memset(buffer, 0, SERVER_LOG_BUFFER_SIZE + 1);
  va_list parameter_pack;

  va_start(parameter_pack, format_string);
  int length =
      vsnprintf(buffer, SERVER_LOG_BUFFER_SIZE, format_string, parameter_pack);
  va_end(parameter_pack);

  if (length > SERVER_LOG_BUFFER_SIZE) {
    return -1;
  }
  return server_log_message(buffer);
}
