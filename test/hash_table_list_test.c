#include "memory.h"

/*
 * Example: Use hash table for pairwise collision rule
 *
 * To see visualization load below file in https://excalidraw.com/
 *   test/hash_table_list.excalidraw
 */

struct entry {
  u32 a;
  u32 b;
  struct entry *next;
};

struct state {
  memory_arena *memory;
  struct entry *entries[256];
  struct entry *entriesFree;
};

static inline u32
EntryHash(struct state *state, u32 key)
{
  /* TODO: better hash function */
  u32 hashMask = ARRAY_COUNT(state->entries) - 1;
  u32 hashValue = key & hashMask;
  return hashValue;
}

static inline void
EntrySet(struct state *state, u32 key, struct entry *entry)
{
  u32 hashValue = EntryHash(state, key);
  state->entries[hashValue] = entry;
}

/*
 * @return entries with same hash
 */
static struct entry *
EntryGet(struct state *state, u32 key)
{
  u32 hashValue = EntryHash(state, key);
  struct entry *entriesWithSameHash = state->entries[hashValue];
  return entriesWithSameHash;
}

/*
 * Pushes entry to in front of list.
 * Entry is created from allocated only if there is no available entry.
 * Time Complexity: If there is no hash collision O(1),
 *                  If there is k number of hash collisions O(k)
 */
static void
EntryAdd(struct state *state, u32 a, u32 b)
{
  struct entry *entriesWithSameHash = EntryGet(state, a);
  struct entry *found = 0;
  for (struct entry *it = entriesWithSameHash; it; it = it->next) {
    if (it->a != a || it->b != b)
      continue;

    found = it;
    break;
  }

  if (!found) {
    if (state->entriesFree) {
      found = state->entriesFree;
      state->entriesFree = state->entriesFree->next;
    } else {
      found = MemoryArenaPushUnaligned(state->memory, sizeof(*found));
    }

    found->next = entriesWithSameHash;
    EntrySet(state, a, found);
  }

  found->a = a;
  found->b = b;
}

/*
 * Removes first entry with same value from list.
 * Removed entry added to free list.
 * Time Complexity: If there is no hash collision O(1),
 *                  If there is k number of hash collisions O(k)
 */
static b8
EntryRemove(struct state *state, u32 a)
{
  b8 removed = 0;

  struct entry *prev = 0;
  for (struct entry *it = EntryGet(state, a); it; it = it->next) {
    if (it->a != a) {
      prev = it;
      continue;
    }

    struct entry *removedEntry = it;
    if (prev)
      prev->next = removedEntry->next;
    else
      EntrySet(state, a, removedEntry->next);

    removedEntry->next = state->entriesFree;
    state->entriesFree = removedEntry;

    removed = 1;
  }

  return removed;
}

// Helper functions for debugging list
static u32
GetEntriesCount(struct entry *entries)
{
  u32 count = 0;
  for (struct entry *it = entries; it; it = it->next)
    count++;
  return count;
}

// TODO: Show error pretty error message when a test fails
enum hash_table_list_test_error {
  HASH_TABLE_LIST_TEST_ERROR_NONE = 0,
  HASH_TABLE_LIST_TEST_ERROR_ENTRY_ADD_ALLOCATED_FROM_MEMORY,
  HASH_TABLE_LIST_TEST_ERROR_ENTRY_ADD_USED_FROM_FREE_LIST,
  HASH_TABLE_LIST_TEST_ERROR_ENTRY_REMOVE_FROM_BEGINNING,
  HASH_TABLE_LIST_TEST_ERROR_ENTRY_REMOVE_FROM_MIDDLE,
  HASH_TABLE_LIST_TEST_ERROR_ENTRY_REMOVE_FROM_END,

  // src: https://mesonbuild.com/Unit-tests.html#skipped-tests-and-hard-errors
  // For the default exitcode testing protocol, the GNU standard approach in
  // this case is to exit the program with error code 77. Meson will detect this
  // and report these tests as skipped rather than failed. This behavior was
  // added in version 0.37.0.
  MESON_TEST_SKIP = 77,
  // In addition, sometimes a test fails set up so that it should fail even if
  // it is marked as an expected failure. The GNU standard approach in this case
  // is to exit the program with error code 99. Again, Meson will detect this
  // and report these tests as ERROR, ignoring the setting of should_fail. This
  // behavior was added in version 0.50.0.
  MESON_TEST_FAILED_TO_SET_UP = 99,
};

