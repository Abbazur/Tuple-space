#ifndef __TUPLE_SPACE_UNIX_DEVICE_H__
#define __TUPLE_SPACE_UNIX_DEVICE_H__

#include "../common/tuple_space_network.h"

ts_device_context_t ts_initialize_unix_device_context(ts_port_t server_port_id);

ts_ipv4_t ts_unix_ipv4_from_str(ts_string_t ip_address);

ts_string_t ts_unix_ipv4_to_str(ts_ipv4_t ip_address);

#endif  // __TUPLE_SPACE_UNIX_DEVICE_H__
