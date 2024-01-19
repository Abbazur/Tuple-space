#include "tuple_space_zsut_device.h"

extern "C" {
#include <string.h>
}

#include <SPI.h>
#include <ZsutEthernet.h>
#include <ZsutEthernetUdp.h>
#include <ZsutFeatures.h>

struct ts_zsut_device_context_t {
  ts_data_recv_cb_t recv_cb;
  ts_data_send_cb_t send_cb;
  ts_clock_ms_cb_t clock_cb;
  ts_device_context_destructor_cb_t destructor_cb;
  ZsutEthernetUDP udp_connection;
  ts_byte_t reserved[TS_DEVICE_CONTEXT_SIZE - sizeof(ZsutEthernetUDP)];
  ts_byte_t current_state;  // ts_device_state_t
  ts_byte_t can_receive_ip;
};

static_assert(sizeof(ts_zsut_device_context_t) == sizeof(ts_device_context_t));

extern "C" {

static ts_size_t ts_zsut_device_recv(void* device_context,
                                     ts_ipv4_t* sender_ip_address,
                                     ts_port_t* sender_port_id,
                                     ts_byte_t* buffer, ts_size_t buffer_size) {
  auto* context = static_cast<ts_zsut_device_context_t*>(device_context);
  if (!context->udp_connection.parsePacket()) {
    return 0;
  }
  int size = context->udp_connection.read(buffer, buffer_size);
  return size < 0 ? 0 : static_cast<ts_size_t>(size);
}

static ts_bool_t ts_zsut_device_send(void* device_context,
                                     ts_ipv4_t target_ip_address,
                                     ts_port_t target_port_id,
                                     const ts_byte_t* buffer,
                                     ts_size_t buffer_size) {
  auto* context = static_cast<ts_zsut_device_context_t*>(device_context);
  context->udp_connection.beginPacket(ZsutIPAddress{target_ip_address},
                                      target_port_id);
  context->udp_connection.write(buffer, buffer_size);
  context->udp_connection.endPacket();
  return buffer_size;
}

static void ts_zsut_device_destructor(void* device_context) {
  auto* context = static_cast<ts_zsut_device_context_t*>(device_context);
  context->udp_connection.~ZsutEthernetUDP();
}

ts_device_context_t ts_initialize_zsut_device_context(ts_port_t port_id) {
  ts_device_context_t context{};
  auto& handle = reinterpret_cast<ts_zsut_device_context_t&>(context);
  context.can_receive_ip = TS_FALSE;
  context.recv_cb = ts_zsut_device_recv;
  context.send_cb = ts_zsut_device_send;
  context.destructor_cb = ts_zsut_device_destructor;
  context.current_state = TS_DEVICE_INITIALIZED;
  handle.udp_connection = ZsutEthernetUDP{};
  handle.udp_connection.begin(port_id);
  return context;
}

ts_ipv4_t ts_zsut_ipv4_from_bytes(ts_byte_t first, ts_byte_t second,
                                  ts_byte_t third, ts_byte_t fourth) {
  return static_cast<uint32_t>(ZsutIPAddress{first, second, third, fourth});
}
}
