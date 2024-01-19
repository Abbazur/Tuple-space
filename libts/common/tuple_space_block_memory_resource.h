#ifndef __TUPLE_SPACE_BLOCK_MEMORY_RESOURCE_H__
#define __TUPLE_SPACE_BLOCK_MEMORY_RESOURCE_H__

#include "tuple_space_serialization.h"

#define TS_MEMORY_RESOURCE_SIZE 512

typedef struct {
  ts_byte_t memory[TS_MEMORY_RESOURCE_SIZE];
  ts_size_t pointer;
} ts_block_memory_resource_t;

void ts_initialize_memory_resource(ts_block_memory_resource_t* resource);

ts_allocator_t ts_memory_resource_get_allocator(
    ts_block_memory_resource_t* resource);

void ts_memory_resource_free_block(ts_block_memory_resource_t* resource);

#endif  // __TUPLE_SPACE_BLOCK_MEMORY_RESOURCE_H__
