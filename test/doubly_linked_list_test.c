#include "memory.h"

/*
 * This implementation is using statically allocated buffer to implement
 * doubly linked list also circular.
 *
 * For visualization open doc/doubly_linked_list.excalidraw at https://excalidraw.com/
 * See also:
 * - https://vimeo.com/649009599 "Andrew Kelley Practical Data Oriented Design (DoD)"
 *   @YouTube: https://www.youtube.com/watch?v=IroPQ150F6c
 */

struct entry {
  u32 prev; // index in buffer
  u32 next; // index in buffer
  u32 value;
};

struct state {
  u32 head;  // initially 0
  u32 tail;  // initially 0
  u32 count; // initially 0
  u32 max;   // only for debug
  memory_arena *memory;
  struct entry *entries;
};

/*
 * Pushes entry to at end of buffer.
 * Time Complexity: o(1)
 */
static void
EntryAppend(struct state *state, u32 value)
{
  u32 index = state->count;
  struct entry *entry = state->entries + index;
  entry->value = value;

  entry->prev = state->head;
  entry->next = state->head;

  if (state->count != 0) {
    struct entry *head = state->entries + state->head;
    struct entry *tail = state->entries + state->tail;

    entry->prev = state->tail;
    head->prev = index;
    tail->next = index;
    state->tail = index;
  }

  state->count++;
  debug_assert(state->count <= state->max && "buffer overflow");
}

/*
 * Pushes entry to at beginning of buffer.
 * Time Complexity: o(1)
 */
static void
EntryPrepend(struct state *state, u32 value)
{
  u32 index = state->count;
  struct entry *entry = state->entries + index;
  entry->value = value;

  entry->prev = state->tail;
  entry->next = state->tail;

  if (state->count != 0) {
    struct entry *head = state->entries + state->head;
    struct entry *tail = state->entries + state->tail;

    entry->next = state->head;
    head->prev = index;
    tail->next = index;
    state->head = index;
  }

  state->count++;
  debug_assert(state->count <= state->max && "buffer overflow");
}

// TODO: Show error pretty error message when a test fails
enum doubly_linked_list_test_error {
  DOUBLY_LINKED_LIST_TEST_ERROR_NONE = 0,
  DOUBLY_LINKED_LIST_TEST_ERROR_ENTRY_APPEND_WRONG_ENTRY_COUNT,
  DOUBLY_LINKED_LIST_TEST_ERROR_ENTRY_APPEND_WRONG_ENTRY_LOOP,
  DOUBLY_LINKED_LIST_TEST_ERROR_ENTRY_APPEND_WRONG_ENTRY_LOOP_REVERSE,
  DOUBLY_LINKED_LIST_TEST_ERROR_ENTRY_PREPEND_WRONG_ENTRY_COUNT,
  DOUBLY_LINKED_LIST_TEST_ERROR_ENTRY_PREPEND_WRONG_ENTRY_LOOP,
  DOUBLY_LINKED_LIST_TEST_ERROR_ENTRY_PREPEND_WRONG_ENTRY_LOOP_REVERSE,

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
  enum doubly_linked_list_test_error errorCode = DOUBLY_LINKED_LIST_TEST_ERROR_NONE;
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

  // EntryAppend(struct state *state, u32 value)
  tempMemory = MemoryTempBegin(&memory);
  {
    struct state state = {
        .memory = tempMemory.arena,
        .max = 10,
    };
    state.entries = MemoryArenaPushUnaligned(tempMemory.arena, sizeof(*state.entries) * state.max);
    EntryAppend(&state, 1);
    EntryAppend(&state, 2);
    EntryAppend(&state, 3);

    {
      u32 expected[] = {1, 2, 3};
      if (state.count != ARRAY_COUNT(expected)) {
        errorCode = DOUBLY_LINKED_LIST_TEST_ERROR_ENTRY_APPEND_WRONG_ENTRY_COUNT;
        goto end;
      }
    }

    {
      u32 expected[] = {1, 2, 3, 1, 2, 3};
      struct entry *it = state.entries + state.head;
      for (u32 index = 0; index < ARRAY_COUNT(expected); index++) {
        if (it->value != expected[index]) {
          errorCode = DOUBLY_LINKED_LIST_TEST_ERROR_ENTRY_APPEND_WRONG_ENTRY_LOOP;
          goto end;
        }

        it = state.entries + it->next;
      }
    }

    {
      u32 expected[] = {3, 2, 1, 3, 2, 1};
      struct entry *it = state.entries + state.tail;
      for (u32 index = 0; index < ARRAY_COUNT(expected); index++) {
        if (it->value != expected[index]) {
          errorCode = DOUBLY_LINKED_LIST_TEST_ERROR_ENTRY_APPEND_WRONG_ENTRY_LOOP_REVERSE;
          goto end;
        }

        it = state.entries + it->prev;
      }
    }
  }
  MemoryTempEnd(&tempMemory);

  // EntryPrepend(struct state *state, u32 value)
  tempMemory = MemoryTempBegin(&memory);
  {
    struct state state = {
        .memory = tempMemory.arena,
        .max = 10,
    };
    state.entries = MemoryArenaPushUnaligned(tempMemory.arena, sizeof(*state.entries) * state.max);
    EntryPrepend(&state, 3);
    EntryPrepend(&state, 2);
    EntryPrepend(&state, 1);

    {
      u32 expected[] = {1, 2, 3};
      if (state.count != ARRAY_COUNT(expected)) {
        errorCode = DOUBLY_LINKED_LIST_TEST_ERROR_ENTRY_PREPEND_WRONG_ENTRY_COUNT;
        goto end;
      }
    }

    {
      u32 expected[] = {1, 2, 3, 1, 2, 3};
      struct entry *it = state.entries + state.head;
      for (u32 index = 0; index < ARRAY_COUNT(expected); index++) {
        if (it->value != expected[index]) {
          errorCode = DOUBLY_LINKED_LIST_TEST_ERROR_ENTRY_PREPEND_WRONG_ENTRY_LOOP;
          goto end;
        }

        it = state.entries + it->next;
      }
    }

    {
      u32 expected[] = {3, 2, 1, 3, 2, 1};
      struct entry *it = state.entries + state.tail;
      for (u32 index = 0; index < ARRAY_COUNT(expected); index++) {
        if (it->value != expected[index]) {
          errorCode = DOUBLY_LINKED_LIST_TEST_ERROR_ENTRY_PREPEND_WRONG_ENTRY_LOOP_REVERSE;
          goto end;
        }

        it = state.entries + it->prev;
      }
    }
  }
  MemoryTempEnd(&tempMemory);

end:
  return (int)errorCode;
}
