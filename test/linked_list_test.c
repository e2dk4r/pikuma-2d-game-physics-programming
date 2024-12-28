#include "memory.h"

struct entry {
  u32 value;
  struct entry *next;
};

struct state {
  memory_arena *memory;
  struct entry *entries;
  struct entry *entriesFree;
};

/*
 * Pushes entry to in front of list.
 * Entry is created from allocated only if there is no available entry.
 * Time Complexity: o(1)
 */
static void
EntryAdd(struct state *state, u32 value)
{
  struct entry *entry = 0;
  if (state->entriesFree) {
    entry = state->entriesFree;
    state->entriesFree = state->entriesFree->next;
  } else {
    entry = MemoryArenaPushUnaligned(state->memory, sizeof(*entry));
  }

  entry->value = value;
  entry->next = state->entries;
  state->entries = entry;
}

/*
 * Removes first entry with same value from list.
 * Removed entry added to free list.
 * Time Complexity: o(n)
 */
static b8
EntryRemove(struct state *state, u32 value)
{
  b8 removed = 0;

  struct entry *prev = 0;
  for (struct entry *it = state->entries; it; it = it->next) {
    if (it->value != value) {
      prev = it;
      continue;
    }

    if (prev) {
      prev->next = it->next;
    } else {
      state->entries = it->next;
    }

    it->next = state->entriesFree;
    state->entriesFree = it;

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
enum linked_list_test_error {
  LINKED_LIST_TEST_ERROR_NONE = 0,
  LINKED_LIST_TEST_ERROR_ENTRY_ADD_ALLOCATED_FROM_MEMORY,
  LINKED_LIST_TEST_ERROR_ENTRY_ADD_USED_FROM_FREE_LIST,
  LINKED_LIST_TEST_ERROR_ENTRY_REMOVE_FROM_BEGINNING,
  LINKED_LIST_TEST_ERROR_ENTRY_REMOVE_FROM_MIDDLE,
  LINKED_LIST_TEST_ERROR_ENTRY_REMOVE_FROM_END,

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
  enum linked_list_test_error errorCode = LINKED_LIST_TEST_ERROR_NONE;
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

    EntryAdd(&state, 1);

    struct entry *notExpected = 0;
    struct entry *value = state.entries;
    if (value == notExpected ||
        // 1 entry allocated allowed
        state.memory->used != sizeof(struct entry)) {
      errorCode = LINKED_LIST_TEST_ERROR_ENTRY_ADD_ALLOCATED_FROM_MEMORY;
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

    struct entry *entryA = MemoryArenaPushUnaligned(state.memory, sizeof(*entryA));
    entryA->value = 2;
    entryA->next = 0;
    state.entries = entryA;
    struct entry *entryC = MemoryArenaPushUnaligned(state.memory, sizeof(*entryC));
    entryC->value = 3;
    entryC->next = 0;
    struct entry *entryB = MemoryArenaPushUnaligned(state.memory, sizeof(*entryB));
    entryB->value = 4;
    entryB->next = entryC;
    state.entriesFree = entryB;

    u64 usedMemoryBefore = state.memory->used;
    EntryAdd(&state, 1);

    /* STATE BEFORE:
     *   list: A
     *   free: B C
     * EXPECTED STATE:
     *   list: B A
     *   free: C
     */
    if (
        // is list correct?
        state.entries != entryB || state.entries->next != entryA ||
        GetEntriesCount(state.entries) != 2
        // is free list correct?
        || state.entriesFree != entryC ||
        GetEntriesCount(state.entriesFree) != 1
        // no allocation allowed
        || state.memory->used != usedMemoryBefore) {
      errorCode = LINKED_LIST_TEST_ERROR_ENTRY_ADD_USED_FROM_FREE_LIST;
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
    struct entry *entryA = MemoryArenaPushUnaligned(state.memory, sizeof(*entryA));
    entryA->value = 11;
    entryA->next = 0;
    struct entry *entryB = MemoryArenaPushUnaligned(state.memory, sizeof(*entryB));
    entryB->value = 12;
    entryB->next = entryA;
    state.entries = entryB;
    // prepare free list
    struct entry *entryC = MemoryArenaPushUnaligned(state.memory, sizeof(*entryC));
    entryC->value = 13;
    entryC->next = 0;
    state.entriesFree = entryC;

    u64 usedMemoryBefore = state.memory->used;
    EntryRemove(&state, entryB->value);

    /* STATE BEFORE:
     *   list: B A
     *   free: C
     * EXPECTED STATE:
     *   list: A
     *   free: B C
     */
    if (
        // is list correct?
        state.entries != entryA ||
        GetEntriesCount(state.entries) != 1
        // is free list correct?
        || state.entriesFree != entryB || state.entriesFree->next != entryC ||
        GetEntriesCount(state.entriesFree) != 2
        // no allocation allowed
        || state.memory->used != usedMemoryBefore) {
      errorCode = LINKED_LIST_TEST_ERROR_ENTRY_REMOVE_FROM_BEGINNING;
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
    struct entry *entryA = MemoryArenaPushUnaligned(state.memory, sizeof(*entryA));
    entryA->value = 11;
    entryA->next = 0;
    struct entry *entryB = MemoryArenaPushUnaligned(state.memory, sizeof(*entryB));
    entryB->value = 12;
    entryB->next = entryA;
    struct entry *entryC = MemoryArenaPushUnaligned(state.memory, sizeof(*entryC));
    entryC->value = 13;
    entryC->next = entryB;
    state.entries = entryC;

    u64 usedMemoryBefore = state.memory->used;
    EntryRemove(&state, entryB->value);

    /* STATE BEFORE:
     *   list: C B A
     *   free:
     * EXPECTED STATE:
     *   list: C A
     *   free: B
     */
    if (
        // is list correct?
        state.entries != entryC || state.entries->next != entryA ||
        GetEntriesCount(state.entries) != 2
        // is free list correct?
        || state.entriesFree != entryB ||
        GetEntriesCount(state.entriesFree) != 1
        // no allocation allowed
        || state.memory->used != usedMemoryBefore) {
      errorCode = LINKED_LIST_TEST_ERROR_ENTRY_REMOVE_FROM_MIDDLE;
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
    struct entry *entryA = MemoryArenaPushUnaligned(state.memory, sizeof(*entryA));
    entryA->value = 11;
    entryA->next = 0;
    struct entry *entryB = MemoryArenaPushUnaligned(state.memory, sizeof(*entryB));
    entryB->value = 12;
    entryB->next = entryA;
    struct entry *entryC = MemoryArenaPushUnaligned(state.memory, sizeof(*entryC));
    entryC->value = 13;
    entryC->next = entryB;
    state.entries = entryC;

    u64 usedMemoryBefore = state.memory->used;
    EntryRemove(&state, entryA->value);

    /* STATE BEFORE:
     *   list: C B A
     *   free:
     * EXPECTED STATE:
     *   list: C B
     *   free: A
     */
    if (
        // is list correct?
        state.entries != entryC || state.entries->next != entryB ||
        GetEntriesCount(state.entries) != 2
        // is free list correct?
        || state.entriesFree != entryA ||
        GetEntriesCount(state.entriesFree) != 1
        // no allocation allowed
        || state.memory->used != usedMemoryBefore) {
      errorCode = LINKED_LIST_TEST_ERROR_ENTRY_REMOVE_FROM_END;
      goto end;
    }
  }
  MemoryTempEnd(&tempMemory);

end:
  return (int)errorCode;
}
