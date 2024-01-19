#ifndef __TUPLE_SPACE_ZSUT_DEVICE_H__
#define __TUPLE_SPACE_ZSUT_DEVICE_H__

#include "../common/tuple_space_network.h"

extern "C" {

ts_device_context_t ts_initialize_zsut_device_context(void);

ts_ipv4_t ts_zsut_ipv4_from_bytes(ts_byte_t first, ts_byte_t second,
                                  ts_byte_t third, ts_byte_t fourth);
}

#endif  // __TUPLE_SPACE_ZSUT_DEVICE_H__
