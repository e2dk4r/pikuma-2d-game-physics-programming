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

  /*****************************************************************
   * PERMANENT STORAGE INITIALIZATION
   *****************************************************************/
  if (!state->isInitialized) {
    // memory
    state->worldArena = (memory_arena){
        .total = memory->permanentStorageSize - sizeof(*state),
        .block = memory->permanentStorage + sizeof(*state),
    };

    state->particle = (particle){
        .position = {1.0f, 2.0f},
        .mass = 5.0f,
    };

    state->isInitialized = 1;
  }

  /*****************************************************************
   * TRANSIENT STORAGE INITIALIZATION
   *****************************************************************/
  transient_state *transientState = memory->transientStorage;
  debug_assert(memory->transientStorageSize >= sizeof(*transientState));
  if (!transientState->isInitialized) {
    transientState->transientArena = (memory_arena){
        .total = memory->transientStorageSize - sizeof(*transientState),
        .block = memory->transientStorage + sizeof(*transientState),
    };

    transientState->isInitialized = 1;
  }

  string_builder *sb = transientState->sb;

  /*****************************************************************
   * PHYSICS
   *****************************************************************/
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

  /*****************************************************************
   * INPUT HANDLING
   *****************************************************************/
  particle *particle = &state->particle;
#if (1 && IS_BUILD_DEBUG)
  {
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("particle { pos: "));
    StringBuilderAppendF32(sb, particle->position.x, 2);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
    StringBuilderAppendF32(sb, particle->position.y, 2);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("}\n"));
    string string = StringBuilderFlush(sb);
    write(STDOUT_FILENO, string.value, string.length);
  }
#endif
  for (u32 controllerIndex = 0; controllerIndex < ARRAY_SIZE(input->controllers); controllerIndex++) {
    game_controller *controller = input->controllers + controllerIndex;
    v2 input = {controller->lsX, controller->lsY};
    f32 inputLengthSq = v2_length_square(input);
    if (inputLengthSq > 1.0f) {
      // make sure input vector length is 1.0
      input = v2_scale(input, 1.0f / SquareRoot(inputLengthSq));
      debug_assert(v2_length(input) <= 1.0f);
    }

    f32 speed = 3.0f;
    particle->position = v2_add(particle->position, v2_scale(input, speed * dt));

  }

  /*****************************************************************
   * RENDER
   *****************************************************************/
  ClearScreen(renderer, COLOR_ZINC_900);
  DrawLine(renderer, particle->position, v2_add(particle->position, (v2){1, 0}), COLOR_RED_500, 5);
  RenderFrame(renderer);
}
