#include <SPI.h>
#include <ZsutEthernet.h>
#include <ZsutEthernetUdp.h>
#include <ZsutFeatures.h>
#include <inttypes.h>
#include <libts.h>
#include <string.h>

static const ts_ipv4_t kServerIp = ts_zsut_ipv4_from_bytes(192, 168, 56, 102);
static const ZsutIPAddress kIpAddress{192, 168, 56, 103};
static uint8_t MacAddress[] = {0x01, 0xff, 0xaa, 0xbb, 0x34, 0x56};
static constexpr ts_port_t kServerPort = 43532;
static constexpr uint16_t kClientPort{3434};

static constexpr ts_size_t kApplication1N = 50;
static constexpr uint32_t kApplication2PeriodMs = 1000;

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

enum class Application1State : uint8_t { kSendRequest, kAquireResponse };

static void application_one_send_request(const ts_allocator_t& allocator,
                                         ts_size_t index) {
  ts_tuple_2_t tuple;
  tuple.fields[0] = ts_tuple_field_set_string("check_if_prime", TS_TRUE);
  tuple.fields[1] = ts_tuple_field_set_uint(index, TS_TRUE);
  auto status =
      ts_application_out(&Connection1, &tuple.fields[0], 2, &allocator);
  Serial.print("Application 1 send tuple to the tuplespace with status ");
  Serial.print(status);
  Serial.print(" with message (\"check_if_prime\", ");
  Serial.print(index);
  Serial.println("\")");
}

static bool application_one_get_response(const ts_allocator_t& allocator,
                                         ts_uint_t& value,
                                         ts_string_t resp_type) {
  ts_tuple_2_t tuple;
  tuple.fields[0] = ts_tuple_field_set_string(resp_type, TS_TRUE);
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
    if (strcmp(task_name, resp_type)) {
      Serial.println("Error, invalid task name");
      return false;
    }
    if (ts_tuple_field_get_uint(&result.tuple[1], &value) !=
        TS_OPERATION_SUCCESS) {
      Serial.println(
          "Error, the tuple does not contain uint at the second place");
      return false;
    }
    Serial.print("Got response for application 1 and type ");
    Serial.print(resp_type);
    Serial.print(" with status ");
    Serial.println(value);
    return true;
  }
  return false;
}

template <ts_size_t N>
static void application_one_print_result(const bool (&table)[N]) {
  for (ts_size_t i = 0; i < N; ++i) {
    Serial.print("Number ");
    Serial.print(i + 2);
    Serial.print(" is ");
    Serial.println(table[i] ? "prime" : "not prime");
  }
}

static void application_one_routine(const ts_allocator_t& allocator) {
  static Application1State app_state = Application1State::kSendRequest;
  static ts_size_t i = 2;
  static bool workers_responses[kApplication1N - 1]{};
  static ts_size_t workers_responds_clock = 0;

  switch (app_state) {
    case Application1State::kSendRequest: {
      application_one_send_request(allocator, i++);
      if (i > kApplication1N) {
        app_state = Application1State::kAquireResponse;
        i = 2;
      }
    }
      return;
    case Application1State::kAquireResponse: {
      ts_uint_t value;
      if (application_one_get_response(allocator, value, "is_prime")) {
        workers_responses[value - 2] = 1;
        ++workers_responds_clock;
      }
      if (application_one_get_response(allocator, value, "is_not_prime")) {
        workers_responses[value - 2] = 0;
        ++workers_responds_clock;
      }
      if (workers_responds_clock == sizeof(workers_responses)) {
        application_one_print_result(workers_responses);
        app_state = Application1State::kSendRequest;
        memset(workers_responses, 0, sizeof(workers_responses));
        workers_responds_clock = 0;
      }
    }
      return;
  }
}

static bool application_two_get_tuple(const ts_allocator_t& allocator,
                                      ts_bool_t& dir_change) {
  ts_tuple_2_t tuple;
  tuple.fields[0] = ts_tuple_field_set_string("direction_changed", TS_TRUE);
  tuple.fields[1] = ts_tuple_field_set_bool(0, TS_FALSE);
  ts_application_in_result_t result =
      ts_application_inp(&Connection2, &tuple.fields[0], 2, &allocator);
  if ((result.error == TS_APPLICATION_NO_ERROR) && (result.tuple != nullptr)) {
    Serial.println("Got task for application 2 from the tuple space");
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
    if (strcmp(task_name, "direction_changed")) {
      Serial.println("Error, invalid task name");
      return false;
    }
    if (ts_tuple_field_get_bool(&result.tuple[1], &dir_change) !=
        TS_OPERATION_SUCCESS) {
      Serial.println(
          "Error, the tuple does not contain bool at the second place");
      return false;
    }
    Serial.print("Got direction change at application 2 with value ");
    Serial.println(dir_change);
    return true;
  }
  return false;
}

static void application_two_routine(const ts_allocator_t& allocator) {
  static size_t first_counter = 0;
  static size_t second_counter = 0;
  static uint32_t last_time = ZsutMillis();

  ts_bool_t dir_change;
  if (application_two_get_tuple(allocator, dir_change)) {
    if (dir_change) {
      ++second_counter;
    } else {
      ++first_counter;
    }
  }
  uint32_t current_time = ZsutMillis();
  if (last_time + kApplication2PeriodMs < current_time) {
    last_time = current_time;
    Serial.println("Pin direction change stats");
    Serial.print("Change direction from 0 -> 1: ");
    Serial.println(first_counter);
    Serial.print("Change direction from 1 -> 0: ");
    Serial.println(second_counter);
  }
}

void loop() {
  ts_allocator_t allocator = ts_memory_resource_get_allocator(&MemoryResource);

  application_one_routine(allocator);

  ts_memory_resource_free_block(&MemoryResource);

  application_two_routine(allocator);

  ts_memory_resource_free_block(&MemoryResource);

  delay(100);
}
