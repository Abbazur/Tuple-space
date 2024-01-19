#include "tuple_space_block_memory_resource.h"

#include <string.h>

static void* ts_block_memory_resource_allocation_cb(void* context,
                                                    ts_size_t allocation_size) {
  ts_block_memory_resource_t* resource = (ts_block_memory_resource_t*)context;
  ts_byte_t* block = &resource->memory[resource->pointer];
  resource->pointer += allocation_size;
  return block;
}

static void ts_block_memory_resource_deallocation_cb(void* context,
                                                     void* memory) {}

void ts_initialize_memory_resource(ts_block_memory_resource_t* resource) {
  memset(resource, 0, sizeof(ts_block_memory_resource_t));
}

ts_allocator_t ts_memory_resource_get_allocator(
    ts_block_memory_resource_t* resource) {
  ts_allocator_t allocator;
  memset(&allocator, 0, sizeof(ts_allocator_t));
  allocator.allocator_cb = ts_block_memory_resource_allocation_cb;
  allocator.deallocator_cb = ts_block_memory_resource_deallocation_cb;
  allocator.context = resource;
  return allocator;
}

void ts_memory_resource_free_block(ts_block_memory_resource_t* resource) {
  memset(resource->memory, 0, TS_MEMORY_RESOURCE_SIZE);
  resource->pointer = 0;
}
