#include "game.h"
#include "color.h"
#include "math.h"
#include "renderer.h"
#include "string_builder.h"

#include "renderer.c"

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
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("particle:"));
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  position:     "));
    StringBuilderAppendF32(sb, particle->position.x, 2);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
    StringBuilderAppendF32(sb, particle->position.y, 2);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  velocity:     "));
    StringBuilderAppendF32(sb, particle->velocity.x, 2);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
    StringBuilderAppendF32(sb, particle->velocity.y, 2);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  acceleration: "));
    StringBuilderAppendF32(sb, particle->acceleration.x, 2);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
    StringBuilderAppendF32(sb, particle->acceleration.y, 2);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
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
  }

  v2 sumOfForces = {0, 0};
  v2 windForce = {2.0f, 0.0f};
  sumOfForces = v2_add(sumOfForces, windForce);
  {
    // F = ma
    // a = F/m
    particle->acceleration = v2_scale(sumOfForces, 1.0f / particle->mass);

    // acceleration = f''(t) = a
    // particle->acceleration = v2_scale((v2){1.0f, 0.0f}, speed);

    // velocity     = ∫f''(t)
    //              = f'(t) = at + v₀
    // position     = ∫f'(t)
    //              = f(t) = ½at² + vt + p₀
    particle->velocity = v2_add(particle->velocity, v2_scale(particle->acceleration, dt));
    particle->position =
        // ½at² + vt + p₀
        v2_add(particle->position, v2_add(
                                       // ½at²
                                       v2_scale(particle->acceleration, 0.5f * Square(dt)),
                                       // + vt
                                       v2_scale(particle->velocity, dt)));
  }

  // Is particle over 10m away from origin?
  if (v2_length_square(particle->position) > Square(10.0f)) {
    particle->position = (v2){0, 0};
    particle->velocity = (v2){0, 0};
  }

  /*****************************************************************
   * RENDER
   *****************************************************************/
  ClearScreen(renderer, COLOR_ZINC_900);
  DrawLine(renderer, (v2){-100, 0}, (v2){100, 0}, COLOR_BLUE_500, 0);
  DrawLine(renderer, (v2){0, -100}, (v2){0, 100}, COLOR_BLUE_500, 0);
  DrawLine(renderer, particle->position, v2_add(particle->position, (v2){-0.1f, 0}), COLOR_RED_500, 5);
  DrawLine(renderer, particle->position, v2_add(particle->position, (v2){0, 0.1f}), COLOR_RED_500, 5);
  RenderFrame(renderer);
}
