#pragma once

// for linked list implementation, see test/linked_list_test.c
// for hash table implementation, see test/hash_table_list_test.c

#include "assert.h"
#include "math.h"
#include "type.h"

#if __has_builtin(__builtin_bzero)
#define bzero(address, size) __builtin_bzero(address, size)
#else
#error bzero must be supported by compiler
#endif

#if __has_builtin(__builtin_alloca)
#define alloca(size) __builtin_alloca(size)
#else
#error alloca must be supported by compiler
#endif

#if __has_builtin(__builtin_memcpy)
#define memcpy(dest, src, n) __builtin_memcpy(dest, src, n)
#else
#error memcpy must be supported by compiler
#endif

typedef struct {
  void *block;
  u64 used;
  u64 total;
} memory_arena;

typedef struct {
  void *block;
  u64 size;
  u64 max;
} memory_chunk;

typedef struct {
  memory_arena *arena;
  u64 startedAt;
} memory_temp;

static memory_arena
MemoryArenaSub(memory_arena *master, u64 size)
{
  debug_assert(master->used + size <= master->total);

  memory_arena sub = {
      .total = size,
      .block = master->block + master->used,
  };

  master->used += size;
  return sub;
}

static void *
MemoryArenaPushUnaligned(memory_arena *mem, u64 size)
{
  debug_assert(mem->used + size <= mem->total);
  void *result = mem->block + mem->used;
  mem->used += size;
  return result;
}

static void *
MemoryArenaPush(memory_arena *mem, u64 size, u64 alignment)
{
  debug_assert(IsPowerOfTwo(alignment));

  void *block = mem->block + mem->used;

  u64 alignmentMask = alignment - 1;
  u64 alignmentResult = ((u64)block & alignmentMask);
  if (alignmentResult != 0) {
    // if it is not aligned
    u64 alignmentOffset = alignment - alignmentResult;
    size += alignmentOffset;
    block += alignmentOffset;
  }

  debug_assert(mem->used + size <= mem->total);
  mem->used += size;

  return block;
}

static memory_chunk *
MemoryArenaPushChunk(memory_arena *mem, u64 size, u64 max)
{
  memory_chunk *chunk = MemoryArenaPush(mem, sizeof(*chunk) + max * sizeof(u8) + max * size, 4);
  chunk->block = (u8 *)chunk + sizeof(*chunk);
  chunk->size = size;
  chunk->max = max;
  for (u64 index = 0; index < chunk->max; index++) {
    u8 *flag = (u8 *)chunk->block + (sizeof(u8) * index);
    *flag = 0;
  }
  return chunk;
}

static inline b8
MemoryChunkIsDataAvailableAt(memory_chunk *chunk, u64 index)
{
  u8 *flags = (u8 *)chunk->block;
  return *(flags + index);
}

static inline void *
MemoryChunkGetDataAt(memory_chunk *chunk, u64 index)
{
  void *dataBlock = (u8 *)chunk->block + chunk->max;
  void *result = dataBlock + index * chunk->size;
  return result;
}

static void *
MemoryChunkPush(memory_chunk *chunk)
{
  void *result = 0;
  void *dataBlock = chunk->block + sizeof(u8) * chunk->max;
  for (u64 index = 0; index < chunk->max; index++) {
    u8 *flag = chunk->block + sizeof(u8) * index;
    if (*flag == 0) {
      result = dataBlock + index * chunk->size;
      *flag = 1;
      return result;
    }
  }

  return result;
}

static void
MemoryChunkPop(memory_chunk *chunk, void *block)
{
  void *dataBlock = chunk->block + sizeof(u8) * chunk->max;
  debug_assert((block >= dataBlock && block <= dataBlock + (chunk->size * chunk->max)) &&
               "this block is not belong to this chunk");
  u64 index = ((u64)block - (u64)dataBlock) / chunk->size;
  u8 *flag = chunk->block + sizeof(u8) * index;
  *flag = 0;
}

static memory_temp
MemoryTempBegin(memory_arena *arena)
{
  return (memory_temp){
      .arena = arena,
      .startedAt = arena->used,
  };
}

static void
MemoryTempEnd(memory_temp *tempMemory)
{
  memory_arena *arena = tempMemory->arena;
  arena->used = tempMemory->startedAt;
}

#define __cleanup_memory_temp__ __attribute__((cleanup(MemoryTempEnd)))

#include "text.h"
static struct string
MemoryArenaPushString(memory_arena *arena, u64 size)
{
  return (struct string){
      .value = MemoryArenaPush(arena, size, 4),
      .length = size,
  };
}
