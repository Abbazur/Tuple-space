#include "tuple_space_unix_device.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef int32_t ts_socket_t;

typedef struct {
  ts_data_recv_cb_t recv_cb;
  ts_data_send_cb_t send_cb;
  ts_clock_ms_cb_t clock_cb;
  ts_device_context_destructor_cb_t destructor_cb;
  ts_socket_t socket;
  ts_byte_t reserved[TS_DEVICE_CONTEXT_SIZE - sizeof(ts_socket_t)];
  ts_byte_t current_state;  // ts_device_state_t
  ts_byte_t can_receive_ip;
} ts_unix_device_context_t;

static ts_bool_t ts_unix_device_receive_has_data(
    ts_unix_device_context_t* handle) {
  fd_set set;
  struct timeval wait_time = {0};
  FD_ZERO(&set);
  FD_SET(handle->socket, &set);

  int status = select(handle->socket + 1, &set, NULL, NULL, &wait_time);
  return status > 0;
}

static ts_size_t ts_unix_device_receive(void* device_context,
                                        ts_ipv4_t* sender_ip_address,
                                        ts_port_t* sender_port_id,
                                        ts_byte_t* buffer,
                                        ts_size_t buffer_size) {
  ts_unix_device_context_t* handle = (ts_unix_device_context_t*)device_context;
  if (!ts_unix_device_receive_has_data(handle)) {
    return 0;
  }
  struct sockaddr_in address;
  memset(&address, 0, sizeof(struct sockaddr_in));
  socklen_t length = sizeof(struct sockaddr_in);
  ssize_t result = recvfrom(handle->socket, (char*)buffer, buffer_size, 0,
                            (struct sockaddr*)&address, &length);
  if (result > 0) {
    *sender_ip_address = address.sin_addr.s_addr;
    *sender_port_id = ntohs(address.sin_port);
  }
  return result < 0 ? 0 : (ts_size_t)result;
}

static ts_bool_t ts_unix_device_send(void* device_context,
                                     ts_ipv4_t target_ip_address,
                                     ts_port_t target_port_id,
                                     const ts_byte_t* buffer,
                                     ts_size_t buffer_size) {
  ts_unix_device_context_t* handle = (ts_unix_device_context_t*)device_context;
  struct sockaddr_in address;
  memset(&address, 0, sizeof(struct sockaddr_in));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = target_ip_address;
  address.sin_port = htons(target_port_id);
  return sendto(handle->socket, buffer, buffer_size, 0,
                (const struct sockaddr*)&address,
                sizeof(struct sockaddr_in)) == buffer_size;
}

static ts_uint_t ts_unix_device_clock_ms(void) {
  struct timeval timev;
  gettimeofday(&timev, NULL);

  return timev.tv_sec * 1000 + timev.tv_usec / 1000;
}

static void ts_unix_device_destructor(void* device_context) {
  ts_unix_device_context_t* handle = (ts_unix_device_context_t*)device_context;
  if (handle->socket >= 0) {
    close(handle->socket);
  }
}

static ts_bool_t ts_initialize_socket(ts_unix_device_context_t* device,
                                      ts_port_t server_port) {
  if ((device->socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    return TS_OPERATION_FAILURE;
  }
  struct sockaddr_in address;
  memset(&address, 0, sizeof(struct sockaddr_in));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(server_port);
  if (bind(device->socket, (const struct sockaddr*)&address,
           sizeof(struct sockaddr_in)) < 0) {
    return TS_OPERATION_FAILURE;
  }
  if (fcntl(device->socket, F_SETFL, O_NONBLOCK) < 0) {
    return TS_OPERATION_FAILURE;
  }
  return TS_OPERATION_SUCCESS;
}

ts_device_context_t ts_initialize_unix_device_context(
    ts_port_t server_port_id) {
  ts_device_context_t context;
  memset(&context, 0, sizeof(ts_device_context_t));
  ts_unix_device_context_t* handle = (ts_unix_device_context_t*)&context;

  if (ts_initialize_socket(handle, server_port_id) == TS_OPERATION_FAILURE) {
    context.current_state = TS_DEVICE_UNINITIALIZED;
    return context;
  }
  context.recv_cb = ts_unix_device_receive;
  context.send_cb = ts_unix_device_send;
  context.clock_cb = ts_unix_device_clock_ms;
  context.destructor_cb = ts_unix_device_destructor;
  context.can_receive_ip = TS_TRUE;
  context.current_state = TS_DEVICE_INITIALIZED;
  return context;
}

ts_ipv4_t ts_unix_ipv4_from_str(ts_string_t ip_address) {
  return inet_addr(ip_address);
}

ts_string_t ts_unix_ipv4_to_str(ts_ipv4_t ip_address) {
  struct in_addr paddr;
  paddr.s_addr = ip_address;
  return inet_ntoa(paddr);
}
