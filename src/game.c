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
    state->effectsEntropy = RandomSeed(29);
    random_series *effectsEntropy = &state->effectsEntropy;

    // entities
    state->entityMax = 4;
    state->entities = MemoryArenaPush(worldArena, sizeof(*state->entities) * state->entityMax, 4);
    u32 entityIndex = 1; // 0 is invalid entity index;

    if (1) { // circle
      entity *entity = state->entities + entityIndex;

      entity->mass = RandomBetween(effectsEntropy, 1.0f, 10.0f);
      entity->invMass = 1.0f / entity->mass;
      entity->position = V2(RandomBetween(effectsEntropy, 3.0f, 5.0f), 0.0f);
      entity->volume = VolumeCircle(worldArena, RandomBetween(effectsEntropy, 0.5f, 3.0f));
      entity->I = VolumeGetMomentOfInertia(entity->volume, entity->mass);
      entity->invI = 1.0f / entity->I;

      entity->color = COLOR_PURPLE_300;

      entityIndex++;
    }
    if (1) { // circle
      entity *entity = state->entities + entityIndex;

      entity->mass = RandomBetween(effectsEntropy, 1.0f, 10.0f);
      entity->invMass = 1.0f / entity->mass;
      entity->position = V2(RandomBetween(effectsEntropy, -5.0f, -3.0f), 0.0f);
      entity->volume = VolumeCircle(worldArena, RandomBetween(effectsEntropy, 0.5f, 3.0f));
      entity->I = VolumeGetMomentOfInertia(entity->volume, entity->mass);
      entity->invI = 1.0f / entity->I;

      entity->color = COLOR_BLUE_300;

      entityIndex++;
    }
    if (0) { // box
      entity *entity = state->entities + entityIndex;

      entity->mass = RandomBetween(effectsEntropy, 1.0f, 10.0f);
      entity->invMass = 1.0f / entity->mass;
      entity->volume = VolumeBox(worldArena, 1.0f, 1.0f);
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

#if IS_BUILD_DEBUG
  // To visualize physics
  ClearScreen(renderer, COLOR_ZINC_900);
#endif

  /*
   * - Apply forces
   */
  for (u32 entityIndex = 1; entityIndex < state->entityCount; entityIndex++) {
    struct entity *entity = state->entities + entityIndex;
    b8 isLastEntity = entityIndex == state->entityCount - 1;

    // apply input force
    if (isLastEntity)
      v2_add_ref(&entity->netForce, v2_scale(inputForce, 50.0f));

    // apply drag force
    v2 dragForce = GenerateDragForce(entity, 3.1f);
    v2_add_ref(&entity->netForce, dragForce);

    // apply damping force
    v2 dampingForce = GenerateDampingForce(entity, 0.8f);
    v2_add_ref(&entity->netForce, dampingForce);

#if 0
    // do not apply any force
    entity->netForce = V2(0.0f, 0.0f);
    entity->netTorque = 0.0f;
#endif
  }

  /*
   * - Integrate applied forces
   */
  for (u32 entityIndex = 1; entityIndex < state->entityCount; entityIndex++) {
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
      StringBuilderAppendU64(sb, entityIndex);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  volume: "));
      switch (entity->volume->type) {
      case VOLUME_TYPE_CIRCLE: {
        volume_circle *circle = VolumeGetCircle(entity->volume);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("circle radius: "));
        StringBuilderAppendF32(sb, circle->radius, 2);
      } break;
      case VOLUME_TYPE_BOX: {
        volume_box *box = VolumeGetBox(entity->volume);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("box width: "));
        StringBuilderAppendF32(sb, box->width, 2);
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" height: "));
        StringBuilderAppendF32(sb, box->height, 2);
      } break;
      default: {
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("unknown"));
      } break;
      }
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" mass: "));
      StringBuilderAppendF32(sb, entity->mass, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("kg"));
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

      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  F:   "));
      StringBuilderAppendF32(sb, entity->netForce.x, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
      StringBuilderAppendF32(sb, entity->netForce.y, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  θ: "));
      StringBuilderAppendF32(sb, entity->rotation, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  ω: "));
      StringBuilderAppendF32(sb, entity->angularVelocity, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  α: "));
      StringBuilderAppendF32(sb, entity->angularAcceleration, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  τ: "));
      StringBuilderAppendF32(sb, entity->netTorque, 2);

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
     * COLLISION DETECTION
     */

    for (u32 entityBIndex = entityIndex + 1; entityBIndex < state->entityCount; entityBIndex++) {
      u32 entityAIndex = entityIndex;
      // entity cannot collide with itself
      if (entityBIndex == entityAIndex)
        continue;

      struct entity *entityA = entity;
      struct entity *entityB = state->entities + entityBIndex;
      b8 isColliding = 0;
      contact contact = {};

      if (entityA->volume->type == VOLUME_TYPE_CIRCLE && entityB->volume->type == VOLUME_TYPE_CIRCLE) {
        volume_circle *circleA = VolumeGetCircle(entityA->volume);
        volume_circle *circleB = VolumeGetCircle(entityB->volume);

        v2 distance = v2_sub(entityB->position, entityA->position);
        isColliding = v2_length_square(distance) <= Square(circleA->radius + circleB->radius);
        if (isColliding) {
          contact.normal = v2_normalize(distance);
          contact.start = v2_sub(entityB->position, v2_scale(contact.normal, circleB->radius));
          contact.end = v2_add(entityA->position, v2_scale(contact.normal, circleA->radius));
          contact.depth = v2_length(v2_sub(contact.end, contact.start));

          DrawRect(renderer, RectCenterDim(contact.start, V2(0.1f, 0.1f)), COLOR_BLUE_200);
          DrawRect(renderer, RectCenterDim(contact.end, V2(0.1f, 0.1f)), COLOR_BLUE_700);
          DrawLine(renderer, contact.start, v2_add(contact.start, v2_scale(contact.normal, 0.3f)), COLOR_BLUE_500,
                   0.1f);
        }
      }

      entityA->isColliding = isColliding;
      entityB->isColliding = isColliding;

#if (1 && IS_BUILD_DEBUG)
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("Entity #"));
      StringBuilderAppendU64(sb, entityAIndex);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" and #"));
      StringBuilderAppendU64(sb, entityBIndex);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" is "));
      if (isColliding)
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("colliding."));
      else
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("NOT colliding."));
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
      string string = StringBuilderFlush(sb);
      write(STDOUT_FILENO, string.value, string.length);
#endif
    }

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
  // Disabled only for rendering physics
#if !IS_BUILD_DEBUG
  ClearScreen(renderer, COLOR_ZINC_900);
#endif

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
  for (u32 entityIndex = 1; entityIndex < state->entityCount; entityIndex++) {
    struct entity *entity = state->entities + entityIndex;

    v4 color = entity->color;

    if (entity->isColliding) {
      color = COLOR_RED_500;
    }

    switch (entity->volume->type) {
    case VOLUME_TYPE_CIRCLE: {
      volume_circle *circle = VolumeGetCircle(entity->volume);
      DrawCircle(renderer, entity->position, circle->radius, entity->rotation, color);
    } break;
    case VOLUME_TYPE_BOX: {
      volume_box *box = VolumeGetBox(entity->volume);
      v2 dim = {box->width, box->height};
      rect rect = RectCenterDim(entity->position, dim);
      f32 rotation = entity->rotation;
      DrawRectRotated(renderer, rect, rotation, color);
    } break;
    default: {
      breakpoint("drawing volume type not implemented");
    } break;
    }
  }

  RenderFrame(renderer);
}
