#include "game.h"
#include "color.h"
#include "math.h"
#include "renderer.h"
#include "string_builder.h"

#if IS_BUILD_DEBUG
#include <unistd.h> // write()
#endif

void
GameUpdateAndRender(game_memory *memory, game_input *input, game_renderer *renderer)
{
  game_state *state = memory->permanentStorage;
  debug_assert(memory->permanentStorageSize >= sizeof(*state));

  if (!state->isInitialized) {
    state->worldArena = (memory_arena){
        .total = memory->permanentStorageSize - sizeof(*state),
        .block = memory->permanentStorage + sizeof(*state),
    };

    state->isInitialized = 1;
  }

  transient_state *transientState = memory->transientStorage;
  debug_assert(memory->transientStorageSize >= sizeof(*transientState));
  if (!transientState->isInitialized) {
    transientState->transientArena = (memory_arena){
        .total = memory->transientStorageSize - sizeof(*transientState),
        .block = memory->transientStorage + sizeof(*transientState),
    };

    u64 MEGABYTES = 1 << 20;
    transientState->sbArena = MemoryArenaSub(&transientState->transientArena, 1 * MEGABYTES);

    transientState->isInitialized = 1;
  }

#if IS_BUILD_DEBUG
  __cleanup_memory_temp__ memory_temp sbMemory = MemoryTempBegin(&transientState->sbArena);
  {
    string *outBuffer = MemoryArenaPush(sbMemory.arena, sizeof(*outBuffer), 4);
    *outBuffer = MemoryArenaPushString(sbMemory.arena, 256);
    string *stringBuffer = MemoryArenaPush(sbMemory.arena, sizeof(*stringBuffer), 4);
    *stringBuffer = MemoryArenaPushString(sbMemory.arena, 32);
    transientState->sb = (string_builder){
        .outBuffer = outBuffer,
        .stringBuffer = stringBuffer,
    };
  }
  string_builder *sb = &transientState->sb;
#endif

  f32 dt = input->dt;
  debug_assert(dt > 0);
#if (0 && IS_BUILD_DEBUG)
  {
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("dt: "));
    StringBuilderAppendF32(sb, dt, 4);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
    string string = StringBuilderFlush(sb);
    write(STDOUT_FILENO, string.value, string.length);
  }
#endif

  static v2 position = {0, 0};
  f32 speed = 5.0f;
  for (u32 controllerIndex = 0; controllerIndex < ARRAY_SIZE(input->controllers); controllerIndex++) {
    game_controller *controller = input->controllers + controllerIndex;
    v2 input = {controller->lsX, controller->lsY};
    f32 inputLengthSq = v2_length_square(input);
    if (inputLengthSq > 1.0f) {
      // make sure input vector length is 1.0
      input = v2_scale(input, 1.0f / SquareRoot(inputLengthSq));
      debug_assert(v2_length(input) <= 1.0f);
    }
    position = v2_add(position, v2_scale(input, speed * dt));

#if (0 && IS_BUILD_DEBUG)
    if (v2_length_square(input) > 0.001f) {
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("controller #"));
      StringBuilderAppendU64(sb, controllerIndex);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(": input ("));
      StringBuilderAppendF32(sb, input.x, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(","));
      StringBuilderAppendF32(sb, input.y, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(")"));
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
      string string = StringBuilderFlush(sb);
      write(STDOUT_FILENO, string.value, string.length);
    }
#endif
  }

  ClearScreen(renderer, COLOR_ZINC_900);
  DrawLine(renderer, position, v2_add(position, (v2){1, 0}), COLOR_RED_500, 5);
  RenderFrame(renderer);
}
