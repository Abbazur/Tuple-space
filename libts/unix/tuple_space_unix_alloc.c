#include "tuple_space_unix_alloc.h"

#include <stdlib.h>
#include <string.h>

static void* ts_unix_allocator(void* /*allocator_context*/,
                               ts_size_t allocated_size) {
  return malloc(allocated_size);
}

static void ts_unix_deallocator(void* /*allocator_context*/, void* memory_ptr) {
  free(memory_ptr);
}

ts_allocator_t ts_initialize_unix_allocator(void) {
  ts_allocator_t allocator;
  memset(&allocator, 0, sizeof(ts_allocator_t));
  allocator.allocator_cb = ts_unix_allocator;
  allocator.deallocator_cb = ts_unix_deallocator;
  allocator.context = NULL;
  return allocator;
}
