#include "game.h"
#include "color.h"
#include "math.h"
#include "renderer.h"
#include "string_builder.h"

#include "physics.c"
#include "random.c"
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
    memory_arena *worldArena = &state->worldArena;

    state->effectsEntropy = RandomSeed(1);
    random_series *effectsEntropy = &state->effectsEntropy;

    /* 4 particles placed in square form and connected by 6 springs
     * ┌───┐
     * │╲ ╱│
     * │ ╳ │
     * │╱ ╲│
     * └───┘
     */

    state->entityMax = 4;
    state->entityCount = 4;
    state->entities = MemoryArenaPush(worldArena, sizeof(*state->entities) * state->entityMax, 4);
    v2 positions[] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
    };
    for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++) {
      struct entity *entity = state->entities + entityIndex;
      *entity = (struct entity){
          .position = positions[entityIndex],
          // .mass = RandomBetween(effectsEntropy, 1.0f, 5.0f),
          .mass = 2.0f,
      };
      entity->invMass = 1.0f / entity->mass;
    }

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
   * TIME
   *****************************************************************/
  f32 dt = input->dt;
  debug_assert(dt > 0);
  state->time += dt;
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
  global b8 impulse = 0;
  v2 mousePosition = {};
  v2 inputForce = {};
  for (u32 controllerIndex = 0; controllerIndex < ARRAY_COUNT(input->controllers); controllerIndex++) {
    game_controller *controller = input->controllers + controllerIndex;

    v2 input = {controller->lsX, controller->lsY};
    if (v2_length_square(input) > 1.0f) {
      v2_normalize_ref(&input);
      // NOTE: disabled assertion because of floating point error
      // debug_assert(v2_length_square(input) == 1.0f);
    }
    v2_add_ref(&inputForce, input);

    if (controllerIndex == GAME_CONTROLLER_KEYBOARD_AND_MOUSE_INDEX) {
      v2 surfaceHalfDim = RectGetHalfDim(RendererGetSurfaceRect(renderer));
      mousePosition = v2_hadamard((v2){controller->rsX, controller->rsY}, // [-1.0, 1.0]
                                  surfaceHalfDim);

      if (controller->lb) {
        impulse = 1;
      }

      if (impulse && !controller->lb) {
        impulse = 0;

        entity *lastEntity = state->entities + state->entityCount - 1;
        v2 diff = v2_sub(lastEntity->position, mousePosition);
        f32 impulseMagnitude = v2_length(diff) * 5.0f;
        v2 impulseDirection = v2_normalize(diff);
        v2 impulseVector = v2_scale(impulseDirection, impulseMagnitude);
        lastEntity->velocity = impulseVector;

#if (1 && IS_BUILD_DEBUG)
        {
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("impulse: "));
          StringBuilderAppendF32(sb, impulseVector.x, 2);
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(","));
          StringBuilderAppendF32(sb, impulseVector.y, 2);
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
          string string = StringBuilderFlush(sb);
          write(STDOUT_FILENO, string.value, string.length);
        }
#endif
      }
    }
  }

  /*****************************************************************
   * PHYSICS
   *****************************************************************/
  rect groundRect = {
      .min = {-1000.0f, -1000.0f},
      .max = {1000.0f, -5.8f},
  };

  /*
   * - Apply forces
   */
  for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++) {
    struct entity *entity = state->entities + entityIndex;
    b8 isLastEntity = entityIndex == state->entityCount - 1;
    struct entity *nextEntity = state->entities + (entityIndex + 1) % state->entityCount;

    // apply input force
    if (isLastEntity)
      v2_add_ref(&entity->netForce, v2_scale(inputForce, 50.0f));

    // apply weight force
    v2 weightForce = GenerateWeightForce(entity);
    v2_add_ref(&entity->netForce, weightForce);
    if (IsPointInsideRect(entity->position, groundRect))
      v2_add_ref(&entity->netForce, v2_neg(weightForce));

    // apply drag force
    v2 dragForce = GenerateDragForce(entity, 0.001f);
    v2_add_ref(&entity->netForce, dragForce);

    // apply spring force
    // explanation:
    // - "Multiple spring-mass system" @00:00:28
    //   - https://www.youtube.com/watch?v=3wfMPxORS-4
    //   - https://cdn.kastatic.org/ka-youtube-converted/3wfMPxORS-4.mp4/3wfMPxORS-4.mp4
    f32 springConstant = 1000.0f;
    f32 restLength = 1.0f;
    f32 dampingConstant = 8.5f;

    v2 anchorPosition;
    v2 springForce;
    v2 dampingForce;

    anchorPosition = nextEntity->position;
    springForce = GenerateSpringForce(entity, anchorPosition, restLength, springConstant);
    v2_add_ref(&entity->netForce, springForce);
    v2_add_ref(&nextEntity->netForce, v2_neg(springForce));
    dampingForce = GenerateDampingForce(entity, dampingConstant);
    v2_add_ref(&entity->netForce, dampingForce);
    v2_add_ref(&nextEntity->netForce, v2_neg(dampingForce));

    struct entity *crossEntity = entityIndex + 2 < state->entityCount ? state->entities + entityIndex + 2 : 0;
    if (crossEntity) {
      anchorPosition = crossEntity->position;
      springForce = GenerateSpringForce(entity, anchorPosition, restLength, springConstant);
      v2_add_ref(&entity->netForce, springForce);
      v2_add_ref(&crossEntity->netForce, v2_neg(springForce));
      dampingForce = GenerateDampingForce(entity, dampingConstant);
      v2_add_ref(&entity->netForce, dampingForce);
      v2_add_ref(&crossEntity->netForce, v2_neg(dampingForce));
    }
  }

  /*
   * - Integrate applied forces
   */
  for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++) {
    struct entity *entity = state->entities + entityIndex;
    b8 isLastEntity = entityIndex == state->entityCount - 1;

#if 0
    // constant acceleration
    // acceleration = f''(t) = a
    f32 speed = 30.0f; // unit: m/s
    entity->acceleration = v2_scale((v2){1.0f, 0.0f}, speed);
#endif

    // F = ma
    // a = F/m
    entity->acceleration = v2_scale(entity->netForce, entity->invMass);

    // velocity     = ∫f''(t)
    //              = f'(t) = at + v₀
    v2_add_ref(&entity->velocity, v2_scale(entity->acceleration, dt));
    // position     = ∫f'(t)
    //              = f(t) = ½at² + vt + p₀
    v2_add_ref(&entity->position, v2_add(
                                      // ½at²
                                      v2_scale(entity->acceleration, 0.5f * Square(dt)),
                                      // + vt
                                      v2_scale(entity->velocity, dt)));

#if (1 && IS_BUILD_DEBUG)
    {
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("particle #"));
      StringBuilderAppendU64(sb, entityIndex + 1);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  pos: "));
      StringBuilderAppendF32(sb, entity->position.x, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
      StringBuilderAppendF32(sb, entity->position.y, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  vel: "));
      StringBuilderAppendF32(sb, entity->velocity.x, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
      StringBuilderAppendF32(sb, entity->velocity.y, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  acc: "));
      StringBuilderAppendF32(sb, entity->acceleration.x, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
      StringBuilderAppendF32(sb, entity->acceleration.y, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  Fnet:"));
      StringBuilderAppendF32(sb, entity->netForce.x, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
      StringBuilderAppendF32(sb, entity->netForce.y, 2);

      if (isLastEntity) {
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        StringBuilderAppendString(
            sb, &STRING_FROM_ZERO_TERMINATED("****************************************************************"));
      }
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      string string = StringBuilderFlush(sb);
      write(STDOUT_FILENO, string.value, string.length);
    }
#endif

    // clear forces
    entity->netForce = (v2){0.0f, 0.0f};

    /*
     * COLLISION
     */
    // TODO: Ground collision is broken
    if (IsPointInsideRect(entity->position, groundRect)) {
      v2 groundNormal = {0.0f, 1.0f};

      // reflect
      // v' = v - 2(v∙n)n
      entity->velocity =
          v2_sub(entity->velocity, v2_scale(groundNormal, 2.0f * v2_dot(entity->velocity, groundNormal)));
    }
  }

  /*****************************************************************
   * RENDER
   *****************************************************************/
  ClearScreen(renderer, COLOR_ZINC_900);

#if (0 && IS_BUILD_DEBUG)
  // Cartesian coordinate system
  DrawLine(renderer, (v2){-100, 0}, (v2){100, 0}, COLOR_BLUE_500, 0);
  DrawLine(renderer, (v2){0, -100}, (v2){0, 100}, COLOR_BLUE_500, 0);
#endif

  // ground
  DrawRect(renderer, groundRect, COLOR_GRAY_800);

  // mouse
  DrawCrosshair(renderer, mousePosition, 0.5f, COLOR_RED_500);

  if (impulse) {
    entity *lastEntity = state->entities + state->entityCount - 1;
    DrawLine(renderer, lastEntity->position, mousePosition, COLOR_BLUE_300, 1);
  }

  // entities
  for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++) {
    struct entity *entity = state->entities + entityIndex;

    f32 massNormalized = entity->mass / 10.0f /* maximum mass */;
    u32 colorIndex = (u32)(Lerp(0.0f, ARRAY_COUNT(COLORS) / 11, massNormalized));
    const v4 *color = COLORS + colorIndex * 11 + 6;

    DrawCircle(renderer, entity->position, 0.01f + entity->mass / 10.0f, *color);

    struct entity *nextEntity = entityIndex + 1 < state->entityCount ? state->entities + entityIndex + 1 : 0;
    if (nextEntity) {
      DrawLine(renderer, entity->position, nextEntity->position, COLOR_RED_500, 1);
    } else {
      struct entity *firstEntity = state->entities + 0;
      DrawLine(renderer, entity->position, firstEntity->position, COLOR_RED_500, 1);
    }
    struct entity *crossEntity = entityIndex + 2 < state->entityCount ? state->entities + entityIndex + 2 : 0;
    if (crossEntity) {
      DrawLine(renderer, entity->position, crossEntity->position, COLOR_RED_500, 1);
    }
  }

  RenderFrame(renderer);
}
