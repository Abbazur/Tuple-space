#ifndef __SERVER_LOG_H__
#define __SERVER_LOG_H__

#include <inttypes.h>
#include <stddef.h>

#define SERVER_LOG_BUFFER_SIZE 1024

int32_t server_current_date(char* buffer, size_t size);

uint64_t server_current_time_ms(void);

int32_t server_log(const char* format_string, ...);

#endif  // __SERVER_LOG_H__
