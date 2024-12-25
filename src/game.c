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

    // entropy
    state->effectsEntropy = RandomSeed(1);
    random_series *effectsEntropy = &state->effectsEntropy;

    // entities
    state->entityMax = 4;
    state->entities = MemoryArenaPush(worldArena, sizeof(*state->entities) * state->entityMax, 4);
    u32 entityIndex = 0;

    {
      entity *entity = state->entities + entityIndex;

      entity->mass = RandomBetween(effectsEntropy, 1.0f, 10.0f);
      entity->invMass = 1.0f / entity->mass;
      entity->volume = VolumeCircle(worldArena, 0.5f);
      entity->I = VolumeGetMomentOfInertia(entity->volume, entity->mass);
      entity->invI = 1.0f / entity->I;

      entityIndex++;
    }

    state->entityCount = entityIndex;

    // state is ready
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
  b8 impulse = 0;
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

      if (controller->lb.isDown) {
        impulse = 1;
      }

      if (controller->lb.wasDown) {
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

    // apply damping force
    v2 dampingForce = GenerateDampingForce(entity, 0.8f);
    v2_add_ref(&entity->netForce, dampingForce);

    f32 torque = 0.5f;
    entity->netTorque += torque;
  }

  /*
   * - Integrate applied forces
   */
  for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++) {
    struct entity *entity = state->entities + entityIndex;
    b8 isLastEntity = entityIndex == state->entityCount - 1;

    /* LINEAR KINEMATICS
     *
     * The rate at which "position" p changes is called "velocity" v.
     *   v = ∆p/∆t
     *
     * The rate at which v changes is called "acceleration" a.
     *   a = ∆v/∆t
     *
     * a = f''(t)
     * v = ∫f''(t)
     *   = f'(t)
     *   = at + v₀
     * p = ∫f'(t)
     *   = f(t)
     *   = ½at² + vt + p₀
     *
     * Newton's Law of motion
     *   F = ma
     *   where F is force,
     *         m is mass,
     *         a is acceleration.
     *
     * a = F/m
     */

    // a = F/m
    entity->acceleration = v2_scale(entity->netForce, entity->invMass);

    // v = at + v₀
    v2_add_ref(&entity->velocity, v2_scale(entity->acceleration, dt));

    // p = ½at² + vt + p₀
    v2_add_ref(&entity->position, v2_add(
                                      // ½at²
                                      v2_scale(entity->acceleration, 0.5f * Square(dt)),
                                      // + vt
                                      v2_scale(entity->velocity, dt)));

    /* ANGULAR KINEMATICS
     *
     * As the body rotates, "angle" θ will change. The rate at which θ changes
     * is called "angular velocity", ω.
     *   ω = ∆θ/∆t
     *
     * Likewise as body rotates, the rate at which ω changes is called "angular
     * acceleration", α.
     *   α = ∆ω/∆t
     *
     * α = f''(t)
     * ω = ∫f''(t)
     *   = f'(t)
     *   = αt + ω₀
     * θ = ∫f'(t)
     *   = ½αt² + ωt + θ₀
     *
     * Angular motion analogous to linear motion.
     *   τ = I α
     *   where τ is torque,
     *           Rotational motion.
     *         I is moment of inertia.
     *           Measures how much an object "resists" to change its angular
     *           acceleration.
     *           a.k.a. angular mass
     *           unit: kg m²
     *
     * α = τ/I
     */

    // α = τ/I
    entity->angularAcceleration = entity->netTorque * entity->invI;
    // ω = αt + ω₀
    entity->angularVelocity += entity->angularAcceleration * dt;
    // θ  = ½αt² + ωt + θ₀
    entity->rotation += 0.5f * entity->angularAcceleration * Square(dt) + entity->angularVelocity * dt;

#if (1 && IS_BUILD_DEBUG)
    {
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("entity #"));
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
    entity->netTorque = 0.0f;

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
    u32 colorIndex = (u32)(Lerp(0.0f, COLOR_PALETTE_COUNT, massNormalized));
    const v4 *color = COLORS + colorIndex * COLOR_PALETTE_COUNT + COLOR_LEVEL_COUNT / 2;

    switch (entity->volume->type) {
    case VOLUME_TYPE_CIRCLE: {
      volume_circle *circle = VolumeGetCircle(entity->volume);
      entity->angularAcceleration = -0.0f;
      DrawCircle(renderer, entity->position, circle->radius, entity->rotation, *color);
    } break;
    default: {
      breakpoint("drawing volume type not implemented");
    } break;
    }
  }

  RenderFrame(renderer);
}
