#include <SPI.h>
#include <ZsutEthernet.h>
#include <ZsutEthernetUdp.h>
#include <ZsutFeatures.h>
#include <inttypes.h>
#include <libts.h>
#include <string.h>

static const ts_ipv4_t kServerIp = ts_zsut_ipv4_from_bytes(192, 168, 56, 102);
static const ZsutIPAddress kIpAddress{192, 168, 56, 105};
static uint8_t MacAddress[] = {0x01, 0xff, 0xda, 0xbb, 0xfd, 0x56};
static constexpr ts_port_t kServerPort = 43532;
static constexpr uint16_t kClientPort{3466};

static constexpr ts_size_t kApplication1N = 50;

static ts_application_context_t Context{};
static ts_application_connection_t Connection1{};
static ts_application_connection_t Connection2{};
static ts_block_memory_resource_t MemoryResource{};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting");
  ZsutEthernet.begin(MacAddress, kIpAddress);
  if (!ts_initialize_application_context(
          &Context, ts_initialize_zsut_device_context(kClientPort), kServerIp,
          kServerPort, 5, 1000)) {
    Serial.println("Application context initialization failed");
  }

  Connection1 = ts_application_create_connection(&Context, 1);
  Connection2 = ts_application_create_connection(&Context, 2);
  delay(100);

  ts_initialize_memory_resource(&MemoryResource);
}

static bool application_one_get_task(const ts_allocator_t& allocator,
                                     ts_uint_t& value) {
  ts_tuple_2_t tuple;
  tuple.fields[0] = ts_tuple_field_set_string("check_if_prime", TS_TRUE);
  tuple.fields[1] = ts_tuple_field_set_uint(0, TS_FALSE);
  ts_application_in_result_t result =
      ts_application_inp(&Connection1, &tuple.fields[0], 2, &allocator);
  if ((result.error == TS_APPLICATION_NO_ERROR) && (result.tuple != nullptr)) {
    Serial.println("Got task for application 1 from the tuple space");
    ts_string_t task_name;
    if (result.tuple_size != 2) {
      Serial.println("Error, got tuple with invalid length");
      return false;
    }
    if (ts_tuple_field_get_string(&result.tuple[0], &task_name) !=
        TS_OPERATION_SUCCESS) {
      Serial.println(
          "Error, the tuple does not contain string at the first place");
      return false;
    }
    Serial.println(result.tuple[0].data.string_field);
    if (strcmp(task_name, "check_if_prime")) {
      Serial.println("Error, invalid task name");
      return false;
    }
    if (ts_tuple_field_get_uint(&result.tuple[1], &value) !=
        TS_OPERATION_SUCCESS) {
      Serial.println(
          "Error, the tuple does not contain uint at the second place");
      return false;
    }
    return true;
  } else {
    Serial.println("Didn't got task for application 1");
  }
  return false;
}

static bool is_prime(ts_uint_t value) {
  for (ts_size_t i = 2; i * i <= value; ++i) {
    if ((value % i) == 0) {
      return false;
    }
  }
  return true;
}

static void application_one_send_result(const ts_allocator_t& allocator,
                                        ts_uint_t value,
                                        ts_string_t respond_type) {
  ts_tuple_2_t tuple;
  tuple.fields[0] = ts_tuple_field_set_string(respond_type, TS_TRUE);
  tuple.fields[1] = ts_tuple_field_set_uint(value, TS_TRUE);
  auto status =
      ts_application_out(&Connection1, &tuple.fields[0], 2, &allocator);
  Serial.print("Application one has send an information that the number ");
  Serial.print(value);
  Serial.print(" is ");
  Serial.print(respond_type);
  Serial.print(" with status ");
  Serial.println(status);
}

static void application_one_routine(const ts_allocator_t& allocator) {
  ts_uint_t value;
  if (!application_one_get_task(allocator, value)) {
    return;
  }
  Serial.print("Application task has received task to check if the ");
  Serial.print(value);
  Serial.println(" is prime");
  if (is_prime(value)) {
    application_one_send_result(allocator, value, "is_prime");
  } else {
    application_one_send_result(allocator, value, "is_not_prime");
  }
}

// change = 0 : 0 -> 1
// change = 1 : 1 -> 0
static void application_two_send_state(const ts_allocator_t& allocator,
                                       bool change) {
  ts_tuple_2_t tuple;
  tuple.fields[0] = ts_tuple_field_set_string("direction_changed", TS_TRUE);
  tuple.fields[1] = ts_tuple_field_set_bool(change, TS_TRUE);
  auto status =
      ts_application_out(&Connection2, &tuple.fields[0], 2, &allocator);
  Serial.print("Send GPIO pin direction change with value ");
  Serial.println(change);
}

static void application_two_routine(const ts_allocator_t& allocator) {
  static uint16_t status = 0;

  uint16_t new_status = ZsutDigitalRead();
  if (new_status != status) {
    application_two_send_state(allocator, status);
    status = new_status;
  }
}

void loop() {
  Serial.println("Working");
  ts_allocator_t allocator = ts_memory_resource_get_allocator(&MemoryResource);

  application_one_routine(allocator);

  ts_memory_resource_free_block(&MemoryResource);

  application_two_routine(allocator);

  ts_memory_resource_free_block(&MemoryResource);

  delay(100);
}