int
main(void)
{
  enum hash_table_list_test_error errorCode = HASH_TABLE_LIST_TEST_ERROR_NONE;
  memory_arena memory;
  memory_temp tempMemory;

  {
    u64 KILOBYTES = 1 << 10;
    u64 total = 1 * KILOBYTES;
    memory = (memory_arena){.block = alloca(total), .total = total};
    if (memory.block == 0) {
      errorCode = MESON_TEST_FAILED_TO_SET_UP;
      goto end;
    }
    bzero(memory.block, memory.total);
  }

  // EntryAdd(struct state *state, u32 value)
  tempMemory = MemoryTempBegin(&memory);
  {
    struct state state = {
        .memory = tempMemory.arena,
        .entries = 0,
        .entriesFree = 0,
    };

    u32 key = 1;
    EntryAdd(&state, key, 1000);

    struct entry *notExpected = 0;
    struct entry *value = EntryGet(&state, key);
    if (value == notExpected ||
        // 1 entry allocated allowed
        state.memory->used != sizeof(struct entry)) {
      errorCode = HASH_TABLE_LIST_TEST_ERROR_ENTRY_ADD_ALLOCATED_FROM_MEMORY;
      goto end;
    }
  }
  MemoryTempEnd(&tempMemory);

  tempMemory = MemoryTempBegin(&memory);
  {
    struct state state = {
        .memory = tempMemory.arena,
        .entries = 0,
        .entriesFree = 0,
    };

    // prepare list
    u32 key = 1;
    struct entry *entryA = MemoryArenaPushUnaligned(state.memory, sizeof(*entryA));
    entryA->a = key;
    entryA->b = 1000;
    entryA->next = 0;
    state.entries[entryA->a] = entryA;
    // prepare free list
    struct entry *entryB = MemoryArenaPushUnaligned(state.memory, sizeof(*entryB));
    entryB->a = 100;
    entryB->b = 1500;
    entryB->next = 0;
    struct entry *entryC = MemoryArenaPushUnaligned(state.memory, sizeof(*entryC));
    entryC->a = 200;
    entryC->b = 2000;
    entryC->next = entryB;
    state.entriesFree = entryC;

    u64 usedMemoryBefore = state.memory->used;
    EntryAdd(&state, key, 9999);

    /* STATE BEFORE:
     *   list: 1: A
     *         2:
     *         3:
     *   free: C B
     * EXPECTED STATE:
     *   list: 1: C A
     *         2:
     *         3:
     *   free: B
     */
    if (
        // is list correct?
        state.entries[key] != entryC || state.entries[key]->next != entryA ||
        GetEntriesCount(state.entries[key]) != 2
        // is free list correct?
        || state.entriesFree != entryB ||
        GetEntriesCount(state.entriesFree) != 1
        // no allocation allowed
        || state.memory->used != usedMemoryBefore) {
      errorCode = HASH_TABLE_LIST_TEST_ERROR_ENTRY_ADD_USED_FROM_FREE_LIST;
      goto end;
    }
  }
  MemoryTempEnd(&tempMemory);

  // EntryRemove(struct state *state, u32 value)
  tempMemory = MemoryTempBegin(&memory);
  {
    struct state state = {
        .memory = tempMemory.arena,
        .entries = 0,
        .entriesFree = 0,
    };

    // prepare list
    u32 key = 1;
    struct entry *entryA = MemoryArenaPushUnaligned(state.memory, sizeof(*entryA));
    entryA->a = key + 9999;
    entryA->b = 11;
    entryA->next = 0;
    struct entry *entryB = MemoryArenaPushUnaligned(state.memory, sizeof(*entryB));
    entryB->a = key + 9999;
    entryB->b = 12;
    entryB->next = entryA;
    struct entry *entryC = MemoryArenaPushUnaligned(state.memory, sizeof(*entryC));
    entryC->a = key;
    entryC->b = 13;
    entryC->next = entryB;
    state.entries[key] = entryC;
    // prepare free list
    struct entry *entryD = MemoryArenaPushUnaligned(state.memory, sizeof(*entryD));
    entryD->a = 3;
    entryD->b = 14;
    entryD->next = 0;
    state.entriesFree = entryD;

    u64 usedMemoryBefore = state.memory->used;
    EntryRemove(&state, key);

    /* STATE BEFORE:
     *   list: 1: C B A
     *         2:
     *         3:
     *   free: D
     * EXPECTED STATE:
     *   list: 1: B A
     *         2:
     *         3:
     *   free: C D
     */
    if (
        // is list correct?
        state.entries[key] != entryB || state.entries[key]->next != entryA ||
        GetEntriesCount(state.entries[key]) != 2
        // is free list correct?
        || state.entriesFree != entryC || state.entriesFree->next != entryD ||
        GetEntriesCount(state.entriesFree) != 2
        // no allocation allowed
        || state.memory->used != usedMemoryBefore) {
      errorCode = HASH_TABLE_LIST_TEST_ERROR_ENTRY_REMOVE_FROM_BEGINNING;
      goto end;
    }
  }
  MemoryTempEnd(&tempMemory);

  tempMemory = MemoryTempBegin(&memory);
  {
    struct state state = {
        .memory = tempMemory.arena,
        .entries = 0,
        .entriesFree = 0,
    };

    // prepare list
    u32 key = 1;
    struct entry *entryA = MemoryArenaPushUnaligned(state.memory, sizeof(*entryA));
    entryA->a = key + 9999;
    entryA->b = 11;
    entryA->next = 0;
    struct entry *entryB = MemoryArenaPushUnaligned(state.memory, sizeof(*entryB));
    entryB->a = key;
    entryB->b = 12;
    entryB->next = entryA;
    struct entry *entryC = MemoryArenaPushUnaligned(state.memory, sizeof(*entryC));
    entryC->a = key + 9999;
    entryC->b = 13;
    entryC->next = entryB;
    state.entries[key] = entryC;
    // prepare free list
    struct entry *entryD = MemoryArenaPushUnaligned(state.memory, sizeof(*entryD));
    entryD->a = 3;
    entryD->b = 14;
    entryD->next = 0;
    state.entriesFree = entryD;

    u64 usedMemoryBefore = state.memory->used;
    EntryRemove(&state, key);

    /* STATE BEFORE:
     *   list: 1: C B A
     *         2:
     *         3:
     *   free: D
     * EXPECTED STATE:
     *   list: 1: C A
     *         2:
     *         3:
     *   free: B D
     */
    if (
        // is list correct?
        state.entries[key] != entryC || state.entries[key]->next != entryA ||
        GetEntriesCount(state.entries[key]) != 2
        // is free list correct?
        || state.entriesFree != entryB || state.entriesFree->next != entryD ||
        GetEntriesCount(state.entriesFree) != 2
        // no allocation allowed
        || state.memory->used != usedMemoryBefore) {
      errorCode = HASH_TABLE_LIST_TEST_ERROR_ENTRY_REMOVE_FROM_MIDDLE;
      goto end;
    }
  }
  MemoryTempEnd(&tempMemory);

  tempMemory = MemoryTempBegin(&memory);
  {
    struct state state = {
        .memory = tempMemory.arena,
        .entries = 0,
        .entriesFree = 0,
    };

    // prepare list
    u32 key = 1;
    struct entry *entryA = MemoryArenaPushUnaligned(state.memory, sizeof(*entryA));
    entryA->a = key;
    entryA->b = 11;
    entryA->next = 0;
    struct entry *entryB = MemoryArenaPushUnaligned(state.memory, sizeof(*entryB));
    entryB->a = key + 9999;
    entryB->b = 12;
    entryB->next = entryA;
    struct entry *entryC = MemoryArenaPushUnaligned(state.memory, sizeof(*entryC));
    entryC->a = key + 9999;
    entryC->b = 13;
    entryC->next = entryB;
    state.entries[key] = entryC;
    // prepare free list
    struct entry *entryD = MemoryArenaPushUnaligned(state.memory, sizeof(*entryD));
    entryD->a = 3;
    entryD->b = 14;
    entryD->next = 0;
    state.entriesFree = entryD;

    u64 usedMemoryBefore = state.memory->used;
    EntryRemove(&state, key);

    /* STATE BEFORE:
     *   list: 1: C B A
     *         2:
     *         3:
     *   free: D
     * EXPECTED STATE:
     *   list: 1: C B
     *         2:
     *         3:
     *   free: A D
     */
    if (
        // is list correct?
        state.entries[key] != entryC || state.entries[key]->next != entryB ||
        GetEntriesCount(state.entries[key]) != 2
        // is free list correct?
        || state.entriesFree != entryA || state.entriesFree->next != entryD ||
        GetEntriesCount(state.entriesFree) != 2
        // no allocation allowed
        || state.memory->used != usedMemoryBefore) {
      errorCode = HASH_TABLE_LIST_TEST_ERROR_ENTRY_REMOVE_FROM_MIDDLE;
      goto end;
    }
  }
  MemoryTempEnd(&tempMemory);

end:
  return (int)errorCode;
}
